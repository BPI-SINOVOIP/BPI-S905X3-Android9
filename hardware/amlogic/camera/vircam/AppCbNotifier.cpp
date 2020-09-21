/*
 * Copyright (C) 2011 The Android Open Source Project
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




#define LOG_TAG "CAMHAL_AppCbNotifier   "

#include "VirtualCamHal.h"
#include "VideoMetadata.h"
#include "Encoder_libjpeg.h"
#include <MetadataBufferType.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include "NV12_resize.h"

#include <gralloc_priv.h>
#ifndef ALIGN
#define ALIGN(b,w) (((b)+((w)-1))/(w)*(w))
#endif

namespace android {

const int AppCbNotifier::NOTIFIER_TIMEOUT = -1;
KeyedVector<void*, sp<Encoder_libjpeg> > gVEncoderQueue;

void AppCbNotifierEncoderCallback(void* main_jpeg,
                                        void* thumb_jpeg,
                                        CameraFrame::FrameType type,
                                        void* cookie1,
                                        void* cookie2,
                                        void* cookie3)
{
    if (cookie1) {
        AppCbNotifier* cb = (AppCbNotifier*) cookie1;
        cb->EncoderDoneCb(main_jpeg, thumb_jpeg, type, cookie2, cookie3);
    }
}

/*--------------------NotificationHandler Class STARTS here-----------------------------*/

