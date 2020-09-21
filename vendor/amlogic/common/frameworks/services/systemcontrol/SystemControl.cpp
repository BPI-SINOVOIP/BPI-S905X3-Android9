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

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0

#include <fcntl.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
//#include <binder/PermissionCache.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <private/android_filesystem_config.h>
#include <pthread.h>

#include "SystemControl.h"
#include "keymaster_hidl_hal_test.h"

using ::android::hardware::keymaster::V3_0::check_AttestationKey;


namespace android {

SystemControl* SystemControl::instantiate(const char *cfgpath) {
    SystemControl *syscontrol = new SystemControl(cfgpath);

    //in full treble mode, only use hwbinder or vndbinder, can not use binder
    android::status_t ret = defaultServiceManager()->addService(
            String16("system_control"), syscontrol);

    if (ret != android::OK) {
        ALOGE("Couldn't register system control service!");
    }
    ALOGI("instantiate add system_control service result:%d", ret);
    return syscontrol;
}

SystemControl::SystemControl(const char *path)
    : mLogLevel(LOG_LEVEL_DEFAULT) {

    pUbootenv = new Ubootenv();
    pSysWrite = new SysWrite();

    pDisplayMode = new DisplayMode(path, pUbootenv);
    pDisplayMode->init();
    pDimension = new Dimension(pDisplayMode, pSysWrite);

    //if ro.firstboot is true, we should clear first boot flag
    const char* firstBoot = pUbootenv->getValue("ubootenv.var.firstboot");
    if (firstBoot && (strcmp(firstBoot, "1") == 0)) {
        ALOGI("ubootenv.var.firstboot first_boot:%s, clear it to 0", firstBoot);
        if ( pUbootenv->updateValue("ubootenv.var.firstboot", "0") < 0 )
            ALOGE("set firstboot to 0 fail");
    }
}

SystemControl::~SystemControl() {
    delete pUbootenv;
    delete pSysWrite;
    delete pDisplayMode;
    delete pDimension;
}

int SystemControl::permissionCheck() {
    // codes that require permission check
    IPCThreadState* ipc = IPCThreadState::self();
    const int pid = ipc->getCallingPid();
    const int uid = ipc->getCallingUid();

    if ((uid != AID_GRAPHICS) && (uid != AID_MEDIA) &&( uid != AID_MEDIA_CODEC)
           /*&& !PermissionCache::checkPermission(String16("droidlogic.permission.SYSTEM_CONTROL"), pid, uid)*/) {
        ALOGE("Permission Denial: "
                "can't use system control service pid=%d, uid=%d", pid, uid);
        return PERMISSION_DENIED;
    }

    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("system_control service permissionCheck pid=%d, uid=%d", pid, uid);
    }

    return NO_ERROR;
}

//read write property and sysfs
bool SystemControl::getProperty(const String16& key, String16& value) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool ret = pSysWrite->getProperty(String8(key).string(), buf);
    value.setTo(String16(buf));

    return ret;
}

bool SystemControl::getPropertyString(const String16& key, String16& value, String16& def) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool ret = pSysWrite->getPropertyString(String8(key).string(), (char *)buf, String8(def).string());
    value.setTo(String16(buf));
    return ret;
}

int32_t SystemControl::getPropertyInt(const String16& key, int32_t def) {
    return pSysWrite->getPropertyInt(String8(key).string(), def);
}

int64_t SystemControl::getPropertyLong(const String16& key, int64_t def) {
    return pSysWrite->getPropertyLong(String8(key).string(), def);
}

bool SystemControl::getPropertyBoolean(const String16& key, bool def) {
    return pSysWrite->getPropertyBoolean(String8(key).string(), def);
}

void SystemControl::setProperty(const String16& key, const String16& value) {
    if (NO_ERROR == permissionCheck()) {
        pSysWrite->setProperty(String8(key).string(), String8(value).string());
        traceValue(String16("setProperty"), key, value);
    }
}

bool SystemControl::readSysfs(const String16& path, String16& value) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("readSysfs"), path, value);

        char buf[MAX_STR_LEN] = {0};
        bool ret = pSysWrite->readSysfs(String8(path).string(), buf);
        value.setTo(String16(buf));
        return ret;
    }

    return false;
}

bool SystemControl::writeSysfs(const String16& path, const String16& value) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("writeSysfs"), path, value);

        return pSysWrite->writeSysfs(String8(path).string(), String8(value).string());
    }

    return false;
}

