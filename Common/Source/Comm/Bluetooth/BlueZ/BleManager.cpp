/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BleManager.cpp
 */
#include "BleManager.h"

#include <exception>
#include <future>
#include <memory>
#include <vector>

#include "DeviceSession.h"
#include "DeviceProxy.h"

#include "MessageLog.h"

#define Log(...) BlueZDebugLog(_T("[BleManager]") __VA_ARGS__)

BleManager& BleManager::Instance() {
  static BleManager manager;
  return manager;
}

BleManager::BleManager() {
  Log("---------------- Initializing BleManager ----------------");
  try {
    Start();
  }
  catch (const std::exception& e) {
    // Do not throw from a function-local static initializer: the next
    // Instance() call would retry the whole construction (and spawn a new
    // worker thread each time). Start() can be retried explicitly instead.
    Log("Unable to start BLE scanner : %s", e.what());
  }
}

BleManager::~BleManager() {
  Stop();
}

// ---------------------------------------------------------------------------
// Lifecycle (any thread)
// ---------------------------------------------------------------------------

void BleManager::Start() {
  std::lock_guard lock(lifecycle_mutex_);
  if (running_) {
    return;
  }

  // Dispatcher thread -> worker thread: handlers only post, they never touch
  // sessions. Must be subscribed before scanner_->Start() to not miss events.
  device_found_sub_ =
      scanner_->SubscribeDeviceFound([this](const DeviceInfo& info) {
        runtime_.Post([this, info] {
          HandleDeviceFound(info);
        });
      });

  device_lost_sub_ =
      scanner_->SubscribeDeviceLost([this](const DeviceInfo& info) {
        runtime_.Post([this, info] {
          HandleDeviceLost(info);
        });
      });

  try {
    // Scans for BLE devices known by BlueZ on D-Bus (does not start
    // Bluetooth discovery).
    scanner_->Start(runtime_.Connection());
  }
  catch (...) {
    device_found_sub_.Reset();
    device_lost_sub_.Reset();
    throw;
  }

  running_ = true;

  // Sessions opened before a restart: re-evaluate their availability, the
  // scanner does not emit "found" events for devices it discovers at startup.
  std::vector<std::shared_ptr<DeviceSession>> sessions;
  {
    std::lock_guard registry_lock(mutex_);
    sessions = registry_.Sessions();
  }
  for (auto& session : sessions) {
    runtime_.Post([this, session] {
      SyncAvailability(session);
    });
  }
}

void BleManager::Stop() {
  std::lock_guard lock(lifecycle_mutex_);
  if (!running_) {
    return;
  }
  running_ = false;

  // No more found/lost events. Reset() waits for handlers running on other
  // threads, those only post to the worker so this can not deadlock.
  scanner_->Stop();
  device_found_sub_.Reset();
  device_lost_sub_.Reset();

  std::vector<std::shared_ptr<DeviceSession>> sessions;
  {
    std::lock_guard registry_lock(mutex_);
    sessions = registry_.Sessions();
  }

  // Tear every session down on the worker (FIFO: runs after all the tasks
  // already queued) and wait for it.
  std::promise<void> done;
  auto finished = done.get_future();

  runtime_.Post([&] {
    for (auto& session : sessions) {
      try {
        session->desired_connected_.store(false, std::memory_order_release);

        const bool was_connected = session->State() == DeviceState::Connected;

        if (auto proxy = session->CurrentProxy()) {
          try {
            proxy->Disconnect();
          }
          catch (const std::exception& e) {
            Log("%s : Disconnect failed : %s", session->Address().c_str(),
                e.what());
          }
        }

        // Dropping the proxy also drops the property subscriptions, so no
        // more tasks will be posted for this session.
        session->DetachProxy();

        if (session->State() != DeviceState::Unavailable) {
          session->TransitionTo(DeviceState::Disconnected);
        }
        if (was_connected) {
          session->Disconnected();
        }
      }
      catch (...) {
        Log("%s : Unexpected error during stop", session->Address().c_str());
      }
    }
    done.set_value();
  });

  finished.wait();

  Log("---------------- BleManager Stop ----------------");
}

// ---------------------------------------------------------------------------
// Public API (any thread)
// ---------------------------------------------------------------------------

std::shared_ptr<DeviceSession> BleManager::Open(const std::string& address) {
  std::shared_ptr<DeviceSession> session;
  {
    std::lock_guard lock(mutex_);

    if (auto existing = registry_.Find(address)) {
      return existing;
    }

    Log("%s : Opening new session", address.c_str());
    session = std::make_shared<DeviceSession>(address);
    registry_.Add(session);
  }

  // The initial availability is evaluated on the worker, the queue is FIFO so
  // it is done before any RequestConnect posted afterwards.
  runtime_.Post([this, session] {
    SyncAvailability(session);
  });

  return session;
}

