/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef EXAMPLE_CAMERA_H_
#define EXAMPLE_CAMERA_H_

#include <system/camera_metadata.h>
#include "Camera.h"

namespace default_camera_hal {
// ExampleCamera is an example for a specific camera device. The Camera object
// contains all logic common between all cameras (e.g. front and back cameras),
// while a specific camera device (e.g. ExampleCamera) holds all specific
// metadata and logic about that device.
class ExampleCamera : public Camera {
    public:
        explicit ExampleCamera(int id);
        ~ExampleCamera();

    private:
        // Initialize static camera characteristics for individual device
        camera_metadata_t *initStaticInfo();
        // Initialize whole device (templates/etc) when opened
        int initDevice();
        // Initialize each template metadata controls
        int setPreviewTemplate(Metadata m);
        int setStillTemplate(Metadata m);
        int setRecordTemplate(Metadata m);
        int setSnapshotTemplate(Metadata m);
        int setZslTemplate(Metadata m);
        // Verify settings are valid for a capture with this device
        bool isValidCaptureSettings(const camera_metadata_t* settings);
};
} // namespace default_camera_hal

#endif // CAMERA_H_
