/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayer2-JNI"
#include "utils/Log.h"

#include <sys/stat.h>

#include <media/AudioResamplerPublic.h>
#include <media/DataSourceDesc.h>
#include <media/MediaHTTPService.h>
#include <media/MediaAnalyticsItem.h>
#include <media/NdkWrapper.h>
#include <media/stagefright/Utils.h>
#include <media/stagefright/foundation/ByteUtils.h>  // for FOURCC definition
#include <mediaplayer2/JAudioTrack.h>
#include <mediaplayer2/mediaplayer2.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <utils/threads.h>
#include "jni.h"
#include <nativehelper/JNIHelp.h>
#include "android/native_window_jni.h"
#include "android_runtime/Log.h"
#include "utils/Errors.h"  // for status_t
#include "utils/KeyedVector.h"
#include "utils/String8.h"
#include "android_media_BufferingParams.h"
#include "android_media_Media2HTTPService.h"
#include "android_media_Media2DataSource.h"
#include "android_media_MediaMetricsJNI.h"
#include "android_media_PlaybackParams.h"
#include "android_media_SyncParams.h"
#include "android_media_VolumeShaper.h"

#include "android_os_Parcel.h"
#include "android_util_Binder.h"
#include <binder/Parcel.h>

// Modular DRM begin
#define FIND_CLASS(var, className) \
var = env->FindClass(className); \
LOG_FATAL_IF(! (var), "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, fieldName, fieldDescriptor) \
var = env->GetMethodID(clazz, fieldName, fieldDescriptor); \
LOG_FATAL_IF(! (var), "Unable to find method " fieldName);

struct StateExceptionFields {
    jmethodID init;
    jclass classId;
};

static StateExceptionFields gStateExceptionFields;
// Modular DRM end

// ----------------------------------------------------------------------------

using namespace android;

using media::VolumeShaper;

// ----------------------------------------------------------------------------

struct fields_t {
    jfieldID    context;
    jfieldID    surface_texture;

    jmethodID   post_event;

    jmethodID   proxyConfigGetHost;
    jmethodID   proxyConfigGetPort;
    jmethodID   proxyConfigGetExclusionList;
};
static fields_t fields;

static BufferingParams::fields_t gBufferingParamsFields;
static PlaybackParams::fields_t gPlaybackParamsFields;
static SyncParams::fields_t gSyncParamsFields;
static VolumeShaperHelper::fields_t gVolumeShaperFields;

static Mutex sLock;

static bool ConvertKeyValueArraysToKeyedVector(
        JNIEnv *env, jobjectArray keys, jobjectArray values,
        KeyedVector<String8, String8>* keyedVector) {

    int nKeyValuePairs = 0;
    bool failed = false;
    if (keys != NULL && values != NULL) {
        nKeyValuePairs = env->GetArrayLength(keys);
        failed = (nKeyValuePairs != env->GetArrayLength(values));
    }

    if (!failed) {
        failed = ((keys != NULL && values == NULL) ||
                  (keys == NULL && values != NULL));
    }

    if (failed) {
        ALOGE("keys and values arrays have different length");
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return false;
    }

    for (int i = 0; i < nKeyValuePairs; ++i) {
        // No need to check on the ArrayIndexOutOfBoundsException, since
        // it won't happen here.
        jstring key = (jstring) env->GetObjectArrayElement(keys, i);
        jstring value = (jstring) env->GetObjectArrayElement(values, i);

        const char* keyStr = env->GetStringUTFChars(key, NULL);
        if (!keyStr) {  // OutOfMemoryError
            return false;
        }

        const char* valueStr = env->GetStringUTFChars(value, NULL);
        if (!valueStr) {  // OutOfMemoryError
            env->ReleaseStringUTFChars(key, keyStr);
            return false;
        }

        keyedVector->add(String8(keyStr), String8(valueStr));

        env->ReleaseStringUTFChars(key, keyStr);
        env->ReleaseStringUTFChars(value, valueStr);
        env->DeleteLocalRef(key);
        env->DeleteLocalRef(value);
    }
    return true;
}

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class JNIMediaPlayer2Listener: public MediaPlayer2Listener
{
public:
    JNIMediaPlayer2Listener(JNIEnv* env, jobject thiz, jobject weak_thiz);
    ~JNIMediaPlayer2Listener();
    virtual void notify(int64_t srcId, int msg, int ext1, int ext2,
                        const Parcel *obj = NULL) override;
private:
    JNIMediaPlayer2Listener();
    jclass      mClass;     // Reference to MediaPlayer2 class
    jobject     mObject;    // Weak ref to MediaPlayer2 Java object to call on
};

JNIMediaPlayer2Listener::JNIMediaPlayer2Listener(JNIEnv* env, jobject thiz, jobject weak_thiz)
{

    // Hold onto the MediaPlayer2 class for use in calling the static method
    // that posts events to the application thread.
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        ALOGE("Can't find android/media/MediaPlayer2Impl");
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);

    // We use a weak reference so the MediaPlayer2 object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject  = env->NewGlobalRef(weak_thiz);
}

JNIMediaPlayer2Listener::~JNIMediaPlayer2Listener()
{
    // remove global references
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}

void JNIMediaPlayer2Listener::notify(int64_t srcId, int msg, int ext1, int ext2, const Parcel *obj)
{
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    if (obj && obj->dataSize() > 0) {
        jobject jParcel = createJavaParcelObject(env);
        if (jParcel != NULL) {
            Parcel* nativeParcel = parcelForJavaObject(env, jParcel);
            nativeParcel->setData(obj->data(), obj->dataSize());
            env->CallStaticVoidMethod(mClass, fields.post_event, mObject,
                    srcId, msg, ext1, ext2, jParcel);
            env->DeleteLocalRef(jParcel);
        }
    } else {
        env->CallStaticVoidMethod(mClass, fields.post_event, mObject,
                srcId, msg, ext1, ext2, NULL);
    }
    if (env->ExceptionCheck()) {
        ALOGW("An exception occurred while notifying an event.");
        LOGW_EX(env);
        env->ExceptionClear();
    }
}