std::shared_ptr<DeviceSession> BleManager::Find(const std::string& address) {
  std::lock_guard lock(mutex_);
  return registry_.Find(address);
}

void BleManager::RequestConnect(std::shared_ptr<DeviceSession> session) {
  if (!session) {
    Log("Session is null, cannot request connect");
    return;
  }
  runtime_.Post([this, session = std::move(session)] {
    ConnectNow(session);
  });
}

void BleManager::RequestDisconnect(std::shared_ptr<DeviceSession> session) {
  if (!session) {
    Log("Session is null, cannot request disconnect");
    return;
  }
  runtime_.Post([this, session = std::move(session)] {
    DisconnectNow(session);
  });
}

void BleManager::PostProxyConnectionChanged(
    std::weak_ptr<DeviceSession> weak_session,
    std::weak_ptr<DeviceProxy> weak_proxy, bool connected) {
  runtime_.Post([this, weak_session = std::move(weak_session),
                 weak_proxy = std::move(weak_proxy), connected] {
    auto session = weak_session.lock();
    auto proxy = weak_proxy.lock();
    if (!session || !proxy) {
      return;
    }
    if (connected) {
      HandleProxyConnected(session, proxy);
    }
    else {
      HandleProxyDisconnected(session, proxy);
    }
  });
}

// ---------------------------------------------------------------------------
// Worker thread only
// ---------------------------------------------------------------------------

void BleManager::SyncAvailability(
    const std::shared_ptr<DeviceSession>& session) {
  if (scanner_->Find(session->Address())) {
    session->MarkAvailable();
  }
  else if (session->State() == DeviceState::Disconnected) {
    session->MarkUnavailable();
  }
}

void BleManager::ConnectNow(const std::shared_ptr<DeviceSession>& session) {
  // The user may have called Disconnect() since the request was posted (or
  // since the reconnect timer was armed).
  if (!session->DesiredConnected()) {
    Log("%s : Connection not desired anymore, skipping",
        session->Address().c_str());
    return;
  }

  switch (session->State()) {
    case DeviceState::Connected:
      Log("%s : Device is already connected", session->Address().c_str());
      return;
    case DeviceState::Connecting:
      Log("%s : Device is already connecting", session->Address().c_str());
      return;
    default:
      break;  // Disconnected, Reconnecting, Unavailable: re-evaluated below
  }

  auto info = scanner_->Find(session->Address());
  if (!info) {
    Log("%s : Device is unavailable, waiting until it is found again",
        session->Address().c_str());
    session->MarkUnavailable();
    return;
  }

  session->MarkAvailable();  // Unavailable -> Disconnected, no-op otherwise
  session->TransitionTo(DeviceState::Connecting);

  try {
    Log("%s : Attempting to connect", session->Address().c_str());

    auto proxy =
        std::make_shared<DeviceProxy>(runtime_.Connection(), info->path());

    // Attach first: from now on a "Connected" change is delivered (posted to
    // this worker, so processed after this function returns) and nothing can
    // slip between the IsConnected() check and the subscription.
    session->AttachProxy(proxy);

    if (proxy->IsConnected()) {
      // BlueZ may not emit a signal for a device that is already connected,
      // and Connect() would fail with AlreadyConnected on some versions.
      Log("%s : Already connected : notifying", session->Address().c_str());
      session->OnConnected();
      proxy->DiscoverServices();
    }
    else {
      // Connect() may block until the D-Bus timeout: run it on connect_pool_,
      // not on the worker. The result comes back through HandleConnectResult().
      connect_pool_.Post([this, weak_session = std::weak_ptr(session),
                          weak_proxy = std::weak_ptr(proxy)] {
        // Holding the proxy for the whole call keeps the DeviceProxy alive even
        // if the session drops it meanwhile.
        auto proxy = weak_proxy.lock();
        if (!proxy) {
          return;
        }

        std::string error;
        try {
          proxy->Connect();
        }
        catch (const std::exception& e) {
          error = e.what();
        }
        catch (...) {
          error = "unexpected error";
        }

        // Back to the worker: it is the only thread allowed to touch the
        // session.
        runtime_.Post(
            [this, weak_session, weak_proxy, error = std::move(error)] {
              auto session = weak_session.lock();
              auto proxy = weak_proxy.lock();
              if (session && proxy) {
                HandleConnectResult(session, proxy, error);
              }
            });
      });
    }
  }
  catch (const std::exception& e) {
    HandleConnectFailure(session, e.what());
  }
  catch (...) {
    HandleConnectFailure(session, "unexpected error");
  }
}

