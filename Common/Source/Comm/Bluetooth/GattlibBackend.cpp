/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   GattlibBackend.cpp
 *
 * NOTE: this file must never include any LK8000 header that (transitively)
 * includes utils/uuid.h -- see the comment in GattlibBackend.h.
 *
 * gattlib.h transitively includes BlueZ's <bluetooth/sdp.h>, which declares
 * its own unrelated "uuid_t" (a legacy SDP struct) -- the exact same global
 * name as LK8000's own uuid_t class (utils/uuid.h), used throughout the rest
 * of the program. The two never appear together in one translation unit,
 * but sharing the same name still confused this project's -flto whole-
 * program build (GCC's own -Wodr warned about it: "type 'struct uuid_t'
 * violates the C++ One Definition Rule"). This first showed up as a real
 * on-device hang in unrelated code, not just the warning -- LTO's cross-TU
 * type merging is undefined behavior once two same-named-but-different
 * types exist, and that UB isn't confined to this file. Rename BlueZ's
 * before including anything that declares it, so this TU's uuid_t is a
 * genuinely distinct type by name, not just by (unenforced) intent.
 */

#include "GattlibBackend.h"

#define uuid_t bluez_sdp_uuid_t
#include <gattlib.h>

#include <gio/gio.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace gattlib_backend {

namespace {

// gattlib only allows one gattlib_mainloop() running per process at a time
// (it errors out with GATTLIB_BUSY otherwise); this ref-counts a single
// background thread pumping it across every BlueZGattSensor instance.
class Mainloop {
 public:
  static Mainloop& Instance() {
    static Mainloop instance;
    return instance;
  }

  void Acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ref_count_++ == 0) {
      // GDBus's compiled-in default system bus address doesn't necessarily
      // match where this device's own dbus-daemon actually listens (e.g.
      // "/run/dbus/system_bus_socket" vs. "/var/run/dbus/system_bus_socket"
      // -- confirmed on a real Kobo: gattlib_adapter_open() failed with
      // "Could not connect: No such file or directory" until this was set).
      // setenv(..., 0) leaves a deliberate external override in place.
      setenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/var/run/dbus/system_bus_socket", 0);
      shutdown_ = false;
      thread_ = std::thread([this] { gattlib_mainloop(&Mainloop::KeepAlive, this); });
    }
  }

  void Release() {
    std::thread joiner;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (--ref_count_ == 0) {
        shutdown_ = true;
        cv_.notify_all();
        joiner = std::move(thread_);
      }
    }
    if (joiner.joinable()) {
      joiner.join();
    }
  }

 private:
  static void* KeepAlive(void* arg) {
    auto* self = static_cast<Mainloop*>(arg);
    std::unique_lock<std::mutex> lock(self->mutex_);
    self->cv_.wait(lock, [self] { return self->shutdown_; });
    return nullptr;
  }

  std::mutex mutex_;
  std::condition_variable cv_;
  bool shutdown_ = false;
  int ref_count_ = 0;
  std::thread thread_;
};

// gattlib_adapter_scan_enable() blocks until scan_disable()/timeout, but its
// DBus/BlueZ-backed implementation keeps a single, non-thread-safe
// `ble_scan` struct per adapter object -- and gattlib_adapter_open(nullptr,
// ...) always resolves to the same ref-counted adapter (one hci0 per
// process), so two callers (two BlueZGattSensor Connections, or a
// Connection racing the scan-menu's StartScan()) calling
// gattlib_adapter_scan_enable() concurrently race on that shared struct:
// each call's internal memset()/re-init (dbus/gattlib_adapter.c,
// _gattlib_adapter_scan_enable_with_filter) clobbers the other's in-flight
// scan state (timeout id, signal-handler ids, is_scanning flag), and the
// blocking call can end up waiting on a "scan stopped" signal that never
// arrives for it specifically -- a real hang, confirmed against gattlib
// 0.7.2's own source. Serialize every scan_enable() call process-wide so
// only one is ever in flight; scan_disable() itself stays unlocked, since
// that's what unblocks whichever call currently holds this.
std::mutex g_scan_mutex;

