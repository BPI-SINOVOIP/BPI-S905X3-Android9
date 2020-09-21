/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NETUTILS_STATUSOR_H
#define NETUTILS_STATUSOR_H

#include <cassert>
#include "netdutils/Status.h"

namespace android {
namespace netdutils {

// Wrapper around a combination of Status and application value type.
// T may be any copyable or movable type.
template <typename T>
class StatusOr {
  public:
    StatusOr() = default;
    StatusOr(const Status status) : mStatus(status) { assert(!isOk(status)); }
    StatusOr(const T& value) : mStatus(status::ok), mValue(value) {}
    StatusOr(T&& value) : mStatus(status::ok), mValue(std::move(value)) {}

    // Move constructor ok (if T supports move)
    StatusOr(StatusOr&&) = default;
    // Move assignment ok (if T supports move)
    StatusOr& operator=(StatusOr&&) = default;
    // Copy constructor ok (if T supports copy)
    StatusOr(const StatusOr&) = default;
    // Copy assignment ok (if T supports copy)
    StatusOr& operator=(const StatusOr&) = default;

    // Return const references to wrapped type
    // It is an error to call value() when !isOk(status())
    const T& value() const & { return mValue; }
    const T&& value() const && { return mValue; }

    // Return rvalue references to wrapped type
    // It is an error to call value() when !isOk(status())
    T& value() & { return mValue; }
    T&& value() && { return mValue; }

    // Return status assigned in constructor
    const Status status() const { return mStatus; }

    // Implict cast to Status
    operator Status() const { return status(); }

  private:
    Status mStatus = status::undefined;
    T mValue;
};

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const StatusOr<T>& s) {
    return os << "StatusOr[status: " << s.status() << "]";
}

#define ASSIGN_OR_RETURN_IMPL(tmp, lhs, stmt) \
    auto tmp = (stmt);                        \
    RETURN_IF_NOT_OK(tmp);                    \
    lhs = std::move(tmp.value());

#define ASSIGN_OR_RETURN_CONCAT(line, lhs, stmt) \
    ASSIGN_OR_RETURN_IMPL(__CONCAT(_status_or_, line), lhs, stmt)

// Macro to allow exception-like handling of error return values.
//
// If the evaluation of stmt results in an error, return that error
// from the current function. Otherwise, assign the result to lhs.
//
// This macro supports both move and copy assignment operators. lhs
// may be either a new local variable or an existing non-const
// variable accessible in the current scope.
//
// Example usage:
// StatusOr<MyType> foo() { ... }
//
// ASSIGN_OR_RETURN(auto myVar, foo());
// ASSIGN_OR_RETURN(myExistingVar, foo());
// ASSIGN_OR_RETURN(myMemberVar, foo());
#define ASSIGN_OR_RETURN(lhs, stmt) ASSIGN_OR_RETURN_CONCAT(__LINE__, lhs, stmt)

}  // namespace netdutils
}  // namespace android

#endif /* NETUTILS_STATUSOR_H */