bool SystemControl::writeSysfs(const String16& path, const char *value, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("writeSysfs"), path, size);

        bool ret = pSysWrite->writeSysfs(String8(path).string(), value, size);
        return ret;
    }

    return false;
}

bool SystemControl::writeUnifyKey(const String16& path, const String16& value) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("writeUnifyKey"), path, value);

        bool ret = pSysWrite->writeUnifyKey(String8(path).string(), String8(value).string());
        return ret;
    }
    return false;
}

bool SystemControl::readUnifyKey(const String16& path __unused, String16& value __unused) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue(String16("readSysfs"), path, value);

        char buf[MAX_STR_LEN] = {0};
        bool ret = pSysWrite->readUnifyKey(String8(path).string(), buf);
        value.setTo(String16(buf));
        return ret;
    }*/
    return false;
}

bool SystemControl::writePlayreadyKey(const String16& path, const char *buff, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("writePlayreadyKey"), path, size);

        bool ret = pSysWrite->writePlayreadyKey(String8(path).string(), buff, size);
        return ret;
    }
    return false;
}

int32_t SystemControl::readPlayreadyKey(const String16& path __unused, char *value, int size) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue(String16("readUnifyKey"), path, size);

        int len = pSysWrite->readPlayreadyKey(String8(path).string(), value, size);
        return len;
    }*/
    return 0;
}


int32_t SystemControl::readAttestationKey(const String16& node, const String16& name, char *value, int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("readAttestationKey"), node, name);

        int len= pSysWrite->readAttestationKey(String8(node).string(), String8(name).string(), value, size);
        return len;
    }
    return 0;
}

bool SystemControl::checkAttestationKey() {
    if (NO_ERROR == permissionCheck()) {
        ALOGD("SystemControl checkAttestationKey");
        bool ret = check_AttestationKey();
        return ret;
    }
    return false;
}

bool SystemControl::writeAttestationKey(const String16& node, const String16& name, const char *buff, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("writeAttestationKey"), node, name);

        bool ret = pSysWrite->writeAttestationKey(String8(node).string(), String8(name).string(), buff, size);
        return ret;
    }
    return false;
}


int32_t SystemControl::readHdcpRX22Key(char *value __attribute__((unused)), int size __attribute__((unused))) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue(String16("readHdcpRX22Key"), String16(""), size);
        int len = pDisplayMode->readHdcpRX22Key(value, size);
        return len;
    }*/
    return 0;
}

bool SystemControl::writeHdcpRX22Key(const char *value, const int size) {
   if (NO_ERROR == permissionCheck()) {
        traceValue(String16("writeHdcp22Key"), String16(""), size);

        bool ret = pDisplayMode->writeHdcpRX22Key(value, size);
        return ret;
    }
    return false;
}

int32_t SystemControl::readHdcpRX14Key(char *value __attribute__((unused)), int size __attribute__((unused))) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue(String16("readHdcpRX14Key"), String16(""), size);
        int len = pDisplayMode->readHdcpRX14Key(value, size);
        return len;
    }*/
    return 0;
}

bool SystemControl::writeHdcpRX14Key(const char *value, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("writeHdcp14Key"), String16(""), size);

        bool ret = pDisplayMode->writeHdcpRX14Key(value, size);
        return ret;
    }
    return false;
}

bool SystemControl::writeHdcpRXImg(const String16& path) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("writeSysfs"), path, String16(""));

        return pDisplayMode->writeHdcpRXImg(String8(path).string());
    }

    return false;
}


//set or get uboot env
bool SystemControl::getBootEnv(const String16& key, String16& value) {
    const char* p_value = pUbootenv->getValue(String8(key).string());
	if (p_value) {
        value.setTo(String16(p_value));
        return true;
	}
    return false;
}

void SystemControl::setBootEnv(const String16& key, const String16& value) {
    if (NO_ERROR == permissionCheck()) {
        pUbootenv->updateValue(String8(key).string(), String8(value).string());
        traceValue(String16("setBootEnv"), key, value);
    }
}

void SystemControl::getDroidDisplayInfo(int &type __unused, String16& socType __unused, String16& defaultUI __unused,
        int &fb0w __unused, int &fb0h __unused, int &fb0bits __unused, int &fb0trip __unused,
        int &fb1w __unused, int &fb1h __unused, int &fb1bits __unused, int &fb1trip __unused) {
    /*if (NO_ERROR == permissionCheck()) {
        char bufType[MAX_STR_LEN] = {0};
        char bufUI[MAX_STR_LEN] = {0};
        pDisplayMode->getDisplayInfo(type, bufType, bufUI);
        socType.setTo(String16(bufType));
        defaultUI.setTo(String16(bufUI));
        pDisplayMode->getFbInfo(fb0w, fb0h, fb0bits, fb0trip, fb1w, fb1h, fb1bits, fb1trip);
    }*/
}

