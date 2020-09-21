/*
 * Copyright (C) 2012 The Android Open Source Project
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
 * This class simulates a hardware JPEG compressor.  It receives image buffers
 * in RGBA_8888 format, processes them in a worker thread, and then pushes them
 * out to their destination stream.
 */

#ifndef HW_EMULATOR_CAMERA2_JPEG_H
#define HW_EMULATOR_CAMERA2_JPEG_H

#include "utils/Thread.h"
#include "utils/Mutex.h"
#include "utils/Timers.h"
#include "Base.h"
#include <hardware/camera3.h>
#include <utils/List.h>
#include <stdio.h>

#include <libexif/exif-entry.h>
#include <libexif/exif-data.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-loader.h>
#include <libexif/exif-mem.h>

extern "C" {
#include <jpeglib.h>
}

namespace android {

struct CaptureRequest {
    uint32_t         frameNumber;
    camera3_stream_buffer *buf;
    Buffers         *sensorBuffers;
    bool    mNeedThumbnail;
};

typedef struct _exif_buffer {
    unsigned char *data;
    unsigned int size;
} exif_buffer;

class JpegCompressor: private Thread, public virtual RefBase {
  public:

    JpegCompressor();
    ~JpegCompressor();

    struct JpegListener {
        // Called when JPEG compression has finished, or encountered an error
        virtual void onJpegDone(const StreamBuffer &jpegBuffer,
                bool success, CaptureRequest &r) = 0;
        // Called when the input buffer for JPEG is not needed any more,
        // if the buffer came from the framework.
        virtual void onJpegInputDone(const StreamBuffer &inputBuffer) = 0;
        virtual ~JpegListener();
    };

    // Start compressing COMPRESSED format buffers; JpegCompressor takes
    // ownership of the Buffers vector.
    status_t start();
    status_t setlistener(JpegListener *listener);
    void queueRequest(CaptureRequest &r);

    // Compress and block until buffer is complete.
    status_t compressSynchronous(Buffers *buffers);

    status_t cancel();

    bool isBusy();
    bool isStreamInUse(uint32_t id);

    bool waitForDone(nsecs_t timeout);
    ssize_t GetMaxJpegBufferSize();
    void SetMaxJpegBufferSize(ssize_t size);
    void SetExifInfo(struct ExifInfo info);
    status_t Create_Exif_Use_Libexif();
    exif_buffer *get_exif_buffer();
    void exif_entry_set_string (ExifData * pEdata, ExifIfd eEifd, ExifTag eEtag, const char *s);
    void exif_entry_set_short (ExifData * pEdata, ExifIfd eEifd, ExifTag eEtag, ExifShort n);
    void exif_entry_set_long (ExifData * pEdata, ExifIfd eEifd, ExifTag eEtag, ExifLong n);
    void exif_entry_set_rational (ExifData * pEdata, ExifIfd eEifd, ExifTag eEtag, ExifRational r);
    void exif_entry_set_undefined (ExifData * pEdata, ExifIfd eEifd, ExifTag eEtag, exif_buffer * buf);
    ExifEntry *init_tag(ExifData *exif, ExifIfd ifd, ExifTag tag);
    ExifEntry *create_tag(ExifData *exif, ExifIfd ifd, ExifTag tag, size_t len);
    void exif_entry_set_gps_coord(ExifData * pEdata, ExifTag eEtag,
         ExifRational r1, ExifRational r2, ExifRational r3);
    void exif_entry_set_gps_altitude(ExifData * pEdata, ExifTag eEtag, ExifRational r1);
    void exif_entry_set_gps_coord_ref(ExifData * pEdata, ExifTag eEtag, const char *s);
    void exif_entry_set_gps_altitude_ref(ExifData * pEdata, ExifTag eEtag, ExifByte n);

    // TODO: Measure this
    static const size_t kMaxJpegSize = 8000000;
    ssize_t mMaxbufsize;

  private:
    Mutex mBusyMutex;
    bool mIsBusy;
    Condition mDone;
    bool mSynchronous;

    Mutex mMutex;

    List<CaptureRequest*> mInJpegRequestQueue;
    Condition     mInJpegRequestSignal;
    //camera3_stream_buffer *tempHalbuffers;
    //Buffers         *tempBuffers;
    CaptureRequest mJpegRequest;
    bool mExitJpegThread;
    bool mNeedexif;
    int mMainJpegSize, mThumbJpegSize;
    uint8_t *mSrcThumbBuffer;
    uint8_t *mDstThumbBuffer;
    Buffers *mBuffers;
    int mPendingrequest;
    JpegListener *mListener;
    struct ExifInfo mInfo;
    StreamBuffer mJpegBuffer, mAuxBuffer;
    bool mFoundJpeg, mFoundAux;
    //jpeg_compress_struct mCInfo;

    struct JpegError : public jpeg_error_mgr {
        JpegCompressor *parent;
    };
    j_common_ptr mJpegErrorInfo;

    struct JpegDestination : public jpeg_destination_mgr {
        JpegCompressor *parent;
    };

    bool checkError(const char *msg);
    status_t compress();

    status_t thumbcompress();
    void cleanUp();

    /**
     * Inherited Thread virtual overrides
     */
  private:
    virtual status_t readyToRun();
    virtual bool threadLoop();
};

} // namespace android

#endif