// Polls for g_scan_mutex rather than blocking on it outright: whichever
// scan currently holds it (the device-scan menu's effectively-unbounded
// StartScan(), or another Connect()'s own 10s discovery) can hold it for a
// while, and both StopScan() and Disconnect() join() the thread that's
// waiting here -- on the main/UI thread in StopScan()'s case (dialog
// close). Checking `cancelled` every 100ms instead of just locking keeps
// that join() bounded instead of turning into the same kind of hang this
// mutex was added to fix.
bool AcquireScanSlot(const std::atomic<bool>& cancelled) {
  while (!cancelled.load()) {
    if (g_scan_mutex.try_lock()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return false;
}

std::string UuidToString(const uuid_t& uuid) {
  char buf[64] = {0};
  gattlib_uuid_to_string(&uuid, buf, sizeof(buf));
  return std::string(buf);
}

uuid_t ToGattlibUuid(const uuid128_t& b) {
  char str[64];
  snprintf(str, sizeof(str),
          "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
          b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
          b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
  uuid_t out{};
  gattlib_string_to_uuid(str, strlen(str), &out);
  return out;
}

uuid128_t ParseUuidString(const std::string& s) {
  uuid128_t out{};
  unsigned bytes[16] = {0};
  sscanf(s.c_str(), "%2x%2x%2x%2x-%2x%2x-%2x%2x-%2x%2x-%2x%2x%2x%2x%2x%2x",
        &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5],
        &bytes[6], &bytes[7], &bytes[8], &bytes[9], &bytes[10], &bytes[11],
        &bytes[12], &bytes[13], &bytes[14], &bytes[15]);
  for (int i = 0; i < 16; ++i) {
    out[i] = static_cast<uint8_t>(bytes[i]);
  }
  return out;
}

uuid128_t ToUuid128(const uuid_t& uuid) {
  return ParseUuidString(UuidToString(uuid));
}

} // namespace

struct Connection {
  gattlib_adapter_t* adapter = nullptr;
  // Written from whichever thread gattlib invokes its connect/disconnect
  // callbacks on (not necessarily the one that created this Connection);
  // atomic so DiscoverAndSubscribe() can safely poll it for an unsolicited
  // disconnect racing its own in-flight discovery calls.
  std::atomic<gattlib_connection_t*> conn{nullptr};
  gattlib_stream_t* stream = nullptr;
  Callbacks callbacks;
  std::string address;
  uuid_t write_uuid{};
  bool has_write_uuid = false;
  std::atomic<bool> shutting_down{false};
  // Serializes HandleConnect()/the on-disconnect handler (both invoked by
  // gattlib on its own GLib-main-loop thread) against Disconnect() (called
  // from LK8000's port-management thread, e.g. GattSensor::Close()) tearing
  // this Connection down concurrently. Confirmed on a real device: without
  // this, Disconnect() can close the adapter / delete this Connection while
  // DiscoverAndSubscribe()'s in-flight gattlib_discover_primary/char() call
  // -- or a reconnect kicked off by the on-disconnect handler -- is still
  // using it, crashing inside gattlib itself ("assertion 'G_IS_DBUS_PROXY
  // (proxy)' failed" followed by a SIGSEGV, seen in kobo/error.log). This is
  // what actually closes that race; the self->conn checks in
  // DiscoverAndSubscribe() only narrowed it. Recursive because gattlib_connect()
  // can invoke OnConnectCb synchronously, inline, on the calling thread for
  // fast-fail cases -- including from the on-disconnect handler's own
  // reconnect call below, which already holds this lock.
  std::recursive_mutex callback_mutex;
  // Characteristic UUID string -> owning service UUID, filled in during
  // discovery; gattlib's notification callback only carries the
  // characteristic UUID, not its service.
  std::unordered_map<std::string, uuid_t> char_to_service;

  // Thread running the pre-connect discovery scan (see Connect()).
  std::thread scan_before_connect_thread;
  std::atomic<bool> connect_attempted{false};
};

namespace {

void OnConnectCb(gattlib_adapter_t* adapter, const char* dst,
                 gattlib_connection_t* connection, int error, void* user_data);

bool DiscoverAndSubscribe(Connection* self, gattlib_connection_t* conn) {
  // Each of the gattlib_* calls below is a synchronous D-Bus round-trip
  // that can take a noticeable moment; an unsolicited disconnect (BlueZ
  // dropping the device, e.g.) racing this discovery sequence has been
  // observed to crash inside gattlib itself (invalid GDBusProxy assertion
  // followed by a SIGSEGV) rather than fail cleanly -- bail out early on
  // every disconnect gattlib has told us about via our own callback (see
  // the disconnect handler in HandleConnect(), which clears self->conn)
  // instead of continuing to hand it a connection that's going away. This
  // narrows the race rather than closing it: it can't catch an invalidation
  // that happens strictly inside one already-in-flight gattlib call.
  if (self->conn.load() != conn) {
    return false;
  }

  gattlib_primary_service_t* services = nullptr;
  int services_count = 0;
  if (gattlib_discover_primary(conn, &services, &services_count) != GATTLIB_SUCCESS) {
    return false;
  }

  if (self->conn.load() != conn) {
    free(services);
    return false;
  }

  gattlib_characteristic_t* characteristics = nullptr;
  int char_count = 0;
  if (gattlib_discover_char(conn, &characteristics, &char_count) != GATTLIB_SUCCESS) {
    free(services);
    return false;
  }

  if (self->conn.load() != conn) {
    free(characteristics);
    free(services);
    return false;
  }

  self->char_to_service.clear();

  for (int i = 0; i < char_count && self->conn.load() == conn; ++i) {
    const gattlib_characteristic_t& ch = characteristics[i];

    const gattlib_primary_service_t* owning_service = nullptr;
    for (int j = 0; j < services_count; ++j) {
      if (ch.handle >= services[j].attr_handle_start && ch.handle <= services[j].attr_handle_end) {
        owning_service = &services[j];
        break;
      }
    }
    if (owning_service == nullptr) {
      continue;
    }

    self->char_to_service[UuidToString(ch.uuid)] = owning_service->uuid;

    const uuid128_t backend_service = ToUuid128(owning_service->uuid);
    const uuid128_t backend_char = ToUuid128(ch.uuid);

    if (!self->callbacks.should_enable_notification(self->callbacks.user_data, backend_service, backend_char)) {
      continue;
    }

    if (ch.properties & GATTLIB_CHARACTERISTIC_NOTIFY) {
      uuid_t notify_uuid = ch.uuid;
      gattlib_notification_start(conn, &notify_uuid);
    }
    else if (ch.properties & GATTLIB_CHARACTERISTIC_READ) {
      void* buffer = nullptr;
      size_t buffer_len = 0;
      uuid_t read_uuid = ch.uuid;
      if (gattlib_read_char_by_uuid(conn, &read_uuid, &buffer, &buffer_len) == GATTLIB_SUCCESS) {
        self->callbacks.on_characteristic_changed(self->callbacks.user_data, backend_service, backend_char,
                                                   static_cast<const uint8_t*>(buffer), buffer_len);
        gattlib_characteristic_free_value(buffer);
      }
    }
  }

  free(characteristics);
  free(services);
  return true;
}

void HandleConnect(Connection* self, gattlib_connection_t* conn, int error) {
  std::lock_guard<std::recursive_mutex> callback_lock(self->callback_mutex);

  if (self->shutting_down.load()) {
    // Disconnect() is tearing this Connection down (possibly already gone
    // by the time this callback got scheduled) -- don't touch it, and don't
    // leak a connection nobody else will now close.
    if (conn != nullptr) {
      gattlib_disconnect(conn, false /* wait_disconnection */);
    }
    return;
  }

  if (error != GATTLIB_SUCCESS || conn == nullptr) {
    self->callbacks.on_connected(self->callbacks.user_data, false);
    return;
  }

  self->conn = conn;
  gattlib_register_on_disconnect(conn, [](gattlib_connection_t*, void* user_data) {
    auto* self = static_cast<Connection*>(user_data);
    std::lock_guard<std::recursive_mutex> callback_lock(self->callback_mutex);

    self->conn = nullptr;
    self->stream = nullptr; // was bound to the now-dead connection

    self->callbacks.on_disconnected(self->callbacks.user_data);

    if (self->shutting_down.load()) {
      return;
    }

    // Single reconnect attempt, mirroring the Android backend's
    // gatt.connect() retry on an unsolicited disconnect. Don't also report
    // failure here on a non-success return: gattlib_connect() invokes
    // OnConnectCb synchronously, inline, on at least the "Cannot find
    // connection" class of fast-fail (confirmed on a real device) --
    // calling on_connected(false) again here on top of that double-fires
    // it for the exact same failure.
    gattlib_connect(self->adapter, self->address.c_str(), GATTLIB_CONNECTION_OPTIONS_NONE, &OnConnectCb, self);
  }, self);

  gattlib_register_notification(conn, [](const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {
    auto* self = static_cast<Connection*>(user_data);
    auto it = self->char_to_service.find(UuidToString(*uuid));
    const uuid_t service_uuid = (it != self->char_to_service.end()) ? it->second : uuid_t{};
    self->callbacks.on_characteristic_changed(self->callbacks.user_data, ToUuid128(service_uuid),
                                              ToUuid128(*uuid), data, data_length);
  }, self);

  const bool ok = DiscoverAndSubscribe(self, conn);

  if (ok && self->has_write_uuid && self->conn.load() == conn) {
    uuid_t write_uuid = self->write_uuid;
    uint16_t mtu = 0;
    // Best-effort: Write() falls back to an unchunked write if this failed.
    gattlib_write_char_by_uuid_stream_open(conn, &write_uuid, &self->stream, &mtu);
  }

  self->callbacks.on_connected(self->callbacks.user_data, ok);
}

void OnConnectCb(gattlib_adapter_t* adapter, const char* dst,
                 gattlib_connection_t* connection, int error, void* user_data) {
  HandleConnect(static_cast<Connection*>(user_data), connection, error);
}

} // namespace

struct ScanHandle {
  gattlib_adapter_t* adapter = nullptr;
  void (*callback)(void*, const char*, const char*, bool) = nullptr;
  void* user_data = nullptr;
  std::thread thread;
  std::mutex seen_mutex;
  std::unordered_map<std::string, bool> seen; // address -> is_classic_spp
  std::atomic<bool> stop_requested{false};
};

namespace {

// True if BlueZ reports this address as BR/EDR-capable (i.e. it should use
// BT_SPP:, not BLE:). Checked via the device's "Class" property rather
// than "UUIDs": UUIDs stays empty ("ServicesResolved": false) until BlueZ
// has actually connected and done an SDP lookup, which hasn't happened yet
// for a device that's merely been seen during a scan -- confirmed on a
// real device. "Class" (Class of Device) is a BR/EDR-only concept BlueZ
// only ever sets for BR/EDR-capable devices; a pure-LE peripheral (e.g. a
// real Meshtastic node, confirmed on a real device) simply has no Class
// property at all, cached or not.
//
// A plain, synchronous GDBus property fetch -- must never run on the GLib
// main loop thread itself (the thread dispatching OnDiscoveredCb below),
// or it would deadlock waiting for a reply that thread would otherwise
// deliver.
bool IsClassicCapable(const std::string& address) {
  // Defensive: same fix as Mainloop::Acquire() below, in case this ever
  // runs before that has (it shouldn't, in practice, since a scan can't
  // discover anything before StartScan() has already called Acquire()).
  setenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/var/run/dbus/system_bus_socket", 0);

  std::string dev_id = address;
  for (char& c : dev_id) {
    if (c == ':') c = '_';
  }
  const std::string path = "/org/bluez/hci0/dev_" + dev_id;

  GError* error = nullptr;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
      G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, nullptr,
      "org.bluez", path.c_str(), "org.bluez.Device1", nullptr, &error);
  if (!proxy) {
    if (error) {
      g_error_free(error);
    }
    return false;
  }

  GVariant* class_of_device = g_dbus_proxy_get_cached_property(proxy, "Class");
  const bool found = (class_of_device != nullptr);
  if (class_of_device) {
    g_variant_unref(class_of_device);
  }
  g_object_unref(proxy);
  return found;
}

void OnDiscoveredCb(gattlib_adapter_t* adapter, const char* addr, const char* name, void* user_data) {
  auto* handle = static_cast<ScanHandle*>(user_data);
  const std::string address = addr;
  const std::string name_copy = name ? name : "";

  {
    std::lock_guard<std::mutex> lock(handle->seen_mutex);
    auto it = handle->seen.find(address);
    if (it != handle->seen.end()) {
      handle->callback(handle->user_data, address.c_str(), name_copy.c_str(), it->second);
      return;
    }
  }

  // First time seeing this address this scan: check it off-thread (see
  // IsClassicCapable's comment), then cache the result for next time.
  std::thread([handle, address, name_copy]() {
    const bool is_classic_spp = IsClassicCapable(address);
    {
      std::lock_guard<std::mutex> lock(handle->seen_mutex);
      handle->seen[address] = is_classic_spp;
    }
    handle->callback(handle->user_data, address.c_str(), name_copy.c_str(), is_classic_spp);
  }).detach();
}

} // namespace

ScanHandle* StartScan(void (*callback)(void*, const char*, const char*, bool), void* user_data) {
  auto* handle = new ScanHandle();
  handle->callback = callback;
  handle->user_data = user_data;

  Mainloop::Instance().Acquire();

  if (gattlib_adapter_open(nullptr, &handle->adapter) != GATTLIB_SUCCESS) {
    Mainloop::Instance().Release();
    delete handle;
    return nullptr;
  }

  // gattlib_adapter_scan_enable() blocks until `timeout` elapses or
  // gattlib_adapter_scan_disable() is called from another thread -- run it
  // on its own thread so StartScan() itself returns immediately; the
  // effectively-unbounded timeout means it really runs until StopScan().
  handle->thread = std::thread([handle] {
    constexpr size_t kScanTimeoutSeconds = 3600;
    if (AcquireScanSlot(handle->stop_requested)) {
      if (!handle->stop_requested.load()) {
        gattlib_adapter_scan_enable(handle->adapter, &OnDiscoveredCb, kScanTimeoutSeconds, handle);
      }
      g_scan_mutex.unlock();
    }
  });

  return handle;
}

void StopScan(ScanHandle* handle) {
  if (!handle) {
    return;
  }
  handle->stop_requested = true;
  if (handle->adapter) {
    gattlib_adapter_scan_disable(handle->adapter);
  }
  if (handle->thread.joinable()) {
    handle->thread.join();
  }
  if (handle->adapter) {
    gattlib_adapter_close(handle->adapter);
  }
  Mainloop::Instance().Release();
  delete handle;
}

namespace {

// A BLE device gattlib/BlueZ hasn't discovered "recently" has no D-Bus
// Device1 object of its own (unpaired/"temporary" devices are dropped from
// BlueZ's cache once discovery stops), so gattlib_connect() on it fails
// immediately with "Cannot find connection <addr>" -- confirmed on a real
// device, and matching why the Android backend also rescans before
// connecting to a not-yet-bonded device (see
// BluetoothGattClientPort.java's isUnknownType()/startLeScan()). Since the
// config dialog's scan and the actual connect attempt can be arbitrarily
// far apart in time (browse the list, save, close the dialog, and only
// then does RestartCommPorts() actually open the port), always rescan for
// this specific address immediately before connecting, rather than
// assuming BlueZ still remembers it from whenever it was last seen.
void OnDiscoveredForConnect(gattlib_adapter_t* adapter, const char* addr, const char* /*name*/, void* user_data) {
  auto* self = static_cast<Connection*>(user_data);
  if (strcasecmp(addr, self->address.c_str()) != 0) {
    return;
  }
  bool expected = false;
  if (!self->connect_attempted.compare_exchange_strong(expected, true)) {
    return; // already matched (or connect already attempted) once
  }
  gattlib_adapter_scan_disable(adapter);
  // Don't also report failure here on a non-success return -- see the
  // matching comment on the reconnect-on-disconnect call in HandleConnect().
  gattlib_connect(adapter, addr, GATTLIB_CONNECTION_OPTIONS_NONE, &OnConnectCb, self);
}

void ScanThenConnect(Connection* self) {
  constexpr size_t kDiscoverTimeoutSeconds = 10;
  if (AcquireScanSlot(self->shutting_down)) {
    // Disconnect() may have been requested while this was queued behind
    // another in-flight scan -- don't bother starting a new one.
    if (!self->shutting_down.load()) {
      gattlib_adapter_scan_enable(self->adapter, &OnDiscoveredForConnect, kDiscoverTimeoutSeconds, self);
    }
    g_scan_mutex.unlock();
  }
  // If the target address was never seen (timed out) or was found but
  // gattlib_connect() itself failed synchronously, on_connected(false) is
  // owed here (once) -- either way that's exactly connect_attempted
  // becoming true for the first time without a connection ever being
  // established, matched by OnConnectCb never firing.
  bool expected = false;
  if (self->connect_attempted.compare_exchange_strong(expected, true)) {
    self->callbacks.on_connected(self->callbacks.user_data, false);
  }
}

} // namespace

Connection* Connect(const char* address, const uuid128_t& write_characteristic,
                    bool has_write_characteristic, const Callbacks& callbacks) {
  auto* self = new Connection();
  self->address = address;
  self->callbacks = callbacks;
  if (has_write_characteristic) {
    self->write_uuid = ToGattlibUuid(write_characteristic);
    self->has_write_uuid = true;
  }

  Mainloop::Instance().Acquire();

  if (gattlib_adapter_open(nullptr, &self->adapter) != GATTLIB_SUCCESS) {
    Mainloop::Instance().Release();
    delete self;
    return nullptr;
  }

  self->scan_before_connect_thread = std::thread([self]() { ScanThenConnect(self); });

  return self;
}

void Disconnect(Connection* connection) {
  if (!connection) {
    return;
  }

  {
    // Taking the lock here (before setting shutting_down) closes the gap
    // where HandleConnect()/the on-disconnect handler already read
    // shutting_down as false and is about to act on a stale conn/adapter --
    // they now either haven't started yet (and will see it true once they
    // do get the lock) or are already inside their own critical section, in
    // which case the block below waits for them to finish first.
    std::lock_guard<std::recursive_mutex> callback_lock(connection->callback_mutex);
    connection->shutting_down = true;
  }

  if (connection->adapter) {
    gattlib_adapter_scan_disable(connection->adapter);
  }
  if (connection->scan_before_connect_thread.joinable()) {
    connection->scan_before_connect_thread.join();
  }

  // Wait for any HandleConnect()/on-disconnect callback that was already
  // in flight when shutting_down was set above to finish -- only after this
  // returns is it safe to close the adapter and free `connection` out from
  // under gattlib.
  { std::lock_guard<std::recursive_mutex> callback_lock(connection->callback_mutex); }

  if (connection->stream) {
    gattlib_write_char_stream_close(connection->stream);
    connection->stream = nullptr;
  }
  if (connection->conn) {
    gattlib_disconnect(connection->conn, true /* wait_disconnection */);
    connection->conn = nullptr;
  }
  if (connection->adapter) {
    gattlib_adapter_close(connection->adapter);
    connection->adapter = nullptr;
  }

  Mainloop::Instance().Release();
  delete connection;
}

bool Write(Connection* connection, const void* data, size_t size) {
  if (!connection || !connection->conn || !connection->has_write_uuid) {
    return false;
  }
  if (connection->stream) {
    return gattlib_write_char_stream_write(connection->stream, data, size) == GATTLIB_SUCCESS;
  }
  uuid_t uuid = connection->write_uuid;
  return gattlib_write_char_by_uuid(connection->conn, &uuid, data, size) == GATTLIB_SUCCESS;
}

bool WriteCharacteristic(Connection* connection, const uuid128_t& characteristic,
                         const void* data, size_t size) {
  if (!connection || !connection->conn) {
    return false;
  }
  uuid_t uuid = ToGattlibUuid(characteristic);
  return gattlib_write_char_by_uuid(connection->conn, &uuid, data, size) == GATTLIB_SUCCESS;
}

void ReadCharacteristic(Connection* connection, const uuid128_t& service, const uuid128_t& characteristic) {
  if (!connection || !connection->conn) {
    return;
  }
  // Run the blocking D-Bus round-trip on its own thread so callers (device
  // driver threads holding CritSec_Comm) never stall on it.
  std::thread([connection, service, characteristic] {
    if (!connection->conn) {
      return;
    }
    uuid_t uuid = ToGattlibUuid(characteristic);
    void* buffer = nullptr;
    size_t buffer_len = 0;
    if (gattlib_read_char_by_uuid(connection->conn, &uuid, &buffer, &buffer_len) == GATTLIB_SUCCESS) {
      connection->callbacks.on_characteristic_changed(connection->callbacks.user_data, service, characteristic,
                                                       static_cast<const uint8_t*>(buffer), buffer_len);
      gattlib_characteristic_free_value(buffer);
    }
  }).detach();
}

} // namespace gattlib_backend
