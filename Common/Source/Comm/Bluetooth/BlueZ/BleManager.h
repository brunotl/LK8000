/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BleManager.h
 */
#pragma once

#include "DBusRuntime.h"
#include "DeviceRegistry.h"
#include "DeviceScanner.h"
#include "Subscription.h"
#include "Thread/ThreadPool.h"

#include <memory>
#include <mutex>
#include <string>

class DeviceSession;
class DeviceProxy;

/**
 * Manages Bluetooth Low Energy (BLE) devices through BlueZ.
 *
 * Threading model (actor):
 *   - The DBusRuntime worker thread is the ONLY thread that mutates the state
 *     of sessions (state machine, proxy attach/detach, backoff, reconnect).
 *   - D-Bus dispatcher threads (scanner signals, property signals) never touch
 *     a session directly: they only Post() a task to the worker.
 *   - User threads (Open/Find/Connect/Disconnect) either read thread-safe data
 *     or Post() a request to the worker.
 *   - User-visible events (Connected, Disconnected, StateChanged,
 *     DiscoveredCharacteristics) are therefore notified from the worker thread,
 *     in order. Handlers must not block for long, and must never call
 *     BleManager::Stop().
 *
 * Example usage:
 *   BleManager& manager = BleManager::Instance();
 *   auto session = manager.Open("AA:BB:CC:DD:EE:FF");
 *   session->Connect();
 *   // ...
 *   manager.Stop();
 */
class BleManager {
 public:
  static BleManager& Instance();

  ~BleManager();

  BleManager(const BleManager&) = delete;
  BleManager& operator=(const BleManager&) = delete;

  // Idempotent. Called by the constructor; call it again only to restart
  // after Stop() (or to retry if bluetoothd was not available at startup).
  // Throws if the scanner can not be started.
  void Start();

  // Idempotent. Disconnects every session and releases their proxies.
  // Must not be called from the worker thread (i.e. from an event handler).
  void Stop();

  // Returns the session for this address, creating it if needed.
  // Does NOT connect: call DeviceSession::Connect().
  std::shared_ptr<DeviceSession> Open(const std::string& address);

  // Returns nullptr if no session exists.
  std::shared_ptr<DeviceSession> Find(const std::string& address);

  // Thread-safe, asynchronous. Executed on the worker thread.
  // No-op unless the session wants to be connected (DesiredConnected()).
  void RequestConnect(std::shared_ptr<DeviceSession> session);

  // Thread-safe, asynchronous. Executed on the worker thread.
  // No-op if the session wants to be connected again in the meantime.
  void RequestDisconnect(std::shared_ptr<DeviceSession> session);

  // Called from the D-Bus dispatcher thread by DeviceSession when the
  // "Connected" property of its proxy changes. Only posts to the worker.
  void PostProxyConnectionChanged(std::weak_ptr<DeviceSession> session,
                                  std::weak_ptr<DeviceProxy> proxy,
                                  bool connected);

  // Runs a task on the worker thread.
  void Post(DBusRuntime::Task task) { runtime_.Post(std::move(task)); }

  const std::shared_ptr<DeviceScanner>& Scanner() const noexcept {
    return scanner_;
  }

  DBusRuntime& Runtime() noexcept {
    return runtime_;
  }

 private:
  BleManager();

  // ---- everything below runs on the worker thread only --------------------

  // Align the session state with what the scanner currently sees.
  void SyncAvailability(const std::shared_ptr<DeviceSession>& session);

  void ConnectNow(const std::shared_ptr<DeviceSession>& session);
  void DisconnectNow(const std::shared_ptr<DeviceSession>& session);

  void HandleConnectFailure(const std::shared_ptr<DeviceSession>& session,
                            const char* reason);
  void ScheduleReconnect(const std::shared_ptr<DeviceSession>& session);

  void HandleConnectResult(const std::shared_ptr<DeviceSession>& session,
                           const std::shared_ptr<DeviceProxy>& proxy,
                           std::string error);

  void HandleProxyConnected(const std::shared_ptr<DeviceSession>& session,
                            const std::shared_ptr<DeviceProxy>& proxy);
  void HandleProxyDisconnected(const std::shared_ptr<DeviceSession>& session,
                               const std::shared_ptr<DeviceProxy>& proxy);

  void HandleDeviceFound(const DeviceInfo& info);
  void HandleDeviceLost(const DeviceInfo& info);
  
 private:
  // Declared first so it is destroyed last: everything else may still post to
  // it or talk D-Bus while being torn down.
  DBusRuntime runtime_;

  // Thread pool used for blocking BlueZ calls, currently Device1.Connect(),
  // which may block until the D-Bus timeout (about 25 s).
  //
  // Why it exists:
  //   BleManager is an actor: the DBusRuntime worker is the only thread that
  //   touches session state. A blocking call made on that thread would stall
  //   every other device (connects, reconnect timers, service discovery).
  //
  // How it is used (see ConnectNow() and HandleConnectResult()):
  //   1. ConnectNow(), on the worker, attaches the proxy to the session, then
  //      posts the blocking Connect() to this pool.
  //   2. The pool task only performs the call. It never reads or writes
  //      session state, and it holds the DeviceProxy alive while it runs.
  //   3. It then posts the outcome back to the worker, which calls
  //      HandleConnectResult(). A result whose proxy is no longer the
  //      session's current one (Disconnect(), device lost, Stop()) is stale
  //      and ignored.
  //
  // Sizing: each thread only waits on D-Bus, so threads are cheap, but at most
  // N connections can be in flight at once; further ones wait in the queue.
  //
  // Declared right after runtime_ so that it is destroyed first: its threads
  // post their last results to runtime_ while finishing.
  ThreadPool connect_pool_;

  // Protects registry_ (Open/Find are called from any thread).
  mutable std::mutex mutex_;
  DeviceRegistry registry_;

  // Created once and never reassigned: safe to read from any thread.
  std::shared_ptr<DeviceScanner> scanner_ = std::make_shared<DeviceScanner>();

  // Serializes Start()/Stop().
  std::mutex lifecycle_mutex_;
  bool running_ = false;

  Subscription device_found_sub_;
  Subscription device_lost_sub_;
};