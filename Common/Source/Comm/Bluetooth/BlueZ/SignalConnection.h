/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   SignalConnection.h
 */
#pragma once

#include <dbus-cxx.h>
#include <memory>
#include <string>
#include <functional>

/**
 * A helper class to manage DBus signal connections.
 * It automatically connects to the signal and disconnects upon destruction.
 *
 * This is key for managing the lifetime of DBus signal connections safely.
 *
 * if 'connection' is not explicitly disconnected, signal handler may still be
 * invoked by DBus dispatcher even after the `proxy` is destroyed, this is the
 * main source of deadlocks and undefined behavior.
 */
template <class TypeT>
class SignalConnection final {
 public:
  SignalConnection() = default;

  SignalConnection(const SignalConnection<TypeT>&) = delete;
  SignalConnection& operator=(const SignalConnection<TypeT>&) = delete;

  SignalConnection(SignalConnection<TypeT>&& other) noexcept {
    signal = std::exchange(other.signal, {});
    connection = std::exchange(other.connection, {});
  }

  SignalConnection& operator=(SignalConnection<TypeT>&& other) noexcept {
    if (this != &other) {
      // Disconnect the current connection before taking ownership of the new
      // one otherwise the old connection may still be active and cause
      // undefined behavior.
      connection.disconnect();

      signal = std::exchange(other.signal, {});
      connection = std::exchange(other.connection, {});
    }
    return *this;
  }

  SignalConnection(const std::shared_ptr<DBus::ObjectProxy>& proxy,
                   const std::string& interface_name,
                   const std::string& signal_name,
                   std::function<TypeT>&& slot) {
    signal = proxy->create_signal<TypeT>(interface_name, signal_name);
    connection = signal->connect(std::forward<std::function<TypeT>>(slot));
  }

  ~SignalConnection() {
    disconnect();
  }

  void disconnect() {
    // sigc::connection must be explicitly disconnected since dtor won't do it
    connection.disconnect();
    signal.reset();
  }

 private:
  std::shared_ptr<DBus::SignalProxy<TypeT>> signal;
  sigc::connection connection;
};
