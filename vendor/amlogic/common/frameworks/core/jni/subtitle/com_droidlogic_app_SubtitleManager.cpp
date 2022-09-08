#define LOG_TAG "SubtitleManager-jni"
#include <jni.h>
#include <utils/RefBase.h>
#include <utils/Mutex.h>

#include <utils/Atomic.h>
#include <utils/Log.h>
//#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <dlfcn.h>

#include "SubtitleServerClient.h"

// use the type defines here. not directly use the api.
#include "SubtitleNativeAPI.h"

using android::Mutex;
using amlogic::SubtitleServerClient;
using amlogic::SubtitleListener;

//Must always keep the same as Java final consts!
static const int SUBTITLE_TXT =1;
static const int SUBTITLE_IMAGE =2;
static const int SUBTITLE_CC_JASON = 3;
static const int SUBTITLE_IMAGE_CENTER = 4;


class SubtitleDataListenerImpl;


#define TRACE_CALL 1
#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);


struct JniContext {
    Mutex mLock;
    JavaVM *mJavaVM = NULL;

    jobject mSubtitleManagerObject;
    int mFallBack = true;

    jmethodID mNotifySubtitleEvent;
    jmethodID mNotifySubtitleUIEvent;
    jmethodID mSubtitleTextOrImage;
    jmethodID mUpdateChannedId;
    jmethodID mNotifySubtitleAvail;
    jmethodID mNotifySubtitleInfo;
    jmethodID mExtSubtitle;

    sp<SubtitleServerClient> mSubContext;

    JNIEnv* getJniEnv(bool *needDetach) {
        int ret = -1;
        JNIEnv *env = NULL;
        ret = mJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
        if (ret < 0) {
            ret = mJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                ALOGE("Can't attach thread ret = %d", ret);
                return NULL;
            }
            *needDetach = true;
        }
        return env;
    }

    void DetachJniEnv() {
        int result = mJavaVM->DetachCurrentThread();
        if (result != JNI_OK) {
            ALOGE("thread detach failed: %#x", result);
        }
    }

    jboolean callJava_isExternalSubtitle() {
        bool needDetach = false;
        JNIEnv *env = getJniEnv(&needDetach);
        jboolean  isExtSub = env->CallBooleanMethod(mSubtitleManagerObject, mExtSubtitle);
        if (needDetach) DetachJniEnv();
        return isExtSub;
    }

    jint callJava_subtitleTextOrImage(int paserType) {
        bool needDetach = false;
        JNIEnv *env = getJniEnv(&needDetach);
        jint uiType = env->CallIntMethod(mSubtitleManagerObject, mSubtitleTextOrImage, paserType);
        if (needDetach) DetachJniEnv();
        return uiType;
    }
    void callJava_showTextData(const char *data, int type, int cmd) {
        bool needDetach = false;
        JNIEnv *env = getJniEnv(&needDetach);
        //jstring string = env->NewStringUTF(data);
        jbyteArray byteArray = env->NewByteArray(strlen(data));
        env->SetByteArrayRegion(byteArray, 0, strlen(data),(jbyte *)data);

        // Text data do not care positions, no such info!
        env->CallVoidMethod(mSubtitleManagerObject, mNotifySubtitleEvent, nullptr, byteArray,
                type, /*x,y*/0, 0, /*w,h*/0, 0, /*vw,vh*/0, 0, !(cmd==0));
        env->DeleteLocalRef(byteArray);
        if (needDetach) DetachJniEnv();
   }

    void callJava_showBitmapData(const char *data, int size, int uiType,
            int x, int y, int width, int height,
            int videoWidth, int videoHeight, int cmd) {
        bool needDetach = false;

        if (width <= 0 || height <= 0 || size <= 0) {
            ALOGE("invalid parameter width=%d height=%d size=%d", width, height, size);
        }

        JNIEnv *env = getJniEnv(&needDetach);
        jintArray array = env->NewIntArray(width*height);
        env->SetIntArrayRegion(array, 0, width*height, (jint *)data);
        env->CallVoidMethod(mSubtitleManagerObject, mNotifySubtitleEvent, array, nullptr,
                uiType, x, y, width, height, videoWidth, videoHeight, !(cmd==0));

        env->DeleteLocalRef(array);
        if (needDetach) DetachJniEnv();
    }

    void callJava_notifyDataEvent(int event, int id) {
        bool needDetach = false;
        JNIEnv *env = getJniEnv(&needDetach);
        ALOGD("callJava_notifyDataEvent: %d %d", event, id);
        env->CallVoidMethod(mSubtitleManagerObject, mUpdateChannedId, event, id);
        if (needDetach) DetachJniEnv();
    }

    void callJava_notifySubtitleAvail(int avail) {
        bool needDetach = false;
        JNIEnv *env = getJniEnv(&needDetach);
        ALOGD("callJava_notifySubtitleAvail: %d", avail);
        env->CallVoidMethod(mSubtitleManagerObject, mNotifySubtitleAvail, avail);
        if (needDetach) DetachJniEnv();
    }

    void callJava_notifySubtitleInfo(int what, int extra) {
        bool needDetach = false;
        JNIEnv *env = getJniEnv(&needDetach);
        ALOGD("callJava_notifySubtitleInfo: what:%d, extra:%d", what, extra);
        env->CallVoidMethod(mSubtitleManagerObject, mNotifySubtitleInfo, what, extra);
        if (needDetach) DetachJniEnv();
    }

    void callJava_uiCommand(int command, const std::vector<int> &param) {
        bool needDetach = false;
        JNIEnv *env = getJniEnv(&needDetach);
        ALOGD("callJava_uiCommand:%d", command);

        jintArray array = env->NewIntArray(param.size());
        int *jintData = (int *)malloc(param.size() * sizeof(int));

        for (int i=0; i<param.size(); i++) {
            ALOGD("param: %d %d", i, param[i]);
            jintData[i] = param[i];
        }
        env->SetIntArrayRegion(array, 0, param.size(), (jint *)jintData);
        env->CallVoidMethod(mSubtitleManagerObject, mNotifySubtitleUIEvent, command, array);
        env->DeleteLocalRef(array);
        free(jintData);
        if (needDetach) DetachJniEnv();
    }
};