// ----------------------------------------------------------------------------

static sp<MediaPlayer2> getMediaPlayer(JNIEnv* env, jobject thiz)
{
    Mutex::Autolock l(sLock);
    MediaPlayer2* const p = (MediaPlayer2*)env->GetLongField(thiz, fields.context);
    return sp<MediaPlayer2>(p);
}

static sp<MediaPlayer2> setMediaPlayer(JNIEnv* env, jobject thiz, const sp<MediaPlayer2>& player)
{
    Mutex::Autolock l(sLock);
    sp<MediaPlayer2> old = (MediaPlayer2*)env->GetLongField(thiz, fields.context);
    if (player.get()) {
        player->incStrong((void*)setMediaPlayer);
    }
    if (old != 0) {
        old->decStrong((void*)setMediaPlayer);
    }
    env->SetLongField(thiz, fields.context, (jlong)player.get());
    return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(
    JNIEnv *env, jobject thiz, status_t opStatus, const char* exception, const char *message)
{
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
        if (opStatus != (status_t) OK) {
            sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
            if (mp != 0) {
                int64_t srcId = 0;
                mp->getSrcId(&srcId);
                mp->notify(srcId, MEDIA2_ERROR, opStatus, 0);
            }
        }
    } else {  // Throw exception!
        if ( opStatus == (status_t) INVALID_OPERATION ) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        } else if ( opStatus == (status_t) BAD_VALUE ) {
            jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        } else if ( opStatus == (status_t) PERMISSION_DENIED ) {
            jniThrowException(env, "java/lang/SecurityException", NULL);
        } else if ( opStatus != (status_t) OK ) {
            if (strlen(message) > 230) {
               // if the message is too long, don't bother displaying the status code
               jniThrowException( env, exception, message);
            } else {
               char msg[256];
                // append the status code to the message
               sprintf(msg, "%s: status=0x%X", message, opStatus);
               jniThrowException( env, exception, msg);
            }
        }
    }
}

static void
android_media_MediaPlayer2_handleDataSourceUrl(
        JNIEnv *env, jobject thiz, jboolean isCurrent, jlong srcId,
        jobject httpServiceObj, jstring path, jobjectArray keys, jobjectArray values) {

    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (path == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    const char *tmp = env->GetStringUTFChars(path, NULL);
    if (tmp == NULL) {  // Out of memory
        return;
    }
    ALOGV("handleDataSourceUrl: path %s, srcId %lld", tmp, (long long)srcId);

    if (strncmp(tmp, "content://", 10) == 0) {
        ALOGE("handleDataSourceUrl: content scheme is not supported in native code");
        jniThrowException(env, "java/io/IOException",
                          "content scheme is not supported in native code");
        return;
    }

    sp<DataSourceDesc> dsd = new DataSourceDesc();
    dsd->mId = srcId;
    dsd->mType = DataSourceDesc::TYPE_URL;
    dsd->mUrl = tmp;

    env->ReleaseStringUTFChars(path, tmp);
    tmp = NULL;

    // We build a KeyedVector out of the key and val arrays
    if (!ConvertKeyValueArraysToKeyedVector(
            env, keys, values, &dsd->mHeaders)) {
        return;
    }

    sp<MediaHTTPService> httpService;
    if (httpServiceObj != NULL) {
        httpService = new JMedia2HTTPService(env, httpServiceObj);
    }
    dsd->mHttpService = httpService;

    status_t err;
    if (isCurrent) {
        err = mp->setDataSource(dsd);
    } else {
        err = mp->prepareNextDataSource(dsd);
    }
    process_media_player_call(env, thiz, err,
            "java/io/IOException", "handleDataSourceUrl failed." );
}

static void
android_media_MediaPlayer2_handleDataSourceFD(
    JNIEnv *env, jobject thiz, jboolean isCurrent, jlong srcId,
    jobject fileDescriptor, jlong offset, jlong length)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (fileDescriptor == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
    int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
    ALOGV("handleDataSourceFD: srcId=%lld, fd=%d (%s), offset=%lld, length=%lld",
          (long long)srcId, fd, nameForFd(fd).c_str(), (long long)offset, (long long)length);

    struct stat sb;
    int ret = fstat(fd, &sb);
    if (ret != 0) {
        ALOGE("handleDataSourceFD: fstat(%d) failed: %d, %s", fd, ret, strerror(errno));
        jniThrowException(env, "java/io/IOException", "handleDataSourceFD failed fstat");
        return;
    }

    ALOGV("st_dev  = %llu", static_cast<unsigned long long>(sb.st_dev));
    ALOGV("st_mode = %u", sb.st_mode);
    ALOGV("st_uid  = %lu", static_cast<unsigned long>(sb.st_uid));
    ALOGV("st_gid  = %lu", static_cast<unsigned long>(sb.st_gid));
    ALOGV("st_size = %llu", static_cast<unsigned long long>(sb.st_size));

    if (offset >= sb.st_size) {
        ALOGE("handleDataSourceFD: offset is out of range");
        jniThrowException(env, "java/lang/IllegalArgumentException",
                          "handleDataSourceFD failed, offset is out of range.");
        return;
    }
    if (offset + length > sb.st_size) {
        length = sb.st_size - offset;
        ALOGV("handleDataSourceFD: adjusted length = %lld", (long long)length);
    }

    sp<DataSourceDesc> dsd = new DataSourceDesc();
    dsd->mId = srcId;
    dsd->mType = DataSourceDesc::TYPE_FD;
    dsd->mFD = fd;
    dsd->mFDOffset = offset;
    dsd->mFDLength = length;

    status_t err;
    if (isCurrent) {
        err = mp->setDataSource(dsd);
    } else {
        err = mp->prepareNextDataSource(dsd);
    }
    process_media_player_call(env, thiz, err,
            "java/io/IOException", "handleDataSourceFD failed." );
}

static void
android_media_MediaPlayer2_handleDataSourceCallback(
    JNIEnv *env, jobject thiz, jboolean isCurrent, jlong srcId, jobject dataSource)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (dataSource == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
    sp<DataSource> callbackDataSource = new JMedia2DataSource(env, dataSource);
    sp<DataSourceDesc> dsd = new DataSourceDesc();
    dsd->mId = srcId;
    dsd->mType = DataSourceDesc::TYPE_CALLBACK;
    dsd->mCallbackSource = callbackDataSource;

    status_t err;
    if (isCurrent) {
        err = mp->setDataSource(dsd);
    } else {
        err = mp->prepareNextDataSource(dsd);
    }
    process_media_player_call(env, thiz, err,
            "java/lang/RuntimeException", "handleDataSourceCallback failed." );
}

static sp<ANativeWindowWrapper>
getVideoSurfaceTexture(JNIEnv* env, jobject thiz) {
    ANativeWindow * const p = (ANativeWindow*)env->GetLongField(thiz, fields.surface_texture);
    return new ANativeWindowWrapper(p);
}

static void
decVideoSurfaceRef(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }

    ANativeWindow * const old_anw = (ANativeWindow*)env->GetLongField(thiz, fields.surface_texture);
    if (old_anw != NULL) {
        ANativeWindow_release(old_anw);
        env->SetLongField(thiz, fields.surface_texture, (jlong)NULL);
    }
}

