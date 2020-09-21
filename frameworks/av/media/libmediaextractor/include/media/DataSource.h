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

#ifndef DATA_SOURCE_H_

#define DATA_SOURCE_H_

#include <sys/types.h>
#include <media/stagefright/MediaErrors.h>
#include <media/DataSourceBase.h>
#include <media/IDataSource.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <drm/DrmManagerClient.h>


namespace android {

class String8;

class DataSource : public DataSourceBase, public virtual RefBase {
public:
    DataSource() {}

    // returns a pointer to IDataSource if it is wrapped.
    virtual sp<IDataSource> getIDataSource() const {
        return nullptr;
    }

    virtual String8 toString() {
        return String8("<unspecified>");
    }

    virtual status_t reconnectAtOffset(off64_t /*offset*/) {
        return ERROR_UNSUPPORTED;
    }

    ////////////////////////////////////////////////////////////////////////////

    // for DRM
    virtual sp<DecryptHandle> DrmInitialization(const char * /*mime*/ = NULL) {
        return NULL;
    }

    virtual String8 getUri() {
        return String8();
    }

    virtual bool getUri(char *uriString, size_t bufferSize) final {
        int ret = snprintf(uriString, bufferSize, "%s", getUri().c_str());
        return ret >= 0 && static_cast<size_t>(ret) < bufferSize;
    }

    virtual String8 getMIMEType() const {
        return String8("application/octet-stream");
    }

protected:
    virtual ~DataSource() {}

private:
    DataSource(const DataSource &);
    DataSource &operator=(const DataSource &);
};

}  // namespace android

#endif  // DATA_SOURCE_H_