JniContext *gJniContext = nullptr;
static inline JniContext *getJniContext() {
    // TODO: check error
    return gJniContext;
}



class SubtitleDataListenerImpl : public SubtitleListener {
public:
    SubtitleDataListenerImpl() {}
    ~SubtitleDataListenerImpl() {}
    // TODO: maybe, we can splict to notify Text and notify Bitmap
    virtual void onSubtitleEvent(const char *data, int size, int parserType,
            int x, int y, int width, int height,
            int videoWidth, int videoHeight, int cmd)
{
        jint uiType = getJniContext()->callJava_subtitleTextOrImage(parserType);

        ALOGD("in onStringSubtitleEvent subtitleType=%d size=%d x=%d, y= %d width=%d height=%d videow=%d, videoh=%d cmd=%d\n",
                uiType, size, x, y, width, height, videoWidth, videoHeight,cmd);
        if (((uiType == 2) || (uiType == 4)) && size > 0) {
            getJniContext()->callJava_showBitmapData(data, size, uiType, x, y, width, height, videoWidth, videoHeight, cmd);
        } else {
            getJniContext()->callJava_showTextData(data, uiType, cmd);
        }

    }

    // TODO: maybe need reconsidrate the name
    virtual void onSubtitleDataEvent(int event, int id) {
        getJniContext()->callJava_notifyDataEvent(event, id);
    }

    void onSubtitleAvail(int avail) {
        getJniContext()->callJava_notifySubtitleAvail(avail);
    }

    void onSubtitleAfdEvent(int afd) {
        //TODO:need mw register and cb
    }

    void onSubtitleDimension(int width, int height){}

    void onMixVideoEvent(int val) {
        //TODO:need mw register and cb
    }

    void onSubtitleLanguage(char *lang) {
        //TODO:need mw register and cb
    }

    void onSubtitleInfo(int what, int extra) {
        ALOGD("onSubtitleInfo:what:%d, extra:%d", what, extra);
        getJniContext()->callJava_notifySubtitleInfo(what, extra);
    }


    // sometime, server may crash, we need clean up in server side.
    virtual void onServerDied() {
        std::vector<int> params;
        getJniContext()->callJava_uiCommand((int)FallthroughUiCmd::CMD_UI_HIDE, params);
    }


    void onSubtitleUIEvent(int uiCmd, const std::vector<int> &params) {
        getJniContext()->callJava_uiCommand(uiCmd, params);
    }
};

static void nativeInit(JNIEnv* env, jobject object, jboolean startFallbackDisplay) {
    if (TRACE_CALL) ALOGD("%s %d", __func__, __LINE__);
    getJniContext()->mSubtitleManagerObject = env->NewGlobalRef(object);
    getJniContext()->mFallBack = startFallbackDisplay;
    if (getJniContext()->mSubContext == nullptr) {
        getJniContext()->mSubContext = new SubtitleServerClient(
            getJniContext()->mFallBack,
            new SubtitleDataListenerImpl(),
            OpenType::TYPE_APPSDK);
    }
}

static void nativeDestroy(JNIEnv* env, jobject object) {
    if (TRACE_CALL) ALOGD("%s %d", __func__, __LINE__);
    getJniContext()->mSubContext = nullptr;
    env->DeleteGlobalRef(getJniContext()->mSubtitleManagerObject);
    getJniContext()->mSubtitleManagerObject = nullptr;

}