static void
setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface, jboolean mediaPlayerMustBeAlive)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        if (mediaPlayerMustBeAlive) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        }
        return;
    }

    decVideoSurfaceRef(env, thiz);

    ANativeWindow* anw = NULL;
    if (jsurface) {
        anw = ANativeWindow_fromSurface(env, jsurface);
        if (anw == NULL) {
            jniThrowException(env, "java/lang/IllegalArgumentException",
                    "The surface has been released");
            return;
        }
    }

    env->SetLongField(thiz, fields.surface_texture, (jlong)anw);

    // This will fail if the media player has not been initialized yet. This
    // can be the case if setDisplay() on MediaPlayer2Impl.java has been called
    // before setDataSource(). The redundant call to setVideoSurfaceTexture()
    // in prepare/prepare covers for this case.
    mp->setVideoSurfaceTexture(new ANativeWindowWrapper(anw));
}

static void
android_media_MediaPlayer2_setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{
    setVideoSurface(env, thiz, jsurface, true /* mediaPlayerMustBeAlive */);
}

static jobject
android_media_MediaPlayer2_getBufferingParams(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    BufferingParams bp;
    BufferingSettings &settings = bp.settings;
    process_media_player_call(
            env, thiz, mp->getBufferingSettings(&settings),
            "java/lang/IllegalStateException", "unexpected error");
    if (env->ExceptionCheck()) {
        return nullptr;
    }
    ALOGV("getBufferingSettings:{%s}", settings.toString().string());

    return bp.asJobject(env, gBufferingParamsFields);
}

static void
android_media_MediaPlayer2_setBufferingParams(JNIEnv *env, jobject thiz, jobject params)
{
    if (params == NULL) {
        return;
    }

    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    BufferingParams bp;
    bp.fillFromJobject(env, gBufferingParamsFields, params);
    ALOGV("setBufferingParams:{%s}", bp.settings.toString().string());

    process_media_player_call(
            env, thiz, mp->setBufferingSettings(bp.settings),
            "java/lang/IllegalStateException", "unexpected error");
}

static void
android_media_MediaPlayer2_playNextDataSource(JNIEnv *env, jobject thiz, jlong srcId)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    process_media_player_call(env, thiz, mp->playNextDataSource((int64_t)srcId),
            "java/io/IOException", "playNextDataSource failed." );
}

static void
android_media_MediaPlayer2_prepare(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    // Handle the case where the display surface was set before the mp was
    // initialized. We try again to make it stick.
    sp<ANativeWindowWrapper> st = getVideoSurfaceTexture(env, thiz);
    mp->setVideoSurfaceTexture(st);

    process_media_player_call( env, thiz, mp->prepareAsync(), "java/io/IOException", "Prepare Async failed." );
}

static void
android_media_MediaPlayer2_start(JNIEnv *env, jobject thiz)
{
    ALOGV("start");
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->start(), NULL, NULL );
}

static void
android_media_MediaPlayer2_stop(JNIEnv *env, jobject thiz)
{
    ALOGV("stop");
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->stop(), NULL, NULL );
}

static void
android_media_MediaPlayer2_pause(JNIEnv *env, jobject thiz)
{
    ALOGV("pause");
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->pause(), NULL, NULL );
}

static jboolean
android_media_MediaPlayer2_isPlaying(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return JNI_FALSE;
    }
    const jboolean is_playing = mp->isPlaying();

    ALOGV("isPlaying: %d", is_playing);
    return is_playing;
}

static void
android_media_MediaPlayer2_setPlaybackParams(JNIEnv *env, jobject thiz, jobject params)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    PlaybackParams pbp;
    pbp.fillFromJobject(env, gPlaybackParamsFields, params);
    ALOGV("setPlaybackParams: %d:%f %d:%f %d:%u %d:%u",
            pbp.speedSet, pbp.audioRate.mSpeed,
            pbp.pitchSet, pbp.audioRate.mPitch,
            pbp.audioFallbackModeSet, pbp.audioRate.mFallbackMode,
            pbp.audioStretchModeSet, pbp.audioRate.mStretchMode);

    AudioPlaybackRate rate;
    status_t err = mp->getPlaybackSettings(&rate);
    if (err == OK) {
        bool updatedRate = false;
        if (pbp.speedSet) {
            rate.mSpeed = pbp.audioRate.mSpeed;
            updatedRate = true;
        }
        if (pbp.pitchSet) {
            rate.mPitch = pbp.audioRate.mPitch;
            updatedRate = true;
        }
        if (pbp.audioFallbackModeSet) {
            rate.mFallbackMode = pbp.audioRate.mFallbackMode;
            updatedRate = true;
        }
        if (pbp.audioStretchModeSet) {
            rate.mStretchMode = pbp.audioRate.mStretchMode;
            updatedRate = true;
        }
        if (updatedRate) {
            err = mp->setPlaybackSettings(rate);
        }
    }
    process_media_player_call(
            env, thiz, err,
            "java/lang/IllegalStateException", "unexpected error");
}

