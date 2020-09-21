/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#include <syslog.h>
#include "vendor_font_util.h"
#include "Fonts.h"
#include <utils/Log.h>
#include <jni.h>
//#include <nativehelper/JNIHelp.h>
//#include <android_runtime/AndroidRuntime.h>

using namespace android;

/* Native interface, it will be call in java code */
void nativeUnCrypt(JNIEnv *env, jclass clazz,jstring src,jstring dest)
{
    const char *src_str =
    (const char *)env->GetStringUTFChars(src,NULL);
    const char *dest_str =
    (const char *)env->GetStringUTFChars( dest, NULL );
    vendor_font_release(src_str,dest_str);
    (env)->ReleaseStringUTFChars(src, (const char *)src_str );
    (env)->ReleaseStringUTFChars(dest, (const char *)dest_str );

}

// ----------------------------------------------------------------------------
static JNINativeMethod gMethods[] =
{
    {
        "nativeUnCrypt", "(Ljava/lang/String;Ljava/lang/String;)V",
        (void *) nativeUnCrypt
    },

};

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

int register_com_droidlogic_tvinput_TvApplication_FileUtils(JNIEnv *env)
{
    jint rc;
    static const char *const kClassPathName = "com/droidlogic/tvinput/TvApplication$FileUtils";
    jclass clazz;
    FIND_CLASS(clazz, kClassPathName);

    if (rc = (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0]))) < 0) {
        env->DeleteLocalRef(clazz);
        return -1;
    }

    env->DeleteLocalRef(clazz);
    return 0;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        goto bail;
    }
    //assert(env != NULL);


    if (register_com_droidlogic_tvinput_TvApplication_FileUtils(env) < 0)
    {
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