void SystemControl::loopMountUnmount(int &isMount, String16& path) {
    if (NO_ERROR == permissionCheck()) {
        traceValue(String16("loopMountUnmount"),
            (isMount==1)?String16("mount"):String16("unmount"), path);

        if (isMount == 1) {
            char mountPath[MAX_STR_LEN] = {0};
            const char *pathStr = String8(path).string();
            strncpy(mountPath, pathStr, strlen(pathStr));

            const char *cmd[4] = {"vdc", "loop", "mount", mountPath};
            vdc_loop(4, (char **)cmd);
        } else {
            const char *cmd[3] = {"vdc", "loop", "unmount"};
            vdc_loop(3, (char **)cmd);
        }
    }
}

void SystemControl::setMboxOutputMode(const String16& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set output mode :%s", String8(mode).string());
    }
    pDisplayMode->setSourceOutputMode(String8(mode).string());
}

bool SystemControl::getModeSupportDeepColorAttr(const std::string& mode,const std::string& color) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("get DeepColor mode :%s color :%s", mode.c_str(),color.c_str());
    }
    return pDisplayMode->getModeSupportDeepColorAttr(mode.c_str(),color.c_str());
}

bool SystemControl::getSupportDispModeList(std::vector<std::string> *supportDispModes) {
    const char *delim = "\n";
    char value[MODE_LEN] = {0};
    hdmi_data_t data;

    pDisplayMode->getHdmiData(&data);
    char *ptr = strtok(data.edid, delim);
    while (ptr != NULL) {
        int len = strlen(ptr);
        if (ptr[len - 1] == '*')
            ptr[len - 1] = '\0';

        (*supportDispModes).push_back(std::string(ptr));
        ptr = strtok(NULL, delim);
    }

    return true;
}

bool SystemControl::getActiveDispMode(std::string *activeDispMode) {
      char mode[MODE_LEN] = {0};
      bool ret = pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, mode);
      *activeDispMode = mode;
      return ret;
}

bool SystemControl::setActiveDispMode(std::string& activeDispMode) {
       setMboxOutputMode(String16(activeDispMode.c_str()));
       return true;
}
int32_t SystemControl::set3DMode(const String16& mode3d) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set 3d mode :%s", String8(mode3d).string());
    }

    return pDimension->set3DMode(String8(mode3d).string());
}

void SystemControl::init3DSetting(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("init3DSetting\n");
    }

    pDimension->init3DSetting();
}

int32_t SystemControl::getVideo3DFormat(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("getVideo3DFormat\n");
    }

    return pDimension->getVideo3DFormat();
}

int32_t SystemControl::getDisplay3DTo2DFormat(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("getDisplay3DTo2DFormat\n");
    }

    return pDimension->getDisplay3DTo2DFormat();
}

bool SystemControl::setDisplay3DTo2DFormat(int format) {
    bool ret = false;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DTo2DFormat format:%d\n", format);
    }

    return pDimension->setDisplay3DTo2DFormat(format);
}

bool SystemControl::setDisplay3DFormat(int format) {
    String16 di3DformatStr;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DFormat format:%d\n", format);
    }

    return pDimension->setDisplay3DFormat(format);
}

int32_t SystemControl::getDisplay3DFormat(void) {
    return pDimension->getDisplay3DFormat();
}

bool SystemControl::setOsd3DFormat(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setOsd3DFormat format:%d\n", format);
    }

    return pDimension->setOsd3DFormat(format);
}

bool SystemControl::switch3DTo2D(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("switch3DTo2D format:%d\n", format);
    }

    return pDimension->switch3DTo2D(format);
}

bool SystemControl::switch2DTo3D(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("switch2DTo3D format:%d\n", format);
    }

    return pDimension->switch2DTo3D(format);
}

void SystemControl::autoDetect3DForMbox() {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("autoDetect3DForMbox\n");
    }

    pDimension->autoDetect3DForMbox();
}

void SystemControl::setDigitalMode(const String16& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set Digital mode :%s", String8(mode).string());
    }

    pDisplayMode->setDigitalMode(String8(mode).string());
}

void SystemControl::setListener(const sp<ISystemControlNotify>& listener) {
    //pDisplayMode->setListener(listener);
}

