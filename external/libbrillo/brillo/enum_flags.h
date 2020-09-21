// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_ENUM_FLAGS_H_
#define LIBBRILLO_BRILLO_ENUM_FLAGS_H_

#include <type_traits>

// This is a helper for generating type-safe bitwise operators for flags that
// are defined by an enumeration.  By default, when a bitwise operation is
// performed on two enumerators of an enumeration, the result is the base type
// (int), not a value of the enumeration:
//
// enum SomeEnumOfFlags {
//    ONE = 1,
//    TWO = 2,
//    THREE = 4,
//     // etc.
// };
//
//  SomeEnumOfFlags flags = static_cast<SomeEnumOfFlags>(ONE | TWO);
//
//  By enabling these operators for an enum type:
//
//  DECLARE_FLAGS_ENUM(SomeEnumOfFlags);
//
//  The syntax is simplified to:
//
//  SomeEnumOfFlags flags = ONE | TWO;
//
//  But the following still does not compile without using a cast (as is
//  expected):
//
//  SomeEnumOfFlags flags = ONE | 2;

// This is the macro used to declare that an enum type |ENUM| should have bit-
// wise operators defined for it.
#define DECLARE_FLAGS_ENUM(ENUM) \
template <typename> struct EnumFlagTraitType; \
template <> struct EnumFlagTraitType<ENUM> { using EnumFlagType = ENUM; }; \
EnumFlagTraitType<ENUM> GetEnumFlagTraitType(ENUM) __attribute__((used));


// Setup the templates used to declare that the operators should exist for a
// given type T.

namespace enum_details {

template <typename T>
using FlagEnumTraits = decltype(GetEnumFlagTraitType(std::declval<T>()));

template <typename T>
using Void = void;

template <typename T, typename = void>
struct IsFlagEnum : std::false_type {};

template <typename T>
struct IsFlagEnum<T, Void<typename FlagEnumTraits<T>::EnumFlagType>> : std::true_type {};

}  // namespace enum_details

// The operators themselves, conditional on having been declared that they are
// flag-style enums.

// T operator~(T&)
template <typename T>
constexpr typename std::enable_if<enum_details::IsFlagEnum<T>::value, T>::type
operator~(const T& l) {
  return static_cast<T>( ~static_cast<typename std::underlying_type<T>::type>(l));
}

// T operator|(T&, T&)
template <typename T>
constexpr typename std::enable_if<enum_details::IsFlagEnum<T>::value, T>::type
operator|(const T& l, const T& r) {
  return static_cast<T>(
             static_cast<typename std::underlying_type<T>::type>(l) |
             static_cast<typename std::underlying_type<T>::type>(r));
}

// T operator&(T&, T&)
template <typename T>
constexpr typename std::enable_if<enum_details::IsFlagEnum<T>::value, T>::type
operator&(const T& l, const T& r) {
  return static_cast<T>(
             static_cast<typename std::underlying_type<T>::type>(l) &
             static_cast<typename std::underlying_type<T>::type>(r));
}

// T operator^(T&, T&)
template <typename T>
constexpr typename std::enable_if<enum_details::IsFlagEnum<T>::value, T>::type operator^(
    const T& l, const T& r) {
  return static_cast<T>(static_cast<typename std::underlying_type<T>::type>(l) ^
                        static_cast<typename std::underlying_type<T>::type>(r));
};

// T operator|=(T&, T&)
template <typename T>
constexpr typename std::enable_if<enum_details::IsFlagEnum<T>::value, T>::type operator|=(
    T& l, const T& r) {
  return l = static_cast<T>(
             static_cast<typename std::underlying_type<T>::type>(l) |
             static_cast<typename std::underlying_type<T>::type>(r));
};

// T operator&=(T&, T&)
template <typename T>
constexpr typename std::enable_if<enum_details::IsFlagEnum<T>::value, T>::type operator&=(
    T& l, const T& r) {
  return l = static_cast<T>(
             static_cast<typename std::underlying_type<T>::type>(l) &
             static_cast<typename std::underlying_type<T>::type>(r));
};

// T operator^=(T&, T&)
template <typename T>
constexpr typename std::enable_if<enum_details::IsFlagEnum<T>::value, T>::type operator^=(
    T& l, const T& r) {
  return l = static_cast<T>(
             static_cast<typename std::underlying_type<T>::type>(l) ^
             static_cast<typename std::underlying_type<T>::type>(r));
};

#endif  // LIBBRILLO_BRILLO_ENUM_FLAGS_H_