void BleManager::DisconnectNow(const std::shared_ptr<DeviceSession>& session) {
  // DeviceSession::Disconnect() cleared the flag; a later Connect() set it
  // again, in which case this request is obsolete.
  if (session->DesiredConnected()) {
    return;
  }

  bool was_connected = false;
  if (auto proxy = session->CurrentProxy()) {
    try {
      was_connected = proxy->IsConnected();
      // Also attempted when not connected yet, to cancel a connection that
      // BlueZ may still be establishing. "NotConnected" errors are expected.
      proxy->Disconnect();
    }
    catch (const std::exception& e) {
      Log("%s : Disconnect failed : %s", session->Address().c_str(), e.what());
    }
  }

  if (!was_connected) {
    // No "Connected=false" signal will come: settle the state ourselves.
    session->DetachProxy();
    if (session->State() != DeviceState::Unavailable) {
      session->TransitionTo(DeviceState::Disconnected);
    }
  }
  // else: the "Connected=false" signal will reach HandleProxyDisconnected,
  // which transitions to Disconnected and notifies.
}

void BleManager::HandleConnectFailure(
    const std::shared_ptr<DeviceSession>& session, const char* reason) {
  Log("%s : Failed to connect : %s", session->Address().c_str(), reason);

  session->DetachProxy();

  if (!session->DesiredConnected()) {
    session->TransitionTo(DeviceState::Disconnected);
    return;
  }

  session->IncreaseBackoff();
  session->TransitionTo(DeviceState::Reconnecting);
  ScheduleReconnect(session);
}

void BleManager::ScheduleReconnect(
    const std::shared_ptr<DeviceSession>& session) {
  if (!session->DesiredConnected()) {
    Log("%s : Session is not desired to be connected, skipping reconnect",
        session->Address().c_str());
    return;
  }

  // One pending timer per session at most.
  if (session->reconnect_scheduled_) {
    return;
  }
  session->reconnect_scheduled_ = true;

  const auto delay = session->CurrentBackoff();
  Log("%s : Scheduling reconnect in %lld ms", session->Address().c_str(),
      static_cast<long long>(delay.count()));

  runtime_.PostDelayed(delay, [this, weak = std::weak_ptr(session)] {
    auto session = weak.lock();
    if (!session) {
      return;
    }
    session->reconnect_scheduled_ = false;

    // Only if nothing else happened meanwhile (user Disconnect(), device lost
    // or found again and already reconnecting, ...). A device that comes back
    // is handled by HandleDeviceFound.
    if (session->State() == DeviceState::Reconnecting) {
      ConnectNow(session);
    }
  });
}

void BleManager::HandleConnectResult(
    const std::shared_ptr<DeviceSession>& session,
    const std::shared_ptr<DeviceProxy>& proxy, std::string error) {
  // The proxy was replaced or detached meanwhile (Disconnect(), device lost,
  // Stop()): this result is stale, ignore it.
  if (session->CurrentProxy() != proxy) {
    return;
  }

  if (!error.empty()) {
    HandleConnectFailure(session, error.c_str());
    return;
  }

  // Success. The "Connected" signal has normally been processed already;
  // otherwise catch up here.
  if (session->State() == DeviceState::Connecting) {
    HandleProxyConnected(session, proxy);
  }
}

void BleManager::HandleProxyConnected(
    const std::shared_ptr<DeviceSession>& session,
    const std::shared_ptr<DeviceProxy>& proxy) {
  // Event coming from a proxy that has been replaced or detached.
  if (session->CurrentProxy() != proxy) {
    return;
  }

  if (!session->DesiredConnected()) {
    // Disconnect() was requested while the connection was in progress.
    DisconnectNow(session);
    return;
  }

  // Already notified by ConnectNow (device was already connected).
  if (session->State() == DeviceState::Connected) {
    return;
  }

  session->OnConnected();
}

void BleManager::HandleProxyDisconnected(
    const std::shared_ptr<DeviceSession>& session,
    const std::shared_ptr<DeviceProxy>& proxy) {
  if (session->CurrentProxy() != proxy) {
    return;
  }

  const auto state = session->State();
  if (state == DeviceState::Unavailable || state == DeviceState::Disconnected) {
    return;  // already handled
  }

  session->OnDisconnected();   // -> Disconnected or Reconnecting
  ScheduleReconnect(session);  // no-op unless desired
}

void BleManager::HandleDeviceFound(const DeviceInfo& info) {
  Log("%s : Device found", info.address().c_str());

  auto session = Find(info.address());
  if (!session) {
    return;
  }

  session->MarkAvailable();

  if (session->DesiredConnected()) {
    ConnectNow(session);
  }
}

void BleManager::HandleDeviceLost(const DeviceInfo& info) {
  auto session = Find(info.address());
  if (!session) {
    return;
  }

  Log("%s : Device lost", info.address().c_str());

  const bool was_connected = session->State() == DeviceState::Connected;

  session->DetachProxy();
  session->MarkUnavailable();

  if (was_connected) {
    session->Disconnected();
  }

  Log("%s : Session marked as unavailable", info.address().c_str());
}