static void nativeUpdateVideoPos(JNIEnv* env, jobject object, jint pos) {
    ALOGD("subtitleShowSub pos:%d\n", pos);
    if (pos > 0 && getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->updateVideoPos(pos);
    } else {
        ALOGE("why pos is negative? %d", pos);
    }
}

static jboolean nativeOpen(JNIEnv* env, jobject object, jstring jpath, jint ioType) {
    if (TRACE_CALL) ALOGD("%s %d", __func__, __LINE__);
    const char *cpath = env->GetStringUTFChars(jpath, nullptr);
    ALOGD("nativeOpen path:%s", cpath);
    bool res = false;


    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->userDataOpen();
        bool isExt = getJniContext()->callJava_isExternalSubtitle();
        ALOGD("isExt? %d", isExt);
        if (isExt) {
            res = getJniContext()->mSubContext->open(cpath, ioType);
        } else {
            res = getJniContext()->mSubContext->open(nullptr, ioType);
        }
    } else {
       ALOGE("Subtitle Connection not established");
    }
    env->ReleaseStringUTFChars(jpath, cpath);
    return true;
}

static void nativeClose(JNIEnv* env, jobject object) {
    if (TRACE_CALL) ALOGD("%s %d %p", __func__, __LINE__, getJniContext()->mSubContext.get());
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->userDataClose();
        getJniContext()->mSubContext->close();
    } else {
       ALOGE("Subtitle Connection not established");
    }
}


static jint nativeTotalSubtitles(JNIEnv* env, jobject object) {
    if (TRACE_CALL) ALOGD("%s %d %p", __func__, __LINE__, getJniContext()->mSubContext.get());
    if (getJniContext()->mSubContext != nullptr) {
        return getJniContext()->mSubContext->totalTracks();
    } else {
        ALOGE("Subtitle Connection not established");
    }

    return -1;
}

static jint nativeInnerSubtitles(JNIEnv* env, jobject object) {
    return nativeTotalSubtitles(env, object);
}

static void nativeSetSubType(JNIEnv* env, jclass clazz, jint type) {
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->setSubType(type);
    } else {
        ALOGE("Subtitle Connection not established");
    }
}

static void nativeSetSctePid(JNIEnv* env, jclass clazz, jint pid) {
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->setSubPid(pid);
    } else {
        ALOGE("Subtitle Connection not established");
    }

}
static void nativeSetPlayerType(JNIEnv* env, jclass clazz, jint type) {
    if (type == 1) {
        ALOGE("SubtitleServerHidlClient nativeSetPlayerType amu ...");
        property_set("media.ctcmediaplayer.enable", "0");
        property_set("media.ctcmediaplayer.ts-stream", "0");
     } else {
        ALOGE("SubtitleServerHidlClient nativeSetPlayerType  ctc ...");
        property_set("media.ctcmediaplayer.enable", "1");
        property_set("media.ctcmediaplayer.ts-stream", "1");
     }
}

static jint nativeGetSubType(JNIEnv* env, jclass clazz) {
    if (getJniContext()->mSubContext != nullptr) {
        return getJniContext()->mSubContext->getSubType();
    } else {
        ALOGE("Subtitle Connection not established");
    }
    return -1;
}

static void nativeResetForSeek(JNIEnv* env, jclass clazz) {
    if (TRACE_CALL) ALOGD("%s %d", __func__, __LINE__);
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->resetForSeek();
    } else {
        ALOGE("Subtitle Connection not established");
    }

}

static void nativeSelectCcChannel(JNIEnv* env, jclass clazz, jint channel) {
    if (TRACE_CALL) ALOGD("%s idx:%d", __func__, __LINE__);
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->selectCcChannel(channel);
    } else {
        ALOGE("Subtitle Connection not established");
    }
}


static jint nativeTtGoHome(JNIEnv* env, jclass clazz) {
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->ttControl(TT_EVENT_INDEXPAGE, -1, -1, -1, -1);
    } else {
        ALOGE("Subtitle Connection not established");
    }
    return 0;
}


static jint nativeTtGotoPage(JNIEnv* env, jclass clazz, jint page, jint subPage) {
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->ttControl(TT_EVENT_GO_TO_PAGE, page, subPage, -1, -1);
    } else {
        ALOGE("Subtitle Connection not established");
    }
    return 0;
}

static jint nativeTtNextPage(JNIEnv* env, jclass clazz, jint dir) {
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->ttControl(dir==1?TT_EVENT_NEXTPAGE:TT_EVENT_PREVIOUSPAGE, -1, -1, -1, -1);
    } else {
        ALOGE("Subtitle Connection not established");
    }
    return 0;
}