void AppCbNotifier::EncoderDoneCb(void* main_jpeg, void* thumb_jpeg, CameraFrame::FrameType type, void* cookie1, void* cookie2)
{
    camera_memory_t* encoded_mem = NULL;
    Encoder_libjpeg::params *main_param = NULL, *thumb_param = NULL;
    size_t jpeg_size;
    uint8_t* src = NULL;
    sp<Encoder_libjpeg> encoder = NULL;

    LOG_FUNCTION_NAME;

    camera_memory_t* picture = NULL;

    {
        Mutex::Autolock lock(mLock);

        if (!main_jpeg) {
            goto exit;
        }

        encoded_mem = (camera_memory_t*) cookie1;
        main_param = (Encoder_libjpeg::params *) main_jpeg;
        jpeg_size = main_param->jpeg_size;
        src = main_param->src;

        if(encoded_mem && encoded_mem->data && (jpeg_size > 0)) {
            if (cookie2) {
                ExifElementsTable* exif = (ExifElementsTable*) cookie2;
                Section_t* exif_section = NULL;

                exif->insertExifToJpeg((unsigned char*) encoded_mem->data, jpeg_size);

                if(thumb_jpeg) {
                    thumb_param = (Encoder_libjpeg::params *) thumb_jpeg;
                    if((thumb_param->in_width>0)&&(thumb_param->in_height>0)&&(thumb_param->out_width>0)&&(thumb_param->out_height>0))
                        exif->insertExifThumbnailImage((const char*)thumb_param->dst,(int)thumb_param->jpeg_size);
                }

                exif_section = FindSection(M_EXIF);

                if (exif_section) {
                    picture = mRequestMemory(-1, jpeg_size + exif_section->Size, 1, NULL);
                    if (picture && picture->data) {
                        exif->saveJpeg((unsigned char*) picture->data, jpeg_size + exif_section->Size);
                    }
                }
                delete exif;
                cookie2 = NULL;
            } else {
                picture = mRequestMemory(-1, jpeg_size, 1, NULL);
                if (picture && picture->data) {
                    memcpy(picture->data, encoded_mem->data, jpeg_size);
                }
            }
        }
    } // scope for mutex lock

    if (!mRawAvailable) {
        dummyRaw();
    } else {
        mRawAvailable = false;
    }

    if (mNotifierState == AppCbNotifier::NOTIFIER_STARTED) {
        mFrameProvider->returnFrame(src, type);
    }

    // Send the callback to the application only if the notifier is started and the message is enabled
    if(picture && (mNotifierState==AppCbNotifier::NOTIFIER_STARTED) &&
                  (mCameraHal->msgTypeEnabled(CAMERA_MSG_COMPRESSED_IMAGE)))
    {
        Mutex::Autolock lock(mBurstLock);
#if 0 //TODO: enable burst mode later
        if ( mBurst )
        {
            `(CAMERA_MSG_BURST_IMAGE, JPEGPictureMemBase, mCallbackCookie);
        }
        else
#endif
        {
            mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, picture, 0, NULL, mCallbackCookie);
        }
    }

 exit:

    if (main_jpeg) {
        free(main_jpeg);
    }

    if (thumb_jpeg) {
        if (((Encoder_libjpeg::params *) thumb_jpeg)->dst) {
            free(((Encoder_libjpeg::params *) thumb_jpeg)->dst);
        }
        free(thumb_jpeg);
    }

    if (encoded_mem) {
        encoded_mem->release(encoded_mem);
    }

    if (picture) {
        picture->release(picture);
    }

    if (cookie2) {
        delete (ExifElementsTable*) cookie2;
    }

    if (mNotifierState == AppCbNotifier::NOTIFIER_STARTED) {
        encoder = gVEncoderQueue.valueFor(src);
        if (encoder.get()) {
            gVEncoderQueue.removeItem(src);
            encoder.clear();
        }
        //mFrameProvider->returnFrame(src, type);
    }

    LOG_FUNCTION_NAME_EXIT;
}

/**
  * NotificationHandler class
  */

///Initialization function for AppCbNotifier
status_t AppCbNotifier::initialize()
{
    LOG_FUNCTION_NAME;

    mMeasurementEnabled = false;

    ///Create the app notifier thread
    mNotificationThread = new NotificationThread(this);
    if(!mNotificationThread.get())
        {
        CAMHAL_LOGEA("Couldn't create Notification thread");
        return NO_MEMORY;
        }

    ///Start the display thread
    status_t ret = mNotificationThread->run("NotificationThread", PRIORITY_URGENT_DISPLAY);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEA("Couldn't run NotificationThread");
        mNotificationThread.clear();
        return ret;
        }

    mUseMetaDataBufferMode = false;
    mUseVideoBuffers = false;
    mRawAvailable = false;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void AppCbNotifier::setCallbacks(VirtualCamHal* cameraHal,
                                        camera_notify_callback notify_cb,
                                        camera_data_callback data_cb,
                                        camera_data_timestamp_callback data_cb_timestamp,
                                        camera_request_memory get_memory,
                                        void *user)
{
    Mutex::Autolock lock(mLock);

    LOG_FUNCTION_NAME;

    mCameraHal = cameraHal;
    mNotifyCb = notify_cb;
    mDataCb = data_cb;
    mDataCbTimestamp = data_cb_timestamp;
    mRequestMemory = get_memory;
    mCallbackCookie = user;

    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::setMeasurements(bool enable)
{
    Mutex::Autolock lock(mLock);

    LOG_FUNCTION_NAME;

    mMeasurementEnabled = enable;

    if (  enable  )
        {
         mFrameProvider->enableFrameNotification(CameraFrame::FRAME_DATA_SYNC);
        }

    LOG_FUNCTION_NAME_EXIT;
}


//All sub-components of Camera HAL call this whenever any error happens
void AppCbNotifier::errorNotify(int error)
{
    LOG_FUNCTION_NAME;

    CAMHAL_LOGEB("AppCbNotifier received error %d", error);

    // If it is a fatal error abort here!
    if((error == CAMERA_ERROR_FATAL) || (error == CAMERA_ERROR_HARD)) {
        //We kill media server if we encounter these errors as there is
        //no point continuing and apps also don't handle errors other
        //than media server death always.
        abort();
        return;
    }

    if (  ( NULL != mCameraHal ) &&
          ( NULL != mNotifyCb ) &&
          ( mCameraHal->msgTypeEnabled(CAMERA_MSG_ERROR) ) )
      {
        CAMHAL_LOGEB("AppCbNotifier mNotifyCb %d", error);
        mNotifyCb(CAMERA_MSG_ERROR, CAMERA_ERROR_UNKNOWN, 0, mCallbackCookie);
      }

    LOG_FUNCTION_NAME_EXIT;
}

bool AppCbNotifier::notificationThread()
{
    bool shouldLive = true;
    status_t ret;

    LOG_FUNCTION_NAME;

    //CAMHAL_LOGDA("Notification Thread waiting for message");
    ret = MSGUTILS::MessageQueue::waitForMsg(&mNotificationThread->msgQ(),
                                            &mEventQ,
                                            &mFrameQ,
                                            AppCbNotifier::NOTIFIER_TIMEOUT);

    //CAMHAL_LOGDA("Notification Thread received message");

    if (mNotificationThread->msgQ().hasMsg()) {
        ///Received a message from CameraHal, process it
        CAMHAL_LOGDA("Notification Thread received message from Camera HAL");
        shouldLive = processMessage();
        if(!shouldLive) {
                CAMHAL_LOGDA("Notification Thread exiting.");
        }
    }

    if(mEventQ.hasMsg()) {
        ///Received an event from one of the event providers
        CAMHAL_LOGDA("Notification Thread received an event from event provider (CameraAdapter)");
        notifyEvent();
     }

    if(mFrameQ.hasMsg()) {
       ///Received a frame from one of the frame providers
       //CAMHAL_LOGDA("Notification Thread received a frame from frame provider (CameraAdapter)");
       notifyFrame();
    }

    LOG_FUNCTION_NAME_EXIT;
    return shouldLive;
}

void AppCbNotifier::notifyEvent()
{
    ///Receive and send the event notifications to app
    MSGUTILS::Message msg;
    LOG_FUNCTION_NAME;
    {
    Mutex::Autolock lock(mLock);
    mEventQ.get(&msg);
    }
    bool ret = true;
    CameraHalEvent *evt = NULL;
    CameraHalEvent::FocusEventData *focusEvtData;
    CameraHalEvent::ZoomEventData *zoomEvtData;
    CameraHalEvent::FaceEventData faceEvtData;
    CameraHalEvent::FocusMoveEventData *focusMoveEvtData;

    if(mNotifierState != AppCbNotifier::NOTIFIER_STARTED)
    {
        return;
    }

    switch(msg.command)
        {
        case AppCbNotifier::NOTIFIER_CMD_PROCESS_EVENT:

            evt = ( CameraHalEvent * ) msg.arg1;

            if ( NULL == evt )
                {
                CAMHAL_LOGEA("Invalid CameraHalEvent");
                return;
                }

            switch(evt->mEventType)
                {
                case CameraHalEvent::EVENT_SHUTTER:

                    if ( ( NULL != mCameraHal ) &&
                          ( NULL != mNotifyCb ) &&
                          ( mCameraHal->msgTypeEnabled(CAMERA_MSG_SHUTTER) ) )
                        {
                            mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
                        }
                    mRawAvailable = false;

                    break;

                case CameraHalEvent::EVENT_FOCUS_LOCKED:
                case CameraHalEvent::EVENT_FOCUS_ERROR:

                    focusEvtData = &evt->mEventData->focusEvent;
                    if ( ( focusEvtData->focusLocked ) &&
                          ( NULL != mCameraHal ) &&
                          ( NULL != mNotifyCb ) &&
                          ( mCameraHal->msgTypeEnabled(CAMERA_MSG_FOCUS) ) )
                        {
                         mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
                         mCameraHal->disableMsgType(CAMERA_MSG_FOCUS);
                        }
                    else if ( focusEvtData->focusError &&
                                ( NULL != mCameraHal ) &&
                                ( NULL != mNotifyCb ) &&
                                ( mCameraHal->msgTypeEnabled(CAMERA_MSG_FOCUS) ) )
                        {
                         mNotifyCb(CAMERA_MSG_FOCUS, false, 0, mCallbackCookie);
                         mCameraHal->disableMsgType(CAMERA_MSG_FOCUS);
                        }

                    break;

                case CameraHalEvent::EVENT_FOCUS_MOVE:

                    focusMoveEvtData = &evt->mEventData->focusMoveEvent;
                    if (  ( NULL != mCameraHal ) &&
                          ( NULL != mNotifyCb ))
                        {
                         mNotifyCb(CAMERA_MSG_FOCUS_MOVE, focusMoveEvtData->focusStart, 0, mCallbackCookie);
                        }

                    break;

                case CameraHalEvent::EVENT_ZOOM_INDEX_REACHED:

                    zoomEvtData = &evt->mEventData->zoomEvent;

                    if ( ( NULL != mCameraHal ) &&
                         ( NULL != mNotifyCb) &&
                         ( mCameraHal->msgTypeEnabled(CAMERA_MSG_ZOOM) ) )
                        {
                        mNotifyCb(CAMERA_MSG_ZOOM, zoomEvtData->currentZoomIndex, zoomEvtData->targetZoomIndexReached, mCallbackCookie);
                        }

                    break;

                case CameraHalEvent::EVENT_FACE:

                    faceEvtData = evt->mEventData->faceEvent;

                    if ( ( NULL != mCameraHal ) &&
                         ( NULL != mNotifyCb) &&
                         ( mCameraHal->msgTypeEnabled(CAMERA_MSG_PREVIEW_METADATA) ) )
                        {
                        // WA for an issue inside CameraService
                        camera_memory_t *tmpBuffer = mRequestMemory(-1, 1, 1, NULL);

                        mDataCb(CAMERA_MSG_PREVIEW_METADATA,
                                tmpBuffer,
                                0,
                                faceEvtData->getFaceResult(),
                                mCallbackCookie);

                        faceEvtData.clear();

                        if ( NULL != tmpBuffer ) {
                            tmpBuffer->release(tmpBuffer);
                        }

                        }

                    break;

                case CameraHalEvent::ALL_EVENTS:
                    break;
                default:
                    break;
                }

            break;
        }

    if ( NULL != evt )
        {
        delete evt;
        }


    LOG_FUNCTION_NAME_EXIT;

}

static void copy2Dto1D(void *dst,
                       void *src,
                       int width,
                       int height,
                       unsigned int srcpixelfmtflag,
                       size_t stride,
                       uint32_t offset,
                       unsigned int bytesPerPixel,
                       size_t length,
                       const char *pixelFormat)
{
    unsigned int alignedRow, row;
    unsigned char *bufferDst, *bufferSrc;
    unsigned char *bufferDstEnd, *bufferSrcEnd;
    uint16_t *bufferSrc_UV;

    unsigned int *y_uv = (unsigned int *)src;

    CAMHAL_LOGDB("copy2Dto1D() y= 0x%x ; uv=0x%x.",y_uv[0], y_uv[1]);
    CAMHAL_LOGDB("pixelFormat= %s; offset=%d; length=%d;width=%d,%d;stride=%d;",
			pixelFormat,offset,length,width,height,stride);

    if (pixelFormat!=NULL) {
        if (strcmp(pixelFormat, CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
            bytesPerPixel = 2;
        } else if (strcmp(pixelFormat, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0 ||
		    strcmp(pixelFormat, CameraParameters::PIXEL_FORMAT_YUV420P) == 0) {
            bytesPerPixel = 1;
            bufferDst = ( unsigned char * ) dst;
            bufferDstEnd = ( unsigned char * ) dst + width*height*bytesPerPixel;
            bufferSrc = ( unsigned char * ) y_uv[0] + offset;
            bufferSrcEnd = ( unsigned char * ) ( ( size_t ) y_uv[0] + length + offset);
            row = width*bytesPerPixel;
            alignedRow = stride-width;
            int stride_bytes = stride / 8;
            uint32_t xOff = offset % stride;
            uint32_t yOff = offset / stride;

            // going to convert from NV12 here and return
            // Step 1: Y plane: iterate through each row and copy
            for ( int i = 0 ; i < height ; i++) {
                memcpy(bufferDst, bufferSrc, row);
                bufferSrc += stride;
                bufferDst += row;
                if ( ( bufferSrc > bufferSrcEnd ) || ( bufferDst > bufferDstEnd ) ) {
                    break;
                }
            }

            //bufferSrc_UV = ( uint16_t * ) ((uint8_t*)y_uv[1] + (stride/2)*yOff + xOff);
            bufferSrc_UV =( uint16_t * ) ( y_uv[0]+stride*height+ (stride/2)*yOff + xOff) ;
            if ((strcmp(pixelFormat, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
			&& (CameraFrame::PIXEL_FMT_NV21 == srcpixelfmtflag) ){
                uint16_t *bufferDst_UV;
                bufferDst_UV = (uint16_t *) (((uint8_t*)dst)+row*height);
                memcpy(bufferDst_UV, bufferSrc_UV, stride*height/2);
#if 0
                // Step 2: UV plane: convert NV12 to NV21 by swapping U & V
                for (int i = 0 ; i < height/2 ; i++, bufferSrc_UV += alignedRow/2) {
                    int n = width;
                    asm volatile (
                    "   pld [%[src], %[src_stride], lsl #2]                         \n\t"
                    "   cmp %[n], #32                                               \n\t"
                    "   blt 1f                                                      \n\t"
                    "0: @ 32 byte swap                                              \n\t"
                    "   sub %[n], %[n], #32                                         \n\t"
                    "   vld2.8  {q0, q1} , [%[src]]!                                \n\t"
                    "   vswp q0, q1                                                 \n\t"
                    "   cmp %[n], #32                                               \n\t"
                    "   vst2.8  {q0,q1},[%[dst]]!                                   \n\t"
                    "   bge 0b                                                      \n\t"
                    "1: @ Is there enough data?                                     \n\t"
                    "   cmp %[n], #16                                               \n\t"
                    "   blt 3f                                                      \n\t"
                    "2: @ 16 byte swap                                              \n\t"
                    "   sub %[n], %[n], #16                                         \n\t"
                    "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
                    "   vswp d0, d1                                                 \n\t"
                    "   cmp %[n], #16                                               \n\t"
                    "   vst2.8  {d0,d1},[%[dst]]!                                   \n\t"
                    "   bge 2b                                                      \n\t"
                    "3: @ Is there enough data?                                     \n\t"
                    "   cmp %[n], #8                                                \n\t"
                    "   blt 5f                                                      \n\t"
                    "4: @ 8 byte swap                                               \n\t"
                    "   sub %[n], %[n], #8                                          \n\t"
                    "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
                    "   vswp d0, d1                                                 \n\t"
                    "   cmp %[n], #8                                                \n\t"
                    "   vst2.8  {d0[0],d1[0]},[%[dst]]!                             \n\t"
                    "   bge 4b                                                      \n\t"
                    "5: @ end                                                       \n\t"
#ifdef NEEDS_ARM_ERRATA_754319_754320
                    "   vmov s0,s0  @ add noop for errata item                      \n\t"
#endif
                    : [dst] "+r" (bufferDst_UV), [src] "+r" (bufferSrc_UV), [n] "+r" (n)
                    : [src_stride] "r" (stride_bytes)
                    : "cc", "memory", "q0", "q1"
                    );
                }
#endif
            } else if( (strcmp(pixelFormat, CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
			&& (CameraFrame::PIXEL_FMT_YV12 == srcpixelfmtflag) ){
			    bufferSrc =(unsigned char *) bufferSrc_UV;
			    bufferDst = (unsigned char *)(((unsigned char*)dst)+row*height);
			    row = ALIGN(stride/2, 16);

			    memcpy(bufferDst, bufferSrc, row*height);
            } else if ( (strcmp(pixelFormat, CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
			&& ( CameraFrame::PIXEL_FMT_NV21 == srcpixelfmtflag) ){
                 uint16_t *bufferDst_U;
                 uint16_t *bufferDst_V;

                // Step 2: UV plane: convert NV12 to YV12 by de-interleaving U & V
                // TODO(XXX): This version of CameraHal assumes NV12 format it set at
                //            camera adapter to support YV12. Need to address for
                //            USBCamera

                bufferDst_U = (uint16_t *) (((uint8_t*)dst)+row*height);
                bufferDst_V = (uint16_t *) (((uint8_t*)dst)+row*height+row*height/4);

                for (int i = 0 ; i < height/2 ; i++, bufferSrc_UV += alignedRow/2) {
                    int n = width;
                    asm volatile (
                    "   pld [%[src], %[src_stride], lsl #2]                         \n\t"
                    "   cmp %[n], #32                                               \n\t"
                    "   blt 1f                                                      \n\t"
                    "0: @ 32 byte swap                                              \n\t"
                    "   sub %[n], %[n], #32                                         \n\t"
                    "   vld2.8  {q0, q1} , [%[src]]!                                \n\t"
                    "   cmp %[n], #32                                               \n\t"
                    "   vst1.8  {q1},[%[dst_v]]!                                    \n\t"
                    "   vst1.8  {q0},[%[dst_u]]!                                    \n\t"
                    "   bge 0b                                                      \n\t"
                    "1: @ Is there enough data?                                     \n\t"
                    "   cmp %[n], #16                                               \n\t"
                    "   blt 3f                                                      \n\t"
                    "2: @ 16 byte swap                                              \n\t"
                    "   sub %[n], %[n], #16                                         \n\t"
                    "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
                    "   cmp %[n], #16                                               \n\t"
                    "   vst1.8  {d1},[%[dst_v]]!                                    \n\t"
                    "   vst1.8  {d0},[%[dst_u]]!                                    \n\t"
                    "   bge 2b                                                      \n\t"
                    "3: @ Is there enough data?                                     \n\t"
                    "   cmp %[n], #8                                                \n\t"
                    "   blt 5f                                                      \n\t"
                    "4: @ 8 byte swap                                               \n\t"
                    "   sub %[n], %[n], #8                                          \n\t"
                    "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
                    "   cmp %[n], #8                                                \n\t"
                    "   vst1.8  {d1[0]},[%[dst_v]]!                                 \n\t"
                    "   vst1.8  {d0[0]},[%[dst_u]]!                                 \n\t"
                    "   bge 4b                                                      \n\t"
                    "5: @ end                                                       \n\t"
#ifdef NEEDS_ARM_ERRATA_754319_754320
                    "   vmov s0,s0  @ add noop for errata item                      \n\t"
#endif
                    : [dst_u] "+r" (bufferDst_U), [dst_v] "+r" (bufferDst_V),
                      [src] "+r" (bufferSrc_UV), [n] "+r" (n)
                    : [src_stride] "r" (stride_bytes)
                    : "cc", "memory", "q0", "q1"
                    );
                }
            }
            return ;

        } else if(strcmp(pixelFormat, CameraParameters::PIXEL_FORMAT_RGB565) == 0) {
            bytesPerPixel = 2;
        }
    }

    bufferDst = ( unsigned char * ) dst;
    bufferSrc = ( unsigned char * ) y_uv[0];
    row = width*bytesPerPixel;
    alignedRow = ( row + ( stride -1 ) ) & ( ~ ( stride -1 ) );

    //iterate through each row
    for ( int i = 0 ; i < height ; i++,  bufferSrc += alignedRow, bufferDst += row) {
        memcpy(bufferDst, bufferSrc, row);
    }
}

void AppCbNotifier::copyAndSendPictureFrame(CameraFrame* frame, int32_t msgType)
{
    camera_memory_t* picture = NULL;
    void *dest = NULL, *src = NULL;

    // scope for lock
    {
        Mutex::Autolock lock(mLock);

        if(mNotifierState != AppCbNotifier::NOTIFIER_STARTED) {
            goto exit;
        }

        picture = mRequestMemory(-1, frame->mLength, 1, NULL);

        if (NULL != picture) {
            dest = picture->data;
            if (NULL != dest) {
                src = (void *) ((unsigned int) frame->mBuffer + frame->mOffset);
                memcpy(dest, src, frame->mLength);
            }
        }
    }

 exit:
    mFrameProvider->returnFrame(frame->mBuffer, (CameraFrame::FrameType) frame->mFrameType);

    if(picture) {
        if((mNotifierState == AppCbNotifier::NOTIFIER_STARTED) &&
           mCameraHal->msgTypeEnabled(msgType)) {
            mDataCb(msgType, picture, 0, NULL, mCallbackCookie);
        }
        picture->release(picture);
    }
}

void AppCbNotifier::copyAndSendPreviewFrame(CameraFrame* frame, int32_t msgType)
{
    camera_memory_t* picture = NULL;
    void* dest = NULL;
    uint8_t* src = NULL;

    // scope for lock
    {
        Mutex::Autolock lock(mLock);

        if(mNotifierState != AppCbNotifier::NOTIFIER_STARTED) {
            goto exit;
        }

        if (!mPreviewMemory || !frame->mBuffer) {
            CAMHAL_LOGDA("Error! One of the buffer is NULL");
            goto exit;
        }

#ifdef AMLOGIC_CAMERA_OVERLAY_SUPPORT
        camera_memory_t* VideoCameraBufferMemoryBase = (camera_memory_t*)frame->mBuffer;
        src = (uint8_t*)VideoCameraBufferMemoryBase->data;
#else
        private_handle_t* gralloc_hnd = (private_handle_t*)frame->mBuffer;
        src = (uint8_t*)gralloc_hnd->base;
#endif
        if (!src) {
            CAMHAL_LOGDA("Error! Src Data buffer is NULL");
            goto exit;
        }

        dest = (void*) mPreviewBufs[mPreviewBufCount];

        CAMHAL_LOGVB("%d:copy2Dto1D(%p, %p, %d, %d, %d, %d, %d, %d,%s)",
                     __LINE__,
                      NULL, //buf,
                      frame->mBuffer,
                      frame->mWidth,
                      frame->mHeight,
                      frame->mPixelFmt,
                      frame->mAlignment,
                      2,
                      frame->mLength,
                      mPreviewPixelFormat);

        if ( NULL != dest ) {
            // data sync frames don't need conversion
            if (CameraFrame::FRAME_DATA_SYNC == frame->mFrameType) {
                if ( (mPreviewMemory->size / MAX_BUFFERS) >= frame->mLength ) {
                    memcpy(dest, (void*) src, frame->mLength);
                } else {
                    memset(dest, 0, (mPreviewMemory->size / MAX_BUFFERS));
                }
            } else {
              if ((NULL == (void*)frame->mYuv[0]) || (NULL == (void*)frame->mYuv[1])){
                CAMHAL_LOGEA("Error! One of the YUV Pointer is NULL");
                goto exit;
              }
              else{
                copy2Dto1D(dest,
                           frame->mYuv,
                           frame->mWidth,
                           frame->mHeight,
                           frame->mPixelFmt,
                           frame->mAlignment,
                           frame->mOffset,
                           2,
                           frame->mLength,
                           mPreviewPixelFormat);
              }
            }
        }
    }

 exit:
    mFrameProvider->returnFrame(frame->mBuffer, (CameraFrame::FrameType) frame->mFrameType);

    if((mNotifierState == AppCbNotifier::NOTIFIER_STARTED) &&
       mCameraHal->msgTypeEnabled(msgType) &&
       (dest != NULL)) {
        mDataCb(msgType, mPreviewMemory, mPreviewBufCount, NULL, mCallbackCookie);
    }

    // increment for next buffer
    mPreviewBufCount = (mPreviewBufCount + 1) % AppCbNotifier::MAX_BUFFERS;
}

status_t AppCbNotifier::dummyRaw()
{
    LOG_FUNCTION_NAME;

    if ( NULL == mRequestMemory ) {
        CAMHAL_LOGEA("Can't allocate memory for dummy raw callback!");
        return NO_INIT;
    }

    if ( ( NULL != mCameraHal ) &&
         ( NULL != mDataCb) &&
         ( NULL != mNotifyCb ) ){

        if ( mCameraHal->msgTypeEnabled(CAMERA_MSG_RAW_IMAGE) ) {
            camera_memory_t *dummyRaw = mRequestMemory(-1, 1, 1, NULL);

            if ( NULL == dummyRaw ) {
                CAMHAL_LOGEA("Dummy raw buffer allocation failed!");
                return NO_MEMORY;
            }

            mDataCb(CAMERA_MSG_RAW_IMAGE, dummyRaw, 0, NULL, mCallbackCookie);

            dummyRaw->release(dummyRaw);
        } else if ( mCameraHal->msgTypeEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY) ) {
            mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

void AppCbNotifier::notifyFrame()
{
    ///Receive and send the frame notifications to app
    MSGUTILS::Message msg;
    CameraFrame *frame;
    MemoryHeapBase *heap;
    MemoryBase *buffer = NULL;
    sp<MemoryBase> memBase;
    void *buf = NULL;

    LOG_FUNCTION_NAME;

    {
        Mutex::Autolock lock(mLock);
        if(!mFrameQ.isEmpty()) {
            mFrameQ.get(&msg);
        } else {
            return;
        }
    }

    bool ret = true;

    frame = NULL;
    switch(msg.command)
        {
        case AppCbNotifier::NOTIFIER_CMD_PROCESS_FRAME:

                frame = (CameraFrame *) msg.arg1;
                if(!frame)
                {
                    break;
                }

                if ( (CameraFrame::RAW_FRAME == frame->mFrameType )&&
                    ( NULL != mCameraHal ) &&
                    ( NULL != mDataCb) &&
                    ( NULL != mNotifyCb ) )
                {
                     
                    if ( mCameraHal->msgTypeEnabled(CAMERA_MSG_RAW_IMAGE) )
                    {
#ifdef COPY_IMAGE_BUFFER
                        copyAndSendPictureFrame(frame, CAMERA_MSG_RAW_IMAGE);
#else
                        //TODO: Find a way to map a Tiler buffer to a MemoryHeapBase
#endif
                    }
                    else 
                    {
                        if ( mCameraHal->msgTypeEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY) ) {
                            mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
                        }
                        mFrameProvider->returnFrame(frame->mBuffer,
                                                    (CameraFrame::FrameType) frame->mFrameType);
                    }
                    mRawAvailable = true;
                }
                else if ( (CameraFrame::IMAGE_FRAME == frame->mFrameType) &&
                          (NULL != mCameraHal) &&
                          (NULL != mDataCb) &&
                          ((CameraFrame::ENCODE_RAW_YUV422I_TO_JPEG & frame->mQuirks) ||
                           (CameraFrame::ENCODE_RAW_RGB24_TO_JPEG & frame->mQuirks)|| 
                           (CameraFrame::ENCODE_RAW_YUV420SP_TO_JPEG & frame->mQuirks)))
                {

                    CAMHAL_LOGDA("IMAGE_FRAME ENCODE_RAW..\n");
                    int encode_quality = 100, tn_quality = 100;
                    int tn_width, tn_height;
                    unsigned int current_snapshot = 0;
                    Encoder_libjpeg::params *main_jpeg = NULL, *tn_jpeg = NULL;
                    void* exif_data = NULL;
                    camera_memory_t* raw_picture = mRequestMemory(-1, frame->mLength, 1, NULL);

                    if(raw_picture) {
                        buf = raw_picture->data;
                    }else{
                        CAMHAL_LOGEA("Error! Main Jpeg encoder request memory fail!");
                        break;
                    }

                    CameraParameters parameters;
                    char *params = mCameraHal->getParameters();
                    const String8 strParams(params);
                    parameters.unflatten(strParams);

                    encode_quality = parameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
                    if (encode_quality < 0 || encode_quality > 100) {
                        encode_quality = 100;
                    }

                    tn_quality = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
                    if (tn_quality < 0 || tn_quality > 100) {
                        tn_quality = 100;
                    }

                    if (CameraFrame::HAS_EXIF_DATA & frame->mQuirks) {
                        exif_data = frame->mCookie2;
                    }

                    main_jpeg = (Encoder_libjpeg::params*)
                                    malloc(sizeof(Encoder_libjpeg::params));
                    if (main_jpeg) {
                        main_jpeg->src = (uint8_t*) frame->mBuffer;
                        main_jpeg->src_size = frame->mLength;
                        main_jpeg->dst = (uint8_t*) buf;
                        main_jpeg->dst_size = frame->mLength;
                        main_jpeg->quality = encode_quality;
                        main_jpeg->in_width = frame->mWidth;
                        main_jpeg->in_height = frame->mHeight;
                        main_jpeg->out_width = frame->mWidth;
                        main_jpeg->out_height = frame->mHeight;
                        if ((CameraFrame::ENCODE_RAW_RGB24_TO_JPEG & frame->mQuirks))
                            main_jpeg->format = CameraProperties::PIXEL_FORMAT_RGB24;
                        else if ((CameraFrame::ENCODE_RAW_YUV422I_TO_JPEG & frame->mQuirks))
                            main_jpeg->format = CameraParameters::PIXEL_FORMAT_YUV422I;
                        else if ((CameraFrame::ENCODE_RAW_YUV420SP_TO_JPEG & frame->mQuirks))
                            main_jpeg->format = CameraParameters::PIXEL_FORMAT_YUV420SP;
                    }

// disable thumbnail for now. preview was stopped and mPreviewBufs was
// cleared, so this won't work.
                    tn_width = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
                    tn_height = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
                    	
                    if(frame->mHeight>frame->mWidth){
                        int temp = tn_width;
                        tn_width = tn_height;
                        tn_height = tn_width;
                    }

                    if ((tn_width > 0) && (tn_height > 0)) {
                        tn_jpeg = (Encoder_libjpeg::params*)
                                      malloc(sizeof(Encoder_libjpeg::params));
                        // if malloc fails just keep going and encode main jpeg
                        if (!tn_jpeg) {
                            tn_jpeg = NULL;
                        }
                    }

                    if (tn_jpeg) {
                        tn_jpeg->dst = (uint8_t*) malloc(tn_width*tn_height*3);
                        if(tn_jpeg->dst){
                            tn_jpeg->src = (uint8_t*) frame->mBuffer;
                            tn_jpeg->src_size = frame->mLength;
                            tn_jpeg->dst_size = tn_width*tn_height*3;
                            tn_jpeg->quality = tn_quality;
                            tn_jpeg->in_width = frame->mWidth;
                            tn_jpeg->in_height = frame->mHeight;
                            tn_jpeg->out_width = tn_width;
                            tn_jpeg->out_height = tn_height;
                            if ((CameraFrame::ENCODE_RAW_RGB24_TO_JPEG & frame->mQuirks))
                                tn_jpeg->format = CameraProperties::PIXEL_FORMAT_RGB24;
                            else if ((CameraFrame::ENCODE_RAW_YUV422I_TO_JPEG & frame->mQuirks))
                                tn_jpeg->format = CameraParameters::PIXEL_FORMAT_YUV422I;
                            else if ((CameraFrame::ENCODE_RAW_YUV420SP_TO_JPEG & frame->mQuirks))
                                tn_jpeg->format = CameraParameters::PIXEL_FORMAT_YUV420SP;
                        }else{
                            free(tn_jpeg);
                            tn_jpeg = NULL;
                            CAMHAL_LOGEA("Error! Thumbnail Jpeg encoder malloc memory fail!");
                        }
                    }

                    CAMHAL_LOGDA("IMAGE_FRAME ENCODE_RAW..\n");
                    sp<Encoder_libjpeg> encoder = new Encoder_libjpeg(main_jpeg,
                                                      tn_jpeg,
                                                      AppCbNotifierEncoderCallback,
                                                      (CameraFrame::FrameType)frame->mFrameType,
                                                      this,
                                                      raw_picture,
                                                      exif_data);
                    encoder->run();
                    gVEncoderQueue.add(frame->mBuffer, encoder);
                    encoder.clear();
                    if (params != NULL)
                    {
                        mCameraHal->putParameters(params);
                    }
                }
                else if ( ( CameraFrame::IMAGE_FRAME == frame->mFrameType ) &&
                             ( NULL != mCameraHal ) &&
                             ( NULL != mDataCb) )
                {

                    // CTS, MTS requirements: Every 'takePicture()' call
                    // who registers a raw callback should receive one
                    // as well. This is  not always the case with
                    // CameraAdapters though.
                    if (!mRawAvailable) {
                        dummyRaw();
                    } else {
                        mRawAvailable = false;
                    }

#ifdef COPY_IMAGE_BUFFER
                    {
                        Mutex::Autolock lock(mBurstLock);
#if 0 //TODO: enable burst mode later
                        if ( mBurst )
                        {
                            `(CAMERA_MSG_BURST_IMAGE, JPEGPictureMemBase, mCallbackCookie);
                        }
                        else
#endif
                        {
                            copyAndSendPictureFrame(frame, CAMERA_MSG_COMPRESSED_IMAGE);
                        }
                    }
#else
                     //TODO: Find a way to map a Tiler buffer to a MemoryHeapBase
#endif
                    }
                else if ( ( CameraFrame::VIDEO_FRAME_SYNC == frame->mFrameType ) &&
                             ( NULL != mCameraHal ) &&
                             ( NULL != mDataCb) &&
                             ( mCameraHal->msgTypeEnabled(CAMERA_MSG_VIDEO_FRAME)  ) )
                {
                    mRecordingLock.lock();
                    if(mRecording)
                    {
                        if(mUseMetaDataBufferMode)
                        {
                            camera_memory_t *videoMedatadaBufferMemory =
                                             (camera_memory_t *) mVideoMetadataBufferMemoryMap.valueFor((uint32_t) frame->mBuffer);
                            video_metadata_t *videoMetadataBuffer = (video_metadata_t *) videoMedatadaBufferMemory->data;

                            if( (NULL == videoMedatadaBufferMemory) || (NULL == videoMetadataBuffer) || (NULL == frame->mBuffer) )
                            {
                                CAMHAL_LOGEA("Error! One of the video buffers is NULL");
                                break;
                            }

                            if ( mUseVideoBuffers )
                            {
                                int vBuf = mVideoMap.valueFor((uint32_t) frame->mBuffer);
                                GraphicBufferMapper &mapper = GraphicBufferMapper::get();
                                Rect bounds;
                                bounds.left = 0;
                                bounds.top = 0;
                                bounds.right = mVideoWidth;
                                bounds.bottom = mVideoHeight;

                                void *y_uv[2];
                                mapper.lock((buffer_handle_t)vBuf, CAMHAL_GRALLOC_USAGE, bounds, y_uv);

                                structConvImage input =  {frame->mWidth,
                                                          frame->mHeight,
                                                          4096,
                                                          IC_FORMAT_YCbCr420_lp,
                                                          (mmByte *)frame->mYuv[0],
                                                          (mmByte *)frame->mYuv[1],
                                                          frame->mOffset};

                                structConvImage output = {mVideoWidth,
                                                          mVideoHeight,
                                                          4096,
                                                          IC_FORMAT_YCbCr420_lp,
                                                          (mmByte *)y_uv[0],
                                                          (mmByte *)y_uv[1],
                                                          0};

                                VT_resizeFrame_Video_opt2_lp(&input, &output, NULL, 0);
                                mapper.unlock((buffer_handle_t)vBuf);
                                videoMetadataBuffer->metadataBufferType = (int) kMetadataBufferTypeCameraSource;
                                videoMetadataBuffer->handle = (void *)vBuf;
                                videoMetadataBuffer->offset = 0;
                            }
                            else
                            {
                                videoMetadataBuffer->metadataBufferType = (int) kMetadataBufferTypeCameraSource;
                                videoMetadataBuffer->handle = frame->mBuffer;
                                videoMetadataBuffer->offset = frame->mOffset;
                            }

                            CAMHAL_LOGVB("mDataCbTimestamp : frame->mBuffer=0x%x, videoMetadataBuffer=0x%x, videoMedatadaBufferMemory=0x%x",
                                            frame->mBuffer, videoMetadataBuffer, videoMedatadaBufferMemory);

                            mDataCbTimestamp(frame->mTimestamp, CAMERA_MSG_VIDEO_FRAME,
                                                videoMedatadaBufferMemory, 0, mCallbackCookie);
                        }
                        else
                        {
                            //TODO: Need to revisit this, should ideally be mapping the TILER buffer using mRequestMemory
                            if( NULL == frame->mBuffer)
                            {
                                CAMHAL_LOGEA("Error! frame->mBuffer is NULL");
                                break;
                            }
#ifdef AMLOGIC_CAMERA_OVERLAY_SUPPORT
                            camera_memory_t* VideoCameraBufferMemoryBase = (camera_memory_t*)frame->mBuffer;
                            if((NULL == VideoCameraBufferMemoryBase)||(NULL == VideoCameraBufferMemoryBase->data))
                            {
                                CAMHAL_LOGEA("Error! one of video buffer is NULL");
                                break;
                            }
                            mDataCbTimestamp(frame->mTimestamp, CAMERA_MSG_VIDEO_FRAME, VideoCameraBufferMemoryBase, 0, mCallbackCookie);
#else
                            camera_memory_t* VideoCameraBufferMemoryBase = (camera_memory_t*)mVideoHeaps.valueFor((uint32_t)frame->mBuffer);
                            private_handle_t* gralloc_hnd = (private_handle_t*)frame->mBuffer;
                            if((!VideoCameraBufferMemoryBase) ||(!gralloc_hnd->base))
                            {
                                CAMHAL_LOGEA("Error! one of video buffer is NULL");
                                break;
                            }
                            uint8_t* src = (uint8_t*)gralloc_hnd->base;
                            uint8_t* dest = (uint8_t*)VideoCameraBufferMemoryBase->data;
                            memcpy(dest,src,frame->mLength);
                            mDataCbTimestamp(frame->mTimestamp, CAMERA_MSG_VIDEO_FRAME, VideoCameraBufferMemoryBase, 0, mCallbackCookie);
#endif					
                        }
                    }
                    mRecordingLock.unlock();
                }
                else if(( CameraFrame::SNAPSHOT_FRAME == frame->mFrameType ) &&
                             ( NULL != mCameraHal ) &&
                             ( NULL != mDataCb) &&
                             ( NULL != mNotifyCb)) {
                    //When enabled, measurement data is sent instead of video data
                    if ( !mMeasurementEnabled ) {
                        copyAndSendPreviewFrame(frame, CAMERA_MSG_POSTVIEW_FRAME);
                    } else {
                        mFrameProvider->returnFrame(frame->mBuffer,
                                                    (CameraFrame::FrameType) frame->mFrameType);
                    }
                }
                else if ( ( CameraFrame::PREVIEW_FRAME_SYNC== frame->mFrameType ) &&
                            ( NULL != mCameraHal ) &&
                            ( NULL != mDataCb) &&
                            ( mCameraHal->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) ) {
                    //When enabled, measurement data is sent instead of video data
                    if ( !mMeasurementEnabled ) {
                        copyAndSendPreviewFrame(frame, CAMERA_MSG_PREVIEW_FRAME);
                    } else {
                        mFrameProvider->returnFrame(frame->mBuffer,
                                                     (CameraFrame::FrameType) frame->mFrameType);
                    }
                }
                else if ( ( CameraFrame::FRAME_DATA_SYNC == frame->mFrameType ) &&
                            ( NULL != mCameraHal ) &&
                            ( NULL != mDataCb) &&
                            ( mCameraHal->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) ) {
                    copyAndSendPreviewFrame(frame, CAMERA_MSG_PREVIEW_FRAME);
                } else {
                    mFrameProvider->returnFrame(frame->mBuffer,
                                                ( CameraFrame::FrameType ) frame->mFrameType);
                    CAMHAL_LOGDB("Frame type 0x%x is still unsupported!", frame->mFrameType);
                }

                break;

        default:

            break;

        };

