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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2014/10/22
 *  @par function description:
 *  - 1 control system sysfs proc env & property
 */

#ifndef ANDROID_SYSTEM_CONTROL_H
#define ANDROID_SYSTEM_CONTROL_H

#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/Mutex.h>
#include <ISystemControlService.h>

#include "SysWrite.h"
#include "common.h"
#include "DisplayMode.h"
#include "Dimension.h"
#include <string>
#include <vector>
#include "CPQControl.h"

#include "ubootenv/Ubootenv.h"

extern "C" int vdc_loop(int argc, char **argv);

namespace android {
// ----------------------------------------------------------------------------

class SystemControl : public BnISystemControlService
{
public:
    SystemControl(const char *path);
    virtual ~SystemControl();

    virtual bool getProperty(const String16& key, String16& value);
    virtual bool getPropertyString(const String16& key, String16& value, String16& def);
    virtual int32_t getPropertyInt(const String16& key, int32_t def);
    virtual int64_t getPropertyLong(const String16& key, int64_t def);

    virtual bool getPropertyBoolean(const String16& key, bool def);
    virtual void setProperty(const String16& key, const String16& value);

    virtual bool readSysfs(const String16& path, String16& value);
    virtual bool writeSysfs(const String16& path, const String16& value);
    virtual bool writeSysfs(const String16& path, const char *value, const int size);

    virtual int32_t readHdcpRX22Key(char *value, int size);
    virtual bool writeHdcpRX22Key(const char *value, const int size);
    virtual int32_t readHdcpRX14Key(char *value, int size);
    virtual bool writeHdcpRX14Key(const char *value, const int size);
    virtual bool writeHdcpRXImg(const String16& path);
    virtual bool writeUnifyKey(const String16& path, const String16& value);
    virtual bool readUnifyKey(const String16& path, String16& value);
    virtual bool writePlayreadyKey(const String16& path, const char *buff, const int size);
    virtual int32_t readPlayreadyKey(const String16& path, char *value, int size);

    virtual bool checkAttestationKey();
    virtual int32_t readAttestationKey(const String16& node, const String16& name, char *value, int size);
    virtual bool writeAttestationKey(const String16& node, const String16& name, const char *buff, const int size);

    virtual void setBootEnv(const String16& key, const String16& value);
    virtual bool getBootEnv(const String16& key, String16& value);

    virtual void getDroidDisplayInfo(int &type, String16& socType, String16& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip);

    virtual void loopMountUnmount(int &isMount, String16& path);

    virtual void setMboxOutputMode(const String16& mode);
    virtual int32_t set3DMode(const String16& mode3d);
    virtual void setDigitalMode(const String16& mode);
    virtual void setListener(const sp<ISystemControlNotify>& listener);
    virtual void setOsdMouseMode(const String16& mode);
    virtual void setOsdMousePara(int x, int y, int w, int h);
    virtual void setPosition(int left, int top, int width, int height);
    virtual void getPosition(const String16& mode, int &x, int &y, int &w, int &h);
    virtual void getDeepColorAttr(const String16& mode, String16& value);
    virtual void saveDeepColorAttr(const String16& mode, const String16& dcValue);
    virtual int64_t resolveResolutionValue(const String16& mode);
    virtual void initDolbyVision(int state);
    virtual void setDolbyVisionEnable(int state);
    virtual bool isTvSupportDolbyVision(String16& mode);
    virtual int32_t getDolbyVisionType();
    virtual void setGraphicsPriority(const String16& mode);
    virtual void getGraphicsPriority(String16& mode);
    virtual void setHdrMode(const String16& mode);
    virtual void setSdrMode(const String16& mode);
    virtual void reInit();
    virtual void instabootResetDisplay(void);
    virtual void setVideoPlayingAxis();
    virtual void init3DSetting(void);
    virtual int32_t getVideo3DFormat(void);
    virtual int32_t getDisplay3DTo2DFormat(void);
    virtual bool setDisplay3DTo2DFormat(int format);
    virtual bool setDisplay3DFormat(int format);
    virtual int32_t getDisplay3DFormat(void);
    virtual bool setOsd3DFormat(int format);
    virtual bool switch3DTo2D(int format);
    virtual bool switch2DTo3D(int format);
    virtual void autoDetect3DForMbox();
    virtual bool getSupportDispModeList(std::vector<std::string> *supportDispModes) ;
    virtual bool getActiveDispMode(std::string *activeDispMode) ;
    virtual bool setActiveDispMode(std::string& activeDispMode) ;
    virtual void isHDCPTxAuthSuccess(int &status);
    virtual void setSinkOutputMode(const String16& mode);

    static SystemControl* instantiate(const char *cfgpath);

    virtual status_t dump(int fd, const Vector<String16>& args);
    virtual bool getModeSupportDeepColorAttr(const std::string& mode,const std::string& color);

    int getLogLevel();
    virtual void getBootanimStatus(int &status);
private:
    int permissionCheck();
    void setLogLevel(int level);
    void traceValue(const String16& type, const String16& key, const String16& value);
    void traceValue(const String16& type, const String16& key, const int size);
    int getProcName(pid_t pid, String16& procName);
    tvin_sig_fmt_t getVideoResolutionToFmt();

    mutable Mutex mLock;

    int mLogLevel;

    SysWrite *pSysWrite;
    DisplayMode *pDisplayMode;
    Dimension *pDimension;
    Ubootenv *pUbootenv;
};

// ----------------------------------------------------------------------------

} // namespace android
#endif // ANDROID_SYSTEM_CONTROL_H
