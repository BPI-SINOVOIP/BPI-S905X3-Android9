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

#define LOG_TAG "ISubTitleService"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <ISubTitleService.h>

namespace android {

// must be kept in sync with ISubTitleService.aidl
enum {
    SUB_OPEN                            = IBinder::FIRST_CALL_TRANSACTION,
    SUB_OPEN_IDX                        = IBinder::FIRST_CALL_TRANSACTION + 1,
    SUB_CLOSE                           = IBinder::FIRST_CALL_TRANSACTION + 2,
    SUB_GET_TOTAL                       = IBinder::FIRST_CALL_TRANSACTION + 3,
    SUB_GET_INNER_TOTAL                 = IBinder::FIRST_CALL_TRANSACTION + 4,
    SUB_GET_EXTERNAL_TOTAL              = IBinder::FIRST_CALL_TRANSACTION + 5,
    SUB_NEXT                            = IBinder::FIRST_CALL_TRANSACTION + 6,
    SUB_PREVIOUS                        = IBinder::FIRST_CALL_TRANSACTION + 7,
    SUB_SHOW_SUB                        = IBinder::FIRST_CALL_TRANSACTION + 8,
    SUB_OPTION                          = IBinder::FIRST_CALL_TRANSACTION + 9,
    SUB_GET_TYPE                        = IBinder::FIRST_CALL_TRANSACTION + 10,
    SUB_GET_TYPE_STR                    = IBinder::FIRST_CALL_TRANSACTION + 11,
    SUB_GET_TYPE_DETIAL                 = IBinder::FIRST_CALL_TRANSACTION + 12,
    SUB_SET_TEXT_COLOR                  = IBinder::FIRST_CALL_TRANSACTION + 13,
    SUB_SET_TEXT_SIZE                   = IBinder::FIRST_CALL_TRANSACTION + 14,
    SUB_SET_GRAVITY                     = IBinder::FIRST_CALL_TRANSACTION + 15,
    SUB_SET_TEXT_STYLE                  = IBinder::FIRST_CALL_TRANSACTION + 16,
    SUB_SET_POS_HEIGHT                  = IBinder::FIRST_CALL_TRANSACTION + 17,
    SUB_SET_IMG_RATIO                   = IBinder::FIRST_CALL_TRANSACTION + 18,
    SUB_CLEAR                           = IBinder::FIRST_CALL_TRANSACTION + 19,
    SUB_RESET_FOR_SEEK                  = IBinder::FIRST_CALL_TRANSACTION + 20,
    SUB_HIDE                            = IBinder::FIRST_CALL_TRANSACTION + 21,
    SUB_DISPLAY                         = IBinder::FIRST_CALL_TRANSACTION + 22,
    SUB_GET_CUR_NAME                    = IBinder::FIRST_CALL_TRANSACTION + 23,
    SUB_GET_NAME                        = IBinder::FIRST_CALL_TRANSACTION + 24,
    SUB_GET_LANGUAGE                    = IBinder::FIRST_CALL_TRANSACTION + 25,
    SUB_LOAD                            = IBinder::FIRST_CALL_TRANSACTION + 26,
    SUB_SET_SURFACE_PARAM               = IBinder::FIRST_CALL_TRANSACTION + 27,
};

class BpSubTitleService : public BpInterface<ISubTitleService>
{
public:
    BpSubTitleService(const sp<IBinder>& impl)
        : BpInterface<ISubTitleService>(impl)
    {
    }

    virtual void open(const String16& path)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeString16(path);
        ALOGV("open path:%s\n", String8(path).string());

        if (remote()->transact(SUB_OPEN, data, &reply) != NO_ERROR) {
            ALOGD("open not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("open caught exception %d\n", err);
        }
        return;
    }

    virtual void openIdx(int32_t idx)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(idx);
        ALOGV("openIdx idx:%d\n", idx);

        if (remote()->transact(SUB_OPEN_IDX, data, &reply) != NO_ERROR) {
            ALOGD("openIdx could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("openIdx caught exception %d\n", err);
        }
        return;
    }

