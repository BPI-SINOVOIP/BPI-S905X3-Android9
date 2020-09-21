/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef __VTS_PROTO_FUZZER_TEST_H__
#define __VTS_PROTO_FUZZER_TEST_H__

#include "ProtoFuzzerUtils.h"

namespace android {
namespace vts {

// Fake random number generator used for testing.
class FakeRandom : public Random {
 public:
  FakeRandom() : Random(0) {}
  virtual uint64_t Rand() override { return 0xffffdeadbeef0000; }
  virtual uint64_t operator()(uint64_t n) override { return n - 2; }
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_PROTO_FUZZER_TEST_H__
