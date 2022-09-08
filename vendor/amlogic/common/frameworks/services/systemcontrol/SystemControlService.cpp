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
 *  @date     2017/09/20
 *  @par function description:
 *  - 1 system control interface
 */

#define LOG_TAG "SystemControlService"
#define LOG_NDEBUG 0

#include <dlfcn.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <private/android_filesystem_config.h>
#include <pthread.h>

#include "SystemControlService.h"
#include "keymaster_hidl_hal_test.h"

using android::hardware::keymaster::V3_0::check_AttestationKey;

#define VIDEO_RGB_SCREEN    "/sys/class/video/rgb_screen"
#define SSC_PATH            "/sys/class/lcd/ss"
#define TEST_SCREEN         "/sys/class/video/test_screen"

namespace android {

int (*func_optimization)(const char *pkg, const char *cls, const std::vector<std::string>& procList);

SystemControlService* SystemControlService::instantiate(const char *cfgpath) {
    SystemControlService *sysControlIntf = new SystemControlService(cfgpath);
    return sysControlIntf;
}

SystemControlService::SystemControlService(const char *path)
    : mLogLevel(LOG_LEVEL_DEFAULT) {

    ALOGI("SystemControlService instantiate begin");

    pUbootenv = new Ubootenv();
    pSysWrite = new SysWrite();

    pDisplayMode = new DisplayMode(path, pUbootenv);
    pDisplayMode->init();

    //Load config file for tvhal
    mTvhalConfigFile = CConfigFile::GetInstance();
    mTvhalConfigFile->LoadFromFile(PQ_CONFIG_DEFAULT_PATH);
    const char *config_value;
    config_value = mTvhalConfigFile->GetString(CFG_SECTION_PQ, CFG_TVHAL_ENABLE, "disable");
    if (strcmp(config_value, "enable") == 0) {
        pCPQControl = NULL;
    } else {
    //load PQ
        pCPQControl = CPQControl::GetInstance();
        pCPQControl->CPQControlInit();
    }

    pDimension = new Dimension(pDisplayMode, pSysWrite);
    //if ro.firstboot is true, we should clear first boot flag
    const char* firstBoot = pUbootenv->getValue("ubootenv.var.firstboot");
    if (firstBoot && (strcmp(firstBoot, "1") == 0)) {
        ALOGI("ubootenv.var.firstboot first_boot:%s, clear it to 0", firstBoot);
        if ( pUbootenv->updateValue("ubootenv.var.firstboot", "0") < 0 )
            ALOGE("set firstboot to 0 fail");
    }

    void* handle = dlopen("liboptimization.so", RTLD_NOW);
    if (NULL != handle) {
        void (*func_init)();
        func_init = (void (*)())dlsym(handle, "_ZN7android10initConfigEv");
        if (NULL != func_init)
            func_init();
        else
            ALOGE("has not find initConfig from liboptimization.so , %s", dlerror());

        func_optimization = (int (*)(const char *, const char *, const std::vector<std::string>&))dlsym(handle, "_ZN7android15appOptimizationEPKcS1_RKNSt3__16vectorINS2_12basic_stringIcNS2_11char_traitsIcEENS2_9allocatorIcEEEENS7_IS9_EEEE");
        if (NULL == func_optimization)
            ALOGE("has not find appOptimization from liboptimization.so , %s", dlerror());
    }
    else
        ALOGE("open liboptimization.so fail%s", dlerror());

    ALOGI("SystemControlService instantiate done");
}

SystemControlService::~SystemControlService() {
    delete pUbootenv;
    delete pSysWrite;
    delete pDisplayMode;
    delete pDimension;

    if (pCPQControl != NULL) {
        pCPQControl->CPQControlUnInit();
        delete pCPQControl;
        pCPQControl = NULL;
    }
}

int SystemControlService::permissionCheck() {
    /*
    // codes that require permission check
    IPCThreadState* ipc = IPCThreadState::self();
    const int pid = ipc->getCallingPid();
    const int uid = ipc->getCallingUid();

    if ((uid != AID_GRAPHICS) && (uid != AID_MEDIA) &&( uid != AID_MEDIA_CODEC) &&
            !PermissionCache::checkPermission(String16("droidlogic.permission.SYSTEM_CONTROL"), pid, uid)) {
        ALOGE("Permission Denial: "
                "can't use system control service pid=%d, uid=%d", pid, uid);
        return PERMISSION_DENIED;
    }

    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("system_control service permissionCheck pid=%d, uid=%d", pid, uid);
    }
    */

    return NO_ERROR;
}

bool SystemControlService::getSupportDispModeList(std::vector<std::string> *supportDispModes) {
    const char *delim = "\n";
    char value[MODE_LEN] = {0};
    hdmi_data_t data;

    pDisplayMode->getHdmiData(&data);
    char *ptr = strtok(data.disp_cap, delim);
    while (ptr != NULL) {
        int len = strlen(ptr);
        if (ptr[len - 1] == '*')
            ptr[len - 1] = '\0';

        (*supportDispModes).push_back(std::string(ptr));
        ptr = strtok(NULL, delim);
    }

    return true;
}

bool SystemControlService::getActiveDispMode(std::string *activeDispMode) {
    char mode[MODE_LEN] = {0};
    bool ret = pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, mode);
    *activeDispMode = mode;
    return ret;
}

bool SystemControlService::setActiveDispMode(std::string& activeDispMode) {
    setSourceOutputMode(activeDispMode.c_str());
    return true;
}

//read write property and sysfs
bool SystemControlService::getProperty(const std::string &key, std::string *value) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool ret = pSysWrite->getProperty(key.c_str(), buf);
    *value = buf;
    return ret;
}

bool SystemControlService::getPropertyString(const std::string &key, std::string *value, const std::string &def) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool ret = pSysWrite->getPropertyString(key.c_str(), (char *)buf, def.c_str());
    *value = buf;
    return ret;
}