exit:

    if ( NULL != frame )
        {
        delete frame;
        }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::frameCallbackRelay(CameraFrame* caFrame)
{
    LOG_FUNCTION_NAME;
    AppCbNotifier *appcbn = (AppCbNotifier*) (caFrame->mCookie);
    appcbn->frameCallback(caFrame);
    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::frameCallback(CameraFrame* caFrame)
{
    ///Post the event to the event queue of AppCbNotifier
    MSGUTILS::Message msg;
    CameraFrame *frame;

    LOG_FUNCTION_NAME;

    if ( NULL != caFrame )
    {
        frame = new CameraFrame(*caFrame);
        if ( NULL != frame )
        {
            msg.command = AppCbNotifier::NOTIFIER_CMD_PROCESS_FRAME;
            msg.arg1 = frame;
            mFrameQ.put(&msg);
        }
        else
        {
            CAMHAL_LOGEA("Not enough resources to allocate CameraFrame");
        }
    }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::flushAndReturnFrames()
{
    MSGUTILS::Message msg;
    CameraFrame *frame;

    Mutex::Autolock lock(mLock);
    while (!mFrameQ.isEmpty()) {
        mFrameQ.get(&msg);
        frame = (CameraFrame*) msg.arg1;
        if (frame) {
            mFrameProvider->returnFrame(frame->mBuffer,
                                        (CameraFrame::FrameType) frame->mFrameType);
        }
    }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::eventCallbackRelay(CameraHalEvent* chEvt)
{
    LOG_FUNCTION_NAME;
    AppCbNotifier *appcbn = (AppCbNotifier*) (chEvt->mCookie);
    appcbn->eventCallback(chEvt);
    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::eventCallback(CameraHalEvent* chEvt)
{

    ///Post the event to the event queue of AppCbNotifier
    MSGUTILS::Message msg;
    CameraHalEvent *event;


    LOG_FUNCTION_NAME;

    if ( NULL != chEvt )
        {

        event = new CameraHalEvent(*chEvt);
        if ( NULL != event )
            {
            msg.command = AppCbNotifier::NOTIFIER_CMD_PROCESS_EVENT;
            msg.arg1 = event;
            {
            Mutex::Autolock lock(mLock);
            mEventQ.put(&msg);
            }
            }
        else
            {
            CAMHAL_LOGEA("Not enough resources to allocate CameraHalEvent");
            }

        }

    LOG_FUNCTION_NAME_EXIT;
}


void AppCbNotifier::flushEventQueue()
{

    {
    Mutex::Autolock lock(mLock);
    mEventQ.clear();
    }
}


bool AppCbNotifier::processMessage()
{
    ///Retrieve the command from the command queue and process it
    MSGUTILS::Message msg;

    LOG_FUNCTION_NAME;

    CAMHAL_LOGDA("+Msg get...");
    mNotificationThread->msgQ().get(&msg);
    CAMHAL_LOGDA("-Msg get...");
    bool ret = true;

    switch(msg.command)
      {
        case NotificationThread::NOTIFIER_EXIT:
          {
            CAMHAL_LOGEA("Received NOTIFIER_EXIT command from Camera HAL");
            mNotifierState = AppCbNotifier::NOTIFIER_EXITED;
            ret = false;
            break;
          }
        default:
          {
            CAMHAL_LOGEA("Error: ProcessMsg() command from Camera HAL");
            break;
          }
      }

    LOG_FUNCTION_NAME_EXIT;

    return ret;


}

AppCbNotifier::~AppCbNotifier()
{
    LOG_FUNCTION_NAME;

    ///Stop app callback notifier if not already stopped
    stop();

    ///Unregister with the frame provider
    if ( NULL != mFrameProvider )
        {
        mFrameProvider->disableFrameNotification(CameraFrame::ALL_FRAMES);
        }

    //unregister with the event provider
    if ( NULL != mEventProvider )
        {
        mEventProvider->disableEventNotification(CameraHalEvent::ALL_EVENTS);
        }

    MSGUTILS::Message msg = {0,0,0,0,0,0};
    msg.command = NotificationThread::NOTIFIER_EXIT;

    ///Post the message to display thread
    mNotificationThread->msgQ().put(&msg);

    //Exit and cleanup the thread
    mNotificationThread->requestExit();
    mNotificationThread->join();

    //Delete the display thread
    mNotificationThread.clear();


    ///Free the event and frame providers
    if ( NULL != mEventProvider )
        {
        ///Deleting the event provider
        CAMHAL_LOGDA("Stopping Event Provider");
        delete mEventProvider;
        mEventProvider = NULL;
        }

    if ( NULL != mFrameProvider )
        {
        ///Deleting the frame provider
        CAMHAL_LOGDA("Stopping Frame Provider");
        delete mFrameProvider;
        mFrameProvider = NULL;
        }

    releaseSharedVideoBuffers();

    LOG_FUNCTION_NAME_EXIT;
}

//Free all video heaps and buffers
void AppCbNotifier::releaseSharedVideoBuffers()
{
    LOG_FUNCTION_NAME;

    if(mUseMetaDataBufferMode)
    {
        camera_memory_t* videoMedatadaBufferMemory;
        for (unsigned int i = 0; i < mVideoMetadataBufferMemoryMap.size();  i++)
        {
            videoMedatadaBufferMemory = (camera_memory_t*) mVideoMetadataBufferMemoryMap.valueAt(i);
            if(NULL != videoMedatadaBufferMemory)
            {
                videoMedatadaBufferMemory->release(videoMedatadaBufferMemory);
                CAMHAL_LOGDB("Released  videoMedatadaBufferMemory=0x%x", (uint32_t)videoMedatadaBufferMemory);
            }
        }

        mVideoMetadataBufferMemoryMap.clear();
        mVideoMetadataBufferReverseMap.clear();
        if (mUseVideoBuffers)
        {
            mVideoMap.clear();
        }
    }
    else
    {
#ifndef AMLOGIC_CAMERA_OVERLAY_SUPPORT
        camera_memory_t* VideoCameraBufferMemoryBase = NULL;
        for (unsigned int i = 0; i < mVideoHeaps.size();  i++)
        {
            VideoCameraBufferMemoryBase = (camera_memory_t*) mVideoHeaps.valueAt(i);
            if(NULL != VideoCameraBufferMemoryBase)
            {
                VideoCameraBufferMemoryBase->release(VideoCameraBufferMemoryBase);
                CAMHAL_LOGDB("Released  VideoCameraBufferMemoryBase=0x%x", (uint32_t)VideoCameraBufferMemoryBase);
            }
        }
#endif
        mVideoMap.clear();
        mVideoHeaps.clear();
    }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::setEventProvider(int32_t eventMask, MessageNotifier * eventNotifier)
{

    LOG_FUNCTION_NAME;
    ///@remarks There is no NULL check here. We will check
    ///for NULL when we get start command from CameraHal
    ///@Remarks Currently only one event provider (CameraAdapter) is supported
    ///@todo Have an array of event providers for each event bitmask
    mEventProvider = new EventProvider(eventNotifier, this, eventCallbackRelay);
    if ( NULL == mEventProvider )
        {
        CAMHAL_LOGEA("Error in creating EventProvider");
        }
    else
        {
        mEventProvider->enableEventNotification(eventMask);
        }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::setFrameProvider(FrameNotifier *frameNotifier)
{
    LOG_FUNCTION_NAME;
    ///@remarks There is no NULL check here. We will check
    ///for NULL when we get the start command from CameraAdapter
    mFrameProvider = new FrameProvider(frameNotifier, this, frameCallbackRelay);
    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Error in creating FrameProvider");
    }
    else
    {
        //Register only for captured images and RAW for now
        //TODO: Register for and handle all types of frames
        mFrameProvider->enableFrameNotification(CameraFrame::IMAGE_FRAME);
        mFrameProvider->enableFrameNotification(CameraFrame::RAW_FRAME);
    }

    LOG_FUNCTION_NAME_EXIT;
}

status_t AppCbNotifier::startPreviewCallbacks(CameraParameters &params, void *buffers, uint32_t *offsets, int fd, size_t length, size_t count)
{
    sp<MemoryHeapBase> heap;
    sp<MemoryBase> buffer;
    unsigned int *bufArr;
    size_t size = 0;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to start video recording without FrameProvider");
        return -EINVAL;
    }

    if ( mPreviewing )
    {
        CAMHAL_LOGDA("+Already previewing");
        return NO_INIT;
    }

    int w,h;
    ///Get preview size
    params.getPreviewSize(&w, &h);

    //Get the preview pixel format
    mPreviewPixelFormat = params.getPreviewFormat();

     if(strcmp(mPreviewPixelFormat, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
    {
        size = w*h*2;
        mPreviewPixelFormat = CameraParameters::PIXEL_FORMAT_YUV422I;
    }
    else if(strcmp(mPreviewPixelFormat, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0 )
    {
        size = (w*h*3)/2;
        mPreviewPixelFormat = CameraParameters::PIXEL_FORMAT_YUV420SP;
    }
    else if( strcmp(mPreviewPixelFormat, (const char *) CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
    {
	int y_size,c_size,c_stride;
	w = ALIGN(w,2);
	y_size = w*h;
	c_stride = ALIGN(w/2, 16);
	c_size = c_stride * h/2;
	size = y_size + c_size*2;

        mPreviewPixelFormat = CameraParameters::PIXEL_FORMAT_YUV420P;
    }
    else if(strcmp(mPreviewPixelFormat, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0)
    {
        size = w*h*2;
        mPreviewPixelFormat = CameraParameters::PIXEL_FORMAT_RGB565;
    }

    mPreviewMemory = mRequestMemory(-1, size, AppCbNotifier::MAX_BUFFERS, NULL);
    if (!mPreviewMemory) {
        return NO_MEMORY;
    }

    for (int i=0; i < AppCbNotifier::MAX_BUFFERS; i++) {
        mPreviewBufs[i] = (unsigned char*) mPreviewMemory->data + (i*size);
    }

    if ( mCameraHal->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME ) ) {
        mFrameProvider->enableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);
    }

    mPreviewBufCount = 0;

    mPreviewing = true;

    LOG_FUNCTION_NAME;

    return NO_ERROR;
}

void AppCbNotifier::setBurst(bool burst)
{
    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mBurstLock);

    mBurst = burst;

    LOG_FUNCTION_NAME_EXIT;
}

void AppCbNotifier::useVideoBuffers(bool useVideoBuffers)
{
    LOG_FUNCTION_NAME;
#ifndef AMLOGIC_CAMERA_OVERLAY_SUPPORT
    mUseVideoBuffers = useVideoBuffers;
    CAMHAL_LOGDB("Set mUseVideoBuffers as %d",(uint32_t)useVideoBuffers);
#endif
    LOG_FUNCTION_NAME_EXIT;
}

bool AppCbNotifier::getUseVideoBuffers()
{
    return mUseVideoBuffers;
}

void AppCbNotifier::setVideoRes(int width, int height)
{
    LOG_FUNCTION_NAME;

    mVideoWidth = width;
    mVideoHeight = height;

    LOG_FUNCTION_NAME_EXIT;
}

status_t AppCbNotifier::stopPreviewCallbacks()
{
    sp<MemoryHeapBase> heap;
    sp<MemoryBase> buffer;

    LOG_FUNCTION_NAME;

    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to stop preview callbacks without FrameProvider");
        return -EINVAL;
    }

    if ( !mPreviewing )
    {
        return NO_INIT;
    }

    mFrameProvider->disableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);

    {
        Mutex::Autolock lock(mLock);
        mPreviewMemory->release(mPreviewMemory);
    }

    mPreviewing = false;

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;

}

status_t AppCbNotifier::useMetaDataBufferMode(bool enable)
{
    mUseMetaDataBufferMode = enable;
    CAMHAL_LOGDB("Set mUseMetaDataBufferMode as %d",(uint32_t)enable);
    return NO_ERROR;
}


status_t AppCbNotifier::startRecording()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mRecordingLock);

    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to start video recording without FrameProvider");
        ret = -1;
    }

    if(mRecording)
    {
        return NO_INIT;
    }

    if ( NO_ERROR == ret )
    {
         mFrameProvider->enableFrameNotification(CameraFrame::VIDEO_FRAME_SYNC);
    }

    mRecording = true;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

//Allocate metadata buffers for video recording
status_t AppCbNotifier::initSharedVideoBuffers(void *buffers, uint32_t *offsets, int fd, size_t length, size_t count, void *vidBufs)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    if(mUseMetaDataBufferMode)
    {
        uint32_t *bufArr = NULL;
        camera_memory_t* videoMedatadaBufferMemory = NULL;

        if(NULL == buffers)
        {
            CAMHAL_LOGEA("Error! Video buffers are NULL");
            return BAD_VALUE;
        }
        bufArr = (uint32_t *) buffers;

        for (uint32_t i = 0; i < count; i++)
        {
            videoMedatadaBufferMemory = mRequestMemory(-1, sizeof(video_metadata_t), 1, NULL);
            if((NULL == videoMedatadaBufferMemory) || (NULL == videoMedatadaBufferMemory->data))
            {
                CAMHAL_LOGEA("Error! Could not allocate memory for Video Metadata Buffers");
                return NO_MEMORY;
            }

            mVideoMetadataBufferMemoryMap.add(bufArr[i], (uint32_t)(videoMedatadaBufferMemory));
            mVideoMetadataBufferReverseMap.add((uint32_t)(videoMedatadaBufferMemory->data), bufArr[i]);
            CAMHAL_LOGDB("bufArr[%d]=0x%x, videoMedatadaBufferMemory=0x%x, videoMedatadaBufferMemory->data=0x%x",
                    i, bufArr[i], (uint32_t)videoMedatadaBufferMemory, (uint32_t)videoMedatadaBufferMemory->data);

            if (vidBufs != NULL)
            {
                uint32_t *vBufArr = (uint32_t *) vidBufs;
                mVideoMap.add(bufArr[i], vBufArr[i]);
                CAMHAL_LOGVB("bufArr[%d]=0x%x, vBuffArr[%d]=0x%x", i, bufArr[i], i, vBufArr[i]);
            }
        }
    }
    else
    {
        uint32_t *bufArr = NULL;
        camera_memory_t* VideoCameraBufferMemoryBase = NULL;

        if(NULL == buffers)
        {
            CAMHAL_LOGEA("Error! Video buffers are NULL");
            return BAD_VALUE;
        }
        bufArr = (uint32_t *) buffers;

        for (uint32_t i = 0; i < count; i++)
        {
 #ifdef AMLOGIC_CAMERA_OVERLAY_SUPPORT
            VideoCameraBufferMemoryBase = (camera_memory_t*)bufArr[i];
 #else
            VideoCameraBufferMemoryBase = mRequestMemory(-1, mVideoWidth*mVideoHeight*3/2, 1, NULL); // only supported nv21 or nv12;
 #endif
            if((NULL == VideoCameraBufferMemoryBase) || (NULL == VideoCameraBufferMemoryBase->data))
            {
                CAMHAL_LOGEA("Error! Could not allocate memory for Video Metadata Buffers");
                return NO_MEMORY;
            }
            mVideoHeaps.add(bufArr[i], (uint32_t)(VideoCameraBufferMemoryBase));
            mVideoMap.add((uint32_t)(VideoCameraBufferMemoryBase->data),bufArr[i]);
            CAMHAL_LOGDB("bufArr[%d]=0x%x, VideoCameraBufferMemoryBase=0x%x, VideoCameraBufferMemoryBase->data=0x%x",
                    i, bufArr[i], (uint32_t)VideoCameraBufferMemoryBase, (uint32_t)VideoCameraBufferMemoryBase->data);
        }
    }
exit:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t AppCbNotifier::stopRecording()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mRecordingLock);

    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to stop video recording without FrameProvider");
        ret = -1;
    }

    if(!mRecording)
    {
        return NO_INIT;
    }

    if ( NO_ERROR == ret )
    {
         mFrameProvider->disableFrameNotification(CameraFrame::VIDEO_FRAME_SYNC);
    }

    ///Release the shared video buffers
    releaseSharedVideoBuffers();

    mRecording = false;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t AppCbNotifier::releaseRecordingFrame(const void* mem)
{
    status_t ret = NO_ERROR;
    void *frame = NULL;

    LOG_FUNCTION_NAME;
    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to stop video recording without FrameProvider");
        ret = -1;
    }

    if ( NULL == mem )
    {
        CAMHAL_LOGEA("Video Frame released is invalid");
        ret = -1;
    }

    if( NO_ERROR != ret )
    {
        return ret;
    }

    if(mUseMetaDataBufferMode)
    {
        video_metadata_t *videoMetadataBuffer = (video_metadata_t *) mem ;
        frame = (void*) mVideoMetadataBufferReverseMap.valueFor((uint32_t) videoMetadataBuffer);
        CAMHAL_LOGVB("Releasing frame with videoMetadataBuffer=0x%x, videoMetadataBuffer->handle=0x%x & frame handle=0x%x\n",
                       videoMetadataBuffer, videoMetadataBuffer->handle, frame);
    }
    else
    {
        frame = (void *)mVideoMap.valueFor((uint32_t)mem);
        //CAMHAL_LOGDB("release recording mem.0x%x, frame:0x%x",(uint32_t)mem,(uint32_t)frame);
    }

    if ( NO_ERROR == ret )
    {
         ret = mFrameProvider->returnFrame(frame, CameraFrame::VIDEO_FRAME_SYNC);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t AppCbNotifier::enableMsgType(int32_t msgType)
{
    if( msgType & (CAMERA_MSG_POSTVIEW_FRAME | CAMERA_MSG_PREVIEW_FRAME) ) {
    //if( msgType & (CAMERA_MSG_PREVIEW_FRAME) ) {
        mFrameProvider->enableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);
    }
    return NO_ERROR;
}

status_t AppCbNotifier::disableMsgType(int32_t msgType)
{
    //if(!mCameraHal->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME | CAMERA_MSG_POSTVIEW_FRAME)) {
    if(!(msgType & (CAMERA_MSG_PREVIEW_FRAME | CAMERA_MSG_POSTVIEW_FRAME))){
        mFrameProvider->disableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);
    }
    return NO_ERROR;
}

