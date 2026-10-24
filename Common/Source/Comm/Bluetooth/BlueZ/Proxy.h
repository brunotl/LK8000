/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   Proxy.h
 */
#pragma once

#include "options.h"
#include <format>
#include "Common.h"


/**
 * @brief Base class for D-Bus object proxies.
 *
 * The Proxy class provides a common interface for creating and managing
 * D-Bus object proxies, method proxies, and property proxies. It handles
 * the underlying D-Bus connection and object proxy creation.
 */
class Proxy {
 public:
  Proxy() = delete;
  virtual ~Proxy() = default;

  Proxy(const Proxy&) = delete;
  Proxy& operator=(const Proxy&) = delete;

  Proxy(Proxy&&) = default;
  Proxy& operator=(Proxy&&) = default;

  explicit Proxy(std::shared_ptr<DBus::ObjectProxy> proxy)
      : proxy_(std::move(proxy)) {}

  Proxy(const std::shared_ptr<DBus::Connection>& connection,
        const std::string& destination,
        const std::string& path)
      : Proxy(CreateProxy(connection, destination, path)) {}

  std::shared_ptr<DBus::Connection> GetConnection() const {
    return proxy_ ? proxy_->connection().lock() : nullptr;
  }

  operator const std::shared_ptr<DBus::ObjectProxy>&() const {
    return proxy_;
  }

 protected:

  static
  std::shared_ptr<DBus::ObjectProxy> CreateProxy(
      const std::shared_ptr<DBus::Connection>& connection,
      const std::string& destination, const std::string& path) {
    return connection->create_object_proxy(destination, path);
  }

  template <typename Signature>
  [[nodiscard]]
  std::shared_ptr<DBus::MethodProxy<Signature>>
  CreateMethod(const std::string& interface_name, const std::string& method_name) {
    auto method = proxy_->create_method<Signature>(interface_name, method_name);
    if (!method) {
      auto error_message = std::format(R"(Failed to create method: "{}.{}")",
                                       interface_name, method_name);
      throw std::runtime_error(error_message);
    }
    return method;
  }

  template <typename TypeT>
  [[nodiscard]]
  std::shared_ptr<DBus::PropertyProxy<TypeT>>
  CreateProperty(const std::string& interface_name, const std::string& property_name) const {
    auto property =
        proxy_->create_property<TypeT>(interface_name, property_name);
    if (!property) {
      auto error_message = std::format(R"(Failed to create property: "{}.{}")",
                                       interface_name, property_name);
      throw std::runtime_error(error_message);
    }
    return property;
  }

 private:
  std::shared_ptr<DBus::ObjectProxy> proxy_;
};