void SystemControl::setOsdMouseMode(const String16& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set osd mouse mode :%s", String8(mode).string());
    }

}

void SystemControl::setOsdMousePara(int x, int y, int w, int h) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set osd mouse parameter x:%d y:%d w:%d h:%d", x, y, w, h);
    }
}

void SystemControl::setPosition(int left, int top, int width, int height) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set position x:%d y:%d w:%d h:%d", left, top, width, height);
    }
    pDisplayMode->setPosition(left, top, width, height);
}

void SystemControl::getPosition(const String16& mode, int &x, int &y, int &w, int &h) {
    int position[4] = { 0, 0, 0, 0 };
    pDisplayMode->getPosition(String8(mode).string(), position);
    x = position[0];
    y = position[1];
    w = position[2];
    h = position[3];
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("get position x:%d y:%d w:%d h:%d", x, y, w, h);
    }
}
void SystemControl::initDolbyVision(int state) {
    if (state == 0) {
        pDisplayMode->initDolbyVision(OUPUT_MODE_STATE_INIT);
    }else {
        pDisplayMode->initDolbyVision(OUPUT_MODE_STATE_SWITCH);
    }
}

void SystemControl::setDolbyVisionEnable(int state) {
    pDisplayMode->setDolbyVisionEnable(state, OUPUT_MODE_STATE_SWITCH);
}

int32_t SystemControl::getDolbyVisionType() {
    return pDisplayMode->getDolbyVisionType();
}

bool SystemControl::isTvSupportDolbyVision(String16& mode) {
    char value[MAX_STR_LEN] = {0};
    bool ret = pDisplayMode->isTvSupportDolbyVision(value);
    mode.setTo(String16(value));
    return ret;
}

void SystemControl::setGraphicsPriority(const String16& mode) {
    pDisplayMode->setGraphicsPriority(String8(mode).string());
}

void SystemControl::getGraphicsPriority(String16& mode) {
    char value[MODE_LEN] = {0};
    pDisplayMode->getGraphicsPriority(value);
    mode.setTo(String16(value));
}

void SystemControl::isHDCPTxAuthSuccess(int &status) {
    int value=0;
    pDisplayMode->isHDCPTxAuthSuccess(&value);
    status = value;
}

void SystemControl::getBootanimStatus(int &status) {
    int value=0;
    pDisplayMode->getBootanimStatus(&value);
    status = value;
}

void SystemControl::setSinkOutputMode(const String16& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set sink output mode :%s", String8(mode).string());
    }

    pDisplayMode->setSinkOutputMode(String8(mode).string());
}

int64_t SystemControl::resolveResolutionValue(const String16& mode) {
    int64_t value = pDisplayMode->resolveResolutionValue(String8(mode).string());
    return value;
}

void SystemControl::saveDeepColorAttr(const String16& mode, const String16& dcValue) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set deep color attr %s\n", String8(dcValue).string());
    }
    char outputmode[64];
    char value[64];
    strcpy(outputmode, String8(mode).string());
    strcpy(value, String8(dcValue).string());
    pDisplayMode->saveDeepColorAttr(outputmode, value);
}

void SystemControl::getDeepColorAttr(const String16& mode, String16& value) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    pDisplayMode->getDeepColorAttr(String8(mode).string(), buf);
    value.setTo(String16(buf));
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("get deep color attr mode %s, value %s ", String8(mode).string(), String8(value).string());
    }
}

void SystemControl::setHdrMode(const String16& mode) {
    pDisplayMode->setHdrMode(String8(mode).string());
}

void SystemControl::setSdrMode(const String16& mode) {
    pDisplayMode->setSdrMode(String8(mode).string());
}

void SystemControl::reInit() {
    pUbootenv->reInit();
}

void SystemControl::instabootResetDisplay() {
    pDisplayMode->reInit();
}

void SystemControl::setVideoPlayingAxis() {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set video playing axis");
    }
    pDisplayMode->setVideoPlayingAxis();
}

void SystemControl::traceValue(const String16& type, const String16& key, const String16& value) {
    if (mLogLevel > LOG_LEVEL_1) {
        String16 procName;
        int pid = IPCThreadState::self()->getCallingPid();
        int uid = IPCThreadState::self()->getCallingUid();

        getProcName(pid, procName);

        ALOGI("%s [ %s ] [ %s ] from pid=%d, uid=%d, process name=%s",
            String8(type).string(), String8(key).string(), String8(value).string(),
            pid, uid,
            String8(procName).string());
    }
}

