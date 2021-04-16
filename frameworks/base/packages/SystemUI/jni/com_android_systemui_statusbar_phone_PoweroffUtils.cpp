//#include <jni.h>
#include <nativehelper/JNIHelp.h>
#include <utils/Log.h>
#include <stdio.h>
#include <string.h>

#define POWEROFF_NODE   "/sys/class/shutdown_sysfs/poweroff_trigger"
#undef LOG_TAG
#define LOG_TAG "PowerOff-JNI"

namespace android {

static void native_Poweroff(JNIEnv* /*env*/, jobject /*obj*/) {
    FILE *fp = NULL;
    fp = fopen(POWEROFF_NODE, "w");

    if (fp == NULL) {
	ALOGE("poweroff_trigger is null");
        return;
    }

    fwrite("3", 1, 1, fp);

    fclose(fp);

    return;
}

static const JNINativeMethod g_methods[] = {
    { "native_Poweroff", "()V", (void*)native_Poweroff },
};

int register_com_android_systemui_statusbar_policy(JNIEnv *env) {
    if (jniRegisterNativeMethods(
            env, "com/android/systemui/statusbar/policy/PoweroffUtils",
        g_methods, NELEM(g_methods)) < 0) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

} // namespace android

int JNI_OnLoad(JavaVM *jvm, void* /*reserved*/) {
    JNIEnv *env;

    if (jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        return JNI_ERR;
    }

    return android::register_com_android_systemui_statusbar_policy(env);
}
