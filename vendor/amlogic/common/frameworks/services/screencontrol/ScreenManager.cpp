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

#define LOG_TAG "ScreenManager"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <android-base/logging.h>

#include <ScreenManager.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <OMX_IVCommon.h>
#include <MetadataBufferType.h>

#include <ui/GraphicBuffer.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <media/stagefright/MediaBuffer.h>

#include <Media2Ts/tspack.h>

#include <gui/ISurfaceComposer.h>
#include <OMX_Component.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include <private/gui/ComposerService.h>

#include <ScreenManager.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/videodev2.h>

namespace android {

#define BOUNDRY 32
#define ALIGN(x) (x + (BOUNDRY) - 1)& ~((BOUNDRY) - 1)

#define MAX_CLIENT 4
static const int64_t VDIN_MEDIA_SOURCE_TIMEOUT_NS = 3000000000LL;

static void VdinDataCallBack(void *user, aml_screen_buffer_info_t *buffer){
    ScreenManager *source = static_cast<ScreenManager *>(user);
    source->dataCallBack(buffer);
    return;
}

ScreenManager::ScreenManager() :
    mCurrentTimestamp(0),
    mFrameRate(30),
    mStarted(false),
    mError(false),
    mNumFramesReceived(0),
    mNumFramesEncoded(0),
    mFirstFrameTimestamp(0),
    mMaxAcquiredBufferCount(4),  // XXX double-check the default
    mUseAbsoluteTimestamps(false),
    mDropFrame(2),
    mBufferGet(NULL),
    mCanvasClientExist(0),
    bufferTimeUs(0),
    mFrameCount(0),
    mTimeBetweenFrameCaptureUs(0),
    mScreenModule(NULL),
    mIsSoftwareEncoder(false),
    mIsScreenRecord(false),
    mScreenDev(NULL){

    mCorpX = mCorpY = mCorpWidth = mCorpHeight =0;

//    mScreenManagerDealer = new MemoryDealer(10240, "ScreenControl");
    int fd = -1;
    fd = open("/dev/amvenc_avc", O_RDWR);
    if (fd < 0) {
        mIsSoftwareEncoder = true;
        ALOGE("%s Open /dev/amvenc_avc failed, use software encoder instead!\n", __FUNCTION__);
    } else {
        close(fd);
        fd = -1;
    }

    mRawBufferQueue.clear();

    ALOGE("[%s %d] Construct", __FUNCTION__, __LINE__);
}

ScreenManager::~ScreenManager() {
    ALOGE("~ScreenManager");
    CHECK(!mStarted);

    reset();

    if (mScreenDev)
        mScreenDev->common.close((struct hw_device_t *)mScreenDev);

//    mScreenManagerDealer.clear();
    delete pSysWrite;
}

ScreenManager* ScreenManager::instantiate() {
    ScreenManager *mScreenControl = new ScreenManager();
    return mScreenControl;
}

status_t ScreenManager::init(int32_t width,
                                    int32_t height,
				    int32_t source_type,
                                    int32_t framerate,
                                    SCREENCONTROLDATATYPE data_type,
                                    int32_t* client_id,
                                    const sp<IGraphicBufferProducer> &gbp) {
    Mutex::Autolock autoLock(mLock);
    int clientTotalNum;
    int clientNum = -1;
    clientTotalNum = mClientList.size();

    ALOGE("[%s %d]  native window:0x%x clientTotalNum:%d width:%d height:%d framerate:%d data_type:%d", __FUNCTION__, __LINE__,
             gbp.get(), clientTotalNum, width, height, framerate, data_type);

    if (clientTotalNum >= MAX_CLIENT) {
        ALOGE("[%s %d] clientTotalNum:%d ", __FUNCTION__, __LINE__, clientTotalNum);
        return !OK;
    }

    ScreenClient* Client_tmp = (ScreenClient*)malloc(sizeof(ScreenClient));

    Client_tmp->width = width;
    Client_tmp->height = height;
    Client_tmp->framerate = framerate;
    Client_tmp->isPrimateClient = 0;
    Client_tmp->data_type = data_type;

    if (clientTotalNum == 0) {
        clientNum = 1;
    } else {
        ScreenClient* client_temp;
        for (int i = 0; i < clientTotalNum; i++) {
            client_temp = mClientList.valueAt(i);
            if (client_temp->mClient_id != i + 1) {
                clientNum = i + 1;
        }
    }

    if (clientNum == -1)
        clientNum = clientTotalNum + 1;
    }

    ALOGE("[%s %d] clientNum:%d clientTotalNum:%d", __FUNCTION__, __LINE__,  clientNum, clientTotalNum);

    Client_tmp->mClient_id = clientNum;
    *client_id = clientNum;

    if (SCREENCONTROL_CANVAS_TYPE == data_type) {
        int client_num = 0;
        client_num = mClientList.size();
        ScreenClient* client_local;

        for (int i = 0; i < client_num; i++) {
            client_local = mClientList.valueAt(i);
            if (client_local->data_type == SCREENCONTROL_CANVAS_TYPE) {
                ALOGE("[%s %d] screen source owned canvas client already, so reject another canvas client", __FUNCTION__, __LINE__);
                return !OK;
            }
        }

        mWidth = width;
        mHeight = height;
	mSourceType = source_type;
	mFrameRate = framerate;
        mBufferSize = mWidth * mHeight * 3/2;

    } else if (SCREENCONTROL_RAWDATA_TYPE == data_type) {
        ALOGE("[%s %d] clientTotalNum:%d width:%d height:%d framerate:%d data_type:%d", __FUNCTION__, __LINE__,
            clientTotalNum, width, height, framerate, data_type);
        if (clientTotalNum == 0) {
            mWidth = width;
            mHeight = height;
	    mSourceType = source_type;
	    mFrameRate = framerate;
        }
    }else if(SCREENCONTROL_HANDLE_TYPE == data_type) {
        if (gbp != NULL) {
            mANativeWindow = new Surface(gbp);
            if (mANativeWindow != NULL) {
                // Set gralloc usage bits for window.
                int err = native_window_set_usage(mANativeWindow.get(), SCREENCONTROL_GRALLOC_USAGE);
                if (err != 0) {
                    ALOGE("native_window_set_usage failed: %s\n", strerror(-err));
                    if (ENODEV == err) {
                        ALOGE("Preview surface abandoned!");
                        mANativeWindow = NULL;
                    }
                }

                ///Set the number of buffers needed for camera preview
                err = native_window_set_buffer_count(mANativeWindow.get(), 4);
                if (err != 0) {
                    ALOGE("native_window_set_buffer_count failed: %s (%d)", strerror(-err), -err);
                    if (ENODEV == err) {
                        ALOGE("Preview surface abandoned!");
                        mANativeWindow = NULL;
                    }
                }

                // Set window geometry
                err = native_window_set_buffers_geometry(
                    mANativeWindow.get(),
                    ALIGN(1280),
                    720,
                    HAL_PIXEL_FORMAT_YCrCb_420_SP);

                if (err != 0) {
                    ALOGE("native_window_set_buffers_geometry failed: %s", strerror(-err));
                    if ( ENODEV == err ) {
                        ALOGE("Surface abandoned!");
                        mANativeWindow = NULL;
                    }
                }
                err = native_window_set_scaling_mode(mANativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
                if (err != 0) {
                    ALOGW("Failed to set scaling mode: %d", err);
                }
            }
        }

        if (clientTotalNum == 0) {
            mWidth = 1280;
            mHeight = 720;
            mBufferSize = mWidth * mHeight * 3/2;
        }
    }

    ALOGE("[%s %d] clientNum:%d", __FUNCTION__, __LINE__, clientNum);
    mClientList.add(clientNum, Client_tmp);

    return OK;
}

status_t ScreenManager::uninit(int32_t client_id) {
    ALOGE("[%s %d] client_id:%d", __FUNCTION__, __LINE__, client_id);
    Mutex::Autolock autoLock(mLock);

    ScreenClient* client;
    client = mClientList.valueFor(client_id);
    mClientList.removeItem(client_id);

    free(client);
    return OK;
}

nsecs_t ScreenManager::getTimestamp() {
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    Mutex::Autolock autoLock(mLock);
    return mCurrentTimestamp;
}

bool ScreenManager::isMetaDataStoredInVideoBuffers() const {
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    return true;
}

int32_t ScreenManager::getFrameRate( )
{
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    Mutex::Autolock autoLock(mLock);
    return mFrameRate;
}

status_t ScreenManager::setVideoRotation(int32_t client_id, int degree)
{
    int angle;

    ALOGE("[%s %d] setVideoRotation degree:%x", __FUNCTION__, __LINE__, degree);

    if (degree == 0)
        angle = 0;
    else if (degree == 1)
        angle = 270;
    else if (degree == 2)
        angle = 180;
    else if (degree == 3)
        angle = 90;
    else {
        ALOGE("degree is not right");
        return !OK;
    }

    if (mScreenDev != NULL) {
        ALOGE("[%s %d] setVideoRotation angle:%x", __FUNCTION__, __LINE__, angle);
        mScreenDev->ops.set_rotation(mScreenDev, angle);
    }

    return OK;
}

status_t ScreenManager::setVideoCrop(int32_t client_id, const int32_t x, const int32_t y, const int32_t width, const int32_t height)
{
    ALOGE("[%s %d] setVideoCrop x:%d y:%d width:%d height:%d", __FUNCTION__, __LINE__, x, y, width, height);

    mCorpX = x;
    mCorpY = y;
    mCorpWidth = width;
    mCorpHeight = height;

    return OK;
}

status_t ScreenManager::start(int32_t client_id)
{
    Mutex::Autolock autoLock(mLock);

    int client_num = mClientList.size();

    if (!mScreenModule || client_num == 1) {
        if (hw_get_module(AML_SCREEN_HARDWARE_MODULE_ID, (const hw_module_t **)&mScreenModule) < 0) {
            ALOGE("[%s %d] can`t get AML_SCREEN_HARDWARE_MODULE_ID module", __FUNCTION__, __LINE__);
            return !OK;
        }

	char sourceType[2];
	int port_type;
	if (mSourceType == AML_CAPTURE_VIDEO) { //video only
	    strcpy(sourceType, "1");
	    char productValue[MODE_LEN] = {0};
	    pSysWrite->getPropertyString(PROP_ISMARCONI, productValue, "0");
	    ALOGE("[%s %d] productValue:%s", __FUNCTION__, __LINE__, productValue);
	    if (!strcmp(productValue, "1")) { //marconi
	        port_type = 0x1100a002;
		ALOGE("[%s %d] sourcetype:%s, port_type is 0x1100a002", __FUNCTION__, __LINE__, sourceType);
	    } else { //other product
	        port_type = 0x1100a001;
	        ALOGE("[%s %d] sourcetype:%s, port_type is 0x1100a001", __FUNCTION__, __LINE__, sourceType);
	    }
        } else if(mSourceType == AML_CAPTURE_OSD_VIDEO) {
            strcpy(sourceType, "1");
	    char productValue[MODE_LEN] = {0};
	    pSysWrite->getPropertyString(PROP_ISMARCONI, productValue, "0");
	    ALOGE("[%s %d] productValue:%s", __FUNCTION__, __LINE__, productValue);
	    if (!strcmp(productValue, "1")) { //marconi
	        port_type = 0x1100a006;
		ALOGE("[%s %d] sourcetype:%s, port_type is 0x1100a0006", __FUNCTION__, __LINE__, sourceType);
	    } else {
		port_type = 0x1100a000;
                ALOGE("[%s %d] sourcetype:%s, port_type is 0x1100a000", __FUNCTION__, __LINE__, sourceType);
	    }
        }else {
            ALOGE("[%s %d] For now ,we don't capture osd only by AML_SCREEN_HARDWARE_MODULE_ID module!", __FUNCTION__, __LINE__);
            return !OK;
        }

        if (mScreenModule->common.methods->open((const hw_module_t *)mScreenModule, sourceType,
                (struct hw_device_t**)&mScreenDev) < 0) {
            mScreenModule = NULL;
            ALOGE("[%s %d] open AML_SCREEN_SOURCE fail", __FUNCTION__, __LINE__);
            return !OK;
        }

        ALOGE("[%s %d] start AML_SCREEN_SOURCE", __FUNCTION__, __LINE__);

        mScreenDev->ops.set_port_type(mScreenDev, port_type);
        mScreenDev->ops.set_frame_rate(mScreenDev, mFrameRate);
        if (mIsSoftwareEncoder && mIsScreenRecord) {
            mScreenDev->ops.set_format(mScreenDev, mWidth, mHeight, V4L2_PIX_FMT_NV12);
        } else {
            mScreenDev->ops.set_format(mScreenDev, mWidth, mHeight, V4L2_PIX_FMT_NV21);
        }
        mScreenDev->ops.setDataCallBack(mScreenDev, VdinDataCallBack, (void*)this);
        mScreenDev->ops.set_amlvideo2_crop(mScreenDev, mCorpX, mCorpY, mCorpWidth, mCorpHeight);
        mScreenDev->ops.start(mScreenDev);
    }

    SCREENCONTROLDATATYPE source_data_type;
    ScreenClient* client;
    ALOGE("[%s %d] client_id:%d client_num:%d", __FUNCTION__, __LINE__, client_id, client_num);
    client = mClientList.valueFor(client_id);
    source_data_type = client->data_type;

    if (SCREENCONTROL_HANDLE_TYPE == source_data_type && mANativeWindow != NULL) {
        mANativeWindow->incStrong((void*)ANativeWindow_acquire);
    }

    if (SCREENCONTROL_CANVAS_TYPE == source_data_type) {
        mCanvasClientExist = 1;
    }

    mStartTimeOffsetUs = 0;
    mNumFramesReceived = mNumFramesEncoded = 0;
    mStartTimeUs = systemTime(SYSTEM_TIME_MONOTONIC)/1000;

    mStarted = true;

    return OK;
}

status_t ScreenManager::setMaxAcquiredBufferCount(size_t count) {
    ALOGE("setMaxAcquiredBufferCount(%d)", count);
    Mutex::Autolock autoLock(mLock);

    CHECK_GT(count, 1);
    mMaxAcquiredBufferCount = count;

    return OK;
}

status_t ScreenManager::setUseAbsoluteTimestamps() {
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    Mutex::Autolock autoLock(mLock);
    mUseAbsoluteTimestamps = true;

    return OK;
}

status_t ScreenManager::stop(int32_t client_id)
{
    Mutex::Autolock autoLock(mLock);

    int client_num = mClientList.size();
    ALOGE("[%s %d] client_num:%d client_id:%d", __FUNCTION__, __LINE__, client_num, client_id);

    if (1 == client_num)
        reset();

    SCREENCONTROLDATATYPE source_data_type;
    ScreenClient* client;
    client = mClientList.valueFor(client_id);
    source_data_type = client->data_type;

    if (SCREENCONTROL_CANVAS_TYPE == source_data_type)
        mCanvasClientExist = 0;

    if (SCREENCONTROL_HANDLE_TYPE == source_data_type && mANativeWindow != NULL) {
        mANativeWindow->decStrong((void*)ANativeWindow_acquire);
    }

    if (SCREENCONTROL_RAWDATA_TYPE == source_data_type) {
        while (!mRawBufferQueue.empty()) {
            ALOGE("[%s %d] free buffer", __FUNCTION__, __LINE__);
            MediaBuffer* rawBuffer = *mRawBufferQueue.begin();
            mRawBufferQueue.erase(mRawBufferQueue.begin());
            if (rawBuffer != NULL)
                rawBuffer->release();
        }
    }
    return OK;
}

sp<MetaData> ScreenManager::getFormat(int32_t client_id)
{
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);

    Mutex::Autolock autoLock(mLock);
    sp<MetaData> meta = new MetaData;

    meta->setInt32(kKeyWidth, mWidth);
    meta->setInt32(kKeyHeight, mHeight);
    if (mIsSoftwareEncoder && mIsScreenRecord) {
        meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420SemiPlanar);
    } else {
        meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420Planar);
    }
    meta->setInt32(kKeyStride, mWidth);
    meta->setInt32(kKeySliceHeight, mHeight);

    meta->setInt32(kKeyFrameRate, mFrameRate);
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_RAW);
    return meta;
}

