/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   Common.h
 *
 * Definitions and helpers shared by BluezGattSensor and BluetoothLeScanner
 * (dbus-cxx 2.6.x).
 */

#pragma once

#include "options.h"

#include <dbus-cxx.h>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "utils/uuid.h"
#include "MessageLog.h"

namespace bluez {

using namespace std::string_literals;

#ifdef DEBUG_BLUETOOTH
  #define BlueZDebugLog(...) DebugLog(_T("[BlueZ]") __VA_ARGS__)
#else
  #define BlueZDebugLog(...)  // Does nothing
#endif
// ---------------------------------------------------------------------------
// Bus names, interfaces and properties
// ---------------------------------------------------------------------------

inline const std::string kBluezServiceName{"org.bluez"};

inline const std::string kObjectManagerInterface{"org.freedesktop.DBus.ObjectManager"};
inline const std::string kAdapterInterface{"org.bluez.Adapter1"};
inline const std::string kDeviceInterface{"org.bluez.Device1"};
inline const std::string kGattServiceInterface{"org.bluez.GattService1"};
inline const std::string kGattCharacteristicInterface{"org.bluez.GattCharacteristic1"};

// ---------------------------------------------------------------------------
// D-Bus types
// ---------------------------------------------------------------------------

// a{sv}
using property_map = std::map<std::string, DBus::Variant>;
// a{sa{sv}}
using interface_map = std::map<std::string, property_map>;
// a{oa{sa{sv}}}  (return type of ObjectManager.GetManagedObjects)
using managed_objects = std::map<DBus::Path, interface_map>;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// DBus::Path derives from std::string, so this works for object paths too.
inline bool StartsWith(const std::string_view& s,
                       const std::string_view& prefix) {
  return s.starts_with(prefix);
}

// Property as string. Throws if it is missing or not a string.
inline std::string GetString(const property_map& props,
                             const std::string& key) {
  return props.at(key).to_string();
}

// Property as string, or nothing if it is missing or not a string.
inline std::optional<std::string> TryGetString(const property_map& props,
                                               const std::string& key) {
  std::optional<std::string> value;
  auto it = props.find(key);
  if (it != props.end()) {
    try {
      value = it->second.to_string();
    }
    catch (const std::exception&) {
      // ignore exceptions and return std::nullopt
    }
  }
  return value;
}

// Property as array of strings. Throws if it is missing or has another type.
// (DBus::Variant::to_vector<T>() is not const, so work on a copy.)
inline std::vector<std::string> GetStringVector(const property_map& props,
                                                const std::string& key) {
  DBus::Variant v = props.at(key);
  return v.to_vector<std::string>();
}

inline uuid_t GetUUID(const property_map& props) {
  return uuid_t(GetString(props, "UUID"s));
}

inline std::vector<std::string> GetFlags(const property_map& props) {
  return GetStringVector(props, "Flags"s);
}

inline std::optional<uint16_t> GetMTU(const property_map& props) {
  try {
    return static_cast<uint16_t>(props.at("MTU").to_uint16());
  }
  catch (const std::exception&) {
    // ignore exceptions and return std::nullopt
  }
  return std::nullopt;
}

inline bool HasSppProfile(const DBus::Variant& Value) {
  if (Value.signature() != "as") {
    return false;
  }
  auto v = Value; // copy required because to_vector is not const
  auto uuids = v.to_vector<std::string>();
  return std::ranges::any_of(uuids, [](const std::string& uuid) {
    return uuid_t(uuid) == "00001101-0000-1000-8000-00805F9B34FB";
  });
}

template<typename... Flags>
bool CheckFlags(const std::vector<std::string>& flags, const Flags&... flag) {
  return std::ranges::any_of(flags, [&](const auto& f) {
    return ( (f == flag) || ... );
  });
}

// ---------------------------------------------------------------------------
// BlueZ object lookup
// ---------------------------------------------------------------------------

inline managed_objects GetManagedObjects(
    const std::shared_ptr<DBus::ObjectProxy>& proxy) {
  auto method = proxy->create_method<managed_objects()>(kObjectManagerInterface,
                                                        "GetManagedObjects");

  if (!method) {
    throw std::runtime_error("Unable to create GetManagedObjects proxy");
  }

  return (*method)();
}

// ObjectManager.GetManagedObjects() on the BlueZ root object.
// Blocking call: don't hold a lock the dispatcher thread may need.
inline managed_objects GetManagedObjects(
    const std::shared_ptr<DBus::Connection>& connection) {
  if (!connection) {
    throw std::runtime_error("No D-Bus connection");
  }
  // The Connection only keeps a weak reference to the ObjectProxy: a
  // temporary one is fine for a single call.
  auto proxy = connection->create_object_proxy(kBluezServiceName, "/");

  return GetManagedObjects(proxy);
}

// First Bluetooth adapter (e.g. "/org/bluez/hci0"), empty if there is none.
inline DBus::Path FindAdapterPath(const managed_objects& objects) {
  auto it = std::ranges::find_if(objects, [](const auto& value) {
    const auto& [path, interfaces] = value;
    return interfaces.count(kAdapterInterface) > 0;
  });

  DBus::Path result;
  if (it != objects.end()) {
    result = it->first;
  }
  return result;
}

}  // namespace bluez
