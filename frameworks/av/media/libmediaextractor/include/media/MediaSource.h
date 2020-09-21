/*

 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef MEDIA_SOURCE_H_

#define MEDIA_SOURCE_H_

#include <sys/types.h>

#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <utils/RefBase.h>

#include <media/MediaTrack.h>

namespace android {

class MediaBuffer;

struct MediaSource : public virtual RefBase {
    MediaSource();

    // To be called before any other methods on this object, except
    // getFormat().
    virtual status_t start(MetaData *params = NULL) = 0;

    // Any blocking read call returns immediately with a result of NO_INIT.
    // It is an error to call any methods other than start after this call
    // returns. Any buffers the object may be holding onto at the time of
    // the stop() call are released.
    // Also, it is imperative that any buffers output by this object and
    // held onto by callers be released before a call to stop() !!!
    virtual status_t stop() = 0;

    // Returns the format of the data output by this media source.
    virtual sp<MetaData> getFormat() = 0;

    // Options that modify read() behaviour. The default is to
    // a) not request a seek
    // b) not be late, i.e. lateness_us = 0
    typedef MediaTrack::ReadOptions ReadOptions;

    // Returns a new buffer of data. Call blocks until a
    // buffer is available, an error is encountered of the end of the stream
    // is reached.
    // End of stream is signalled by a result of ERROR_END_OF_STREAM.
    // A result of INFO_FORMAT_CHANGED indicates that the format of this
    // MediaSource has changed mid-stream, the client can continue reading
    // but should be prepared for buffers of the new configuration.
    virtual status_t read(
            MediaBufferBase **buffer, const ReadOptions *options = NULL) = 0;

    // Causes this source to suspend pulling data from its upstream source
    // until a subsequent read-with-seek. This is currently not supported
    // as such by any source. E.g. MediaCodecSource does not suspend its
    // upstream source, and instead discard upstream data while paused.
    virtual status_t pause() {
        return ERROR_UNSUPPORTED;
    }

    // The consumer of this media source requests the source stops sending
    // buffers with timestamp larger than or equal to stopTimeUs. stopTimeUs
    // must be in the same time base as the startTime passed in start(). If
    // source does not support this request, ERROR_UNSUPPORTED will be returned.
    // If stopTimeUs is invalid, BAD_VALUE will be returned. This could be
    // called at any time even before source starts and it could be called
    // multiple times. Setting stopTimeUs to be -1 will effectively cancel the stopTimeUs
    // set previously. If stopTimeUs is larger than or equal to last buffer's timestamp,
    // source will start to drop buffer when it gets a buffer with timestamp larger
    // than or equal to stopTimeUs. If stopTimeUs is smaller than or equal to last
    // buffer's timestamp, source will drop all the incoming buffers immediately.
    // After setting stopTimeUs, source may still stop sending buffers with timestamp
    // less than stopTimeUs if it is stopped by the consumer.
    virtual status_t setStopTimeUs(int64_t /* stopTimeUs */) {
        return ERROR_UNSUPPORTED;
    }

protected:
    virtual ~MediaSource();

private:
    MediaSource(const MediaSource &);
    MediaSource &operator=(const MediaSource &);
};

}  // namespace android

#endif  // MEDIA_SOURCE_H_