static jobject
android_media_MediaPlayer2_getPlaybackParams(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    PlaybackParams pbp;
    AudioPlaybackRate &audioRate = pbp.audioRate;
    process_media_player_call(
            env, thiz, mp->getPlaybackSettings(&audioRate),
            "java/lang/IllegalStateException", "unexpected error");
    if (env->ExceptionCheck()) {
        return nullptr;
    }
    ALOGV("getPlaybackSettings: %f %f %d %d",
            audioRate.mSpeed, audioRate.mPitch, audioRate.mFallbackMode, audioRate.mStretchMode);

    pbp.speedSet = true;
    pbp.pitchSet = true;
    pbp.audioFallbackModeSet = true;
    pbp.audioStretchModeSet = true;

    return pbp.asJobject(env, gPlaybackParamsFields);
}

static void
android_media_MediaPlayer2_setSyncParams(JNIEnv *env, jobject thiz, jobject params)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    SyncParams scp;
    scp.fillFromJobject(env, gSyncParamsFields, params);
    ALOGV("setSyncParams: %d:%d %d:%d %d:%f %d:%f",
          scp.syncSourceSet, scp.sync.mSource,
          scp.audioAdjustModeSet, scp.sync.mAudioAdjustMode,
          scp.toleranceSet, scp.sync.mTolerance,
          scp.frameRateSet, scp.frameRate);

    AVSyncSettings avsync;
    float videoFrameRate;
    status_t err = mp->getSyncSettings(&avsync, &videoFrameRate);
    if (err == OK) {
        bool updatedSync = scp.frameRateSet;
        if (scp.syncSourceSet) {
            avsync.mSource = scp.sync.mSource;
            updatedSync = true;
        }
        if (scp.audioAdjustModeSet) {
            avsync.mAudioAdjustMode = scp.sync.mAudioAdjustMode;
            updatedSync = true;
        }
        if (scp.toleranceSet) {
            avsync.mTolerance = scp.sync.mTolerance;
            updatedSync = true;
        }
        if (updatedSync) {
            err = mp->setSyncSettings(avsync, scp.frameRateSet ? scp.frameRate : -1.f);
        }
    }
    process_media_player_call(
            env, thiz, err,
            "java/lang/IllegalStateException", "unexpected error");
}

static jobject
android_media_MediaPlayer2_getSyncParams(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    SyncParams scp;
    scp.frameRate = -1.f;
    process_media_player_call(
            env, thiz, mp->getSyncSettings(&scp.sync, &scp.frameRate),
            "java/lang/IllegalStateException", "unexpected error");
    if (env->ExceptionCheck()) {
        return nullptr;
    }

    ALOGV("getSyncSettings: %d %d %f %f",
            scp.sync.mSource, scp.sync.mAudioAdjustMode, scp.sync.mTolerance, scp.frameRate);

    // sanity check params
    if (scp.sync.mSource >= AVSYNC_SOURCE_MAX
            || scp.sync.mAudioAdjustMode >= AVSYNC_AUDIO_ADJUST_MODE_MAX
            || scp.sync.mTolerance < 0.f
            || scp.sync.mTolerance >= AVSYNC_TOLERANCE_MAX) {
        jniThrowException(env,  "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    scp.syncSourceSet = true;
    scp.audioAdjustModeSet = true;
    scp.toleranceSet = true;
    scp.frameRateSet = scp.frameRate >= 0.f;

    return scp.asJobject(env, gSyncParamsFields);
}

static void
android_media_MediaPlayer2_seekTo(JNIEnv *env, jobject thiz, jlong msec, jint mode)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ALOGV("seekTo: %lld(msec), mode=%d", (long long)msec, mode);
    process_media_player_call(env, thiz, mp->seekTo((int64_t)msec, (MediaPlayer2SeekMode)mode),
                              NULL, NULL);
}

static void
android_media_MediaPlayer2_notifyAt(JNIEnv *env, jobject thiz, jlong mediaTimeUs)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ALOGV("notifyAt: %lld", (long long)mediaTimeUs);
    process_media_player_call( env, thiz, mp->notifyAt((int64_t)mediaTimeUs), NULL, NULL );
}

static jint
android_media_MediaPlayer2_getMediaPlayer2State(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return MEDIAPLAYER2_STATE_IDLE;
    }
    return (jint)mp->getMediaPlayer2State();
}

static jint
android_media_MediaPlayer2_getVideoWidth(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int w;
    if (0 != mp->getVideoWidth(&w)) {
        ALOGE("getVideoWidth failed");
        w = 0;
    }
    ALOGV("getVideoWidth: %d", w);
    return (jint) w;
}

static jint
android_media_MediaPlayer2_getVideoHeight(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int h;
    if (0 != mp->getVideoHeight(&h)) {
        ALOGE("getVideoHeight failed");
        h = 0;
    }
    ALOGV("getVideoHeight: %d", h);
    return (jint) h;
}

static jobject
android_media_MediaPlayer2_native_getMetrics(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }

    Parcel p;
    int key = FOURCC('m','t','r','X');
    status_t status = mp->getParameter(key, &p);
    if (status != OK) {
        ALOGD("getMetrics() failed: %d", status);
        return (jobject) NULL;
    }

    p.setDataPosition(0);
    MediaAnalyticsItem *item = new MediaAnalyticsItem;
    item->readFromParcel(p);
    jobject mybundle = MediaMetricsJNI::writeMetricsToBundle(env, item, NULL);

    // housekeeping
    delete item;
    item = NULL;

    return mybundle;
}

static jlong
android_media_MediaPlayer2_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int64_t msec;
    process_media_player_call( env, thiz, mp->getCurrentPosition(&msec), NULL, NULL );
    ALOGV("getCurrentPosition: %lld (msec)", (long long)msec);
    return (jlong) msec;
}

