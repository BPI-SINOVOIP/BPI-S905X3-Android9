/*
 * Copyright (C) 2018 Amlogic Corporation.
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
 *
 *  DESCRIPTION:
 *      add a mediasource interface just like stagefright player for the
 *      input port  data feed of OMX decoder.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>

#include "audio_mediasource.h"

extern "C" int read_buffer(unsigned char *buffer,int size);

//#define LOG_TAG "Audio_Medissource"
//#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace android {

AudioMediaSource::AudioMediaSource()	
{   
	ALOGI("%s %d \n",__FUNCTION__,__LINE__);
}


AudioMediaSource::~AudioMediaSource() 
{
	ALOGI("%s %d \n",__FUNCTION__,__LINE__);
}


}  // namespace android