int32_t SystemControlService::getPropertyInt(const std::string &key, int32_t def) {
    return pSysWrite->getPropertyInt(key.c_str(), def);
}

int64_t SystemControlService::getPropertyLong(const std::string &key, int64_t def) {
    return pSysWrite->getPropertyLong(key.c_str(), def);
}

bool SystemControlService::getPropertyBoolean(const std::string& key, bool def) {
    return pSysWrite->getPropertyBoolean(key.c_str(), def);
}

void SystemControlService::setProperty(const std::string& key, const std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        pSysWrite->setProperty(key.c_str(), value.c_str());
        traceValue("setProperty", key, value);
    }
}

bool SystemControlService::readSysfs(const std::string& path, std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        char buf[MAX_STR_LEN] = {0};
        bool ret = pSysWrite->readSysfs(path.c_str(), buf);
        value = buf;

        traceValue("readSysfs", path, value);
        return ret;
    }

    return false;
}

bool SystemControlService::writeSysfs(const std::string& path, const std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeSysfs", path, value);

        return pSysWrite->writeSysfs(path.c_str(), value.c_str());
    }

    return false;
}

bool SystemControlService::writeSysfs(const std::string& path, const char *value, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeSysfs", path, size);

        bool ret = pSysWrite->writeSysfs(path.c_str(), value, size);
        return ret;
    }

    return false;
}

bool SystemControlService::writeUnifyKey(const std::string& key, const std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeUnifyKey", key, value);

        return pSysWrite->writeUnifyKey(key.c_str(), value.c_str());
    }

    return false;
}

bool SystemControlService::readUnifyKey(const std::string& key, std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        char buf[MAX_STR_LEN] = {0};
        bool ret = pSysWrite->readUnifyKey(key.c_str(), buf);
        value = buf;

        traceValue("readUnifyKey", key, value);
        return ret;
    }

    return false;
}

bool SystemControlService::writePlayreadyKey(const std::string& key, const char *value, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writePlayreadyKey", key, size);

        return pSysWrite->writePlayreadyKey(key.c_str(), value, size);
    }

    return false;
}

int32_t SystemControlService::readPlayreadyKey(const std::string& key, char *value, int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("readPlayreadyKey", key, size);
        int i;
        int len = pSysWrite->readPlayreadyKey(key.c_str(), value, size);

        return len;
    }

    return 0;
}


int32_t SystemControlService::readAttestationKey(const std::string& node, const std::string& name, char *value, int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("readAttestationKey", node, name);
        int i;
        int len= pSysWrite->readAttestationKey(node.c_str(), name.c_str(), value, size);

        ALOGE("come to SystemControlService::writeAttestationKey  size: %d  len = %d \n", size, len);
        for (i = 0; i < 32; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, value[i], value[i], value[i]);
        }

        for (i = 8960; i < 8992; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, value[i], value[i], value[i]);
        }

        return len;
    }
    return 0;
}

bool SystemControlService::writeAttestationKey(const std::string& node, const std::string& name, const char *buff, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeAttestationKey", node, name);
        int i;
        ALOGE("come to SystemControlService::writeAttestationKey  size: %d\n", size);
        for (i = 0; i < 32; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, buff[i], buff[i], buff[i]);
        }

        for (i = 8960; i < 8992; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, buff[i], buff[i], buff[i]);
        }

        for (i = 10000; i < 10016; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, buff[i], buff[i], buff[i]);
        }

        bool ret = pSysWrite->writeAttestationKey(node.c_str(), name.c_str(), buff, size);
        return ret;
    }
    return false;
}

bool SystemControlService::checkAttestationKey() {
    if (NO_ERROR == permissionCheck()) {
        ALOGD("SystemControlService checkAttestationKey \n");
        bool ret = check_AttestationKey();
        return ret;
    }
    return false;
}

int32_t SystemControlService::readHdcpRX22Key(char *value __attribute__((unused)), int size __attribute__((unused))) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue("readHdcpRX14Key", "", size);
        int len = pDisplayMode->readHdcpRX22Key(value, size);
        return len;
    }*/
    return 0;
}

bool SystemControlService::writeHdcpRX22Key(const char *value, const int size) {
   if (NO_ERROR == permissionCheck()) {
        traceValue("writeHdcp22Key", "", size);

        bool ret = pDisplayMode->writeHdcpRX22Key(value, size);
        return ret;
    }
    return false;
}

int32_t SystemControlService::readHdcpRX14Key(char *value __attribute__((unused)), int size __attribute__((unused))) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue("readHdcpRX14Key", "", size);
        int len = pDisplayMode->readHdcpRX14Key(value, size);
        return len;
    }*/
    return 0;
}

bool SystemControlService::writeHdcpRX14Key(const char *value, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeHdcp14Key", "", size);

        bool ret = pDisplayMode->writeHdcpRX14Key(value, size);
        return ret;
    }
    return false;
}

bool SystemControlService::writeHdcpRXImg(const std::string& path) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeSysfs", path, "");

        return pDisplayMode->writeHdcpRXImg(path.c_str());
    }

    return false;
}

//set or get uboot env
bool SystemControlService::getBootEnv(const std::string& key, std::string& value) {
    const char* p_value = pUbootenv->getValue(key.c_str());
        if (p_value) {
        value = p_value;
        return true;
        }
    return false;
}

void SystemControlService::setBootEnv(const std::string& key, const std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        pUbootenv->updateValue(key.c_str(), value.c_str());
        traceValue("setBootEnv", key, value);
    }
}

void SystemControlService::setHdrStrategy(const std::string& value) {
    bool ret;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDolbyvisionType type :%s",value.c_str());
    }
    pDisplayMode->setHdrStrategy(value.c_str());
}