status_t AppCbNotifier::start()
{
    LOG_FUNCTION_NAME;
    if(mNotifierState==AppCbNotifier::NOTIFIER_STARTED)
    {
        CAMHAL_LOGDA("AppCbNotifier already running");
        LOG_FUNCTION_NAME_EXIT;
        return ALREADY_EXISTS;
    }

    ///Check whether initial conditions are met for us to start
    ///A frame provider should be available, if not return error
    if(!mFrameProvider)
    {
        ///AppCbNotifier not properly initialized
        CAMHAL_LOGEA("AppCbNotifier not properly initialized - Frame provider is NULL");
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }

    ///At least one event notifier should be available, if not return error
    ///@todo Modify here when there is an array of event providers
    if(!mEventProvider)
    {
        CAMHAL_LOGEA("AppCbNotifier not properly initialized - Event provider is NULL");
        LOG_FUNCTION_NAME_EXIT;
        ///AppCbNotifier not properly initialized
        return NO_INIT;
    }

    mNotifierState = AppCbNotifier::NOTIFIER_STARTED;
    CAMHAL_LOGDA(" --> AppCbNotifier NOTIFIER_STARTED \n");

    gVEncoderQueue.clear();

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;

}

status_t AppCbNotifier::stop()
{
    LOG_FUNCTION_NAME;

    if(mNotifierState!=AppCbNotifier::NOTIFIER_STARTED)
    {
        CAMHAL_LOGDA("AppCbNotifier already in stopped state");
        LOG_FUNCTION_NAME_EXIT;
        return ALREADY_EXISTS;
    }

    {
        Mutex::Autolock lock(mLock);

        mNotifierState = AppCbNotifier::NOTIFIER_STOPPED;
        CAMHAL_LOGDA(" --> AppCbNotifier NOTIFIER_STOPPED \n");
    }

    while(!gVEncoderQueue.isEmpty()) {
        sp<Encoder_libjpeg> encoder = gVEncoderQueue.valueAt(0);
        if(encoder.get()) {
            encoder->cancel();
            encoder->join();
            encoder.clear();
        }
        gVEncoderQueue.removeItemsAt(0);
    }

    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;
}


/*--------------------NotificationHandler Class ENDS here-----------------------------*/



};
