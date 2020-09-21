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

#include <cstdint>

#include <gtest/gtest.h>

#include "netdutils/Status.h"
#include "netdutils/StatusOr.h"

namespace android {
namespace netdutils {

TEST(StatusTest, smoke) {
    // Expect the following lines to compile
    Status status1(1);
    Status status2(status1);
    Status status3 = status1;
    const Status status4(8);
    const Status status5(status4);
    const Status status6 = status4;

    // Default constructor
    EXPECT_EQ(status::ok, Status());
}

TEST(StatusOrTest, ostream) {
    {
      StatusOr<int> so(11);
      std::stringstream ss;
      ss << so;
      EXPECT_EQ("StatusOr[status: Status[code: 0, msg: ], value: 11]", ss.str());
    }
    {
      StatusOr<int> err(status::undefined);
      std::stringstream ss;
      ss << err;
      EXPECT_EQ("StatusOr[status: Status[code: 2147483647, msg: undefined]]", ss.str());
    }
}

}  // namespace netdutils
}  // namespace android
