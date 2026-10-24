/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceInfo.h
 */
#pragma once

#include <algorithm>
#include <string>
#include <dbus-cxx/path.h>

class DeviceInfo {
 public:
  DeviceInfo() = delete;

  DeviceInfo(const DeviceInfo&) = default;
  DeviceInfo& operator=(const DeviceInfo&) = default;

  DeviceInfo(DeviceInfo&&) = default;
  DeviceInfo& operator=(DeviceInfo&&) = default;

  DeviceInfo(DBus::Path path, std::string address) : path_(std::move(path)), address_(std::move(address)) {}

  const DBus::Path& path() const {
    return path_;
  }

  const std::string& address() const {
    return address_;
  }
  const std::string& name() const {
    if (!alias_.empty()) {
      return alias_;
    }
    if (!name_.empty()) {
      return name_;
    }
    return address();
  }


  bool connected() const {
    return connected_;
  }
  bool paired() const {
    return paired_;
  }
  bool trusted() const {
    return trusted_;
  }
  bool services_resolved() const {
    return services_resolved_;
  }
  bool spp_profile() const {
    return spp_profile_;
  }
  bool is_ble() const {
    return is_ble_;
  }

  void name(std::string name) {
    if (!name.empty() && name != AddressAsAlias()) {
      name_ = std::move(name);
    }
  }

  void alias(std::string alias) {
    if (!alias.empty() && alias != AddressAsAlias()) {
      alias_ = std::move(alias);
    }
    else {
      alias_.clear();
    }
  }

  void connected(bool connected) {
    connected_ = connected;
  }
  void paired(bool paired) {
    paired_ = paired;
  }
  void trusted(bool trusted) {
    trusted_ = trusted;
  }
  void services_resolved(bool services_resolved) {
    services_resolved_ = services_resolved;
  }

  void spp_profile(bool spp_profile) {
    spp_profile_ = spp_profile;
  }
  void is_ble(bool is_ble) {
    is_ble_ = is_ble;
  }

  // When a device has no name, BlueZ sets Alias to the address with ':'
  // replaced by '-'. That is not a name worth showing.
  std::string AddressAsAlias() const {
    std::string address = address_;
    std::replace(address.begin(), address.end(), ':', '-');
    return address;
  }

 private:
  DBus::Path path_;

  std::string address_;
  std::string name_;
  std::string alias_;

  bool connected_ = false;
  bool paired_ = false;
  bool trusted_ = false;
  bool services_resolved_ = false;
  bool spp_profile_ = false;
  bool is_ble_ = false;
};