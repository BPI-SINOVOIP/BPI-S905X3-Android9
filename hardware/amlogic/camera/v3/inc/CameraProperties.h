/*
 * Copyright (C) 2011 The Android Open Source Project
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




#ifndef CAMERA_PROPERTIES_H
#define CAMERA_PROPERTIES_H

#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <cutils/properties.h>

namespace android {

#define EXIF_MAKE_DEFAULT "default_make"
#define EXIF_MODEL_DEFAULT "default_model"

// Class that handles the Camera Properties
class CameraProperties
{
public:
    static const char PIXEL_FORMAT_RGB24[];
    static const char RELOAD_WHEN_OPEN[];
    static const char DEVICE_NAME[];

    static const char DEFAULT_VALUE[];
    static const char PARAMS_DELIMITER[];
    CameraProperties();
    ~CameraProperties();

    // container class passed around for accessing properties
    class Properties
    {
        public:
            Properties()
            {
                mProperties = new DefaultKeyedVector<String8, String8>(String8(DEFAULT_VALUE));
                char property[PROPERTY_VALUE_MAX];
                property_get("ro.product.manufacturer", property, EXIF_MAKE_DEFAULT);
                property[0] = toupper(property[0]);
                set(EXIF_MAKE, property);
                property_get("ro.product.model", property, EXIF_MODEL_DEFAULT);
                property[0] = toupper(property[0]);
                set(EXIF_MODEL, property);
            }
            ~Properties()
            {
                delete mProperties;
            }
            ssize_t set(const char *prop, const char *value);
            ssize_t set(const char *prop, int value);
            const char* get(const char * prop);
            void dump();

        protected:
            const char* keyAt(unsigned int);
            const char* valueAt(unsigned int);

        private:
            DefaultKeyedVector<String8, String8>* mProperties;

    };

    ///Initializes the CameraProperties class
    status_t initialize(int cameraid);
    status_t loadProperties();
    int camerasSupported();
    int getProperties(int cameraIndex, Properties** properties);

private:

    uint32_t mCamerasSupported;
    int mInitialized;
    mutable Mutex mLock;

    Properties mCameraProps[MAX_CAM_NUM_ADD_VCAM];

};

};

#endif //CAMERA_PROPERTIES_H