void SystemControl::traceValue(const String16& type, const String16& key, const int size) {
    if (mLogLevel > LOG_LEVEL_1) {
        String16 procName;
        int pid = IPCThreadState::self()->getCallingPid();
        int uid = IPCThreadState::self()->getCallingUid();

        getProcName(pid, procName);

        ALOGI("%s [ %s ] [ %d ] from pid=%d, uid=%d, process name=%s",
           String8(type).string(), String8(key).string(), size,
           pid, uid,
           String8(procName).string());
    }
}

void SystemControl::setLogLevel(int level) {
    if (level > (LOG_LEVEL_TOTAL - 1)) {
        ALOGE("out of range level=%d, max=%d", level, LOG_LEVEL_TOTAL);
        return;
    }

    mLogLevel = level;
    pSysWrite->setLogLevel(level);
    pDisplayMode->setLogLevel(level);
    pDimension->setLogLevel(level);
}

int SystemControl::getLogLevel() {
    return mLogLevel;
}

int SystemControl::getProcName(pid_t pid, String16& procName) {
    char proc_path[MAX_STR_LEN];
    char cmdline[64];
    int fd;

    strcpy(cmdline, "unknown");

    sprintf(proc_path, "/proc/%d/cmdline", pid);
    fd = open(proc_path, O_RDONLY);
    if (fd >= 0) {
        int rc = read(fd, cmdline, sizeof(cmdline)-1);
        cmdline[rc] = 0;
        close(fd);

        procName.setTo(String16(cmdline));
        return 0;
    }

    return -1;
}

status_t SystemControl::dump(int fd, const Vector<String16>& args) {
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    if (false/*checkCallingPermission(String16("android.permission.DUMP")) == false*/) {
        snprintf(buffer, SIZE, "Permission Denial: "
                "can't dump system_control from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        Mutex::Autolock lock(mLock);

        int len = args.size();
        for (int i = 0; i < len; i ++) {
            String16 debugLevel("-l");
            String16 bootenv("-b");
            String16 display("-d");
            String16 dimension("-dms");
            String16 hdcp("-hdcp");
            String16 help("-h");
            if (args[i] == debugLevel) {
                if (i + 1 < len) {
                    String8 levelStr(args[i+1]);
                    int level = atoi(levelStr.string());
                    setLogLevel(level);

                    result.append(String8::format("Setting log level to %d.\n", level));
                    break;
                }
            }
            else if (args[i] == bootenv) {
                if((i + 3 < len) && (args[i + 1] == String16("set"))){
                    setBootEnv(args[i+2], args[i+3]);

                    result.append(String8::format("set bootenv key:[%s] value:[%s]\n",
                        String8(args[i+2]).string(), String8(args[i+3]).string()));
                    break;
                }
                else if (((i + 2) <= len) && (args[i + 1] == String16("get"))) {
                    if ((i + 2) == len) {
                        result.appendFormat("get all bootenv\n");
                        pUbootenv->printValues();
                    }
                    else {
                        String16 value;
                        getBootEnv(args[i+2], value);

                        result.append(String8::format("get bootenv key:[%s] value:[%s]\n",
                            String8(args[i+2]).string(), String8(value).string()));
                    }
                    break;
                }
                else {
                    result.appendFormat(
                        "dump bootenv format error!! should use:\n"
                        "dumpsys system_control -b [set |get] key value \n");
                }
            }
            else if (args[i] == display) {
                /*
                String8 displayInfo;
                pDisplayMode->dump(displayInfo);
                result.append(displayInfo);*/

                char buf[4096] = {0};
                pDisplayMode->dump(buf);
                result.append(String8(buf));
                break;
            }
            else if (args[i] == dimension) {
                char buf[4096] = {0};
                pDimension->dump(buf);
                result.append(String8(buf));
                break;
            }
            else if (args[i] == hdcp) {
                pDisplayMode->hdcpSwitch();
                break;
            }
            else if (args[i] == help) {
                result.appendFormat(
                    "system_control service use to control the system sysfs property and boot env \n"
                    "in multi-user mode, normal process will have not system privilege \n"
                    "usage: \n"
                    "dumpsys system_control -l value \n"
                    "dumpsys system_control -b [set |get] key value \n"
                    "-l: debug level \n"
                    "-b: set or get bootenv \n"
                    "-d: dump display mode info \n"
                    "-hdcp: stop hdcp and start hdcp tx \n"
                    "-h: help \n");
            }
        }
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

} // namespace android

