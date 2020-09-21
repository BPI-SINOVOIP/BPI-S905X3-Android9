/*
 * Copyright (C) 2006 The Android Open Source Project
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

#ifndef ANDROID_GUI_IIMAGE_PLAYER_SERVICE_H
#define ANDROID_GUI_IIMAGE_PLAYER_SERVICE_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/Binder.h>

namespace android {

    struct IMediaHTTPService;

    // ----------------------------------------------------------------------------

    class IImagePlayerService: public IInterface {
      public:
        DECLARE_META_INTERFACE(ImagePlayerService);

        //this used by native code
        virtual int init() = 0;
        virtual int setDataSource(const char* uri) = 0;
        virtual int setDataSource (
            const sp<IMediaHTTPService> &httpService,
            const char *srcUrl) = 0;
        virtual int setSampleSurfaceSize(int sampleSize, int surfaceW,
                                         int surfaceH) = 0;
        virtual int setRotate(float degrees, int autoCrop) = 0;
        virtual int setScale(float sx, float sy, int autoCrop) = 0;
        virtual int setHWScale(float sc) = 0;
        virtual int setRotateScale(float degrees, float sx, float sy, int autoCrop) = 0;
        virtual int setTranslate(float tx, float ty) = 0;
        virtual int setCropRect(int cropX, int cropY, int cropWidth,
                                int cropHeight) = 0;
        virtual int start() = 0;
        virtual int prepare() = 0;
        virtual int show() = 0;
        virtual int showImage(const char* uri) = 0;
        virtual int release() = 0;
        virtual int notifyProcessDied(const sp<IBinder> &binder) = 0;
    };

    // ----------------------------------------------------------------------------

    class BnImagePlayerService: public BnInterface<IImagePlayerService> {
      public:
        enum {
            IMAGE_INIT = IBinder::FIRST_CALL_TRANSACTION,
            IMAGE_SET_DATA_SOURCE,
            IMAGE_SET_SAMPLE_SURFACE_SIZE,
            IMAGE_SET_ROTATE,
            IMAGE_SET_SCALE,
            IMAGE_SET_ROTATE_SCALE,
            IMAGE_SET_CROP_RECT,
            IMAGE_START,
            IMAGE_PREPARE,
            IMAGE_SHOW,
            IMAGE_SHOW_URI,
            IMAGE_RELEASE,
            IMAGE_SET_DATA_SOURCE_URL,
            IMAGE_NOTIFY_PROCESSDIED,
            IMAGE_SET_TRANSLATE,
            IMAGE_SET_HWSCALE,
        };

        virtual status_t onTransact(uint32_t code, const Parcel& data,
                                    Parcel* reply, uint32_t flags = 0);
    };

    // ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_GUI_IIMAGE_PLAYER_SERVICE_H