status_t ScreenManager::readBuffer(int32_t client_id, sp<IMemory> buffer, int64_t* pts)
{
    Mutex::Autolock autoLock(mLock);

    long buff_info[3] = {0,0,0};
    int ret = 0;
    int count = 0;
    FrameBufferInfo* frame = NULL;
    SCREENCONTROLDATATYPE source_data_type;

    ScreenClient* client;
    client = mClientList.valueFor(client_id);
    source_data_type = client->data_type;

    if (!mStarted) {
        ALOGE("[%s %d]", __FUNCTION__, __LINE__);
        return !OK;
    }

    if (SCREENCONTROL_CANVAS_TYPE == source_data_type) {
        if (mCanvasFramesReceived.empty())
            return !OK;

        frame = *mCanvasFramesReceived.begin();
        mCanvasFramesReceived.erase(mCanvasFramesReceived.begin());

        //ALOGE("ptr:%x canvas:%d", frame->buf_ptr, frame->canvas);

        buff_info[0] = kMetadataBufferTypeCanvasSource;
        buff_info[1] = (long)frame->buf_ptr;
        buff_info[2] = (long)frame->canvas;
        memcpy((uint8_t *)buffer->pointer(), &buff_info[0],sizeof(buff_info));

        *pts = frame->timestampUs;

        ALOGE("[%s %d] buf_ptr:%x canvas:%x pts:%llx size:%d OK:%d", __FUNCTION__, __LINE__,
                frame->buf_ptr, frame->canvas, frame->timestampUs, mCanvasFramesReceived.size(), OK);

        if (frame)
            delete frame;

        return OK;

    }

    if (SCREENCONTROL_RAWDATA_TYPE == source_data_type && !mRawBufferQueue.empty()) {
        MediaBuffer* rawBuffer = *mRawBufferQueue.begin();
        mRawBufferQueue.erase(mRawBufferQueue.begin());
        if (rawBuffer != NULL && buffer != NULL) {
            if (rawBuffer->data()) {
                memmove((char *)buffer->pointer(), (char *)rawBuffer->data(), mWidth*mHeight*3/2);
                rawBuffer->release();
            } else
                ALOGE("[%s] rawBuffer invalid data(null)", __func__);
        } else {
            ALOGE("read buffer error T_T");
            return !OK;
        }
        *pts = 0;
    } else {
        //ALOGE("[%s %d] read raw data fail", __FUNCTION__, __LINE__);
        return !OK;
    }

    if (frame)
        delete frame;

    return OK;
}

