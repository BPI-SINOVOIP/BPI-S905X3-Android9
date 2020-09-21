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

#ifndef _ANDROID_NET_LOWPANCHANNELINFOTEST_H_
#define _ANDROID_NET_LOWPANCHANNELINFOTEST_H_

#include <jni.h>
#include <android/net/lowpan/LowpanChannelInfo.h>

extern "C"
JNIEXPORT jbyteArray Java_android_net_lowpan_LowpanChannelInfoTest_readAndWriteNative(JNIEnv* env, jclass,
        jbyteArray inParcel);

#endif  //  _ANDROID_NET_LOWPANCHANNELINFOTEST_H_