void SystemControlService::getDroidDisplayInfo(int &type __unused, std::string& socType __unused, std::string& defaultUI __unused,
        int &fb0w __unused, int &fb0h __unused, int &fb0bits __unused, int &fb0trip __unused,
        int &fb1w __unused, int &fb1h __unused, int &fb1bits __unused, int &fb1trip __unused) {
    if (NO_ERROR == permissionCheck()) {
       /* char bufType[MAX_STR_LEN] = {0};
        char bufUI[MAX_STR_LEN] = {0};
        pDisplayMode->getDisplayInfo(type, bufType, bufUI);
        socType = bufType;
        defaultUI = bufUI;
        pDisplayMode->getFbInfo(fb0w, fb0h, fb0bits, fb0trip, fb1w, fb1h, fb1bits, fb1trip);*/
    }
}

void SystemControlService::DroidVoldDeathRecipient::serviceDied(uint64_t cookie,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
    //LOG(ERROR) << "DroidVold service died. need release some resources";
}

void SystemControlService::loopMountUnmount(int isMount, const std::string& path) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("loopMountUnmount", (isMount==1)?"mount":"unmount", path);

        /*if (isMount == 1) {
            char mountPath[MAX_STR_LEN] = {0};

            strncpy(mountPath, path.c_str(), path.size());

            const char *cmd[4] = {"vdc", "loop", "mount", mountPath};
            vdc_loop(4, (char **)cmd);
        } else {
            const char *cmd[3] = {"vdc", "loop", "unmount"};
            vdc_loop(3, (char **)cmd);
        }*/

        mDroidVold = IDroidVold::getService();
        if (mDroidVold != nullptr) {
            mDeathRecipient = new DroidVoldDeathRecipient();
            Return<bool> linked = mDroidVold->linkToDeath(mDeathRecipient, 0);
            if (!linked.isOk()) {
                //LOG(ERROR) << "Transaction error in linking to system service death: " << linked.description().c_str();
            } else if (!linked) {
                //LOG(ERROR) << "Unable to link to system service death notifications";
            } else {
                //LOG(INFO) << "Link to system service death notification successful";
            }

            if (isMount == 1) {
                mDroidVold->mount(path, 0xF, 0);
            }
            else {
                mDroidVold->unmount(path);
            }
        }
    }
}

bool SystemControlService::getModeSupportDeepColorAttr(const std::string& mode,const std::string& color) {
    bool ret;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("get DeepColor mode :%s color :%s", mode.c_str(),color.c_str());
    }
    ret = pDisplayMode->getModeSupportDeepColorAttr(mode.c_str(),color.c_str());
    return ret;
}

void SystemControlService::setSourceOutputMode(const std::string& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set output mode :%s", mode.c_str());
    }
    pDisplayMode->setSourceOutputMode(mode.c_str());
}

void SystemControlService::setSinkOutputMode(const std::string& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set sink output mode :%s", mode.c_str());
    }

    pDisplayMode->setSinkOutputMode(mode.c_str());
}

void SystemControlService::setDigitalMode(const std::string& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set Digital mode :%s", mode.c_str());
    }

    pDisplayMode->setDigitalMode(mode.c_str());
}

void SystemControlService::setOsdMouseMode(const std::string& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set osd mouse mode :%s", mode.c_str());
    }

}

void SystemControlService::setOsdMousePara(int x, int y, int w, int h) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set osd mouse parameter x:%d y:%d w:%d h:%d", x, y, w, h);
    }
}

void SystemControlService::setPosition(int left, int top, int width, int height) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set position x:%d y:%d w:%d h:%d", left, top, width, height);
    }
    pDisplayMode->setPosition(left, top, width, height);
}

void SystemControlService::getPosition(const std::string& mode, int &x, int &y, int &w, int &h) {
    int position[4] = { 0, 0, 0, 0 };
    pDisplayMode->getPosition(mode.c_str(), position);
    x = position[0];
    y = position[1];
    w = position[2];
    h = position[3];
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("get position x:%d y:%d w:%d h:%d", x, y, w, h);
    }
}

void SystemControlService::initDolbyVision(int state) {
    if (state == 0) {
    pDisplayMode->initDolbyVision(OUPUT_MODE_STATE_INIT);
    } else {
    pDisplayMode->initDolbyVision(OUPUT_MODE_STATE_SWITCH);
    }
}

void SystemControlService::setDolbyVisionEnable(int state) {
    pDisplayMode->setDolbyVisionEnable(state, OUPUT_MODE_STATE_SWITCH);
}

bool SystemControlService::isTvSupportDolbyVision(std::string& mode) {
    char value[MAX_STR_LEN] = {0};
    bool ret = pDisplayMode->isTvSupportDolbyVision(value);
    mode = value;
    return ret;
}

int32_t SystemControlService::getDolbyVisionType() {
    return pDisplayMode->getDolbyVisionType();
}

void SystemControlService::setGraphicsPriority(const std::string& mode) {
    char value[MODE_LEN];
    strcpy(value, mode.c_str());
    pDisplayMode->setGraphicsPriority(value);
}

void SystemControlService::getGraphicsPriority(std::string& mode) {
    char value[MODE_LEN] = {0};
    pDisplayMode->getGraphicsPriority(value);
    mode = value;
}

void SystemControlService::isHDCPTxAuthSuccess(int &status) {
    int value=0;
    pDisplayMode->isHDCPTxAuthSuccess(&value);
    status = value;
}

void SystemControlService::saveDeepColorAttr(const std::string& mode, const std::string& dcValue) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set deep color attr %s\n", dcValue.c_str());
    }
    char outputmode[64];
    char value[64];
    strcpy(outputmode, mode.c_str());
    strcpy(value, dcValue.c_str());
    pDisplayMode->saveDeepColorAttr(outputmode, value);
}

void SystemControlService::getDeepColorAttr(const std::string& mode, std::string& value) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    pDisplayMode->getDeepColorAttr(mode.c_str(), buf);
    value = buf;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("get deep color attr mode %s, value %s ", mode.c_str(), value.c_str());
    }
}

void SystemControlService::setHdrMode(const std::string& mode) {
    pDisplayMode->setHdrMode(mode.c_str());
}