    virtual void close()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("close\n");

        if (remote()->transact(SUB_CLOSE, data, &reply) != NO_ERROR) {
            ALOGD("close could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("close caught exception %d\n", err);
        }
        return;
    }

    virtual int32_t getTotal()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("getTotal\n");

        if (remote()->transact(SUB_GET_TOTAL, data, &reply) != NO_ERROR) {
            ALOGD("getTotal could not contact remote\n");
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("getTotal caught exception %d\n", err);
            return -1;
        }
        return reply.readInt32();
    }

    virtual void next()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("next\n");

        if (remote()->transact(SUB_NEXT, data, &reply) != NO_ERROR) {
            ALOGD("next could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("next caught exception %d\n", err);
        }
        return;
    }

    virtual void previous()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("previous\n");

        if (remote()->transact(SUB_PREVIOUS, data, &reply) != NO_ERROR) {
            ALOGD("previous could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("previous caught exception %d\n", err);
        }
        return;
    }

    virtual void showSub(int32_t pos)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(pos);
        ALOGV("showSub pos:%d\n", pos);

        if (remote()->transact(SUB_SHOW_SUB, data, &reply) != NO_ERROR) {
            ALOGD("showSub could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("showSub caught exception %d\n", err);
        }
        return;
    }

    virtual void option()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("option\n");

        if (remote()->transact(SUB_OPTION, data, &reply) != NO_ERROR) {
            ALOGD("option could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("option caught exception %d\n", err);
        }
        return;
    }

    virtual int32_t getType()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("getType\n");