static jint nativeTtNextSubPage(JNIEnv* env, jclass clazz, jint dir) {
    if (getJniContext()->mSubContext != nullptr) {
        getJniContext()->mSubContext->ttControl(dir==1?TT_EVENT_NEXTSUBPAGE:TT_EVENT_PREVIOUSSUBPAGE, -1, -1, -1, -1);
    } else {
        ALOGE("Subtitle Connection not established");
    }
    return 0;
}

static void nativeUnCrypt(JNIEnv *env, jclass clazz, jstring src, jstring dest) {
    const char *FONT_VENDOR_LIB = "/vendor/lib/libvendorfont.so";
    const char *FONT_PRODUCT_LIB = "/product/lib/libvendorfont.so";

    // TODO: maybe we need some smart method to get the lib.
    void *handle = dlopen(FONT_PRODUCT_LIB, RTLD_NOW);
    if (handle == nullptr) {
        handle = dlopen(FONT_VENDOR_LIB, RTLD_NOW);
    }

    if (handle == nullptr) {
        ALOGE(" nativeUnCrypt error! cannot open uncrypto lib");
        return;
    }

    typedef void (*fnFontRelease)(const char*, const char*);
    fnFontRelease fn = (fnFontRelease)dlsym(handle, "vendor_font_release");
    if (fn == nullptr) {
        ALOGE(" nativeUnCrypt error! cannot locate symbol vendor_font_release in uncrypto lib");
        dlclose(handle);
        return;
    }

    const char *srcstr = (const char *)env->GetStringUTFChars(src, NULL);
    const char *deststr = (const char *)env->GetStringUTFChars( dest, NULL);

    fn(srcstr, deststr);
    dlclose(handle);

    (env)->ReleaseStringUTFChars(src, (const char *)srcstr);
    (env)->ReleaseStringUTFChars(dest, (const char *)deststr);
}

static JNINativeMethod SubtitleManager_Methods[] = {
    {"nativeInit", "(Z)V", (void *)nativeInit},
    {"nativeDestroy", "()V", (void *)nativeDestroy},
    {"nativeUpdateVideoPos", "(I)V", (void *)nativeUpdateVideoPos},
    {"nativeOpen", "(Ljava/lang/String;I)Z", (void *)nativeOpen},
    {"nativeClose", "()V", (void *)nativeClose},
    {"nativeTotalSubtitles", "()I", (void *)nativeTotalSubtitles},
    {"nativeInnerSubtitles", "()I", (void *)nativeInnerSubtitles},
    {"nativeGetSubType", "()I", (void *)nativeGetSubType},
    {"nativeSetSubType", "(I)V", (void *)nativeSetSubType},
    {"nativeSetPlayerType", "(I)V", (void *)nativeSetPlayerType},
    {"nativeSetSctePid", "(I)V", (void *)nativeSetSctePid},
    {"nativeResetForSeek", "()V", (void *)nativeResetForSeek},
    {"nativeTtGoHome", "()I", (void *)nativeTtGoHome},
    {"nativeTtGotoPage", "(II)I", (void *)nativeTtGotoPage},
    {"nativeTtNextPage", "(I)I", (void *)nativeTtNextPage},
    {"nativeTtNextSubPage", "(I)I", (void *)nativeTtNextSubPage},
    {"nativeSelectCcChannel", "(I)V", (void *)nativeSelectCcChannel},
    {"nativeUnCrypt", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)nativeUnCrypt},

};

int register_com_droidlogic_app_SubtitleManager(JNIEnv *env) {
    static const char *const kClassPathName = "com/droidlogic/app/SubtitleManager";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, SubtitleManager_Methods, NELEM(SubtitleManager_Methods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    GET_METHOD_ID(getJniContext()->mNotifySubtitleEvent, clazz, "notifySubtitleEvent", "([I[BIIIIIIIZ)V");
    GET_METHOD_ID(getJniContext()->mSubtitleTextOrImage, clazz, "subtitleTextOrImage", "(I)I");
    GET_METHOD_ID(getJniContext()->mNotifySubtitleUIEvent, clazz, "notifySubtitleUIEvent", "(I[I)V");
    GET_METHOD_ID(getJniContext()->mUpdateChannedId, clazz, "updateChannedId", "(II)V");
    GET_METHOD_ID(getJniContext()->mNotifySubtitleAvail, clazz, "notifyAvailable", "(I)V");
    GET_METHOD_ID(getJniContext()->mExtSubtitle, clazz, "isExtSubtitle", "()Z");
    GET_METHOD_ID(getJniContext()->mNotifySubtitleInfo, clazz, "notifySubtitleInfo", "(II)V");
    return rc;
}


jint JNI_OnLoad(JavaVM *vm, void *reserved __unused) {
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    gJniContext = new JniContext();
    getJniContext()->mJavaVM = vm;

    if (register_com_droidlogic_app_SubtitleManager(env) < 0) {
        ALOGE("Can't register SubtitleManager JNI");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