status_t ScreenManager::freeBuffer(int32_t client_id, sp<IMemory>buffer) {

    Mutex::Autolock autoLock(mLock);

    if (mStarted == false)
        return OK;

    SCREENCONTROLDATATYPE source_data_type;
    ScreenClient* client;
    client = mClientList.valueFor(client_id);
    source_data_type = client->data_type;

    if (SCREENCONTROL_CANVAS_TYPE == source_data_type) {
        long buff_info[3] = {0,0,0};
        memcpy(&buff_info[0],(uint8_t *)buffer->pointer(), sizeof(buff_info));

        if (mScreenDev)
            mScreenDev->ops.release_buffer(mScreenDev, (long *)buff_info[1]);
        }

    ++mNumFramesEncoded;

    return OK;
}

int ScreenManager::dataCallBack(aml_screen_buffer_info_t *buffer){
    int ret = NO_ERROR;
    long buff_info[3] = {0,0,0};
    int status = OK;
    ANativeWindowBuffer* buf;
    void *src = NULL;
    void *dest = NULL;

    if ((mStarted) && (mError == false)) {
        if (buffer == NULL || (buffer->buffer_mem == 0)) {
            ALOGE("aquire_buffer fail, ptr:0x%x", buffer);
            return BAD_VALUE;
        }
        if ((mCanvasMode == true) && (buffer->buffer_canvas == 0)) {
            mError = true;
            ALOGE("Could get canvas info from device!");
            return BAD_VALUE;
        }

        mFrameCount++;

        ++mNumFramesReceived;
        {
            Mutex::Autolock autoLock(mLock);

            int client_num = 0;
            client_num = mClientList.size();
            ScreenClient* client;

            //first, process hdmi and screencatch.
            for (int i = 0; i < client_num; i++) {
                client = mClientList.valueAt(i);
                switch (client->data_type) {
                    case SCREENCONTROL_HANDLE_TYPE: {
                        src = buffer->buffer_mem;

                        if (mANativeWindow.get() == NULL) {
                            ALOGE("Null window");
                            return BAD_VALUE;
                        }
                        ret = mANativeWindow->dequeueBuffer_DEPRECATED(mANativeWindow.get(), &buf);
                        if (ret != 0) {
                            ALOGE("dequeue buffer failed :%s (%d)",strerror(-ret), -ret);
                            return BAD_VALUE;
                        }
                        mANativeWindow->lockBuffer_DEPRECATED(mANativeWindow.get(), buf);
                        sp<GraphicBuffer> graphicBuffer(new GraphicBuffer(buf->handle, GraphicBuffer::WRAP_HANDLE,buf->width, buf->height,buf->format,buf->layerCount,buf->usage, buf->stride));
                        graphicBuffer->lock(SCREENCONTROL_GRALLOC_USAGE, (void **)&dest);
                        if (dest == NULL) {
                            ALOGE("Invalid Gralloc Handle");
                            return BAD_VALUE;
                        }
                        memcpy(dest, src, mBufferSize);
                        graphicBuffer->unlock();
                        mANativeWindow->queueBuffer_DEPRECATED(mANativeWindow.get(), buf);
                        graphicBuffer.clear();
                        ALOGE("queue one buffer to native window");

                        if (client_num == 1) {//release buffer
                            mScreenDev->ops.release_buffer(mScreenDev, buffer->buffer_mem);
                        }
                    } break;
                    case SCREENCONTROL_RAWDATA_TYPE:{
                        if (mRawBufferQueue.size() < 60) {
                            MediaBuffer* accessUnit = new MediaBuffer(client->width*client->height*3/2);
                            if (accessUnit != NULL) {
                                memmove(accessUnit->data(), buffer->buffer_mem, client->width*client->height*3/2);
                                mRawBufferQueue.push_back(accessUnit);
                            } else {
                                ALOGE("datacallback error: accessUnit or buffer = NULL");
                            }
                        }

                        if (mCanvasClientExist == 0) {//release buffer
                            mScreenDev->ops.release_buffer(mScreenDev, buffer->buffer_mem);
                        }
                    } break;
                    default:{
                        if (mCanvasClientExist == 0) {//release buffer
                            mScreenDev->ops.release_buffer(mScreenDev, buffer->buffer_mem);
                        }
                    }
                }
            }

           //second, process canvas
            if (mCanvasClientExist == 1) {
                for (int i = 0; i < client_num; i++) {
                    client = mClientList.valueAt(i);
                    if (client->data_type == SCREENCONTROL_CANVAS_TYPE) {
                        buff_info[0] = kMetadataBufferTypeCanvasSource;
                        buff_info[1] = (long)buffer->buffer_mem;
                        buff_info[2] = buffer->buffer_canvas;

//                        if (mBufferGet == NULL)
//                            mBufferGet = mScreenManagerDealer->allocate(3*sizeof(unsigned));

                        FrameBufferInfo* frame = new FrameBufferInfo;

                        frame->buf_ptr = buffer->buffer_mem;
                        frame->canvas = buffer->buffer_canvas;
                        frame->timestampUs = 0;
                        mCanvasFramesReceived.push_back(frame);

                        if (status != OK) {
                            mScreenDev->ops.release_buffer(mScreenDev, buffer->buffer_mem);
                        }
                        mCanvasClientExist = 1;
                    }
                }
            }
            mFrameAvailableCondition.signal();
        }
    }
    return ret;
}

status_t ScreenManager::reset(void) {

    ALOGE("[%s %d]", __FUNCTION__, __LINE__);

    FrameBufferInfo* frame = NULL;

    if (!mStarted) {
        ALOGE("ScreenSource::reset X Do nothing");
        return OK;
    }

    {
        mStarted = false;
    }

    if (mScreenDev)
        mScreenDev->ops.stop(mScreenDev);

    {
        mFrameAvailableCondition.signal();
        while (!mCanvasFramesReceived.empty()) {
            frame = *mCanvasFramesReceived.begin();
            mCanvasFramesReceived.erase(mCanvasFramesReceived.begin());
            if (frame != NULL)
                delete frame;
        }
    }

    if (mScreenDev)
        mScreenDev->common.close((struct hw_device_t *)mScreenDev);

    mScreenModule = NULL;

    ALOGE("ScreenSource::reset done");
    return OK;
}

void ScreenManager::ScreenControlDeathRecipient::serviceDied(uint64_t cookie,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
    LOG(ERROR) << "screen control service died. need release some resources";
}

}; // namespace android