        if (remote()->transact(SUB_GET_TYPE, data, &reply) != NO_ERROR) {
            ALOGD("getType could not contact remote\n");
            return 0;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("getType caught exception %d\n", err);
            return 0;
        }
        return reply.readInt32();
    }

    virtual const String16& getTypeStr() const
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("getTypeStr\n");

        if (remote()->transact(SUB_GET_TYPE_STR, data, &reply) != NO_ERROR) {
            ALOGD("getTypeStr could not contact remote\n");
            return String16("");
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("getTypeStr caught exception %d\n", err);
            return String16("");
        }
        return reply.readString16();
    }

    virtual int32_t getTypeDetial()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("getTypeDetial\n");

        if (remote()->transact(SUB_GET_TYPE_DETIAL, data, &reply) != NO_ERROR) {
            ALOGD("getTypeDetial could not contact remote\n");
            return 0;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("getTypeDetial caught exception %d\n", err);
            return 0;
        }
        return reply.readInt32();
    }

    virtual void setTextColor(int32_t color)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(color);
        ALOGV("setTextColor color:%d\n", color);

        if (remote()->transact(SUB_SET_TEXT_COLOR, data, &reply) != NO_ERROR) {
            ALOGD("setTextColor could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("setTextColor caught exception %d\n", err);
        }
        return;
    }

    virtual void setTextSize(int32_t size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(size);
        ALOGV("setTextSize size:%d\n", size);

        if (remote()->transact(SUB_SET_TEXT_SIZE, data, &reply) != NO_ERROR) {
            ALOGD("setTextSize could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("setTextSize caught exception %d\n", err);
        }
        return;
    }

    virtual void setGravity(int32_t gravity)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(gravity);
        ALOGV("setGravity gravity:%d\n", gravity);

        if (remote()->transact(SUB_SET_GRAVITY, data, &reply) != NO_ERROR) {
            ALOGD("setGravity could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("setGravity caught exception %d\n", err);
        }
        return;
    }

    virtual void setTextStyle(int32_t style)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(style);
        ALOGV("setTextStyle style:%d\n", style);

        if (remote()->transact(SUB_SET_TEXT_STYLE, data, &reply) != NO_ERROR) {
            ALOGD("setTextStyle could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("setTextStyle caught exception %d\n", err);
        }
        return;
    }

    virtual void setPosHeight(int32_t height)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(height);
        ALOGV("setPosHeight height:%d\n", height);

        if (remote()->transact(SUB_SET_POS_HEIGHT, data, &reply) != NO_ERROR) {
            ALOGD("setPosHeight could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("setPosHeight caught exception %d\n", err);
        }
        return;
    }

    virtual void setImgRatio(float ratioW, float ratioH, int32_t maxW, int32_t maxH)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeFloat(ratioW);
        data.writeFloat(ratioH);
        data.writeInt32(maxW);
        data.writeInt32(maxH);
        ALOGV("setImgRatio (ratioW, ratioH, maxW, maxH):(%f, %f, d%, %d)\n", ratioW, ratioH, maxW, maxH);

        if (remote()->transact(SUB_SET_IMG_RATIO, data, &reply) != NO_ERROR) {
            ALOGD("setImgRatio could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("setImgRatio caught exception %d\n", err);
        }
        return;
    }

    virtual void clear()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("clear\n");

        if (remote()->transact(SUB_CLEAR, data, &reply) != NO_ERROR) {
            ALOGD("clear could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("clear caught exception %d\n", err);
        }
        return;
    }

    virtual void resetForSeek()
	{
	    Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("resetForSeek\n");

        if (remote()->transact(SUB_RESET_FOR_SEEK, data, &reply) != NO_ERROR) {
            ALOGD("resetForSeek could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("resetForSeek caught exception %d\n", err);
        }
        return;
    }

    virtual void hide()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("hide\n");

        if (remote()->transact(SUB_HIDE, data, &reply) != NO_ERROR) {
            ALOGD("hide could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("hide caught exception %d\n", err);
        }
        return;
    }

    virtual void display()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("display\n");

        if (remote()->transact(SUB_DISPLAY, data, &reply) != NO_ERROR) {
            ALOGD("display could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("display caught exception %d\n", err);
        }
        return;
    }

    virtual const String16& getCurName() const
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        ALOGV("getCurName\n");

        if (remote()->transact(SUB_GET_CUR_NAME, data, &reply) != NO_ERROR) {
            ALOGD("getCurName could not contact remote\n");
            return String16("");
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("getCurName caught exception %d\n", err);
            return String16("");
        }
        return reply.readString16();
    }

    virtual const String16& getName(int32_t idx) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(idx);
        ALOGV("getName idx:%d\n", idx);

        if (remote()->transact(SUB_GET_NAME, data, &reply) != NO_ERROR) {
            ALOGD("getName could not contact remote\n");
            return String16("");
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("getName caught exception %d\n", err);
            return String16("");
        }
        return reply.readString16();
    }

    virtual const String16& getLanguage(int32_t idx) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(idx);
        ALOGV("getLanguage idx:%d\n", idx);

        if (remote()->transact(SUB_GET_LANGUAGE, data, &reply) != NO_ERROR) {
            ALOGD("getLanguage could not contact remote\n");
            return String16("");
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("getLanguage caught exception %d\n", err);
            return String16("");
        }
        return reply.readString16();
    }

    virtual bool load(const String16& path)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeString16(path);
        ALOGV("load path:%s\n", String8(path).string());

        if (remote()->transact(SUB_LOAD, data, &reply) != NO_ERROR) {
            ALOGD("load could not contact remote\n");
            return false;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("load caught exception %d\n", err);
            return false;
        }
        return reply.readInt32() != 0;
	}

    virtual void setSurfaceViewParam(int32_t x, int32_t y, int32_t w, int32_t h)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISubTitleService::getInterfaceDescriptor());
        data.writeInt32(x);
        data.writeInt32(y);
        data.writeInt32(w);
        data.writeInt32(h);
        ALOGV("setSurfaceViewParam (x, y, w, h):(%d, %d, d%, %d)\n", x, y, w, h);

        if (remote()->transact(SUB_SET_SURFACE_PARAM, data, &reply) != NO_ERROR) {
            ALOGD("setSurfaceViewParam could not contact remote\n");
            return;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("setSurfaceViewParam caught exception %d\n", err);
        }
        return;
    }
};

IMPLEMENT_META_INTERFACE(SubTitleService, "com.droidlogic.SubTitleService.ISubTitleService");

// ----------------------------------------------------------------------------

}; // namespace android
