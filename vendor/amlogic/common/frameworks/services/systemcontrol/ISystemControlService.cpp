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
 *  @author   tellen
 *  @version  1.0
 *  @date     2013/04/26
 *  @par function description:
 *  - 1 write property or sysfs from native to java service
 */

#define LOG_TAG "ISystemControl"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <ISystemControlService.h>

namespace android {

class BpSystemControlService : public BpInterface<ISystemControlService>
{
public:
    BpSystemControlService(const sp<IBinder>& impl)
        : BpInterface<ISystemControlService>(impl)
    {
    }

    virtual bool getProperty(const String16& key, String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(key);
        ALOGV("getProperty key:%s\n", String8(key).string());

        if (remote()->transact(GET_PROPERTY, data, &reply) != NO_ERROR) {
            ALOGD("getProperty could not contact remote\n");
            return false;
        }

        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("getProperty caught exception %d\n", err);
            return false;
        }
        value = reply.readString16();
        return true;
    }

    virtual bool getPropertyString(const String16& key, String16& value, String16& def)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(key);
        data.writeString16(def);
        ALOGV("getPropertyString key:%s\n", String8(key).string());

        if (remote()->transact(GET_PROPERTY_STRING, data, &reply) != NO_ERROR) {
            ALOGD("getPropertyString could not contact remote\n");
            return false;
        }

        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("getPropertyString caught exception %d\n", err);
            return false;
        }

        value = reply.readString16();
        return true;
    }

    virtual int32_t getPropertyInt(const String16& key, int32_t def)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(key);
        data.writeInt32(def);
        ALOGV("getPropertyInt key:%s\n", String8(key).string());

        if (remote()->transact(GET_PROPERTY_INT, data, &reply) != NO_ERROR) {
            ALOGE("getPropertyInt could not contact remote\n");
            return -1;
        }

        return reply.readInt32();
    }

    virtual int64_t getPropertyLong(const String16& key, int64_t def)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(key);
        data.writeInt64(def);
        ALOGV("getPropertyLong key:%s\n", String8(key).string());

        if (remote()->transact(GET_PROPERTY_LONG, data, &reply) != NO_ERROR) {
            ALOGE("getPropertyLong could not contact remote\n");
            return -1;
        }

        return reply.readInt64();
    }

    virtual bool getPropertyBoolean(const String16& key, bool def)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(key);
        data.writeInt32(def?1:0);

        ALOGV("getPropertyBoolean key:%s\n", String8(key).string());

        if (remote()->transact(GET_PROPERTY_BOOL, data, &reply) != NO_ERROR) {
            ALOGE("getPropertyBoolean could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual void setProperty(const String16& key, const String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(key);
        data.writeString16(value);
        ALOGV("setProperty key:%s, value:%s\n", String8(key).string(), String8(value).string());

        if (remote()->transact(SET_PROPERTY, data, &reply) != NO_ERROR) {
            ALOGE("setProperty could not contact remote\n");
            return;
        }
    }

    virtual bool readSysfs(const String16& path, String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(path);
        ALOGV("setProperty path:%s\n", String8(path).string());

        if (remote()->transact(READ_SYSFS, data, &reply) != NO_ERROR) {
            ALOGE("readSysfs could not contact remote\n");
            return false;
        }

        value = reply.readString16();
        return true;
    }

    virtual bool writeSysfs(const String16& path, const String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(path);
        data.writeString16(value);
        ALOGV("writeSysfs path:%s, value:%s\n", String8(path).string(), String8(value).string());

        if (remote()->transact(WRITE_SYSFS, data, &reply) != NO_ERROR) {
            ALOGE("writeSysfs could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual bool writeSysfs(const String16& path, const char *value, const int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(path);
        data.writeInt32(size);
        data.write((void *)value, size);
        ALOGV("writeSysfs path:%s, size:%d\n", String8(path).string(), size);

        if (remote()->transact(WRITE_SYSFS_BIN, data, &reply) != NO_ERROR) {
            ALOGE("writeSysfs could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual bool writeUnifyKey(const String16& path, const String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(path);
        data.writeString16(value);
        ALOGV("writeUnifyKey path:%s, value:%s\n", String8(path).string(), String8(path).string());

        if (remote()->transact(WRITE_UNIFY_KEY, data, &reply) != NO_ERROR) {
            ALOGE("writeUnifyKey could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual bool readUnifyKey(const String16& path, String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(path);
        ALOGV("readUnifyKey path:%s\n", String8(path).string());

        if (remote()->transact(READ_UNIFY_KEY, data, &reply) != NO_ERROR) {
            ALOGE("readUnifyKey could not contact remote\n");
            return false;
        }

        value = reply.readString16();
        return reply.readInt32() != 0;
    }

    virtual bool writePlayreadyKey(const String16& path, const char *value, const int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(path);
        data.writeInt32(size);
        data.write((void *)value, size);
        ALOGV("writePlayreadyKey path:%s, size:%d\n", String8(path).string(), size);

        if (remote()->transact(WRITE_PLAYREADY_KEY, data, &reply) != NO_ERROR) {
            ALOGE("writePlayreadyKey could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual int32_t readPlayreadyKey(const String16& path, char *value, int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(path);
        data.writeInt32(size);
        ALOGV("readPlayreadyKey path:%s\n", String8(path).string());

        if (remote()->transact(READ_PLAYREADY_KEY, data, &reply) != NO_ERROR) {
            ALOGE("readPlayreadyKey could not contact remote\n");
            return false;
        }

        int len = reply.readInt32();
        reply.read(value, len);
        ALOGE("readPlayreadyKey len:%d\n", len);
        return len;
    }

    virtual int32_t readAttestationKey(const String16& node, const String16& name, char *value, int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(node);
        data.writeString16(name);
        data.writeInt32(size);
        ALOGE("readAttestationKey size = %d\n", size);

        if (remote()->transact(READ_ATTESTATION_KEY, data, &reply) != NO_ERROR) {
            ALOGE("readAttestationKey could not contact remote\n");
            return false;
        }

        int len = reply.readInt32();
        reply.read(value, len);
        ALOGE("readAttestationKey len:%d\n", len);
        return len;
    }

    virtual bool writeAttestationKey(const String16& node, const String16& name, const char *buff, const int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(node);
        data.writeString16(name);
        data.writeInt32(size);
        data.write((void *)buff, size);
        ALOGE("writeAttestationKey in Isystemcontrolservice size:%d\n", size);

        if (remote()->transact(WRITE_ATTESTATION_KEY, data, &reply) != NO_ERROR) {
            ALOGE("writeAttestationKey could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual int32_t readHdcpRX22Key(char *value, int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(size);
        ALOGE("readHdcpRX22Key size:%d\n", size);

        if (remote()->transact(READ_HDCPRX22_KEY, data, &reply) != NO_ERROR) {
            ALOGE("readHdcp22Key could not contact remote\n");
            return false;
        }

        int len = reply.readInt32();
        reply.read(value, len);
        ALOGE("readHdcpRX22Key len:%d\n", len);
        return len;
    }

    virtual bool writeHdcpRX22Key(const char *value, const int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(size);
        data.write((void *)value, size);
        ALOGV("writeHdcp22Key size:%d\n", size);

        if (remote()->transact(WRITE_HDCPRX22_KEY, data, &reply) != NO_ERROR) {
            ALOGE("writeHdcp22Key could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual bool getModeSupportDeepColorAttr(const std::string& mode,const std::string& color)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(String16(mode.c_str()));
        data.writeString16(String16(color.c_str()));

        if (remote()->transact(GET_MODE_SUPPORT_DEEPCOLOR, data, &reply) != NO_ERROR) {
            ALOGE("getModeSupportDeepColorAttr could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }
    virtual int32_t readHdcpRX14Key(char *value, int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(size);
        ALOGV("readHdcpRX14Key size:%d\n", size);

        if (remote()->transact(READ_HDCPRX14_KEY, data, &reply) != NO_ERROR) {
            ALOGE("readHdcpRX14Key could not contact remote\n");
            return false;
        }

        int len = reply.readInt32();
        reply.read(value, len);
        ALOGE("readHdcpRX14Key len:%d\n", len);
        return len;
    }

    virtual bool writeHdcpRX14Key(const char *value, const int size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(size);
        data.write((void *)value, size);
        ALOGV("writeHdcp14Key size:%d\n", size);

        if (remote()->transact(WRITE_HDCPRX14_KEY, data, &reply) != NO_ERROR) {
            ALOGE("writeHdcp14Key could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual bool writeHdcpRXImg(const String16& path)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(path);
        ALOGV("writeHdcpImg path:%s\n", String8(path).string());

        if (remote()->transact(WRITE_HDCPRX_IMG, data, &reply) != NO_ERROR) {
            ALOGE("writeHdcpImg could not contact remote\n");
            return false;
        }

        return reply.readInt32() != 0;
    }

    virtual bool getBootEnv(const String16& key, String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(key);
        ALOGV("getBootEnv key:%s\n", String8(key).string());

        if (remote()->transact(GET_BOOT_ENV, data, &reply) != NO_ERROR) {
            ALOGD("getBootEnv could not contact remote\n");
            return false;
        }

        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("getBootEnv caught exception %d\n", err);
            return false;
        }
        value = reply.readString16();
        return true;
    }

    virtual void setBootEnv(const String16& key, const String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(key);
        data.writeString16(value);
        ALOGV("setBootEnv key:%s, value:%s\n", String8(key).string(), String8(value).string());

        if (remote()->transact(SET_BOOT_ENV, data, &reply) != NO_ERROR) {
            ALOGE("setBootEnv could not contact remote\n");
            return;
        }
    }

    virtual void getDroidDisplayInfo(int &type, String16& socType, String16& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip)
    {
        /*Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        ALOGV("getDroidDisplayInfo\n");

        if (remote()->transact(GET_DISPLAY_INFO, data, &reply) != NO_ERROR) {
            ALOGE("getDroidDisplayInfo could not contact remote\n");
            return;
        }

        type = reply.readInt32();
        socType = reply.readString16();
        defaultUI = reply.readString16();
        fb0w = reply.readInt32();
        fb0h = reply.readInt32();
        fb0bits = reply.readInt32();
        fb0trip = reply.readInt32();
        fb1w = reply.readInt32();
        fb1h = reply.readInt32();
        fb1bits = reply.readInt32();
        fb1trip = reply.readInt32();*/
    }

    virtual void loopMountUnmount(int &isMount, String16& path)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(isMount);
        data.writeString16(path);
        ALOGV("loop mount unmount isMount:%d, path:%s\n", isMount, String8(path).string());

        if (remote()->transact(LOOP_MOUNT_UNMOUNT, data, &reply) != NO_ERROR) {
            ALOGE("loopMountUnmount could not contact remote\n");
            return;
        }
    }

    virtual void setMboxOutputMode(const String16& mode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(mode);
        ALOGV("set mbox output mode:%s\n", String8(mode).string());

        if (remote()->transact(MBOX_OUTPUT_MODE, data, &reply) != NO_ERROR) {
            ALOGE("set mbox output mode could not contact remote\n");
            return;
        }
    }

    virtual int32_t set3DMode(const String16& mode3d)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(mode3d);

        if (remote()->transact(SET_3D_MODE, data, &reply) != NO_ERROR) {
            ALOGE("set 3d mode could not contact remote\n");
            return -1;
        }

        return reply.readInt32();
    }

    virtual void init3DSetting(void)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        ALOGV("init3DSetting\n");

        if (remote()->transact(INIT_3D_SETTING, data, &reply) != NO_ERROR) {
            ALOGE("init 3D setting could not contact remote\n");
            return;
        }
    }

    virtual int getVideo3DFormat(void)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());

        if (remote()->transact(GET_VIDEO_3D_FORMAT, data, &reply) != NO_ERROR) {
            ALOGE("get video 3D format could not contact remote\n");
            return -1;
        }

        ALOGV("getVideo3DFormat format:%d\n", reply.readInt32());
        return reply.readInt32();
    }

    virtual int getDisplay3DTo2DFormat(void)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());

        if (remote()->transact(GET_VIDEO_3DTO2D_FORMAT, data, &reply) != NO_ERROR) {
            ALOGE("get display 3D to 2D format could not contact remote\n");
            return -1;
        }

        ALOGV("getDisplay3DTo2DFormat format:%d\n", reply.readInt32());
        return reply.readInt32();
    }

    virtual bool setDisplay3DTo2DFormat(int format)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(format);
        ALOGV("setDisplay3DTo2DFormat format:%d\n", format);

        if (remote()->transact(SET_VIDEO_3DTO2D_FORMAT, data, &reply) != NO_ERROR) {
            ALOGE("set display 3D to 2D format could not contact remote\n");
            return false;
        }

        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("set display 3D to 2D format caught exception\n");
            return false;
        }

        return true;
    }

    virtual bool setDisplay3DFormat(int format)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(format);
        ALOGV("setDisplay3DFormat format:%d\n", format);

        if (remote()->transact(SET_DISPLAY_3D_FORMAT, data, &reply) != NO_ERROR) {
            ALOGE("set display 3D format could not contact remote\n");
            return false;
        }

        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("set display 3D format caught exception\n");
            return false;
        }

        return true;
    }

    virtual int getDisplay3DFormat(void)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());

        if (remote()->transact(GET_DISPLAY_3D_FORMAT, data, &reply) != NO_ERROR) {
            ALOGE("get display 3D format could not contact remote\n");
            return -1;
        }

        ALOGV("getDisplay3DFormat format:%d\n", reply.readInt32());
        return reply.readInt32();
    }

    virtual bool setOsd3DFormat(int format)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(format);
        ALOGV("setOsd3DFormat format:%d\n", format);

        if (remote()->transact(SET_OSD_3D_FORMAT, data, &reply) != NO_ERROR) {
            ALOGE("set OSD 3D format could not contact remote\n");
            return false;
        }

        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("set OSD 3D format caught exception\n");
            return false;
        }

        return true;
    }

    virtual bool switch3DTo2D(int format)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(format);
        ALOGV("switch3DTo2D format:%d\n", format);

        if (remote()->transact(SWITCH_3DTO2D, data, &reply) != NO_ERROR) {
            ALOGE("switch 3D to 2D format could not contact remote\n");
            return false;
        }

        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("switch 3D to 2D format caught exception\n");
            return false;
        }

        return true;
    }

    virtual bool switch2DTo3D(int format)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(format);
        ALOGV("switch2DTo3D format:%d\n", format);

        if (remote()->transact(SWITCH_2DTO3D, data, &reply) != NO_ERROR) {
            ALOGE("switch 2D to 3D format could not contact remote\n");
            return false;
        }

        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("switch 2D to 3D format caught exception\n");
            return false;
        }

        return true;
    }

    virtual void autoDetect3DForMbox()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        ALOGV("autoDetect3DForMbox\n");

        if (remote()->transact(AUTO_DETECT_3D, data, &reply) != NO_ERROR) {
            ALOGE("auto detect 3D could not contact remote\n");
            return;
        }
    }

    virtual void setDigitalMode(const String16& mode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(mode);
        ALOGV("set digital mode:%s\n", String8(mode).string());

        if (remote()->transact(SET_DIGITAL_MODE, data, &reply) != NO_ERROR) {
            ALOGE("set digital mode could not contact remote\n");
            return;
        }
    }

    virtual void setOsdMouseMode(const String16& mode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(mode);
        ALOGV("set osd mouse mode:%s\n", String8(mode).string());

        if (remote()->transact(OSD_MOUSE_MODE, data, &reply) != NO_ERROR) {
            ALOGE("set osd mouse mode could not contact remote\n");
            return;
        }
    }

    virtual void setOsdMousePara(int x, int y, int w, int h)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(x);
        data.writeInt32(y);
        data.writeInt32(w);
        data.writeInt32(h);
        ALOGV("set osd mouse parameter x:%d, y:%d, w:%d, h:%d\n", x, y, w, h);

        if (remote()->transact(OSD_MOUSE_PARA, data, &reply) != NO_ERROR) {
            ALOGE("set osd mouse parameter could not contact remote\n");
            return;
        }
    }

    virtual void setPosition(int left, int top, int width, int height)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(left);
        data.writeInt32(top);
        data.writeInt32(width);
        data.writeInt32(height);
        ALOGV("set position x:%d, y:%d, w:%d, h:%d\n", left, top, width, height);

        if (remote()->transact(SET_POSITION, data, &reply) != NO_ERROR) {
            ALOGE("set position could not contact remote\n");
            return;
        }
    }

    virtual void getPosition(const String16& mode, int &x, int &y, int &w, int &h)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(mode);

        if (remote()->transact(GET_POSITION, data, &reply) != NO_ERROR) {
            ALOGE("get position could not contact remote\n");
            return;
        }

        x = reply.readInt32();
        y = reply.readInt32();
        w = reply.readInt32();
        h = reply.readInt32();
        ALOGV("get position x:%d, y:%d, w:%d, h:%d\n", x, y, w, h);
    }

    virtual void saveDeepColorAttr(const String16& mode, const String16& dcValue)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(mode);
        data.writeString16(dcValue);
        ALOGV("set deep color attr %s\n", String8(dcValue).string());

        if (remote()->transact(SAVE_DEEP_COLOR_ATTR, data, &reply) != NO_ERROR) {
            ALOGE("set deep color attr could not contact remote\n");
            return;
        }
    }

    virtual void getDeepColorAttr(const String16& mode, String16& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(mode);

        if (remote()->transact(GET_DEEP_COLOR_ATTR, data, &reply) != NO_ERROR) {
            ALOGE("get deep color attr could not contact remote\n");
            return;
        }

        value = reply.readString16();
        ALOGV("get deep color attr %s\n", String8(value).string());
    }

    virtual void initDolbyVision(int state) {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(state);
        ALOGV("init dolby vision state:%d\n", state);

        if (remote()->transact(INIT_DOLBY_VISION, data, &reply) != NO_ERROR) {
            ALOGE("init dolby vision could not contact remote\n");
            return;
        }
    }

    virtual void setDolbyVisionEnable(int state) {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeInt32(state);
        ALOGV("set dolby vision state:%d\n", state);

        if (remote()->transact(SET_DOLBY_VISION, data, &reply) != NO_ERROR) {
            ALOGE("set dolby vision could not contact remote\n");
            return;
        }
    }

    virtual int32_t getDolbyVisionType() {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(GET_DOLBY_VISION_TYPE, data, &reply) != NO_ERROR) {
            ALOGE("get dolby vision could not contact remote\n");
            return 0;
        }
        return reply.readInt32();
    }

    virtual bool isTvSupportDolbyVision(String16& mode) {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(TV_SUPPORT_DOLBY_VISION, data, &reply) != NO_ERROR) {
            ALOGE("get tv support dolby vision could not contact remote\n");
            return false;
        }
        mode = reply.readString16();
        return reply.readBool();
    }

    virtual void setGraphicsPriority(const String16& mode) {
        Parcel data, reply;
        data.writeString16(mode);
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(SET_DOLBY_VISION_PRIORITY, data, &reply) != NO_ERROR) {
            ALOGE("set dolby vision graphics priority could not contact remote\n");
            return;
        }
    }
    virtual void getGraphicsPriority(String16& mode) {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(GET_DOLBY_VISION_PRIORITY, data, &reply) != NO_ERROR) {
            ALOGD("getGraphicsPriority could not contact remote\n");
            return;
        }
        mode = reply.readString16();
    }

    virtual int64_t resolveResolutionValue(const String16& mode) {
        Parcel data, reply;
        data.writeString16(mode);
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(RESOLVE_RESOLUTION_VALUE, data, &reply) != NO_ERROR) {
            ALOGE("get resolve resolution value could not contact remote\n");
            return -1;
        }
        return reply.readInt64();
    }

    virtual void setHdrMode(const String16& mode) {
        Parcel data, reply;
        data.writeString16(mode);
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(SET_HDR_MODE, data, &reply) != NO_ERROR) {
            ALOGE("set hdr mode could not contact remote\n");
            return;
        }
    }

    virtual void setSdrMode(const String16& mode) {
        Parcel data, reply;
        data.writeString16(mode);
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(SET_SDR_MODE, data, &reply) != NO_ERROR) {
            ALOGE("set sdr mode could not contact remote\n");
            return;
        }
    }

    virtual void reInit(void) {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());

        if (remote()->transact(REINIT, data, &reply) != NO_ERROR) {
            ALOGE("reInit could not contact remote\n");
            return;
        }
    }

    virtual void instabootResetDisplay(void) {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());

        if (remote()->transact(INSTABOOT_RESET_DISPLAY, data, &reply) != NO_ERROR) {
            ALOGE("instabootResetDisplay could not contact remote\n");
            return;
        }
    }

    virtual void setVideoPlayingAxis(void)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());

        ALOGV("setVideoPlayingAxis\n");

        if (remote()->transact(SET_VIDEO_PLAYING, data, &reply) != NO_ERROR) {
            ALOGE("setVideoPlayingAxis could not contact remote\n");
            return;
        }
    }

    virtual void setListener(const sp<ISystemControlNotify>& listener)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(listener));
        if (remote()->transact(SET_LISTENER, data, &reply) != NO_ERROR) {
            ALOGE("set listener could not contact remote\n");
            return;
        }
    }

    virtual bool getSupportDispModeList(std::vector<std::string> *supportDispModes){
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(GET_SUPPORTED_DISPLAYMODE_LIST, data, &reply) != NO_ERROR) {
            ALOGE("getSupportDispModeList could not contact remote\n");
            return false;
        }
        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("get activeDispMode caught exception\n");
            return false;
        }
       std::vector<String16> edid;
       err = reply.readString16Vector(&edid);
       for (auto it = edid.begin(); it != edid.end(); it ++) {
           (*supportDispModes).push_back(std::string(String8(*it).string()));
       }
       return true;
    }

    virtual bool getActiveDispMode(std::string* activeDispMode){
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        if (remote()->transact(GET_ACTIVE_DISPLAYMODE, data, &reply) != NO_ERROR) {
            ALOGE("get activeDispMode could not contact remote\n");
            return false;
        }
        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("get activeDispMode caught exception\n");
            return false;
        }
        String16 curDisplayMode = reply.readString16();
        *activeDispMode = std::string(String8(curDisplayMode).string());
        ALOGV("get active displaymode:%s\n", curDisplayMode.string());
        return true;
    }

    virtual bool setActiveDispMode(std::string& activeDispMode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(String16(activeDispMode.c_str()));
        if (remote()->transact(SET_ACTIVE_DISPLAYMODE, data, &reply) != NO_ERROR) {
            ALOGE("set active displaymode could not contact remote\n");
            return false;
        }
        int32_t err = reply.readInt32();
        if (err == 0) {
            ALOGE("set activeDispMode caught exception\n");
            return false;
        }
        return true;
    }

    virtual void isHDCPTxAuthSuccess(int &status)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());

        if (remote()->transact(IS_AUTHSUCCESS, data, &reply) != NO_ERROR) {
            ALOGE("get is auth success not contact remote\n");
            return;
        }

        status = reply.readInt32();
        ALOGV("is auth success  status :%d\n", status);
    }

    virtual void getBootanimStatus(int &status)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());

        if (remote()->transact(GET_BOOTANIMSTATUS, data, &reply) != NO_ERROR) {
            ALOGE("get boot anim status not contact remote\n");
            return;
        }

        status = reply.readInt32();
        ALOGV("boot anim status :%d\n", status);
    }

    virtual void setSinkOutputMode(const String16& mode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlService::getInterfaceDescriptor());
        data.writeString16(mode);
        ALOGV("set sink output mode:%s\n", String8(mode).string());

        if (remote()->transact(SINK_OUTPUT_MODE, data, &reply) != NO_ERROR) {
            ALOGE("set sink output mode could not contact remote\n");
            return;
        }
    }
};