static jlong
android_media_MediaPlayer2_getDuration(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int64_t msec;
    process_media_player_call( env, thiz, mp->getDuration(&msec), NULL, NULL );
    ALOGV("getDuration: %lld (msec)", (long long)msec);
    return (jlong) msec;
}

static void
android_media_MediaPlayer2_reset(JNIEnv *env, jobject thiz)
{
    ALOGV("reset");
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->reset(), NULL, NULL );
}

static jint
android_media_MediaPlayer2_getAudioStreamType(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    audio_stream_type_t streamtype;
    process_media_player_call( env, thiz, mp->getAudioStreamType(&streamtype), NULL, NULL );
    ALOGV("getAudioStreamType: %d (streamtype)", streamtype);
    return (jint) streamtype;
}

static jboolean
android_media_MediaPlayer2_setParameter(JNIEnv *env, jobject thiz, jint key, jobject java_request)
{
    ALOGV("setParameter: key %d", key);
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    Parcel *request = parcelForJavaObject(env, java_request);
    status_t err = mp->setParameter(key, *request);
    if (err == OK) {
        return true;
    } else {
        return false;
    }
}

static jobject
android_media_MediaPlayer2_getParameter(JNIEnv *env, jobject thiz, jint key)
{
    ALOGV("getParameter: key %d", key);
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    jobject jParcel = createJavaParcelObject(env);
    if (jParcel != NULL) {
        Parcel* nativeParcel = parcelForJavaObject(env, jParcel);
        status_t err = mp->getParameter(key, nativeParcel);
        if (err != OK) {
            env->DeleteLocalRef(jParcel);
            return NULL;
        }
    }
    return jParcel;
}

static void
android_media_MediaPlayer2_setLooping(JNIEnv *env, jobject thiz, jboolean looping)
{
    ALOGV("setLooping: %d", looping);
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setLooping(looping), NULL, NULL );
}

static jboolean
android_media_MediaPlayer2_isLooping(JNIEnv *env, jobject thiz)
{
    ALOGV("isLooping");
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return JNI_FALSE;
    }
    return mp->isLooping() ? JNI_TRUE : JNI_FALSE;
}

static void
android_media_MediaPlayer2_setVolume(JNIEnv *env, jobject thiz, jfloat leftVolume, jfloat rightVolume)
{
    ALOGV("setVolume: left %f  right %f", (float) leftVolume, (float) rightVolume);
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setVolume((float) leftVolume, (float) rightVolume), NULL, NULL );
}

// Sends the request and reply parcels to the media player via the
// binder interface.
static jint
android_media_MediaPlayer2_invoke(JNIEnv *env, jobject thiz,
                                 jobject java_request, jobject java_reply)
{
    sp<MediaPlayer2> media_player = getMediaPlayer(env, thiz);
    if (media_player == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return UNKNOWN_ERROR;
    }

    Parcel *request = parcelForJavaObject(env, java_request);
    Parcel *reply = parcelForJavaObject(env, java_reply);

    request->setDataPosition(0);

    // Don't use process_media_player_call which use the async loop to
    // report errors, instead returns the status.
    return (jint) media_player->invoke(*request, reply);
}

// Sends the new filter to the client.
static jint
android_media_MediaPlayer2_setMetadataFilter(JNIEnv *env, jobject thiz, jobject request)
{
    sp<MediaPlayer2> media_player = getMediaPlayer(env, thiz);
    if (media_player == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return UNKNOWN_ERROR;
    }

    Parcel *filter = parcelForJavaObject(env, request);

    if (filter == NULL ) {
        jniThrowException(env, "java/lang/RuntimeException", "Filter is null");
        return UNKNOWN_ERROR;
    }

    return (jint) media_player->setMetadataFilter(*filter);
}

static jboolean
android_media_MediaPlayer2_getMetadata(JNIEnv *env, jobject thiz, jboolean update_only,
                                      jboolean apply_filter, jobject reply)
{
    sp<MediaPlayer2> media_player = getMediaPlayer(env, thiz);
    if (media_player == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return JNI_FALSE;
    }

    Parcel *metadata = parcelForJavaObject(env, reply);

    if (metadata == NULL ) {
        jniThrowException(env, "java/lang/RuntimeException", "Reply parcel is null");
        return JNI_FALSE;
    }

    metadata->freeData();
    // On return metadata is positioned at the beginning of the
    // metadata. Note however that the parcel actually starts with the
    // return code so you should not rewind the parcel using
    // setDataPosition(0).
    if (media_player->getMetadata(update_only, apply_filter, metadata) == OK) {
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

// This function gets some field IDs, which in turn causes class initialization.
// It is called from a static block in MediaPlayer2, which won't run until the
// first time an instance of this class is used.
static void
android_media_MediaPlayer2_native_init(JNIEnv *env)
{
    jclass clazz;

    clazz = env->FindClass("android/media/MediaPlayer2Impl");
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (fields.context == NULL) {
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;JIIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        return;
    }

    fields.surface_texture = env->GetFieldID(clazz, "mNativeSurfaceTexture", "J");
    if (fields.surface_texture == NULL) {
        return;
    }

    env->DeleteLocalRef(clazz);

    clazz = env->FindClass("android/net/ProxyInfo");
    if (clazz == NULL) {
        return;
    }

    fields.proxyConfigGetHost =
        env->GetMethodID(clazz, "getHost", "()Ljava/lang/String;");

    fields.proxyConfigGetPort =
        env->GetMethodID(clazz, "getPort", "()I");

    fields.proxyConfigGetExclusionList =
        env->GetMethodID(clazz, "getExclusionListAsString", "()Ljava/lang/String;");

    env->DeleteLocalRef(clazz);

    gBufferingParamsFields.init(env);

    // Modular DRM
    FIND_CLASS(clazz, "android/media/MediaDrm$MediaDrmStateException");
    if (clazz) {
        GET_METHOD_ID(gStateExceptionFields.init, clazz, "<init>", "(ILjava/lang/String;)V");
        gStateExceptionFields.classId = static_cast<jclass>(env->NewGlobalRef(clazz));

        env->DeleteLocalRef(clazz);
    } else {
        ALOGE("JNI android_media_MediaPlayer2_native_init couldn't "
              "get clazz android/media/MediaDrm$MediaDrmStateException");
    }

    gPlaybackParamsFields.init(env);
    gSyncParamsFields.init(env);
    gVolumeShaperFields.init(env);
}

static void
android_media_MediaPlayer2_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    ALOGV("native_setup");
    sp<MediaPlayer2> mp = MediaPlayer2::Create();
    if (mp == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }

    // create new listener and give it to MediaPlayer2
    sp<JNIMediaPlayer2Listener> listener = new JNIMediaPlayer2Listener(env, thiz, weak_this);
    mp->setListener(listener);

    // Stow our new C++ MediaPlayer2 in an opaque field in the Java object.
    setMediaPlayer(env, thiz, mp);
}

static void
android_media_MediaPlayer2_release(JNIEnv *env, jobject thiz)
{
    ALOGV("release");
    decVideoSurfaceRef(env, thiz);
    sp<MediaPlayer2> mp = setMediaPlayer(env, thiz, 0);
    if (mp != NULL) {
        // this prevents native callbacks after the object is released
        mp->setListener(0);
        mp->disconnect();
    }
}

static void
android_media_MediaPlayer2_native_finalize(JNIEnv *env, jobject thiz)
{
    ALOGV("native_finalize");
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp != NULL) {
        ALOGW("MediaPlayer2 finalized without being released");
    }
    android_media_MediaPlayer2_release(env, thiz);
}

