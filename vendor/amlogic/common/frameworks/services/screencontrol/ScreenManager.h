/*
**
** Copyright 2008, The Android Open Source Project
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

#ifndef ANDROID_SCREENCONTROLCLIENT_H
#define ANDROID_SCREENCONTROLCLIENT_H

#include <utils/Errors.h>
#include <string>
#include <vector>
#include <utils/RefBase.h>
#include <binder/MemoryDealer.h>
#include <android/native_window.h>
#include <gui/Surface.h>
#include <gui/IGraphicBufferProducer.h>
#include <ui/GraphicBuffer.h>
#include <cutils/compiler.h>
#include <stdint.h>
#include <binder/Binder.h>
#include <sys/types.h>
#include <utils/String16.h>

#include <media/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>

#include <utils/Errors.h>  // for status_t
#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include <binder/BinderService.h>

#include <android/native_window.h>
#include <hardware/hardware.h>
#include "../../../../../hardware/amlogic/screen_source/aml_screen.h"
#include "SysWrite.h"
#include <utils/List.h>
#include <utils/threads.h>

namespace android {

#define SCREENCONTROL_GRALLOC_USAGE  ( GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_NEVER )
#define kMetadataBufferTypeCanvasSource 3
#define MODE_LEN 8
#define PROP_ISMARCONI  "ro.vendor.screencontrol.porttype"

enum SCREENCONTROLDATATYPE{
    SCREENCONTROL_CANVAS_TYPE,
    SCREENCONTROL_HANDLE_TYPE,
    SCREENCONTROL_RAWDATA_TYPE,
};

enum aml_capture_source_type {
    AML_CAPTURE_VIDEO = 0,
    AML_CAPTURE_OSD_VIDEO
};

class ScreenManager  : virtual public RefBase {
public:
    ScreenManager();
    virtual ~ScreenManager();

    virtual status_t init(int32_t width,
                          int32_t height,
			  int32_t source_type,
                          int32_t framerate,
                          SCREENCONTROLDATATYPE data_type,
                          int32_t* client_id,
                          const sp<IGraphicBufferProducer> &gbp);

    virtual status_t uninit(int32_t client_id);

    // For the MediaSource interface for use by StageFrightRecorder:
    virtual status_t start(int32_t client_id);
    virtual status_t stop(int32_t client_id);
    virtual status_t readBuffer(int32_t client_id, sp<IMemory> buffer, int64_t* pts);
    virtual status_t freeBuffer(int32_t client_id, sp<IMemory> buffer);

    static ScreenManager* instantiate();
    virtual sp<MetaData> getFormat(int32_t client_id);

    // Get / Set the frame rate used for encoding. Default fps = 30
    virtual int32_t getFrameRate( );

    // The call for the StageFrightRecorder to tell us that
    // it is done using the MediaBuffer data so that its state
    // can be set to FREE for dequeuing

    // end of MediaSource interface

    // getTimestamp retrieves the timestamp associated with the image
    // set by the most recent call to read()
    //
    // The timestamp is in nanoseconds, and is monotonically increasing. Its
    // other semantics (zero point, etc) are source-dependent and should be
    // documented by the source.
    virtual int64_t getTimestamp();

    // isMetaDataStoredInVideoBuffers tells the encoder whether we will
    // pass metadata through the buffers. Currently, it is force set to true
    virtual bool isMetaDataStoredInVideoBuffers() const;

    // To be called before start()
    virtual status_t setMaxAcquiredBufferCount(size_t count);

    // To be called before start()
    virtual status_t setUseAbsoluteTimestamps();

    virtual int dataCallBack(aml_screen_buffer_info_t *buffer);

    virtual status_t setVideoRotation(int32_t client_id, int degree);
    virtual status_t setVideoCrop(int32_t client_id, const int32_t x, const int32_t y, const int32_t width, const int32_t height);
    bool mIsScreenRecord;

private:
    typedef struct ScreenClient_S{
        int32_t width;
        int32_t height;
        int32_t framerate;
        bool isPrimateClient;
        SCREENCONTROLDATATYPE data_type;
        int32_t mClient_id;
    }ScreenClient;

    typedef struct FrameBufferInfo_s{
        long* buf_ptr;
        unsigned canvas;
        int64_t timestampUs;
    }FrameBufferInfo;

    status_t reset(void);

    // The permenent width and height of SMS buffers
    int mWidth;
    int mHeight;
    int mSourceType;

    bool mIsSoftwareEncoder;
    // mCurrentTimestamp is the timestamp for the current texture. It
    // gets set to mLastQueuedTimestamp each time updateTexImage is called.
    int64_t mCurrentTimestamp;

    // mMutex is the mutex used to prevent concurrent access to the member
    // variables of ScreenSource objects. It must be locked whenever the
    // member variables are accessed.
    Mutex mLock;

    ////////////////////////// For MediaSource
    // Set to a default of 30 fps if not specified by the client side
    int32_t mFrameRate;

    // mStarted is a flag to check if the recording is going on
    bool mStarted;

    // mStarted is a flag to check if the recording is going on
    bool mError;

    // mNumFramesReceived indicates the number of frames recieved from
    // the client side
    int mNumFramesReceived;
    // mNumFramesEncoded indicates the number of frames passed on to the
    // encoder
    int mNumFramesEncoded;

    // mFirstFrameTimestamp is the timestamp of the first received frame.
    // It is used to offset the output timestamps so recording starts at time 0.
    int64_t mFirstFrameTimestamp;
    // mStartTimeNs is the start time passed into the source at start, used to
    // offset timestamps.

    int64_t mStartTimeOffsetUs;

    size_t mMaxAcquiredBufferCount;

    bool mUseAbsoluteTimestamps;

    bool mCanvasMode;

    int64_t mStartTimeUs;
    int64_t bufferTimeUs;
    int64_t mFrameCount;
    Condition mFrameAvailableCondition;
    List<FrameBufferInfo*> mCanvasFramesReceived;
    List<MediaBuffer*> mRawBufferQueue;

    int64_t mTimeBetweenFrameCaptureUs;

    int32_t mDropFrame;

    KeyedVector<int, ScreenClient* > mClientList;
    bool mCanvasClientExist;
//    sp<MemoryDealer> mScreenManagerDealer;
    sp<IMemory> mBufferGet;
    sp<ANativeWindow> mANativeWindow;
    int mBufferSize;

    int32_t mCorpX;
    int32_t mCorpY;
    int32_t mCorpWidth;
    int32_t mCorpHeight;

    SysWrite *pSysWrite = NULL;
    aml_screen_module_t* mScreenModule;
    aml_screen_device_t* mScreenDev;
    struct ScreenControlDeathRecipient : public android::hardware::hidl_death_recipient  {
        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    };
    sp<ScreenControlDeathRecipient> mDeathRecipient = nullptr;
    ScreenManager* mScreenManager;

};

}; // namespace android

#endif // ANDROID_SCREENCONTROLCLIENT_H
