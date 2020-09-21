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
#include <utils/threads.h>

#include "DebugUtils.h"
#include "CameraProperties.h"

#define CAMERA_ROOT         "CameraRoot"
#define CAMERA_INSTANCE     "CameraInstance"

namespace android {

extern "C" void loadCaps(int camera_id, CameraProperties::Properties* params)
{}

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

// Returns the properties class for a specific Camera
// Each value is indexed by the CameraProperties::CameraPropertyIndex enum
int CameraProperties::getProperties(int cameraIndex, CameraProperties::Properties** properties)
{
    LOG_FUNCTION_NAME;

    if((unsigned int)cameraIndex >= mCamerasSupported)
    {
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    *properties = mCameraProps+cameraIndex;

    LOG_FUNCTION_NAME_EXIT;
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////CameraProperties::Properties function/////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ssize_t CameraProperties::Properties::set(const char *prop, const char *value)
{
    if(!prop)
        return -EINVAL;
    if(!value)
        value = DEFAULT_VALUE;

    return mProperties->replaceValueFor(String8(prop), String8(value));
}

ssize_t CameraProperties::Properties::set(const char *prop, int value)
{
    char s_val[30];

    sprintf(s_val, "%d", value);

    return set(prop, s_val);
}

const char* CameraProperties::Properties::get(const char * prop)
{
    String8 value = mProperties->valueFor(String8(prop));
    return value.string();
}

void CameraProperties::Properties::dump()
{
    for (size_t i = 0; i < mProperties->size(); i++)
    {
        CAMHAL_LOGVB("%s = %s\n",
                        mProperties->keyAt(i).string(),
                        mProperties->valueAt(i).string());
    }
}

const char* CameraProperties::Properties::keyAt(unsigned int index)
{
    if(index < mProperties->size())
    {
        return mProperties->keyAt(index).string();
    }
    return NULL;
}

const char* CameraProperties::Properties::valueAt(unsigned int index)
{
    if(index < mProperties->size())
    {
        return mProperties->valueAt(index).string();
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
///////////CameraProperties::const char initialized///////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
const char CameraProperties::PIXEL_FORMAT_RGB24[] = "rgb24";
const char CameraProperties::RELOAD_WHEN_OPEN[]="prop-reload-key";
const char CameraProperties::DEVICE_NAME[] = "device_name";

const char CameraProperties::DEFAULT_VALUE[] = "";
const char CameraProperties::PARAMS_DELIMITER []= ",";
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
///////////CameraProperties::const char initialize finished///////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

};
