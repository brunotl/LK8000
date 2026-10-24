/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   VariantToString.h
 */
#pragma once

#include <dbus-cxx.h>  // or <dbus-cxx/variant.h>

#include <exception>
#include <format>
#include <string>

namespace bluez {

using DBus::priv::VariantIterator;

namespace detail {

// Walks the variant with dbus-cxx's VariantIterator and appends a textual form
// of the value it currently points at to `out`.
//
// Iterator rules (from variantiterator.cpp):
//  - get_*() consumes the current value but does NOT advance the signature, so
//    callers of struct / dict-entry iterators must call next() afterwards.
//  - Array sub-iterators stay valid while there is data left; next() is a no-op
//    for them, so calling it is harmless.
inline void AppendValue(std::string& out, VariantIterator& it, bool nested) {
  switch (it.arg_type()) {
    case DBus::DataType::BOOLEAN:
      out += it.get_bool() ? "true" : "false";
      break;
    case DBus::DataType::BYTE:
      out += std::to_string(it.get_uint8());
      break;
    case DBus::DataType::INT16:
      out += std::to_string(it.get_int16());
      break;
    case DBus::DataType::UINT16:
      out += std::to_string(it.get_uint16());
      break;
    case DBus::DataType::INT32:
      out += std::to_string(it.get_int32());
      break;
    case DBus::DataType::UINT32:
      out += std::to_string(it.get_uint32());
      break;
    case DBus::DataType::INT64:
      out += std::to_string(it.get_int64());
      break;
    case DBus::DataType::UINT64:
      out += std::to_string(it.get_uint64());
      break;
    case DBus::DataType::DOUBLE:
      out += std::to_string(it.get_double());
      break;

    case DBus::DataType::STRING:
    case DBus::DataType::OBJECT_PATH:
    case DBus::DataType::SIGNATURE: {
      // get_string() handles all three. Quote only when nested so top-level
      // output stays identical to the old behaviour.
      std::string s = it.get_string();
      out += nested ? std::format("\"{}\"", s) : s;
      break;
    }

    case DBus::DataType::VARIANT: {
      DBus::Variant inner = it.get_variant();
      VariantIterator inner_it(&inner);
      out += "variant(";
      AppendValue(out, inner_it, true);
      out += ")";
      break;
    }

    case DBus::DataType::ARRAY: {
      const bool is_dict = it.is_dict();
      const bool is_bytes = it.element_type() == DBus::DataType::BYTE;
      VariantIterator sub = it.recurse();
      bool first = true;

      out += is_dict ? "{" : (is_bytes ? "bytes[" : "[");
      while (sub.is_valid()) {
        if (!first) {
          out += is_bytes ? " " : ", ";
        }
        first = false;

        if (is_dict) {
          // Each element is a DICT_ENTRY: recurse once more to get key + value.
          VariantIterator entry = sub.recurse();
          AppendValue(out, entry, true);
          entry.next();
          out += ": ";
          AppendValue(out, entry, true);
          entry.next();
        }
        else if (is_bytes) {
          out += std::format("{:02x}", sub.get_uint8());
        }
        else {
          AppendValue(out, sub, true);
        }
        sub.next();
      }
      out += is_dict ? "}" : "]";
      break;
    }

    case DBus::DataType::STRUCT: {
      VariantIterator sub = it.recurse();
      bool first = true;

      out += "(";
      while (sub.is_valid()) {
        if (!first) {
          out += ", ";
        }
        first = false;
        AppendValue(out, sub, true);
        sub.next();
      }
      out += ")";
      break;
    }

    case DBus::DataType::INVALID:
      out += "<invalid>";
      break;

    default:
      out += std::format("<{}>", static_cast<char>(it.arg_type()));
      break;
  }
}

}  // namespace detail

// Convert a DBus::Variant to a string for logging purposes.
// Handles arrays (vector), dicts (map), structs (tuple) and nested variants.
//
//   as          -> ["a", "b"]
//   a{sv}       -> {"Name": variant("hci0"), "Powered": variant(true)}
//   (si)        -> ("x", 3)
//   aay         -> [bytes[01 02] bytes[03]]  (byte arrays print as hex)
//   v(as)       -> variant(["a"])
inline std::string ToString(const DBus::Variant& value) {
  if (value.type() == DBus::DataType::INVALID) {
    return "<invalid>";
  }
  try {
    VariantIterator it(&value);
    std::string out;
    detail::AppendValue(out, it, /*nested=*/false);
    return out;
  }
  catch (const std::exception& e) {
    // This is only used for logging, so never let it throw.
    return std::format("<error: {}>", e.what());
  }
}

}  // namespace bluez