void SystemControlService::setSdrMode(const std::string& mode) {
    pDisplayMode->setSdrMode(mode.c_str());
}

int64_t SystemControlService::resolveResolutionValue(const std::string& mode) {
    int64_t value = pDisplayMode->resolveResolutionValue(mode.c_str());
    return value;
}

void SystemControlService::setListener(const sp<SystemControlNotify>& listener) {
    pDisplayMode->setListener(listener);
}

void SystemControlService::setAppInfo(const std::string& pkg, const std::string& cls, const std::vector<std::string>& procList) {
    //ALOGI("setAppInfo pkg :%s, cls:%s", pkg.c_str(), cls.c_str());
    if (func_optimization != NULL) {
        func_optimization(pkg.c_str(), cls.c_str(), procList);
    }
}

bool SystemControlService::getPrefHdmiDispMode(std::string *prefDispMode) {
    char mode[MODE_LEN] = {0};
    bool ret = pDisplayMode->getPrefHdmiDispMode(mode);
    *prefDispMode = mode;
    return ret;
}

//3D
int32_t SystemControlService::set3DMode(const std::string& mode3d) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set 3d mode :%s", mode3d.c_str());
    }

    return pDimension->set3DMode(mode3d.c_str());
}

void SystemControlService::init3DSetting(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("init3DSetting\n");
    }

    pDimension->init3DSetting();
}

int32_t SystemControlService::getVideo3DFormat(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("getVideo3DFormat\n");
    }

    return pDimension->getVideo3DFormat();
}

int32_t SystemControlService::getDisplay3DTo2DFormat(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("getDisplay3DTo2DFormat\n");
    }

    return pDimension->getDisplay3DTo2DFormat();
}

bool SystemControlService::setDisplay3DTo2DFormat(int format) {
    bool ret = false;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DTo2DFormat format:%d\n", format);
    }

    return pDimension->setDisplay3DTo2DFormat(format);
}

bool SystemControlService::setDisplay3DFormat(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DFormat format:%d\n", format);
    }

    return pDimension->setDisplay3DFormat(format);
}

int32_t SystemControlService::getDisplay3DFormat(void) {
    return pDimension->getDisplay3DFormat();
}

bool SystemControlService::setOsd3DFormat(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setOsd3DFormat format:%d\n", format);
    }

    return pDimension->setOsd3DFormat(format);
}

bool SystemControlService::switch3DTo2D(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("switch3DTo2D format:%d\n", format);
    }

    return pDimension->switch3DTo2D(format);
}

bool SystemControlService::switch2DTo3D(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("switch2DTo3D format:%d\n", format);
    }

    return pDimension->switch2DTo3D(format);
}

void SystemControlService::autoDetect3DForMbox() {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("autoDetect3DForMbox\n");
    }

    pDimension->autoDetect3DForMbox();
}
//3D end

//PQ
int SystemControlService::loadPQSettings(source_input_param_t source_input_param)
{
    if (pCPQControl != NULL) {
        return pCPQControl->LoadPQSettings();
    } else {
        return -1;
    }
}

int SystemControlService::sysSSMReadNTypes(int id, int data_len, int offset)
{
    if (pCPQControl != NULL) {
        return pCPQControl->Cpq_SSMReadNTypes(id, data_len, offset);
    } else {
        return -1;
    }
}


int SystemControlService::sysSSMWriteNTypes(int id, int data_len, int data_buf, int offset)
{
    if (pCPQControl != NULL) {
        return pCPQControl->Cpq_SSMWriteNTypes(id, data_len, data_buf, offset);
    } else {
        return -1;
    }
}

int SystemControlService::getActualAddr(int id)
{
    if (pCPQControl != NULL) {
        return pCPQControl->Cpq_GetSSMActualAddr(id);
    } else {
        return -1;
    }
}

int SystemControlService::getActualSize(int id)
{
    if (pCPQControl != NULL) {
        return pCPQControl->Cpq_GetSSMActualSize(id);
    } else {
        return -1;
    }
}

int SystemControlService::getSSMStatus(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->Cpq_GetSSMStatus();
    } else {
        return -1;
    }
}

int SystemControlService::setPQmode(int pq_mode, int is_save, int is_autoswitch)
{
    if (pCPQControl != NULL) {
            return pCPQControl->SetPQMode(pq_mode, is_save, is_autoswitch);
    } else {
        return -1;
    }
}

int SystemControlService::getPQmode(void)
{
    if (pCPQControl != NULL) {
        return (int)pCPQControl->GetPQMode();
    } else {
        return -1;
    }
}

int SystemControlService::savePQmode(int pq_mode)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SavePQMode(pq_mode);
    } else {
        return -1;
    }
}

int SystemControlService::setColorTemperature(int temp_mode, int is_save)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetColorTemperature(temp_mode, is_save);
    } else {
        return -1;
    }
}

int SystemControlService::getColorTemperature(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetColorTemperature();
    } else {
        return -1;
    }
}

int SystemControlService::saveColorTemperature(int temp_mode)
{
	if (pCPQControl != NULL) {
        return pCPQControl->SaveColorTemperature(temp_mode);
    } else {
        return -1;
    }
}

int SystemControlService::setColorTemperatureUserParam(int mode, int isSave, int param_type, int value)
{
	if (pCPQControl != NULL) {
        return pCPQControl->SetColorTemperature(mode, isSave, (rgb_ogo_type_t)param_type, value);
    } else {
        return -1;
    }
}

tcon_rgb_ogo_t SystemControlService::getColorTemperatureUserParam(void)
{
    tcon_rgb_ogo_t ColorTemperatureUserParam;
    memset(&ColorTemperatureUserParam, 0, sizeof(tcon_rgb_ogo_t));
	if (pCPQControl != NULL) {
        ColorTemperatureUserParam = pCPQControl->GetColorTemperatureUserParam();
    }

    return ColorTemperatureUserParam;
}

int SystemControlService::setBrightness(int value, int is_save)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetBrightness(value, is_save);
    } else {
        return -1;
    }
}