IMPLEMENT_META_INTERFACE(SystemControlService, "droidlogic.ISystemControlService");

// ----------------------------------------------------------------------------

status_t BnISystemControlService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case GET_PROPERTY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 value;
            bool result = getProperty(data.readString16(), value);
            reply->writeInt32(result);
            reply->writeString16(value);
            return NO_ERROR;
        }
        case GET_PROPERTY_STRING: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 key = data.readString16();
            String16 def = data.readString16();
            String16 value;
            bool result = getPropertyString(key, value, def);
            reply->writeInt32(result);
            reply->writeString16(value);
            return NO_ERROR;
        }
        case GET_PROPERTY_INT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 key = data.readString16();
            int32_t def = data.readInt32();
            int result = getPropertyInt(key, def);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case GET_PROPERTY_LONG: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 key = data.readString16();
            int64_t def = data.readInt64();

            int64_t result = getPropertyLong(key, def);
            reply->writeInt64(result);
            return NO_ERROR;
        }
        case GET_PROPERTY_BOOL: {
            CHECK_INTERFACE(ISystemControlService, data, reply);

            String16 key = data.readString16();
            int32_t def = data.readInt32();
            bool result = getPropertyBoolean(key, (def!=0));
            reply->writeInt32(result?1:0);
            return NO_ERROR;
        }
        case SET_PROPERTY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 key = data.readString16();
            String16 val = data.readString16();
            setProperty(key, val);
            return NO_ERROR;
        }
        case READ_SYSFS: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 sys = data.readString16();
            String16 value;
            readSysfs(sys, value);
            reply->writeString16(value);
            return NO_ERROR;
        }
        case WRITE_SYSFS: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 sys = data.readString16();
            String16 value = data.readString16();
            bool result = writeSysfs(sys, value);
            reply->writeInt32(result?1:0);
            return NO_ERROR;
        }
        case WRITE_SYSFS_BIN: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 sys = data.readString16();
            int size = data.readInt32();
            void *value = (void *)malloc(size);
            data.read(value, size);
            bool result = writeSysfs(sys, (char *)value, size);
            reply->writeInt32(result?1:0);
            free(value);
            return NO_ERROR;
        }
        case WRITE_UNIFY_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 sys = data.readString16();
            String16 value = data.readString16();
            bool result = writeUnifyKey(sys, value);
            reply->writeInt32(result?1:0);
            return NO_ERROR;
        }
        case READ_UNIFY_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 sys = data.readString16();
            String16 value;
            bool result = readUnifyKey(sys, value);
            reply->writeString16(value);
            reply->writeInt32(result?1:0);
            return NO_ERROR;
        }
        case WRITE_PLAYREADY_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 sys = data.readString16();
            int size = data.readInt32();
            void *value = (void *)malloc(size);
            data.read(value, size);
            bool result = writePlayreadyKey(sys, (char *)value, size);
            reply->writeInt32(result?1:0);
            free(value);
            return NO_ERROR;
        }
        case READ_PLAYREADY_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 sys = data.readString16();
            int size = data.readInt32();
            char *value = (char *)malloc(size);
            int len = readPlayreadyKey(sys, (char *)value, size);
            reply->writeInt32(len);
            reply->write(value, len);
            free(value);
            return NO_ERROR;
        }
        case WRITE_ATTESTATION_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 node = data.readString16();
            String16 name = data.readString16();
            int size = data.readInt32();
            void *value = (void *)malloc(size);
            data.read(value, size);
            bool result = writeAttestationKey(node, name, (char *)value, size);
            reply->writeInt32(result?1:0);
            return NO_ERROR;
        }
        case READ_ATTESTATION_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 node = data.readString16();
            String16 name = data.readString16();
            int size = data.readInt32();
            char *value = (char *)malloc(size);
            int len = readAttestationKey(node, name, (char *)value, size);
            reply->writeInt32(len);
            reply->write(value, len);
            free(value);
            return NO_ERROR;
        }
        case READ_HDCPRX22_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int size = data.readInt32();
            char *value = (char *)malloc(size);
            int len = readHdcpRX22Key((char *)value, size);
            reply->writeInt32(len);
            reply->write(value, len);
            free(value);
            return NO_ERROR;
        }
        case WRITE_HDCPRX22_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int size = data.readInt32();
            void *value = (void *)malloc(size);
            data.read(value, size);
            bool result = writeHdcpRX22Key((char *)value, size);
            reply->writeInt32(result?1:0);
            return NO_ERROR;
        }
        case READ_HDCPRX14_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int size = data.readInt32();
            char *value = (char *)malloc(size);
            int len = readHdcpRX14Key((char *)value, size);
            reply->writeInt32(len);
            reply->write(value, len);
            free(value);
            return NO_ERROR;
        }
        case WRITE_HDCPRX14_KEY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int size = data.readInt32();
            void *value = (void *)malloc(size);
            data.read(value, size);
            bool result = writeHdcpRX14Key((char *)value, size);
            reply->writeInt32(result?1:0);
            return NO_ERROR;
        }
        case WRITE_HDCPRX_IMG: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 sys = data.readString16();
            bool result = writeHdcpRXImg(sys);
            reply->writeInt32(result?1:0);
            return NO_ERROR;
        }
        case GET_BOOT_ENV: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 value;
            bool result = getBootEnv(data.readString16(), value);
            reply->writeInt32(result);
            reply->writeString16(value);
            return NO_ERROR;
        }
        case SET_BOOT_ENV: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 key = data.readString16();
            String16 val = data.readString16();
            setBootEnv(key, val);
            return NO_ERROR;
        }
        case GET_DISPLAY_INFO: {
            String16 socType;
            String16 defaultUI;
            int type, fb0w, fb0h, fb0bits, fb0trip, fb1w, fb1h, fb1bits, fb1trip;

            CHECK_INTERFACE(ISystemControlService, data, reply);
            getDroidDisplayInfo(type, socType, defaultUI, fb0w, fb0h, fb0bits, fb0trip, fb1w, fb1h, fb1bits, fb1trip);

            /*reply->writeInt32(type);
            reply->writeString16(socType);
            reply->writeString16(defaultUI);
            reply->writeInt32(fb0w);
            reply->writeInt32(fb0h);
            reply->writeInt32(fb0bits);
            reply->writeInt32(fb0trip);
            reply->writeInt32(fb1w);
            reply->writeInt32(fb1h);
            reply->writeInt32(fb1bits);
            reply->writeInt32(fb1trip);*/
            return NO_ERROR;
        }
        case LOOP_MOUNT_UNMOUNT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int isMount = data.readInt32();
            String16 path = data.readString16();
            loopMountUnmount(isMount, path);
            return NO_ERROR;
        }
        case MBOX_OUTPUT_MODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            setMboxOutputMode(mode);
            return NO_ERROR;
        }
        case SET_3D_MODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode3d = data.readString16();
            int result = set3DMode(mode3d);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case SET_DIGITAL_MODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            setDigitalMode(mode);
            return NO_ERROR;
        }
        case OSD_MOUSE_MODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            setOsdMouseMode(mode);
            return NO_ERROR;
        }
        case OSD_MOUSE_PARA: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t x = data.readInt32();
            int32_t y = data.readInt32();
            int32_t w = data.readInt32();
            int32_t h = data.readInt32();
            setOsdMousePara(x, y, w, h);
            return NO_ERROR;
        }
        case SET_POSITION: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t x = data.readInt32();
            int32_t y = data.readInt32();
            int32_t w = data.readInt32();
            int32_t h = data.readInt32();
            setPosition(x, y, w, h);
            return NO_ERROR;
        }
        case GET_POSITION: {
            int x, y, w, h;
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            getPosition(mode, x, y, w, h);
            reply->writeInt32(x);
            reply->writeInt32(y);
            reply->writeInt32(w);
            reply->writeInt32(h);
            return NO_ERROR;
        }
        case INIT_DOLBY_VISION: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t state = data.readInt32();
            initDolbyVision(state);
            return NO_ERROR;
        }
        case SET_DOLBY_VISION: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t state = data.readInt32();
            setDolbyVisionEnable(state);
            return NO_ERROR;
        }
        case GET_DOLBY_VISION_TYPE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t value;
            value = getDolbyVisionType();
            reply->writeInt32(value);
            return NO_ERROR;
        }
        case TV_SUPPORT_DOLBY_VISION: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 value;
            bool state = isTvSupportDolbyVision(value);
            reply->writeString16(value);
            reply->writeBool(state);
            return NO_ERROR;
        }
        case REINIT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            reInit();
            return NO_ERROR;
        }
        case INSTABOOT_RESET_DISPLAY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            instabootResetDisplay();
            return NO_ERROR;
        }
        case SET_VIDEO_PLAYING:{
            CHECK_INTERFACE(ISystemControlService, data, reply);
            setVideoPlayingAxis();
            return NO_ERROR;
        }
        case SET_LISTENER: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            sp<ISystemControlNotify> listener = interface_cast<ISystemControlNotify>(data.readStrongBinder());
            setListener(listener);
            return NO_ERROR;
        }

        case INIT_3D_SETTING: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            init3DSetting();
            return NO_ERROR;
        }
        case GET_VIDEO_3D_FORMAT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int result = getVideo3DFormat();
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case GET_VIDEO_3DTO2D_FORMAT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int result = getDisplay3DTo2DFormat();
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case SET_VIDEO_3DTO2D_FORMAT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t format = data.readInt32();
            int result = setDisplay3DTo2DFormat(format);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case SET_DISPLAY_3D_FORMAT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t format = data.readInt32();
            int result = setDisplay3DFormat(format);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case GET_DISPLAY_3D_FORMAT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int result = getDisplay3DFormat();
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case SET_OSD_3D_FORMAT: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t format = data.readInt32();
            int result = setOsd3DFormat(format);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case SWITCH_3DTO2D: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t format = data.readInt32();
            int result = switch3DTo2D(format);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case SWITCH_2DTO3D: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            int32_t format = data.readInt32();
            int result = switch2DTo3D(format);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case AUTO_DETECT_3D: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            autoDetect3DForMbox();
            return NO_ERROR;
        }
        case GET_SUPPORTED_DISPLAYMODE_LIST: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            std::vector<std::string> supportDispModes;
            bool result = getSupportDispModeList(&supportDispModes);
            std::vector<String16> edid;
            for (auto it = supportDispModes.begin(); it != supportDispModes.end(); it ++) {
                edid.push_back(String16((*it).c_str()));
            }
            reply->writeInt32(result);
            reply->writeString16Vector(edid);
            return NO_ERROR;
        }
        case GET_ACTIVE_DISPLAYMODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            std::string value;
            int result = getActiveDispMode(&value);
            reply->writeInt32(result);
            reply->writeString16(String16(value.c_str()));
            return NO_ERROR;
        }
        case SET_ACTIVE_DISPLAYMODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            std::string value = std::string(String8(mode).string());
            int result =setActiveDispMode(value);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case IS_AUTHSUCCESS: {
            int status;
            CHECK_INTERFACE(ISystemControlService, data, reply);
            isHDCPTxAuthSuccess(status);
            reply->writeInt32(status);
            return NO_ERROR;
        }
        case GET_BOOTANIMSTATUS: {
            int status;
            CHECK_INTERFACE(ISystemControlService, data, reply);
            getBootanimStatus(status);
            reply->writeInt32(status);
            return NO_ERROR;
        }
        case SAVE_DEEP_COLOR_ATTR: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            String16 value = data.readString16();
            saveDeepColorAttr(mode, value);
            return NO_ERROR;
        }
        case GET_DEEP_COLOR_ATTR: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            String16 value;
            getDeepColorAttr(mode, value);
            reply->writeString16(value);
            return NO_ERROR;
        }
        case GET_MODE_SUPPORT_DEEPCOLOR: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 str16mode = data.readString16();
            String16 str16value = data.readString16();
            std::string mode = std::string(String8(str16mode).string());
            std::string value = std::string(String8(str16value).string());
            bool result = getModeSupportDeepColorAttr(mode, value);
            reply->writeBool(result);
            return NO_ERROR;
        }
        case RESOLVE_RESOLUTION_VALUE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            int64_t value;
            value = resolveResolutionValue(mode);
            reply->writeInt64(value);
            return NO_ERROR;
        }
        case SET_HDR_MODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            setHdrMode(mode);
            return NO_ERROR;
        }
        case SET_SDR_MODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            setSdrMode(mode);
            return NO_ERROR;
        }
        case SINK_OUTPUT_MODE: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            setSinkOutputMode(mode);
            return NO_ERROR;
        }
        case SET_DOLBY_VISION_PRIORITY: {
            CHECK_INTERFACE(ISystemControlService, data, reply);
            String16 mode = data.readString16();
            setGraphicsPriority(mode);
            return NO_ERROR;
        }
        case GET_DOLBY_VISION_PRIORITY: {
            String16 value;
            getGraphicsPriority(value);
            reply->writeString16(value);
            return NO_ERROR;
        }
        default: {
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
    // should be unreachable
    return NO_ERROR;
}

}; // namespace android
