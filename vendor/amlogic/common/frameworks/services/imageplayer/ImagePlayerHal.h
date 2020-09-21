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

#ifndef ANDROID_DROIDLOGIC_IMAGESERVER_V1_0_IMAGEPLAYERHAL_H
#define ANDROID_DROIDLOGIC_IMAGESERVER_V1_0_IMAGEPLAYERHAL_H


#include <vendor/amlogic/hardware/imageserver/1.0/IImageService.h>
#include <vendor/amlogic/hardware/imageserver/1.0/types.h>
#include "ImagePlayerService.h"

namespace vendor {
    namespace amlogic {
        namespace hardware {
            namespace imageserver {
                namespace V1_0 {
                    namespace implementation {
                        using ::vendor::amlogic::hardware::imageserver::V1_0::IImageService;
                        using ::vendor::amlogic::hardware::imageserver::V1_0::Result;
                        using ::android::hardware::hidl_string;
                        using ::android::hardware::Return;
                        using ::android::hardware::Void;
                        using ::android::sp;
                        using ::android::ImagePlayerService;

                        class ImagePlayerHal : public IImageService {
                          public:
                            ImagePlayerHal();
                            ~ImagePlayerHal();

                            Return<Result> init() override;
                            Return<Result> setDataSource(const hidl_string& uri) override;
                            Return<Result> setSampleSurfaceSize(int32_t sampleSize, int32_t surfaceW,
                                                                int32_t surfaceH) override;
                            Return<Result> setRotate(float degrees, int32_t autoCrop) override;
                            Return<Result> setScale(float sx, float sy, int32_t autoCrop) override;
                            Return<Result> setHWScale(float sc) override;

                            Return<Result> setRotateScale(float degrees, float sx, float sy,
                                                          int32_t autoCrop) override;

                            Return<Result> setTranslate(float tx, float ty) override;
                            Return<Result> setCropRect(int32_t cropX, int32_t cropY, int32_t cropWidth,
                                                       int32_t cropHeight) override;

                            Return<Result> start() override;

                            Return<Result> prepare() override;

                            Return<Result> show() override;

                            Return<Result> showImage(const hidl_string& uri) override;

                            Return<Result> release() override;

                          private:
                            void handleServiceDeath(uint32_t cookie);
                            ImagePlayerService* mImagePlayer;
                            class  DeathRecipient : public android::hardware::hidl_death_recipient  {
                              public:
                                DeathRecipient(sp<ImagePlayerHal> sch);

                                // hidl_death_recipient interface
                                void serviceDied(uint64_t cookie,
                                                 const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
                              private:
                                sp<ImagePlayerHal> mImagePlayerHal;
                            };
                            sp<DeathRecipient> mDeathRecipient;
                        };//ImagePlayer
                    } //namespace implementation
                }//namespace V1_0
            } //namespace imageserver
        }//namespace hardware
    } //namespace android
} //namespace vendor

#endif // ANDROID_DROIDLOGIC_IMAGESERVER_V1_0_IMAGEPLAYERHAL_H