int SystemControlService::getBrightness(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetBrightness();
    } else {
        return -1;
    }
}

int SystemControlService::saveBrightness(int value)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SaveBrightness(value);
    } else {
        return -1;
    }
}

int SystemControlService::setContrast(int value, int is_save)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetContrast(value, is_save);
    } else {
        return -1;
    }
}

int SystemControlService::getContrast(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetContrast();
    } else {
        return -1;
    }
}

int SystemControlService::saveContrast(int value)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SaveContrast(value);
    } else {
        return -1;
    }
}

int SystemControlService::setSaturation(int value, int is_save)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetSaturation(value, is_save);
    } else {
        return -1;
    }
}

int SystemControlService::getSaturation(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetSaturation();
    } else {
        return -1;
    }
}

int SystemControlService::saveSaturation(int value)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SaveSaturation(value);
    } else {
        return -1;
    }
}

int SystemControlService::setHue(int value, int is_save )
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetHue(value, is_save);
    } else {
        return -1;
    }
}

int SystemControlService::getHue(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetHue();
    } else {
        return -1;
    }
}

int SystemControlService::saveHue(int value)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SaveHue(value);
    } else {
        return -1;
    }
}

int SystemControlService::setSharpness(int value, int is_enable, int is_save)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetSharpness(value, is_enable, is_save);
    } else {
        return -1;
    }
}

int SystemControlService::getSharpness(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetSharpness();
    } else {
        return -1;
    }
}

int SystemControlService::saveSharpness(int value)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SaveSharpness(value);
    } else {
        return -1;
    }
}

int SystemControlService::setNoiseReductionMode(int nr_mode, int is_save)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetNoiseReductionMode(nr_mode, is_save);
    } else {
        return -1;
    }
}

int SystemControlService::getNoiseReductionMode(void)
{
    if (pCPQControl != NULL) {
        return (int)pCPQControl->GetNoiseReductionMode();
    } else {
        return -1;
    }
}

int SystemControlService::saveNoiseReductionMode(int nr_mode)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SaveNoiseReductionMode(nr_mode);
    } else {
        return -1;
    }
}

int SystemControlService::setEyeProtectionMode(int source_input, int enable, int isSave)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetEyeProtectionMode((tv_source_input_t)source_input, enable, isSave);
    } else {
        return -1;
    }
}

int SystemControlService::getEyeProtectionMode(int source_input)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetEyeProtectionMode((tv_source_input_t)source_input);
    } else {
        return -1;
    }
}

int SystemControlService::setGammaValue(int gamma_curve, int is_save)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetGammaValue((vpp_gamma_curve_t)gamma_curve, is_save);
    } else {
        return -1;
    }
}

int SystemControlService::getGammaValue()
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetGammaValue();
    } else {
        return -1;
    }
}

void SystemControlService::setDisplayModeListener(const sp<SystemControlNotify>& listener)
{
    mDisplayListener = listener;
}

void SystemControlService::SendDisplayMode(int mode) {
    if (mDisplayListener != NULL) {
        ALOGI("set displaymode callback\n");
        mDisplayListener->onSetDisplayMode(mode);
    }
}

int SystemControlService::setDisplayMode(int source_input, int mode, int isSave)
{
    if (pCPQControl != NULL) {
        SendDisplayMode(mode);
        return pCPQControl->SetDisplayMode((vpp_display_mode_t)mode, isSave);
    } else {
        return -1;
    }
}


int SystemControlService::getDisplayMode(int source_input)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetDisplayMode();
    } else {
        return -1;
    }
}

int SystemControlService::saveDisplayMode(int source_input, int mode)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SaveDisplayMode((vpp_display_mode_t)mode);
    } else {
        return -1;
    }
}

int SystemControlService::setBacklight(int value, int isSave)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetBacklight(value, isSave);
    } else {
        return -1;
    }
}

int SystemControlService::getBacklight(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetBacklight();
    } else {
        return -1;
    }
}

int SystemControlService::saveBacklight(int value)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SaveBacklight(value);
    } else {
        return -1;
    }
}

int SystemControlService::setDynamicBacklight(int mode, int isSave)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetDynamicBacklight((Dynamic_backlight_status_t)mode, isSave);
    } else {
        return -1;
    }
}

int SystemControlService::getDynamicBacklight(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetDynamicBacklight();
    } else {
        return -1;
    }
}

int SystemControlService::setLocalContrastMode(int mode, int isSave)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetLocalContrastMode((local_contrast_mode_t)mode, isSave);
    } else {
        return -1;
    }
}

int SystemControlService::getLocalContrastMode()
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetLocalContrastMode();
    } else {
        return -1;
    }
}

int SystemControlService::setColorBaseMode(int mode, int isSave)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetColorBaseMode((vpp_color_basemode_t)mode, isSave);
    } else {
        return -1;
    }
}

int SystemControlService::getColorBaseMode()
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetColorBaseMode();
    } else {
        return -1;
    }
}

bool SystemControlService::checkLdimExist(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->isFileExist(LDIM_PATH);
    } else {
        return -1;
    }
}

int SystemControlService::factoryResetPQMode(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryResetPQMode();
    } else {
        return -1;
    }
}

