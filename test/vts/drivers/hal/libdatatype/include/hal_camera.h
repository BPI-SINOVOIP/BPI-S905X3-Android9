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

#ifndef __vts_libdatatype_hal_camera_h__
#define __vts_libdatatype_hal_camera_h__

#include <hardware/camera.h>
#include <hardware/camera_common.h>
#include <hardware/hardware.h>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

namespace android {
namespace vts {

// Generates a camera_module_callbacks data structure.
extern camera_module_callbacks_t* GenerateCameraModuleCallbacks();

// Return the pointer to a camera_notify_callback function.
extern camera_notify_callback GenerateCameraNotifyCallback();

// Return the pointer to a camera_data_callback function.
extern camera_data_callback GenerateCameraDataCallback();

// Return the pointer to a camera_data_timestamp_callback function.
extern camera_data_timestamp_callback GenerateCameraDataTimestampCallback();

// Return the pointer to a camera_request_memory function.
extern camera_request_memory GenerateCameraRequestMemory();

// Generates a camera_info data structure.
extern camera_info_t* GenerateCameraInfo();

// Generates a camera_info data structure using a given protobuf msg's values.
extern camera_info_t* GenerateCameraInfoUsingMessage(
    const VariableSpecificationMessage& msg);

// Converts camera_info to a protobuf message.
extern bool ConvertCameraInfoToProtobuf(camera_info_t* raw,
                                        VariableSpecificationMessage* msg);

}  // namespace vts
}  // namespace android

#endif