static void android_media_MediaPlayer2_set_audio_session_id(JNIEnv *env,  jobject thiz,
        jint sessionId) {
    ALOGV("set_session_id(): %d", sessionId);
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAudioSessionId((audio_session_t) sessionId), NULL,
            NULL);
}

static jint android_media_MediaPlayer2_get_audio_session_id(JNIEnv *env,  jobject thiz) {
    ALOGV("get_session_id()");
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }

    return (jint) mp->getAudioSessionId();
}

static void
android_media_MediaPlayer2_setAuxEffectSendLevel(JNIEnv *env, jobject thiz, jfloat level)
{
    ALOGV("setAuxEffectSendLevel: level %f", level);
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAuxEffectSendLevel(level), NULL, NULL );
}

static void android_media_MediaPlayer2_attachAuxEffect(JNIEnv *env,  jobject thiz, jint effectId) {
    ALOGV("attachAuxEffect(): %d", effectId);
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->attachAuxEffect(effectId), NULL, NULL );
}

/////////////////////////////////////////////////////////////////////////////////////
// Modular DRM begin

// TODO: investigate if these can be shared with their MediaDrm counterparts
static void throwDrmStateException(JNIEnv *env, const char *msg, status_t err)
{
    ALOGE("Illegal DRM state exception: %s (%d)", msg, err);

    jobject exception = env->NewObject(gStateExceptionFields.classId,
            gStateExceptionFields.init, static_cast<int>(err),
            env->NewStringUTF(msg));
    env->Throw(static_cast<jthrowable>(exception));
}

// TODO: investigate if these can be shared with their MediaDrm counterparts
static bool throwDrmExceptionAsNecessary(JNIEnv *env, status_t err, const char *msg = NULL)
{
    const char *drmMessage = "Unknown DRM Msg";

    switch (err) {
    case ERROR_DRM_UNKNOWN:
        drmMessage = "General DRM error";
        break;
    case ERROR_DRM_NO_LICENSE:
        drmMessage = "No license";
        break;
    case ERROR_DRM_LICENSE_EXPIRED:
        drmMessage = "License expired";
        break;
    case ERROR_DRM_SESSION_NOT_OPENED:
        drmMessage = "Session not opened";
        break;
    case ERROR_DRM_DECRYPT_UNIT_NOT_INITIALIZED:
        drmMessage = "Not initialized";
        break;
    case ERROR_DRM_DECRYPT:
        drmMessage = "Decrypt error";
        break;
    case ERROR_DRM_CANNOT_HANDLE:
        drmMessage = "Unsupported scheme or data format";
        break;
    case ERROR_DRM_TAMPER_DETECTED:
        drmMessage = "Invalid state";
        break;
    default:
        break;
    }

    String8 vendorMessage;
    if (err >= ERROR_DRM_VENDOR_MIN && err <= ERROR_DRM_VENDOR_MAX) {
        vendorMessage = String8::format("DRM vendor-defined error: %d", err);
        drmMessage = vendorMessage.string();
    }

    if (err == BAD_VALUE) {
        jniThrowException(env, "java/lang/IllegalArgumentException", msg);
        return true;
    } else if (err == ERROR_DRM_NOT_PROVISIONED) {
        jniThrowException(env, "android/media/NotProvisionedException", msg);
        return true;
    } else if (err == ERROR_DRM_RESOURCE_BUSY) {
        jniThrowException(env, "android/media/ResourceBusyException", msg);
        return true;
    } else if (err == ERROR_DRM_DEVICE_REVOKED) {
        jniThrowException(env, "android/media/DeniedByServerException", msg);
        return true;
    } else if (err == DEAD_OBJECT) {
        jniThrowException(env, "android/media/MediaDrmResetException",
                          "mediaserver died");
        return true;
    } else if (err != OK) {
        String8 errbuf;
        if (drmMessage != NULL) {
            if (msg == NULL) {
                msg = drmMessage;
            } else {
                errbuf = String8::format("%s: %s", msg, drmMessage);
                msg = errbuf.string();
            }
        }
        throwDrmStateException(env, msg, err);
        return true;
    }
    return false;
}

