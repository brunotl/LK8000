/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZSppPort.cpp
 */

#include "externs.h"
#include "BlueZSppPort.h"
#include "Comm/PortConfig.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

// RFCOMM channel numbers are a 5-bit field: 1-30 are the only valid values.
// There's no SDP client here to look up which one a given device's SPP
// service is actually registered on (unlike Android's
// createRfcommSocketToServiceRecord(), which resolves this via Android's
// own BLuetooth stack) -- channel 1 is the common convention, but by no
// means universal: a real SkyDrop SPP vario, confirmed by probing every
// channel in turn, only accepts connections on channel 5. So: just try
// them all in order and use whichever one actually connects, rather than
// gambling on a single hardcoded number.
constexpr uint8_t kFirstRfcommChannel = 1;
constexpr uint8_t kLastRfcommChannel = 30;

// Per-attempt connect timeout. RFCOMM connect() on a channel the peer isn't
// listening on typically comes back fast (ECONNREFUSED at the RFCOMM layer,
// baseband link already up), but if the peer is unreachable entirely the
// kernel will block for the page/L2CAP timeout instead -- without a bound,
// probing 30 channels against an absent device could stall the caller
// (TTYPort::Initialize(), invoked from RestartCommPorts() on the main
// thread) for a minute or more.
constexpr int kConnectTimeoutMs = 3000;

enum class ConnectResult { kConnected, kRefused, kUnreachable };

// Tries to connect() an RFCOMM socket to `dst` on `channel`, non-blocking
// with a bounded wait. On kConnected, `out_sock` holds the connected fd
// (blocking mode restored). kRefused means "wrong channel, try the next
// one"; kUnreachable means the peer itself isn't answering -- the caller
// should stop probing further channels rather than repeat the same wait.
ConnectResult TryConnectChannel(const bdaddr_t& dst, uint8_t channel, int& out_sock) {
  int sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (sock < 0) {
    return ConnectResult::kUnreachable;
  }

  const int flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);

  struct sockaddr_rc addr {};
  addr.rc_family = AF_BLUETOOTH;
  addr.rc_bdaddr = dst;
  addr.rc_channel = channel;

  int ret = connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
  if (ret != 0 && errno == EINPROGRESS) {
    struct pollfd pfd {};
    pfd.fd = sock;
    pfd.events = POLLOUT;
    const int poll_ret = poll(&pfd, 1, kConnectTimeoutMs);
    if (poll_ret <= 0) {
      close(sock);
      return ConnectResult::kUnreachable; // timed out or poll() error
    }
    int so_error = 0;
    socklen_t so_error_len = sizeof(so_error);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len);
    errno = so_error;
    ret = so_error == 0 ? 0 : -1;
  }

  if (ret != 0) {
    const bool refused = (errno == ECONNREFUSED);
    close(sock);
    return refused ? ConnectResult::kRefused : ConnectResult::kUnreachable;
  }

  fcntl(sock, F_SETFL, flags); // restore blocking mode for the caller
  out_sock = sock;
  return ConnectResult::kConnected;
}

} // namespace

BlueZSppPort::BlueZSppPort(unsigned idx, const tstring& address)
    : TTYPort(idx, address, 115200, bit8N1) {
}

void BlueZSppPort::ReleaseRfcommDev() {
  if (_rfcomm_dev_id < 0) {
    return;
  }
  int ctl = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (ctl >= 0) {
    struct rfcomm_dev_req req {};
    req.dev_id = _rfcomm_dev_id;
    req.flags = (1 << RFCOMM_HANGUP_NOW);
    ioctl(ctl, RFCOMMRELEASEDEV, &req);
    close(ctl);
  }
  _rfcomm_dev_id = -1;
}

tstring BlueZSppPort::GetDevicePath() {
  // A previous call may have created a /dev/rfcommN that Initialize() then
  // failed to open/use -- RestartCommPorts() retries this on a timer, so
  // without releasing the old one first, each retry orphans another rfcomm
  // device until the kernel runs out of slots.
  ReleaseRfcommDev();

  bdaddr_t dst;
  if (str2ba(GetPortName(), &dst) < 0) {
    return _T("");
  }

  int sock = -1;
  uint8_t channel = 0;
  for (uint8_t c = kFirstRfcommChannel; c <= kLastRfcommChannel; ++c) {
    const ConnectResult result = TryConnectChannel(dst, c, sock);
    if (result == ConnectResult::kConnected) {
      channel = c;
      break;
    }
    if (result == ConnectResult::kUnreachable) {
      // Peer isn't answering at all -- further channels will just repeat
      // the same timeout, so give up now instead of stalling for minutes.
      break;
    }
    // kRefused: this channel just isn't the right one, try the next.
  }
  if (sock < 0) {
    return _T("");
  }

  struct sockaddr_rc local {};
  socklen_t local_len = sizeof(local);
  getsockname(sock, reinterpret_cast<struct sockaddr*>(&local), &local_len);

  struct rfcomm_dev_req req {};
  req.dev_id = -1; // let the kernel pick a free /dev/rfcommN
  req.flags = (1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP);
  req.src = local.rc_bdaddr;
  req.dst = dst;
  req.channel = channel;

  int dev_id = ioctl(sock, RFCOMMCREATEDEV, &req);
  close(sock); // ownership of the DLC has transferred to the new tty device
  if (dev_id < 0) {
    return _T("");
  }

  _rfcomm_dev_id = dev_id;
  char path[32];
  snprintf(path, sizeof(path), "/dev/rfcomm%d", dev_id);

  // Even on devtmpfs, the tty node's actual appearance under /dev is done
  // by an internal kernel worker thread and can lag slightly behind
  // RFCOMMCREATEDEV returning -- opening immediately after can race it and
  // fail with ENOENT despite the device now existing. Poll briefly for it
  // rather than assume it's already there.
  struct stat st;
  for (int i = 0; i < 20 && stat(path, &st) != 0; ++i) {
    usleep(10000); // 10ms
  }

  return path;
}

bool BlueZSppPort::Close() {
  const bool ok = TTYPort::Close();
  ReleaseRfcommDev();
  return ok;
}
