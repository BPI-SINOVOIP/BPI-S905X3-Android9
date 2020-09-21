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

#ifndef MEDIA_BUFFER_BASE_H_

#define MEDIA_BUFFER_BASE_H_

namespace android {

class MediaBufferBase;
class MetaDataBase;

class MediaBufferObserver {
public:
    MediaBufferObserver() {}
    virtual ~MediaBufferObserver() {}

    virtual void signalBufferReturned(MediaBufferBase *buffer) = 0;

private:
    MediaBufferObserver(const MediaBufferObserver &);
    MediaBufferObserver &operator=(const MediaBufferObserver &);
};

class MediaBufferBase {
public:
    static MediaBufferBase *Create(size_t size);

    // If MediaBufferGroup is set, decrement the local reference count;
    // if the local reference count drops to 0, return the buffer to the
    // associated MediaBufferGroup.
    //
    // If no MediaBufferGroup is set, the local reference count must be zero
    // when called, whereupon the MediaBuffer is deleted.
    virtual void release() = 0;

    // Increments the local reference count.
    // Use only when MediaBufferGroup is set.
    virtual void add_ref() = 0;

    virtual void *data() const = 0;
    virtual size_t size() const = 0;

    virtual size_t range_offset() const = 0;
    virtual size_t range_length() const = 0;

    virtual void set_range(size_t offset, size_t length) = 0;

    virtual MetaDataBase& meta_data() = 0;

    // Clears meta data and resets the range to the full extent.
    virtual void reset() = 0;

    virtual void setObserver(MediaBufferObserver *group) = 0;

    // Returns a clone of this MediaBufferBase increasing its reference
    // count. The clone references the same data but has its own range and
    // MetaData.
    virtual MediaBufferBase *clone() = 0;

    virtual int refcount() const = 0;

    virtual int localRefcount() const = 0;
    virtual int remoteRefcount() const = 0;

    virtual ~MediaBufferBase() {};
};

}  // namespace android

#endif  // MEDIA_BUFFER_BASE_H_