static Vector<uint8_t> JByteArrayToVector(JNIEnv *env, jbyteArray const &byteArray)
{
    Vector<uint8_t> vector;
    size_t length = env->GetArrayLength(byteArray);
    vector.insertAt((size_t)0, length);
    env->GetByteArrayRegion(byteArray, 0, length, (jbyte *)vector.editArray());
    return vector;
}

static void android_media_MediaPlayer2_prepareDrm(JNIEnv *env, jobject thiz,
                    jbyteArray uuidObj, jbyteArray drmSessionIdObj)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (uuidObj == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    Vector<uint8_t> uuid = JByteArrayToVector(env, uuidObj);

    if (uuid.size() != 16) {
        jniThrowException(
                          env,
                          "java/lang/IllegalArgumentException",
                          "invalid UUID size, expected 16 bytes");
        return;
    }

    Vector<uint8_t> drmSessionId = JByteArrayToVector(env, drmSessionIdObj);

    if (drmSessionId.size() == 0) {
        jniThrowException(
                          env,
                          "java/lang/IllegalArgumentException",
                          "empty drmSessionId");
        return;
    }

    status_t err = mp->prepareDrm(uuid.array(), drmSessionId);
    if (err != OK) {
        if (err == INVALID_OPERATION) {
            jniThrowException(
                              env,
                              "java/lang/IllegalStateException",
                              "The player must be in prepared state.");
        } else if (err == ERROR_DRM_CANNOT_HANDLE) {
            jniThrowException(
                              env,
                              "android/media/UnsupportedSchemeException",
                              "Failed to instantiate drm object.");
        } else {
            throwDrmExceptionAsNecessary(env, err, "Failed to prepare DRM scheme");
        }
    }
}

static void android_media_MediaPlayer2_releaseDrm(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    status_t err = mp->releaseDrm();
    if (err != OK) {
        if (err == INVALID_OPERATION) {
            jniThrowException(
                              env,
                              "java/lang/IllegalStateException",
                              "Can not release DRM in an active player state.");
        }
    }
}
// Modular DRM end
// ----------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////
// AudioRouting begin
static jboolean android_media_MediaPlayer2_setOutputDevice(JNIEnv *env, jobject thiz, jint device_id)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return false;
    }
    return mp->setOutputDevice(device_id) == NO_ERROR;
}

static jint android_media_MediaPlayer2_getRoutedDeviceId(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return AUDIO_PORT_HANDLE_NONE;
    }
    return mp->getRoutedDeviceId();
}

static void android_media_MediaPlayer2_enableDeviceCallback(
        JNIEnv* env, jobject thiz, jboolean enabled)
{
    sp<MediaPlayer2> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }

    status_t status = mp->enableAudioDeviceCallback(enabled);
    if (status != NO_ERROR) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        ALOGE("enable device callback failed: %d", status);
    }
}

// AudioRouting end
// ----------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////
// AudioTrack.StreamEventCallback begin
static void android_media_MediaPlayer2_native_on_tear_down(JNIEnv *env __unused,
        jobject thiz __unused, jlong callbackPtr, jlong userDataPtr)
{
    JAudioTrack::callback_t callback = (JAudioTrack::callback_t) callbackPtr;
    if (callback != NULL) {
        callback(JAudioTrack::EVENT_NEW_IAUDIOTRACK, (void *) userDataPtr, NULL);
    }
}

static void android_media_MediaPlayer2_native_on_stream_presentation_end(JNIEnv *env __unused,
        jobject thiz __unused, jlong callbackPtr, jlong userDataPtr)
{
    JAudioTrack::callback_t callback = (JAudioTrack::callback_t) callbackPtr;
    if (callback != NULL) {
        callback(JAudioTrack::EVENT_STREAM_END, (void *) userDataPtr, NULL);
    }
}

static void android_media_MediaPlayer2_native_on_stream_data_request(JNIEnv *env __unused,
        jobject thiz __unused, jlong jAudioTrackPtr, jlong callbackPtr, jlong userDataPtr)
{
    JAudioTrack::callback_t callback = (JAudioTrack::callback_t) callbackPtr;
    JAudioTrack* track = (JAudioTrack *) jAudioTrackPtr;
    if (callback != NULL && track != NULL) {
        JAudioTrack::Buffer* buffer = new JAudioTrack::Buffer();

        size_t bufferSizeInFrames = track->frameCount();
        audio_format_t format = track->format();

        size_t bufferSizeInBytes;
        if (audio_has_proportional_frames(format)) {
            bufferSizeInBytes =
                    bufferSizeInFrames * audio_bytes_per_sample(format) * track->channelCount();
        } else {
            // See Javadoc of AudioTrack::getBufferSizeInFrames().
            bufferSizeInBytes = bufferSizeInFrames;
        }

        uint8_t* byteBuffer = new uint8_t[bufferSizeInBytes];
        buffer->mSize = bufferSizeInBytes;
        buffer->mData = (void *) byteBuffer;

        callback(JAudioTrack::EVENT_MORE_DATA, (void *) userDataPtr, buffer);

        if (buffer->mSize > 0 && buffer->mData == byteBuffer) {
            track->write(buffer->mData, buffer->mSize, true /* Blocking */);
        }

        delete[] byteBuffer;
        delete buffer;
    }
}


// AudioTrack.StreamEventCallback end
// ----------------------------------------------------------------------------

