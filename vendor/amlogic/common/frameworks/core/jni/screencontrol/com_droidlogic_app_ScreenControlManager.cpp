/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "screencontrol-jni"
#include "com_droidlogic_app_ScreenControlManager.h"

static sp<ScreenControlClient> spScreenCtrl = NULL;

static sp<ScreenControlClient>& getScreenControlClient()
{
    if (spScreenCtrl == NULL)
        spScreenCtrl = new ScreenControlClient();
    return spScreenCtrl;
}

static void ConnectScreenControl(JNIEnv *env __unused, jclass clazz __unused)
{
    ALOGI("Connect Screen Control");
}

static jint ScreenControlCapScreen(JNIEnv *env, jobject clazz, jint left, jint top,
	jint right, jint bottom, jint width, jint height, jint sourceType, jstring jfilename)
{
	sp<ScreenControlClient>& scc = getScreenControlClient();
	if (scc != NULL) {
		const char *filename = env->GetStringUTFChars(jfilename, nullptr);
		return spScreenCtrl->startScreenCap(left, top, right,
			bottom, width, height, sourceType, filename);
	} else
		return -1;
}

static jint ScreenControlRecordScreen(JNIEnv *env, jobject clazz, jint width, jint height,
	jint frameRate, jint bitRate, jint limitTimeSec, jint sourceType, jstring jfilename)
{
	sp<ScreenControlClient>& scc = getScreenControlClient();
	if (scc != NULL) {
		const char *filename = env->GetStringUTFChars(jfilename, nullptr);
		return spScreenCtrl->startScreenRecord(width, height, 
			frameRate, bitRate, limitTimeSec, sourceType, filename);
	} else
		return -1;
}

static jbyteArray ScreenControlCapScreenBuffer(JNIEnv *env, jobject clazz, jint left, 
	jint top, jint right, jint bottom, jint width, jint height, jint sourceType)
{
	sp<ScreenControlClient>& scc = getScreenControlClient();
	if (scc != NULL) {
		jbyte *buffer = NULL;
		int bufferSize = 0;
		int ret = NO_ERROR;

		ret = spScreenCtrl->startScreenCapBuffer(left, top, right, bottom, width,
					height, sourceType, (void **)&buffer, &bufferSize);
		jbyteArray arr = env->NewByteArray(bufferSize);
		env->SetByteArrayRegion(arr, 0, bufferSize, (jbyte *)buffer);
		delete [] buffer;
		return arr;
	} else
		return NULL;
}

static JNINativeMethod ScreenControl_Methods[] = {
	{"native_ConnectScreenControl", "()V", (void *) ConnectScreenControl },
	{"native_ScreenCap", "(IIIIIIILjava/lang/String;)I", (void *) ScreenControlCapScreen},
	{"native_ScreenRecord", "(IIIIIILjava/lang/String;)I", (void *) ScreenControlRecordScreen},
	{"native_ScreenCapBuffer", "(IIIIIII)[B", (void *) ScreenControlCapScreenBuffer},
};

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

int register_com_droidlogic_app_ScreenControlManager(JNIEnv *env)
{
    static const char *const kClassPathName = "com/droidlogic/app/ScreenControlManager";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, ScreenControl_Methods, NELEM(ScreenControl_Methods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    return rc;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved __unused)
{
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_com_droidlogic_app_ScreenControlManager(env) < 0)
    {
        ALOGE("Can't register DtvkitGlueClient");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}



