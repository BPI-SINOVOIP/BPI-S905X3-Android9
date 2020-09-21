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
 *  @author   xiaoliang.wang
 *  @version  1.0
 *  @date     2015/07/07
 *  @par function description:
 *  - 1 invoke subtile service from native to java
 */

#ifndef ANDROID_ISUBTITLESERVICE_H
#define ANDROID_ISUBTITLESERVICE_H

#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <utils/String8.h>
#include <utils/String16.h>

namespace android {

// ----------------------------------------------------------------------------

// must be kept in sync with interface defined in ISubTitleService.aidl
class ISubTitleService : public IInterface
{
public:
    DECLARE_META_INTERFACE(SubTitleService);

    virtual void open(const String16& path) = 0;
    virtual void openIdx(int32_t idx) = 0;
    virtual void close() = 0;
    virtual int32_t getTotal() = 0;
    virtual void next() = 0;
    virtual void previous() = 0;
    virtual void showSub(int32_t pos) = 0;
    virtual void option() = 0;
    virtual int32_t getType() = 0;
    virtual const String16& getTypeStr() const = 0;
    virtual int32_t getTypeDetial() = 0;
    virtual void setTextColor(int32_t color) = 0;
    virtual void setTextSize(int32_t size) = 0;
    virtual void setGravity(int32_t gravity) = 0;
    virtual void setTextStyle(int32_t style) = 0;
    virtual void setPosHeight(int32_t height) = 0;
    virtual void setImgRatio(float ratioW, float ratioH, int32_t maxW, int32_t maxH) = 0;
    virtual void clear() = 0;
    virtual void resetForSeek() = 0;
    virtual void hide() = 0;
    virtual void display() = 0;
    virtual const String16& getCurName() const = 0;
    virtual const String16& getName(int32_t idx) const = 0;
    virtual const String16& getLanguage(int32_t idx) const = 0;
    virtual bool load(const String16& path) = 0;
    virtual void setSurfaceViewParam(int x, int y, int w, int h) = 0;
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_ISUBTITLESERVICE_H