#pragma once

#include "module_builder.hpp"

namespace codegen::literals {

inline value operator""_i8(unsigned long long v) {
  return constant<int8_t>(v);
}

inline value operator""_i16(unsigned long long v) {
  return constant<int16_t>(v);
}

inline value operator""_i32(unsigned long long v) {
  return constant<int32_t>(v);
}

inline value operator""_i64(unsigned long long v) {
  return constant<int64_t>(v);
}

inline value operator""_u8(unsigned long long v) {
  return constant<uint8_t>(v);
}

inline value operator""_u16(unsigned long long v) {
  return constant<uint16_t>(v);
}

inline value operator""_u32(unsigned long long v) {
  return constant<uint32_t>(v);
}

inline value operator""_u64(unsigned long long v) {
  return constant<uint64_t>(v);
}

inline value operator""_f32(long double v) {
  return constant<float>(v);
}

/*
inline value operator""_f64(long double v) {
  return constant<double>(v);
}
*/

} // namespace codegen::literals
