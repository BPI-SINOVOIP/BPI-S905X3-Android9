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

// Mock for state delegate interfaces.

#ifndef V4L2_CAMERA_HAL_METADATA_STATE_DELEGATE_INTERFACE_MOCK_H_
#define V4L2_CAMERA_HAL_METADATA_STATE_DELEGATE_INTERFACE_MOCK_H_

#include <gmock/gmock.h>

#include "state_delegate_interface.h"

namespace v4l2_camera_hal {

template <typename T>
class StateDelegateInterfaceMock : public StateDelegateInterface<T> {
 public:
  StateDelegateInterfaceMock(){};
  MOCK_METHOD1_T(GetValue, int(T*));
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_CONTROL_DELEGATE_INTERFACE_MOCK_H_