int SystemControlService::factorySetPQMode_Brightness(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;

    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetPQMode_Brightness(srcInputParam, pq_mode, value);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetPQMode_Brightness(int inputSrc, int sigFmt, int transFmt, int pq_mode)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;

    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetPQMode_Brightness(srcInputParam, pq_mode);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetPQMode_Contrast(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;

    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetPQMode_Contrast(srcInputParam, pq_mode, value);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetPQMode_Contrast(int inputSrc, int sigFmt, int transFmt, int pq_mode)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetPQMode_Contrast(srcInputParam, pq_mode);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetPQMode_Saturation(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetPQMode_Saturation(srcInputParam, pq_mode, value);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetPQMode_Saturation(int inputSrc, int sigFmt, int transFmt, int pq_mode)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetPQMode_Saturation(srcInputParam, pq_mode);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetPQMode_Hue(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetPQMode_Hue(srcInputParam, pq_mode, value);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetPQMode_Hue(int inputSrc, int sigFmt, int transFmt, int pq_mode)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetPQMode_Hue(srcInputParam, pq_mode);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetPQMode_Sharpness(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetPQMode_Sharpness(srcInputParam, pq_mode, value);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetPQMode_Sharpness(int inputSrc, int sigFmt, int transFmt, int pq_mode)
{
    source_input_param_t srcInputParam;
    srcInputParam.source_input = (tv_source_input_t)inputSrc;
    srcInputParam.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    srcInputParam.trans_fmt= (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetPQMode_Sharpness(srcInputParam, pq_mode);
    } else {
        return -1;
    }
}

int SystemControlService::factoryResetColorTemp(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryResetColorTemp();
    } else {
        return -1;
    }
}

int SystemControlService::factorySetOverscan(int inputSrc, int sigFmt, int transFmt, int he_value, int hs_value,
                                             int ve_value, int vs_value)
{
    tvin_cutwin_t cutwin_t;
    cutwin_t.he = he_value;
    cutwin_t.hs = hs_value;
    cutwin_t.ve = ve_value;
    cutwin_t.vs = vs_value;

    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt= (tvin_trans_fmt_t)transFmt;

    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetOverscanParam(source_input_param, cutwin_t);
    } else {
        return -1;
    }
}

tvin_cutwin_t SystemControlService::factoryGetOverscan(int inputSrc, int sigFmt, int transFmt)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt= (tvin_trans_fmt_t)transFmt;
    tvin_cutwin_t cutwin_t;

    if (pCPQControl != NULL) {
        tvin_cutwin_t cutwin_t = pCPQControl->FactoryGetOverscanParam(source_input_param);
        return cutwin_t;
    } else {
        cutwin_t.he = 0;
        cutwin_t.hs = 0;
        cutwin_t.ve = 0;
        cutwin_t.vs = 0;
        return cutwin_t;
    }
}

int SystemControlService::factorySetNolineParams(int inputSrc, int sigFmt, int transFmt, int type, int osd0_value,
int osd25_value,
                                                          int osd50_value, int osd75_value, int osd100_value)
{
    noline_params_t noline_params;
    noline_params.osd0   = osd0_value;
    noline_params.osd25  = osd25_value;
    noline_params.osd50  = osd50_value;
    noline_params.osd75  = osd75_value;
    noline_params.osd100 = osd100_value;

    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt= (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetNolineParams(source_input_param, type, noline_params);
    } else {
        return -1;
    }
}

noline_params_t SystemControlService::factoryGetNolineParams(int inputSrc, int sigFmt, int transFmt, int type)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt= (tvin_trans_fmt_t)transFmt;
    noline_params_t param;

    if (pCPQControl != NULL) {
        pCPQControl->FactoryGetNolineParams(source_input_param, type);
        return param;
    } else {
        param.osd0 = 0;
        param.osd25 = 0;
        param.osd50 = 0;
        param.osd75 = 0;
        param.osd100 = 0;
        return param;
    }
}

int SystemControlService::factoryGetColorTemperatureParams(int colorTemp_mode)
{
    tcon_rgb_ogo_t params;
    memset(&params, 0, sizeof(tcon_rgb_ogo_t));
    if (pCPQControl != NULL) {
        return pCPQControl->GetColorTemperatureParams((vpp_color_temperature_mode_t)colorTemp_mode, &params);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetParamsDefault()
{
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetParamsDefault();
    } else {
        return -1;
    }
}

int SystemControlService::factorySSMRestore(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->FcatorySSMRestore();
    } else {
        return -1;
    }
}

int SystemControlService::factoryResetNonlinear(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryResetNonlinear();
    } else {
        return -1;
    }
}

int SystemControlService::factorySetGamma(int gamma_r, int gamma_g, int gamma_b)
{
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetGamma(gamma_r, gamma_g, gamma_b);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetHdrMode(int mode)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetHDRMode(mode);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetHdrMode(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetHDRMode();
    } else {
        return -1;
    }
}

int SystemControlService::SSMRecovery(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->Cpq_SSMRecovery();
    } else {
        return -1;
    }
}

int SystemControlService::setPLLValues(source_input_param_t source_input_param)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetPLLValues(source_input_param);
    } else {
        return -1;
    }
}

int SystemControlService::setCVD2Values(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetCVD2Values();
    } else {
        return -1;
    }
}

int SystemControlService::setCurrentSourceInfo(int sourceInput, int sigFmt, int transFmt)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)sourceInput;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->SetCurrentSourceInputInfo(source_input_param);
    } else {
        return -1;
    }
}

source_input_param_t SystemControlService::getCurrentSourceInfo(void)
{
    if (pCPQControl != NULL) {
        return pCPQControl->GetCurrentSourceInputInfo();
    } else {
        source_input_param_t source_input_param;
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
        source_input_param.source_input = SOURCE_MPEG;
        source_input_param.trans_fmt = TVIN_TFMT_2D;
        return source_input_param;
    }
}

int SystemControlService::setCurrentHdrInfo(int hdrInfo)
{
    if (pCPQControl != NULL) {
        return pCPQControl->SetCurrentHdrInfo(hdrInfo);
    } else {
        return -1;
    }
}

int SystemControlService::setDtvKitSourceEnable(int isEnable)
{
    int ret = -1;
    if (pCPQControl != NULL) {
        if (isEnable) {
            ret = pCPQControl->SetDtvKitSourceEnable(true);
        } else {
            ret = pCPQControl->SetDtvKitSourceEnable(false);
        }
    }

    return ret;
}

