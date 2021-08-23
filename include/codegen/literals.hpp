#pragma once

#include "module_builder.hpp"

namespace codegen::literals {

inline value<int8_t> operator""_i8(unsigned long long v) {
  return constant<int8_t>(v);
}

inline value<int16_t> operator""_i16(unsigned long long v) {
  return constant<int16_t>(v);
}

inline value<int32_t> operator""_i32(unsigned long long v) {
  return constant<int32_t>(v);
}

inline value<int64_t> operator""_i64(unsigned long long v) {
  return constant<int64_t>(v);
}

inline value<uint8_t> operator""_u8(unsigned long long v) {
  return constant<uint8_t>(v);
}

inline value<uint16_t> operator""_u16(unsigned long long v) {
  return constant<uint16_t>(v);
}

inline value<uint32_t> operator""_u32(unsigned long long v) {
  return constant<uint32_t>(v);
}

inline value<uint64_t> operator""_u64(unsigned long long v) {
  return constant<uint64_t>(v);
}

inline value<float> operator""_f32(long double v) {
  return constant<float>(v);
}

inline value<double> operator""_f64(long double v) {
  return constant<double>(v);
}

} // namespace codegen::literals
