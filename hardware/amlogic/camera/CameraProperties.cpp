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

//#define LOG_NDEBUG 0
#define LOG_TAG "CameraProperties       "

//#include "CameraHal.h"
#include <utils/threads.h>

#include "DebugUtils.h"
#include "CameraProperties.h"

#define CAMERA_ROOT         "CameraRoot"
#define CAMERA_INSTANCE     "CameraInstance"

namespace android {

extern "C" int CameraAdapter_CameraNum();
extern "C" void loadCaps(int camera_id, CameraProperties::Properties* params);

/*********************************************************
 CameraProperties - public function implemetation
**********************************************************/

CameraProperties::CameraProperties() : mCamerasSupported(0)
{
    LOG_FUNCTION_NAME;

    mCamerasSupported = 0;
    mInitialized = 0;

    LOG_FUNCTION_NAME_EXIT;
}

CameraProperties::~CameraProperties()
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;
}

// Initializes the CameraProperties class
status_t CameraProperties::initialize(int cameraid)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;

    Mutex::Autolock lock(mLock);

    CAMHAL_LOGDB("%s, mCamerasSupported=%d\n",
            mInitialized?"initialized":"no initialize", mCamerasSupported);

    if( !mInitialized ){

        int temp = CameraAdapter_CameraNum();
        for ( int i = 0; i < temp; i++) {
            mInitialized |= (1 << cameraid);
            mCamerasSupported ++;
            mCameraProps[i].set(CameraProperties::CAMERA_SENSOR_INDEX, i);
            loadCaps(i, &mCameraProps[i]);
            mCameraProps[i].dump();
        }

    }else{

        if(!strcmp( mCameraProps[cameraid].get(CameraProperties::RELOAD_WHEN_OPEN), "1")){
            CAMHAL_LOGDB("cameraid %d reload\n", cameraid);
            loadCaps(cameraid, &mCameraProps[cameraid]);
        }else{
            CAMHAL_LOGDA("device don't need reload\n"); 
        }

    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;

}

extern "C" int CameraAdapter_Capabilities(CameraProperties::Properties* properties_array,
                                          const unsigned int starting_camera,
                                          const unsigned int camera_num);

///Loads all the Camera related properties
status_t CameraProperties::loadProperties()
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    CAMHAL_LOGDA("this func delete!!!\n"); 
    return ret;
}

// Returns the number of Cameras found
int CameraProperties::camerasSupported()
{
    LOG_FUNCTION_NAME;
    return mCamerasSupported;
}

};