int SystemControlService::setwhiteBalanceGainRed(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }

    if (pCPQControl == NULL) {
        return -1;
    }

    ret = pCPQControl->FactorySetColorTemp_Rgain(inputSrc, colortemp_mode, value);
    if (ret != -1) {
        ALOGI("save the red gain to flash");
        ret = pCPQControl->FactorySaveColorTemp_Rgain(inputSrc, colortemp_mode, value);
    }
    return ret;
}

int SystemControlService::setwhiteBalanceGainGreen(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }

    if (pCPQControl == NULL) {
        return -1;
    }

    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemp_Ggain(inputSrc, colortemp_mode, value);
    if (ret != -1) {
        ALOGI("save the green gain to flash");
        ret = pCPQControl->FactorySaveColorTemp_Ggain(inputSrc, colortemp_mode, value);
    }
    return ret;
}

int SystemControlService::setwhiteBalanceGainBlue(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }

    if (pCPQControl == NULL) {
        return -1;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemp_Bgain(inputSrc, colortemp_mode, value);
    if (ret != -1) {
        ALOGI("save the blue gain to flash");
        ret = pCPQControl->FactorySaveColorTemp_Bgain(inputSrc, colortemp_mode, value);
    }
    return ret;
}

int SystemControlService::setwhiteBalanceOffsetRed(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }

    if (pCPQControl == NULL) {
        return -1;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemp_Roffset(inputSrc, colortemp_mode, value);
    if (ret != -1) {
        ALOGI("save the red offset to flash");
        ret = pCPQControl->FactorySaveColorTemp_Roffset(inputSrc, colortemp_mode, value);
    }
    return ret;
}

int SystemControlService::setwhiteBalanceOffsetGreen(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }

    if (pCPQControl == NULL) {
        return -1;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemp_Goffset(inputSrc, colortemp_mode, value);
    if (ret != -1) {
        ALOGI("save the green offset to flash");
        ret = pCPQControl->FactorySaveColorTemp_Goffset(inputSrc, colortemp_mode, value);
    }

    return ret;
}

int SystemControlService::setwhiteBalanceOffsetBlue(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }

    if (pCPQControl == NULL) {
        return -1;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemp_Boffset(inputSrc, colortemp_mode, value);
    if (ret != -1) {
        ALOGI("save the green offset to flash");
        ret = pCPQControl->FactorySaveColorTemp_Boffset(inputSrc, colortemp_mode, value);
    }

    return ret;
}

int SystemControlService::getwhiteBalanceGainRed(int inputSrc, int sigFmt, int transFmt, int colortemp_mode) {

    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetColorTemp_Rgain(inputSrc, colortemp_mode);
    } else {
        return -1;
    }
}

int SystemControlService::getwhiteBalanceGainGreen(int inputSrc, int sigFmt, int transFmt, int colortemp_mode) {

    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetColorTemp_Ggain(inputSrc, colortemp_mode);
    } else {
        return -1;
    }
}

int SystemControlService::getwhiteBalanceGainBlue(int inputSrc, int sigFmt, int transFmt, int colortemp_mode) {

    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetColorTemp_Bgain(inputSrc, colortemp_mode);
    } else {
        return -1;
    }
}

int SystemControlService::getwhiteBalanceOffsetRed(int inputSrc, int sigFmt, int transFmt, int colortemp_mode) {

    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetColorTemp_Roffset(inputSrc, colortemp_mode);
    } else {
        return -1;
    }
}

int SystemControlService::getwhiteBalanceOffsetGreen(int inputSrc, int sigFmt, int transFmt, int colortemp_mode) {

    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetColorTemp_Goffset(inputSrc, colortemp_mode);
    } else {
        return -1;
    }
}

int SystemControlService::getwhiteBalanceOffsetBlue(int inputSrc, int sigFmt, int transFmt, int colortemp_mode) {

    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetColorTemp_Boffset(inputSrc, colortemp_mode);
    } else {
        return -1;
    }
}

int SystemControlService::saveWhiteBalancePara(int inputSrc, int sigFmt, int transFmt, int colorTemp_mode, int r_gain
, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset) {
    int ret = 0;

    if (pCPQControl == NULL) {
        return -1;
    }
    ret |= pCPQControl->SaveColorTemperature(colorTemp_mode);
    ret |= pCPQControl->FactorySaveColorTemp_Rgain(inputSrc, colorTemp_mode, r_gain);
    ret |= pCPQControl->FactorySaveColorTemp_Ggain(inputSrc, colorTemp_mode, g_gain);
    ret |= pCPQControl->FactorySaveColorTemp_Bgain(inputSrc, colorTemp_mode, b_gain);
    ret |= pCPQControl->FactorySaveColorTemp_Roffset(inputSrc, colorTemp_mode, r_offset);
    ret |= pCPQControl->FactorySaveColorTemp_Goffset(inputSrc, colorTemp_mode, g_offset);
    ret |= pCPQControl->FactorySaveColorTemp_Boffset(inputSrc, colorTemp_mode, b_offset);

    return ret;
}

int SystemControlService::getRGBPattern() {
    if (pCPQControl != NULL) {
        return pCPQControl->GetRGBPattern();
    } else {
        return -1;
    }
}

int SystemControlService::setRGBPattern(int r, int g, int b) {
    if (pCPQControl != NULL) {
        return pCPQControl->SetRGBPattern(r, g, b);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetDDRSSC(int step) {
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetDDRSSC(step);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetDDRSSC() {
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetDDRSSC();
    } else {
        return -1;
    }
}

int SystemControlService::setLVDSSSC(int step) {
    if (pCPQControl != NULL) {
        return pCPQControl->SetLVDSSSC(step);
    } else {
        return -1;
    }
}
int SystemControlService::factorySetLVDSSSC(int step) {
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetLVDSSSC(step);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetLVDSSSC() {
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetLVDSSSC();
    } else {
        return -1;
    }
}


int SystemControlService::whiteBalanceGrayPatternClose() {
    return 0;
}

int SystemControlService::whiteBalanceGrayPatternOpen() {
    return 0;
}

int SystemControlService::whiteBalanceGrayPatternSet(int value) {
    if (pCPQControl != NULL) {
        return pCPQControl->SetGrayPattern(value);
    } else {
        return -1;
    }
}

int SystemControlService::whiteBalanceGrayPatternGet() {
    if (pCPQControl != NULL) {
        return pCPQControl->GetGrayPattern();
    } else {
        return -1;
    }
}

int SystemControlService::setDnlpParams(int inputSrc, int sigFmt, int transFmt, int level)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->SetDnlpMode(level);
    } else {
        return -1;
    }
}

