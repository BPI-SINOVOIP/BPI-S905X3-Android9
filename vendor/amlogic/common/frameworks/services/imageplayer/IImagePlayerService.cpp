/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "ImagePlayerService"

#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

#include <media/IMediaHTTPService.h>

#include <IImagePlayerService.h>

// ---------------------------------------------------------------------------

namespace android {

    class BpImagePlayerService : public BpInterface<IImagePlayerService> {
      public:
        BpImagePlayerService(const sp<IBinder>& impl)
            : BpInterface<IImagePlayerService>(impl) {
        }

        virtual int init() {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            remote()->transact(BnImagePlayerService::IMAGE_INIT, data, &reply);
            return NO_ERROR;
        }

        virtual int setDataSource (
            const sp<IMediaHTTPService> &httpService,
            const char *srcUrl) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            data.writeInt32(httpService != NULL);

            if (httpService != NULL) {
                //data.writeStrongBinder(httpService->asBinder());
                data.writeStrongBinder(IInterface::asBinder(httpService));
            }

            data.writeCString(srcUrl);
            remote()->transact(BnImagePlayerService::IMAGE_SET_DATA_SOURCE_URL, data,
                               &reply);
            return reply.readInt32();
        }

        virtual int notifyProcessDied (
            const sp<IBinder> &binder) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            data.writeInt32(binder != NULL);

            if (binder != NULL) {
                data.writeStrongBinder(binder);
            }

            remote()->transact(BnImagePlayerService::IMAGE_NOTIFY_PROCESSDIED, data,
                               &reply);
            return reply.readInt32();
        }

        virtual int setDataSource(const char* uri) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            data.writeCString(uri);
            remote()->transact(BnImagePlayerService::IMAGE_INIT, data, &reply);
            return NO_ERROR;
        }

        virtual int setSampleSurfaceSize(int sampleSize, int surfaceW, int surfaceH) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());

            data.writeInt32(sampleSize);
            data.writeInt32(surfaceW);
            data.writeInt32(surfaceH);
            remote()->transact(BnImagePlayerService::IMAGE_SET_SAMPLE_SURFACE_SIZE, data,
                               &reply);
            return NO_ERROR;
        }

        virtual int setRotate(float degrees, int autoCrop) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());

            data.writeFloat(degrees);
            data.writeInt32(autoCrop);
            remote()->transact(BnImagePlayerService::IMAGE_SET_ROTATE, data, &reply);
            return NO_ERROR;
        }

        virtual int setScale(float sx, float sy, int autoCrop) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());

            data.writeFloat(sx);
            data.writeFloat(sy);
            data.writeInt32(autoCrop);
            remote()->transact(BnImagePlayerService::IMAGE_SET_SCALE, data, &reply);
            return NO_ERROR;
        }

        virtual int setHWScale(float sc) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());

            data.writeFloat(sc);
            remote()->transact(BnImagePlayerService::IMAGE_SET_HWSCALE, data, &reply);
            return NO_ERROR;
        }

        virtual int setRotateScale(float degrees, float sx, float sy, int autoCrop) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());

            data.writeFloat(degrees);
            data.writeFloat(sx);
            data.writeFloat(sy);
            data.writeInt32(autoCrop);
            remote()->transact(BnImagePlayerService::IMAGE_SET_ROTATE_SCALE, data, &reply);
            return NO_ERROR;
        }

        virtual int setTranslate(float tx, float ty) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());

            data.writeFloat(tx);
            data.writeFloat(ty);
            remote()->transact(BnImagePlayerService::IMAGE_SET_TRANSLATE, data, &reply);
            return NO_ERROR;
        }

        virtual int setCropRect(int cropX, int cropY, int cropWidth, int cropHeight) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());

            data.writeInt32(cropX);
            data.writeInt32(cropY);
            data.writeInt32(cropWidth);
            data.writeInt32(cropHeight);
            remote()->transact(BnImagePlayerService::IMAGE_SET_CROP_RECT, data, &reply);
            return NO_ERROR;
        }

        virtual int start() {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            remote()->transact(BnImagePlayerService::IMAGE_START, data, &reply);
            return NO_ERROR;
        }

        virtual int prepare() {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            remote()->transact(BnImagePlayerService::IMAGE_PREPARE, data, &reply);
            return NO_ERROR;
        }

        virtual int show() {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            remote()->transact(BnImagePlayerService::IMAGE_SHOW, data, &reply);
            return NO_ERROR;
        }

        virtual int showImage(const char* uri) {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            data.writeCString(uri);
            remote()->transact(BnImagePlayerService::IMAGE_SHOW_URI, data, &reply);
            return NO_ERROR;
        }

        virtual int release() {
            Parcel data, reply;
            data.writeInterfaceToken(IImagePlayerService::getInterfaceDescriptor());
            remote()->transact(BnImagePlayerService::IMAGE_RELEASE, data, &reply);
            return NO_ERROR;
        }
    };

    IMPLEMENT_META_INTERFACE(ImagePlayerService, "droidlogic.IImagePlayerService");

    // ----------------------------------------------------------------------

    status_t BnImagePlayerService::onTransact(
        uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
        switch (code) {
            case IMAGE_INIT: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = init();
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_DATA_SOURCE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                String8 path(data.readString16());
                int result = setDataSource(path);
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_SAMPLE_SURFACE_SIZE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = setSampleSurfaceSize(data.readInt32(), data.readInt32(),
                                                  data.readInt32());
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_ROTATE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = setRotate(data.readFloat(), data.readInt32());
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_SCALE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = setScale(data.readFloat(), data.readFloat(), data.readInt32());
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_HWSCALE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = setHWScale(data.readFloat());
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_ROTATE_SCALE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = setRotateScale(data.readFloat(), data.readFloat(),
                                            data.readFloat(), data.readInt32());
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_TRANSLATE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = setTranslate(data.readFloat(), data.readFloat());
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_CROP_RECT: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = setCropRect(data.readInt32(), data.readInt32(), data.readInt32(),
                                         data.readInt32());
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_START: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = start();
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_PREPARE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = prepare();
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SHOW: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = show();
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SHOW_URI: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                String8 path(data.readString16());
                int result = showImage(path);
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_RELEASE: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);
                int result = release();
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_SET_DATA_SOURCE_URL: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);

                sp<IMediaHTTPService> httpService;

                if (data.readInt32()) {
                    httpService =
                        interface_cast<IMediaHTTPService>(data.readStrongBinder());
                }

                String8 srcUrl(data.readString16());
                //const char* srcUrl = data.readCString();
                int result = setDataSource(httpService, srcUrl);
                reply->writeInt32(result);
                return NO_ERROR;
            }

            case IMAGE_NOTIFY_PROCESSDIED: {
                CHECK_INTERFACE(IImagePlayerService, data, reply);

                sp<IBinder> binder;

                if (data.readInt32()) {
                    binder = data.readStrongBinder();
                }

                int result = notifyProcessDied(binder);
                reply->writeInt32(result);
                return NO_ERROR;
            }

            default: {
                return BBinder::onTransact(code, data, reply, flags);
            }
        }

        // should be unreachable
        return NO_ERROR;
    }

    // ----------------------------------------------------------------------------

};