static const JNINativeMethod gMethods[] = {
    {
        "nativeHandleDataSourceUrl",
        "(ZJLandroid/media/Media2HTTPService;Ljava/lang/String;[Ljava/lang/String;"
        "[Ljava/lang/String;)V",
        (void *)android_media_MediaPlayer2_handleDataSourceUrl
    },
    {
        "nativeHandleDataSourceFD",
        "(ZJLjava/io/FileDescriptor;JJ)V",
        (void *)android_media_MediaPlayer2_handleDataSourceFD
    },
    {
        "nativeHandleDataSourceCallback",
        "(ZJLandroid/media/Media2DataSource;)V",
        (void *)android_media_MediaPlayer2_handleDataSourceCallback
    },
    {"nativePlayNextDataSource", "(J)V",                        (void *)android_media_MediaPlayer2_playNextDataSource},
    {"_setVideoSurface",    "(Landroid/view/Surface;)V",        (void *)android_media_MediaPlayer2_setVideoSurface},
    {"getBufferingParams", "()Landroid/media/BufferingParams;", (void *)android_media_MediaPlayer2_getBufferingParams},
    {"_setBufferingParams", "(Landroid/media/BufferingParams;)V", (void *)android_media_MediaPlayer2_setBufferingParams},
    {"_prepare",            "()V",                              (void *)android_media_MediaPlayer2_prepare},
    {"_start",              "()V",                              (void *)android_media_MediaPlayer2_start},
    {"_stop",               "()V",                              (void *)android_media_MediaPlayer2_stop},
    {"native_getMediaPlayer2State", "()I",                      (void *)android_media_MediaPlayer2_getMediaPlayer2State},
    {"getVideoWidth",       "()I",                              (void *)android_media_MediaPlayer2_getVideoWidth},
    {"getVideoHeight",      "()I",                              (void *)android_media_MediaPlayer2_getVideoHeight},
    {"native_getMetrics",   "()Landroid/os/PersistableBundle;", (void *)android_media_MediaPlayer2_native_getMetrics},
    {"_setPlaybackParams", "(Landroid/media/PlaybackParams;)V", (void *)android_media_MediaPlayer2_setPlaybackParams},
    {"getPlaybackParams", "()Landroid/media/PlaybackParams;", (void *)android_media_MediaPlayer2_getPlaybackParams},
    {"_setSyncParams",     "(Landroid/media/SyncParams;)V",  (void *)android_media_MediaPlayer2_setSyncParams},
    {"getSyncParams",     "()Landroid/media/SyncParams;",   (void *)android_media_MediaPlayer2_getSyncParams},
    {"_seekTo",             "(JI)V",                            (void *)android_media_MediaPlayer2_seekTo},
    {"_notifyAt",           "(J)V",                             (void *)android_media_MediaPlayer2_notifyAt},
    {"_pause",              "()V",                              (void *)android_media_MediaPlayer2_pause},
    {"isPlaying",           "()Z",                              (void *)android_media_MediaPlayer2_isPlaying},
    {"getCurrentPosition",  "()J",                              (void *)android_media_MediaPlayer2_getCurrentPosition},
    {"getDuration",         "()J",                              (void *)android_media_MediaPlayer2_getDuration},
    {"_release",            "()V",                              (void *)android_media_MediaPlayer2_release},
    {"_reset",              "()V",                              (void *)android_media_MediaPlayer2_reset},
    {"_getAudioStreamType", "()I",                              (void *)android_media_MediaPlayer2_getAudioStreamType},
    {"setParameter",        "(ILandroid/os/Parcel;)Z",          (void *)android_media_MediaPlayer2_setParameter},
    {"getParameter",        "(I)Landroid/os/Parcel;",           (void *)android_media_MediaPlayer2_getParameter},
    {"setLooping",          "(Z)V",                             (void *)android_media_MediaPlayer2_setLooping},
    {"isLooping",           "()Z",                              (void *)android_media_MediaPlayer2_isLooping},
    {"_setVolume",          "(FF)V",                            (void *)android_media_MediaPlayer2_setVolume},
    {"native_invoke",       "(Landroid/os/Parcel;Landroid/os/Parcel;)I",(void *)android_media_MediaPlayer2_invoke},
    {"native_setMetadataFilter", "(Landroid/os/Parcel;)I",      (void *)android_media_MediaPlayer2_setMetadataFilter},
    {"native_getMetadata", "(ZZLandroid/os/Parcel;)Z",          (void *)android_media_MediaPlayer2_getMetadata},
    {"native_init",         "()V",                              (void *)android_media_MediaPlayer2_native_init},
    {"native_setup",        "(Ljava/lang/Object;)V",            (void *)android_media_MediaPlayer2_native_setup},
    {"native_finalize",     "()V",                              (void *)android_media_MediaPlayer2_native_finalize},
    {"getAudioSessionId",   "()I",                              (void *)android_media_MediaPlayer2_get_audio_session_id},
    {"_setAudioSessionId",   "(I)V",                             (void *)android_media_MediaPlayer2_set_audio_session_id},
    {"_setAuxEffectSendLevel", "(F)V",                          (void *)android_media_MediaPlayer2_setAuxEffectSendLevel},
    {"_attachAuxEffect",     "(I)V",                             (void *)android_media_MediaPlayer2_attachAuxEffect},
    // Modular DRM
    { "_prepareDrm", "([B[B)V",                                 (void *)android_media_MediaPlayer2_prepareDrm },
    { "_releaseDrm", "()V",                                     (void *)android_media_MediaPlayer2_releaseDrm },

    // AudioRouting
    {"native_setOutputDevice", "(I)Z",                          (void *)android_media_MediaPlayer2_setOutputDevice},
    {"native_getRoutedDeviceId", "()I",                         (void *)android_media_MediaPlayer2_getRoutedDeviceId},
    {"native_enableDeviceCallback", "(Z)V",                     (void *)android_media_MediaPlayer2_enableDeviceCallback},

    // StreamEventCallback for JAudioTrack
    {"native_stream_event_onTearDown",                "(JJ)V",  (void *)android_media_MediaPlayer2_native_on_tear_down},
    {"native_stream_event_onStreamPresentationEnd",   "(JJ)V",  (void *)android_media_MediaPlayer2_native_on_stream_presentation_end},
    {"native_stream_event_onStreamDataRequest",       "(JJJ)V", (void *)android_media_MediaPlayer2_native_on_stream_data_request},
};

// This function only registers the native methods
static int register_android_media_MediaPlayer2Impl(JNIEnv *env)
{
    return AndroidRuntime::registerNativeMethods(env,
                "android/media/MediaPlayer2Impl", gMethods, NELEM(gMethods));
}

jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_android_media_MediaPlayer2Impl(env) < 0) {
        ALOGE("ERROR: MediaPlayer2 native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

// KTHXBYE
