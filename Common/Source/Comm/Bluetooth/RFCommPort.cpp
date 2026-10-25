/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   RFCommPort.cpp
 */

// To fix uuid_t type conflict with sdp_uuid_t, we temporarily redefine uuid_t
// as sdp_uuid_t before including the BlueZ headers.
#define uuid_t sdp_uuid_t

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#undef uuid_t  // Restore the original uuid_t definition after including BlueZ
               // headers

#include "RFCommPort.h"
#include "MessageLog.h"

#ifdef DEBUG_BLUETOOTH
#define Log(...) DebugLog(_T("[BlueZ][RFCommPort]") __VA_ARGS__)
#else
#define Log(...)  // Does nothing
#endif

namespace {

sdp_uuid_t svc_spp_uuid() {
  sdp_uuid_t spp;
  sdp_uuid16_create(&spp, SERIAL_PORT_SVCLASS_ID);
  return spp;
}

// Asks the remote device's SDP server which RFCOMM channel serves `svc_uuid`.
std::optional<uint8_t> find_rfcomm_channel(const bdaddr_t& target, sdp_uuid_t svc_uuid) {
  const bdaddr_t any{};  // BDADDR_ANY (macro is not usable from C++)
  sdp_session_t* session = sdp_connect(&any, &target, SDP_RETRY_IF_BUSY);
  if (!session) {
    return std::nullopt;
  }

  sdp_list_t* search = sdp_list_append(nullptr, &svc_uuid);
  uint32_t range = 0x0000ffff;
  sdp_list_t* attrs = sdp_list_append(nullptr, &range);
  sdp_list_t* response = nullptr;

  std::optional<uint8_t> channel;
  if (sdp_service_search_attr_req(session, search, SDP_ATTR_REQ_RANGE, attrs, &response) == 0) {
    for (sdp_list_t* r = response; r; r = r->next) {
      auto* rec = static_cast<sdp_record_t*>(r->data);
      sdp_list_t* protos = nullptr;
      if (sdp_get_access_protos(rec, &protos) == 0) {
        int ch = sdp_get_proto_port(protos, RFCOMM_UUID);

        auto free_func = reinterpret_cast<sdp_list_func_t>(sdp_list_free);
        sdp_list_foreach(protos, free_func, nullptr);
        sdp_list_free(protos, nullptr);

        if (ch > 0 && !channel) {
          channel = static_cast<uint8_t>(ch);
        }
      }
      sdp_record_free(rec);
    }
  }
  sdp_list_free(response, nullptr);
  sdp_list_free(search, nullptr);
  sdp_list_free(attrs, nullptr);
  sdp_close(session);
  return channel;
}

}  // namespace

bool RFCommPort::Connect() {
  try {
    mSocket = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (mSocket < 0) {
      throw std::system_error(errno, std::generic_category(), "socket");
    }

    bdaddr_t addr{};
    if (str2ba(GetPortName(), &addr) < 0) {
      throw std::system_error(errno, std::generic_category(), "str2ba");
    }

    auto opt_channel = find_rfcomm_channel(addr, svc_spp_uuid());
    uint8_t channel = opt_channel.value_or(1);

    sockaddr_rc remote = {
      .rc_family = AF_BLUETOOTH,
      .rc_bdaddr = addr,
      .rc_channel = channel
    };

    if (::connect(mSocket, reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) < 0) {
      throw std::system_error(errno, std::generic_category(), "connect");
    }

    return true;
  }
  catch (const std::exception& e) {
    Log(_T("Connect failed: %s"), e.what());
    // Handle the error, e.g., log it or clean up resources
    if (mSocket >= 0) {
      ::close(mSocket);
      mSocket = -1;
    }
    return false;
  }
}
