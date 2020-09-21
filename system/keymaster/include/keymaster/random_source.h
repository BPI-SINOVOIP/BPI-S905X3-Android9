/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef INCLUDE_KEYMASTER_RANDOM_SOURCE_H_
#define INCLUDE_KEYMASTER_RANDOM_SOURCE_H_

#include <stdint.h>
#include <hardware/keymaster_defs.h>

namespace keymaster {

class RandomSource {
protected:
    virtual ~RandomSource () {}

public:
    /**
     * Generates \p length random bytes, placing them in \p buf.
     */
    virtual keymaster_error_t GenerateRandom(uint8_t* buffer, size_t length) const = 0;
};

} // namespace keymaster

#endif  // INCLUDE_KEYMASTER_RANDOM_SOURCE_H_
