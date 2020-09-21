/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef KEYSTORE_KEYSTORE_ATTESTATION_ID_H_
#define KEYSTORE_KEYSTORE_ATTESTATION_ID_H_

#include <utils/Errors.h>
#include <vector>

namespace android {
namespace security {

template <typename T> class StatusOr {
  public:
    StatusOr(const status_t error) : _status(error), _value() {}
    StatusOr(const T& value) : _status(NO_ERROR), _value(value) {}
    StatusOr(T&& value) : _status(NO_ERROR), _value(value) {}

    operator const T&() const { return _value; }
    operator T&() { return _value; }
    operator T &&() && { return std::move(_value); }

    bool isOk() const { return NO_ERROR == _status; }

    ::android::status_t status() const { return _status; }

    const T& value() const & { return _value; }
    T& value() & { return _value; }
    T&& value() && { return std::move(_value); }

  private:
    ::android::status_t _status;
    T _value;
};

/**
 * Gathers the attestation id for the application determined by uid by querying the package manager
 * As of this writing uids can be shared in android, which is why the asn.1 encoded attestation
 * application id may contain more than one package info followed by a set of digests of the
 * packages signing certificates.
 *
 * @returns the asn.1 encoded attestation application id or an error code. Check the result with
 *          .isOk() before accessing.
 */
StatusOr<std::vector<uint8_t>> gather_attestation_application_id(uid_t uid);

}  // namespace security
}  // namespace android
#endif  // KEYSTORE_KEYSTORE_ATTESTATION_ID_H_
