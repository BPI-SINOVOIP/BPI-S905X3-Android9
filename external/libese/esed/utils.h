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

#ifndef ESED_PN81A_UTILS_H_
#define ESED_PN81A_UTILS_H_

#include <functional>
#include <string>

#include <hidl/Status.h>

#include <apdu/apdu.h>

#include "pn81a.h"

namespace android {
namespace esed {
namespace pn81a {

// HIDL
using ::android::hardware::Status;


// libapdu
using ::android::CommandApdu;
using ResponseApdu = ::android::ResponseApdu<const std::vector<uint8_t>>;

/**
 * Reads a 32-bit integer from an iterator.
 * @param first The first position to read from.
 * @return A tuple of the value read and an iterator to one after the last position read from.
 */
template<typename InputIt>
std::tuple<uint32_t, InputIt> readBigEndianInt32(InputIt first) {
    uint32_t ret;
    auto it = first;
    ret  = *it++ << 24;
    ret |= *it++ << 16;
    ret |= *it++ <<  8;
    ret |= *it++;
    return {ret, it};
}

/**
 * Writes a 32-bit integer to the iterator in big-endian format.
 * @param value The value to write.
 * @param first The first position in the iterator.
 * @return An iterator to one after the last position written to.
 */
template<typename OutputIt>
OutputIt writeBigEndian(const uint32_t value, OutputIt first) {
    auto it = first;
    *it++ = 0xff & (value >> 24);
    *it++ = 0xff & (value >> 16);
    *it++ = 0xff & (value >>  8);
    *it++ = 0xff & value;
    return it;
}

/**
 * Transceive a command with the eSE and perform common error checking. When the
 * handler is called, it has been checked that the transmission to and reception
 * from the eSE was successful and that the response is in a valid format.
 */
template<typename T, T OK, T FAILED>
T transceive(::android::esed::EseInterface& ese, const CommandApdu& command,
                  std::function<T(const ResponseApdu&)> handler = {}) {
    // +1 for max size of extended response, +2 for status bytes
    constexpr size_t MAX_RESPONSE_SIZE = std::numeric_limits<uint16_t>::max() + 1 + 2;
    std::vector<uint8_t> responseBuffer(MAX_RESPONSE_SIZE);
    const int ret = ese.transceive(command.vector(), responseBuffer);

    // Check eSE communication was successful
    if (ret < 0) {
        std::string errMsg = "Failed to transceive data between AP and eSE";
        if (ese.error()) {
            errMsg += " (" + std::to_string(ese.error_code()) + "): " + ese.error_message();
        } else {
            errMsg += ": reason unknown";
        }
        LOG(ERROR) << errMsg;
        return FAILED;
    }
    const size_t recvd = static_cast<size_t>(ret);

    // Need to recalculate the maximum response size if this fails
    if (recvd > MAX_RESPONSE_SIZE) {
        LOG(ERROR) << "eSE response was longer than the buffer, check the buffer size.";
        return FAILED;
    }
    responseBuffer.resize(recvd);

    // Check for ISO 7816-4 APDU response format errors
    ResponseApdu apdu{responseBuffer};
    if (!apdu.ok()) {
        LOG(ERROR) << "eSE response was invalid.";
        return FAILED;
    }

    // Call handler if one was provided
    if (handler) {
        return handler(apdu);
    }

    return OK;
}

/**
 * Checks that the amount of data in the response APDU matches the expected
 * value.
 */
template<typename T, T OK, T FAILED>
T checkLength(const ResponseApdu& apdu, const size_t size) {
    if (apdu.dataSize() != size) {
        LOG(ERROR) << "eSE response was the wrong length.";
        return FAILED;
    }
    return OK;
}

/**
 * Checks that the response APDU does no encode an error and that the amount of
 * data matches the expected value.
 */
template<typename T, T OK, T FAILED>
T checkNoErrorAndLength(const ResponseApdu& apdu, const size_t size) {
    if (apdu.isError()) {
        LOG(ERROR) << "eSE operation failed";
        return FAILED;
    }
    return checkLength<T, OK, FAILED>(apdu, size);
}

} // namespace android
} // namespace esed
} // namespace pn81a

#endif // ESED_PN81A_UTILS_H_
