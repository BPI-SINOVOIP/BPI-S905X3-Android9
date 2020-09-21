/*
add a mediasource interface just like stagefright player
for the input port  data feed of OMX decoder
author: jian.xu@amlogic.com
27/2/2013
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>

#include "audio_mediasource.h"

extern "C" int read_buffer(unsigned char *buffer, int size);

//#define LOG_TAG "Audio_Medissource"
//#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace android
{

AudioMediaSource::AudioMediaSource()
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
}


AudioMediaSource::~AudioMediaSource()
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
}


}  // namespace android

