#pragma once

namespace codegen {

template<typename...>
inline constexpr bool false_v = false;

template<typename T>
concept Pointer = std::is_pointer_v<typename T::value_type>;

template<typename S>
concept Size = std::is_same_v<typename S::value_type, int32_t> ||
               std::is_same_v<typename S::value_type, int64_t>;

};
