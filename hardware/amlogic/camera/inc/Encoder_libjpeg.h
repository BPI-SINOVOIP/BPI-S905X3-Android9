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

/**
* @file Encoder_libjpeg.h
*
* This defines API for camerahal to encode YUV using libjpeg
*
*/

#ifndef ANDROID_CAMERA_HARDWARE_ENCODER_LIBJPEG_H
#define ANDROID_CAMERA_HARDWARE_ENCODER_LIBJPEG_H

#include <utils/threads.h>
#include <utils/RefBase.h>

#ifdef AMLOGIC_HW_JPEGENC
#include "jpegenc.h"
#endif

extern "C" {
#include "jhead.h"
}
namespace android {
/**
 * libjpeg encoder class - uses libjpeg to encode yuv
 */

#define MAX_EXIF_TAGS_SUPPORTED 30
typedef void (*encoder_libjpeg_callback_t) (void* main_jpeg,
                                            void* thumb_jpeg,
                                            CameraFrame::FrameType type,
                                            void* cookie1,
                                            void* cookie2,
                                            void* cookie3);

static const char TAG_MODEL[] = "Model";
static const char TAG_MAKE[] = "Make";
static const char TAG_FOCALLENGTH[] = "FocalLength";
static const char TAG_DATETIME[] = "DateTime";
static const char TAG_IMAGE_WIDTH[] = "ImageWidth";
static const char TAG_IMAGE_LENGTH[] = "ImageLength";
static const char TAG_GPS_LAT[] = "GPSLatitude";
static const char TAG_GPS_LAT_REF[] = "GPSLatitudeRef";
static const char TAG_GPS_LONG[] = "GPSLongitude";
static const char TAG_GPS_LONG_REF[] = "GPSLongitudeRef";
static const char TAG_GPS_ALT[] = "GPSAltitude";
static const char TAG_GPS_ALT_REF[] = "GPSAltitudeRef";
static const char TAG_GPS_MAP_DATUM[] = "GPSMapDatum";
static const char TAG_GPS_PROCESSING_METHOD[] = "GPSProcessingMethod";
static const char TAG_GPS_VERSION_ID[] = "GPSVersionID";
static const char TAG_GPS_TIMESTAMP[] = "GPSTimeStamp";
static const char TAG_GPS_DATESTAMP[] = "GPSDateStamp";
static const char TAG_ORIENTATION[] = "Orientation";

class ExifElementsTable {
    public:
        ExifElementsTable() :
           gps_tag_count(0), exif_tag_count(0), position(0),
           jpeg_opened(false) { }
        ~ExifElementsTable();

        status_t insertElement(const char* tag, const char* value);
        void insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size);
        status_t insertExifThumbnailImage(const char*, int);
        void saveJpeg(unsigned char* picture, size_t jpeg_size);
        static const char* degreesToExifOrientation(const char*);
        static void stringToRational(const char*, unsigned int*, unsigned int*);
        static bool isAsciiTag(const char* tag);
    private:
        ExifElement_t table[MAX_EXIF_TAGS_SUPPORTED];
        unsigned int gps_tag_count;
        unsigned int exif_tag_count;
        unsigned int position;
        bool jpeg_opened;
};

class Encoder_libjpeg : public Thread {
    /* public member types and variables */
    public:
        struct params {
            uint8_t* src;
            int src_size;
            uint8_t* dst;
            int dst_size;
            int quality;
            int in_width;
            int in_height;
            int out_width;
            int out_height;
            const char* format;
            size_t jpeg_size;
         };
    /* private member types and variables */
    public:
        enum format {
            YUV420SP,
            YUV422I,
            RGB24,
        };
    /* public member functions */
    public:
        Encoder_libjpeg(params* main_jpeg,
                        params* tn_jpeg,
                        encoder_libjpeg_callback_t cb,
                        CameraFrame::FrameType type,
                        void* cookie1,
                        void* cookie2,
                        void* cookie3)
            : Thread(false), mMainInput(main_jpeg), mThumbnailInput(tn_jpeg), mCb(cb),
              mCancelEncoding(false), mCookie1(cookie1), mCookie2(cookie2), mCookie3(cookie3),
              mType(type), mThumb(NULL) {
            this->incStrong(this);
        }

        ~Encoder_libjpeg() {
            CAMHAL_LOGVB("~Encoder_libjpeg(%p)", this);
        }

        virtual bool threadLoop() {
            size_t size = 0;
            sp<Encoder_libjpeg> tn = NULL;
            if (mThumbnailInput) {
                // start thread to encode thumbnail
                mThumb = new Encoder_libjpeg(mThumbnailInput, NULL, NULL, mType, NULL, NULL, NULL);
                mThumb->run();
            }

            // encode our main image
            size = encode(mMainInput);

            // check if it is main jpeg thread
            if(mThumb.get()) {
                // wait until tn jpeg thread exits.
                mThumb->join();
                mThumb.clear();
                mThumb = NULL;
            }

            if(mCb) {
                mCb(mMainInput, mThumbnailInput, mType, mCookie1, mCookie2, mCookie3);
            }

            // encoder thread runs, self-destructs, and then exits
            this->decStrong(this);
            return false;
        }

        void cancel() {
           if (mThumb.get()) {
               mThumb->cancel();
           }
           mCancelEncoding = true;
        }

    private:
        params* mMainInput;
        params* mThumbnailInput;
        encoder_libjpeg_callback_t mCb;
        bool mCancelEncoding;
        void* mCookie1;
        void* mCookie2;
        void* mCookie3;
        CameraFrame::FrameType mType;
        sp<Encoder_libjpeg> mThumb;

        size_t encode(params*);
#ifdef AMLOGIC_HW_JPEGENC
        hw_jpegenc_t hw_info;
#endif
};

}

#endif
