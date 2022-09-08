#define LOG_TAG "SubtitleAPI"

#include <jni.h>
#include <utils/RefBase.h>
#include <utils/Mutex.h>

#include <utils/Log.h>

#include "SubtitleNativeAPI.h"

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

static JavaVM   *g_JavaVM = NULL;

typedef void (*AmlSubtitleDataCb)(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing);


static void _subtitleAfdCallback(int event) {
    ALOGD("%s %d", __func__, event);
}
static void _subtitleChannelUpdateCallback(int event, int id) {
    ALOGD("%s %d %d", __func__, event, id);
}
static void _subtitleAvailableCallback(int event) {
    ALOGD("%s %d", __func__, event);
}

static void _subtitleDimesionCallback(int width, int height) {
    ALOGD("%s width=%d height=%d", __func__, width, height);
}


static jlong native_SubtitleCreate(JNIEnv* env, jobject object) {
    jlong session = (jlong)amlsub_Create();
    ALOGD("%s, handle=%llx", __func__, session);

    // regist callback
    amlsub_RegistAfdEventCB((AmlSubtitleHnd)session, _subtitleAfdCallback);
    amlsub_RegistOnChannelUpdateCb((AmlSubtitleHnd)session, _subtitleChannelUpdateCallback);
    amlsub_RegistOnSubtitleAvailCb((AmlSubtitleHnd)session, _subtitleAvailableCallback);
    amlsub_RegistGetDimesionCb((AmlSubtitleHnd)session, _subtitleDimesionCallback);

    return session;
}

jboolean native_SubtitleDestroy(JNIEnv* env, jobject object, jlong handle) {
    AmlSubtitleStatus r = amlsub_Destroy((AmlSubtitleHnd)handle);
    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}

jboolean native_SubtitleOpen(JNIEnv* env, jobject object, jlong handle, int ioType,
        int subType, int pid, int videoFmt, int channelId, int ancId, int cmpositionId) {
    AmlSubtitleParam param;
    memset(&param, 0, sizeof(AmlSubtitleParam));
    param.extSubPath = nullptr; // We do not play external file (e.g, .sub, .idx), keep this null.

    // Not all the parameter is need in a percific subtype, no need type just keep 0.
    param.ioSource = (AmlSubtitleIOType)ioType;
    param.subtitleType = subType;
    param.pid = pid;
    param.videoFormat = videoFmt;
    param.channelId = channelId;
    param.ancillaryPageId = ancId;
    param.compositionPageId = cmpositionId;
    AmlSubtitleStatus r = amlsub_Open((AmlSubtitleHnd)handle, &param);
    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}

jboolean native_SubtitleClose(JNIEnv* env, jobject object, jlong handle) {
    AmlSubtitleStatus r= amlsub_Close((AmlSubtitleHnd)handle);
    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}

jboolean native_Show(JNIEnv* env, jobject object, jlong handle) {
    AmlSubtitleStatus r= amlsub_UiShow((AmlSubtitleHnd)handle);
    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}

jboolean native_Hide(JNIEnv* env, jobject object, jlong handle) {
    AmlSubtitleStatus r= amlsub_UiHide((AmlSubtitleHnd)handle);
    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}

jboolean native_DisplayRect(JNIEnv* env, jobject object, jlong handle, int x, int y, int w, int h) {
    AmlSubtitleStatus r= amlsub_UiSetSurfaceViewRect((AmlSubtitleHnd)handle, x, y, w, h);
    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}


jboolean native_TTgoHome(JNIEnv* env, jobject object, jlong handle) {
    AmlTeletextCtrlParam param;
    param.event = TT_EVENT_INDEXPAGE;
    AmlSubtitleStatus r= amlsub_TeletextControl((AmlSubtitleHnd)handle, &param);

    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}

jboolean native_TTnextPage(JNIEnv* env, jobject object, jlong handle, jboolean inCrease){
    AmlTeletextCtrlParam param;
    param.event = TT_EVENT_NEXTPAGE;
    AmlSubtitleStatus r= amlsub_TeletextControl((AmlSubtitleHnd)handle, &param);

    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}

jboolean native_TTnextSubPage(JNIEnv* env, jobject object, jlong handle, jboolean inCrease){
    AmlTeletextCtrlParam param;
    param.event = TT_EVENT_NEXTSUBPAGE;
    AmlSubtitleStatus r= amlsub_TeletextControl((AmlSubtitleHnd)handle, &param);

    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}

jboolean native_TTgotoPage(JNIEnv* env, jobject object, jlong handle, int pageNo, int subPageNo){
    AmlTeletextCtrlParam param;
    param.event = TT_EVENT_GO_TO_PAGE;
    param.magazine = pageNo;
    param.page = subPageNo;
    AmlSubtitleStatus r= amlsub_TeletextControl((AmlSubtitleHnd)handle, &param);

    ALOGD("%s, handle=%llx result is: %d\n", __func__, handle, r);
    return r == AmlSubtitleStatus::SUB_STAT_OK;
}


static JNINativeMethod SubtitleSimpleDemo_Methods[] = {
    {"nativeSubtitleCreate", "()J", (void *)native_SubtitleCreate },
    {"nativeSubtitleDestroy", "(J)Z", (void *)native_SubtitleDestroy },
    {"nativeSubtitleOpen", "(JIIIIIII)Z", (void *)native_SubtitleOpen },
    {"nativeSubtitleClose", "(J)Z", (void *) native_SubtitleClose},
    {"nativeShow", "(J)Z", (void *)native_Show },
    {"nativeHide", "(J)Z", (void *)native_Hide },
    {"nativeDisplayRect", "(JIIII)Z", (void *)native_DisplayRect },

    {"nativeTTgoHome", "(J)Z", (void *)native_TTgoHome },
    {"nativeTTnextPage", "(JZ)Z", (void *)native_TTnextPage },
    {"nativeTTnextSubPage", "(JZ)Z", (void *)native_TTnextSubPage },
    {"nativeTTgotoPage", "(JII)Z", (void *)native_TTgotoPage },
};

int register_com_example_SubtitleSimpleDemo(JNIEnv *env)
{
    static const char *const kClassPathName = "com.example.SubtitleSimpleDemo.SubtitleAPI";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, SubtitleSimpleDemo_Methods, NELEM(SubtitleSimpleDemo_Methods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    return rc;
}


jint JNI_OnLoad(JavaVM *vm, void *reserved __unused) {
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }

    //assert(env != NULL);
    g_JavaVM = vm;

    if (register_com_example_SubtitleSimpleDemo(env) < 0) {
        ALOGE("Can't register DtvkitGlueClient");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}