int SystemControlService::getDnlpParams(int inputSrc, int sigFmt, int transFmt) {
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    int level = 0;
    if (pCPQControl != NULL) {
        return pCPQControl->GetDnlpMode();
    } else {
        return -1;
    }
}

int SystemControlService::factorySetDnlpParams(int inputSrc, int sigFmt, int transFmt, int level, int final_gain) {
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetDNLPCurveParams(source_input_param, level, final_gain);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetDnlpParams(int inputSrc, int sigFmt, int transFmt, int level) {
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetDNLPCurveParams(source_input_param, level);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetBlackExtRegParams(int inputSrc, int sigFmt, int transFmt, int val) {
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetBlackExtRegParams(source_input_param, val);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetBlackExtRegParams(int inputSrc, int sigFmt, int transFmt) {
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetBlackExtRegParams(source_input_param);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetColorParams(int inputSrc, int sigFmt, int transFmt, int color_type, int color_param, int val)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetRGBCMYFcolorParams(source_input_param, color_type, color_param, val);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetColorParams(int inputSrc, int sigFmt, int transFmt, int color_type, int color_param)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sigFmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)transFmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetRGBCMYFcolorParams(source_input_param, color_type, color_param);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type, int val)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sig_fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetNoiseReductionParams(source_input_param, (vpp_noise_reduction_mode_t)nr_mode,param_type, val);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sig_fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetNoiseReductionParams(source_input_param, (vpp_noise_reduction_mode_t)nr_mode,param_type);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val) {
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sig_fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetCTIParams(source_input_param, param_type, val);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sig_fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetCTIParams(source_input_param, param_type);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sig_fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetDecodeLumaParams(source_input_param, param_type, val);
    } else {
        return -1;
    }
}

int SystemControlService::factoryGetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sig_fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetDecodeLumaParams(source_input_param, param_type);
    } else {
        return -1;
    }
}

int SystemControlService::factorySetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD, int param_type
, int val) {
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sig_fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactorySetSharpnessParams(source_input_param, (Sharpness_timing_e)isHD, param_type, val);
    } else {
        return -1;
    }
}
int SystemControlService::factoryGetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD,int param_type)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)inputSrc;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)sig_fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    if (pCPQControl != NULL) {
        return pCPQControl->FactoryGetSharpnessParams(source_input_param, (Sharpness_timing_e)isHD, param_type);
    } else {
        return -1;
    }
}

tvpq_databaseinfo_t SystemControlService::getPQDatabaseInfo(int dataBaseName) {
    tvpq_databaseinfo_t pq_databaseinfo;
    if (pCPQControl != NULL) {
        pq_databaseinfo = pCPQControl->GetDBVersionInfo((db_name_t)dataBaseName);
    }
    return pq_databaseinfo;
}

//PQ end

//FBC start
void SystemControlService::setFBCUpgradeListener(const sp<SystemControlNotify>& listener)
{
    mNotifyListener = listener;
}

int SystemControlService::StartUpgradeFBC(const std::string&file_name, int mode, int upgrade_blk_size)
{
    char buf[256] = {0};
    strcpy(buf, file_name.c_str());
    return CFbcCommunication::GetSingletonFBC()->fbcStartUpgrade(buf,  mode, upgrade_blk_size);
}

int SystemControlService::UpdateFBCUpgradeStatus(int status, int param)
{
    ALOGI("%s: state = %d, param = %d\n", __FUNCTION__, status, param);
    mNotifyListener->onFBCUpgradeEvent(status, param);
    return 0;
}

//FBC end
void SystemControlService::traceValue(const std::string& type, const std::string& key, const std::string& value) {
    if (mLogLevel > LOG_LEVEL_1) {
        /*
        String16 procName;
        int pid = IPCThreadState::self()->getCallingPid();
        int uid = IPCThreadState::self()->getCallingUid();

        getProcName(pid, procName);

        ALOGI("%s [ %s ] [ %s ] from pid=%d, uid=%d, process name=%s",
            String8(type).string(), String8(key).string(), String8(value).string(),
            pid, uid,
            String8(procName).string());
            */
    }
}

void SystemControlService::traceValue(const std::string& type, const std::string& key, const int size) {
    if (mLogLevel > LOG_LEVEL_1) {
        /*
        String16 procName;
        int pid = IPCThreadState::self()->getCallingPid();
        int uid = IPCThreadState::self()->getCallingUid();

        getProcName(pid, procName);

        ALOGI("%s [ %s ] [ %d ] from pid=%d, uid=%d, process name=%s",
           String8(type).string(), String8(key).string(), size,
           pid, uid,
           String8(procName).string());
           */
    }
}

void SystemControlService::setLogLevel(int level) {
    if (level > (LOG_LEVEL_TOTAL - 1)) {
        ALOGE("out of range level=%d, max=%d", level, LOG_LEVEL_TOTAL);
        return;
    }

    mLogLevel = level;
    pSysWrite->setLogLevel(level);
    pDisplayMode->setLogLevel(level);
    pDimension->setLogLevel(level);
}

int SystemControlService::getLogLevel() {
    return mLogLevel;
}

int SystemControlService::getProcName(pid_t pid, String16& procName) {
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

status_t SystemControlService::dump(int fd, const Vector<String16>& args) {
#if 0
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

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
            if ((i + 3 < len) && (args[i + 1] == String16("set"))) {
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

    write(fd, result.string(), result.size());
#endif
    return NO_ERROR;
}

} // namespace android

