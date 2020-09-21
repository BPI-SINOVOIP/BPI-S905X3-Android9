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
 *  @date     2017/10/18
 *  @par function description:
 *  - 1 system control apis for other vendor process
 */

#define LOG_TAG "SystemControlClient"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <android-base/logging.h>
#include <SystemControlClient.h>

namespace android {

Mutex SystemControlClient::mLock;

SystemControlClient::SystemControlClient() {
    //mSysCtrl = ISystemControl::getService();
    Mutex::Autolock _l(mLock);
    connect();
}

void SystemControlClient::connect() {
    mSysCtrl = ISystemControl::tryGetService();
    while (mSysCtrl == nullptr) {
         usleep(200 * 1000);//sleep 200ms
         mSysCtrl = ISystemControl::tryGetService();
    };
    if (mSysCtrl == nullptr) {
        ALOGE("tryGet system control daemon Service");
        return;
    }
    if (mDeathRecipient == nullptr) {
        mDeathRecipient = new SystemControlDeathRecipient(this);
    }
    Return<bool> linked = mSysCtrl->linkToDeath(mDeathRecipient, /*cookie*/ 0);
    if (!linked.isOk()) {
        LOG(ERROR) << "Transaction error in linking to system service death: " << linked.description().c_str();
    } else if (!linked) {
        LOG(ERROR) << "Unable to link to system service death notifications";
    } else {
        LOG(INFO) << "Link to system service death notification successful";
    }

    if (mSystemControlHidlCallback == nullptr) {
        mSystemControlHidlCallback = new SystemControlHidlCallback(this);
    }
    mSysCtrl->setCallback(mSystemControlHidlCallback);
}


SystemControlClient::~SystemControlClient() {

}

bool SystemControlClient::getProperty(const std::string& key, std::string& value) {
    mSysCtrl->getProperty(key, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });

    return true;
}

bool SystemControlClient::getPropertyString(const std::string& key, std::string& value, const std::string& def) {
    mSysCtrl->getPropertyString(key, def, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });

    return true;
}

int32_t SystemControlClient::getPropertyInt(const std::string& key, int32_t def) {
    int32_t result;
    mSysCtrl->getPropertyInt(key, def, [&result](const Result &ret, const int32_t& v) {
        if (Result::OK == ret) {
            result = v;
        }
    });
    return result;
}

int64_t SystemControlClient::getPropertyLong(const std::string& key, int64_t def) {
    int64_t result;
    mSysCtrl->getPropertyLong(key, def, [&result](const Result &ret, const int64_t& v) {
        if (Result::OK == ret) {
            result = v;
        }
    });
    return result;
}

bool SystemControlClient::getPropertyBoolean(const std::string& key, bool def) {
    bool result;
    mSysCtrl->getPropertyBoolean(key, def, [&result](const Result &ret, const bool& v) {
        if (Result::OK == ret) {
            result = v;
        }
    });
    return result;
}

void SystemControlClient::setProperty(const std::string& key, const std::string& value) {
    mSysCtrl->setProperty(key, value);
}

bool SystemControlClient::readSysfs(const std::string& path, std::string& value) {
    mSysCtrl->readSysfs(path, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });

    return true;
}

bool SystemControlClient::writeSysfs(const std::string& path, const std::string& value) {
    Result rtn = mSysCtrl->writeSysfs(path, value);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::writeSysfs(const std::string& path, const char *value, const int size) {
    int i;
    hidl_array<int32_t, 4096> result;
    for (i = 0; i < size; ++i) {
        result[i] = value[i];
    }

    for (; i < 4096; ++i) {
        result[i] = 0;
    }

    Result rtn = mSysCtrl->writeSysfsBin(path, result, size);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::writeUnifyKey(const std::string& key, const std::string& value) {
    Result rtn = mSysCtrl->writeUnifyKey(key, value);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::readUnifyKey(const std::string& key, std::string& value) {
    mSysCtrl->readUnifyKey(key, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });

    return true;
}

bool SystemControlClient::writePlayreadyKey(const std::string& key, const char *value, const int size) {
    int i;
    hidl_array<int32_t, 4096> result;
    for (i = 0; i < size; ++i) {
        result[i] = value[i];
    }

    for (; i < 4096; ++i) {
        result[i] = 0;
    }

    Result rtn = mSysCtrl->writePlayreadyKey(key, result, size);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

int32_t SystemControlClient::readPlayreadyKey(const std::string& key, char *value, int size) {
    hidl_array<int32_t, 4096> result;
    int32_t len;
    int j;
    mSysCtrl->readPlayreadyKey(key, size, [&result, &len](const Result &ret, const hidl_array<int32_t, 4096> v, const int32_t& l) {
        if (Result::OK == ret) {
            for (int i = 0; i < l; ++i) {
                result[i] = v[i];
            }
            len = l;
        }
    });

    for (j = 0; j < len; ++j) {
        value[j] = result[j];
    }

    return len;
}

int32_t SystemControlClient::readAttestationKey(const std::string& node, const std::string& name, char *value, int size) {
    hidl_array<int32_t, 10240> result;
    int32_t len;
    int j;
    mSysCtrl->readAttestationKey(node, name, size, [&result, &len](const Result &ret, const hidl_array<int32_t, 10240> v, const int32_t& l) {
        if (Result::OK == ret) {
            for (int i = 0; i < l; ++i) {
                result[i] = v[i];
            }
            len = l;
        }
    });

    for (j = 0; j < len; ++j) {
        value[j] = result[j];
    }

    return len;
}

bool SystemControlClient::writeAttestationKey(const std::string& node, const std::string& name, const char *buff, const int size) {
    int i;
    hidl_array<int32_t, 10240> result;

    for (i = 0; i < size; ++i) {
        result[i] = buff[i];
    }

    for (; i < 10240; ++i) {
        result[i] = 0;
    }

    Result rtn = mSysCtrl->writeAttestationKey(node, name, result);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::checkAttestationKey() {
    LOG(INFO) << "SystemControlClient checkAttestationKey";
    Result rtn = mSysCtrl->checkAttestationKey();
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

int32_t SystemControlClient::readHdcpRX22Key(char *value, int size) {
    hidl_array<int32_t, 4096> result;
    int32_t len;
    int j;
    mSysCtrl->readHdcpRX22Key(size, [&result, &len](const Result &ret, const hidl_array<int32_t, 4096> v, const int32_t& l) {
        if (Result::OK == ret) {
            for (int i = 0; i < l; ++i) {
                result[i] = v[i];
            }
            len = l;
        }
    });

    for (j = 0; j < len; ++j) {
        value[j] = result[j];
    }

    return len;
}

bool SystemControlClient::writeHdcpRX22Key(const char *value, const int size) {
    int i;
    hidl_array<int32_t, 4096> result;
    for (i = 0; i < size; ++i) {
        result[i] = value[i];
    }

    for (; i < 4096; ++i) {
        result[i] = 0;
    }

    Result rtn = mSysCtrl->writeHdcpRX22Key(result, size);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

int32_t SystemControlClient::readHdcpRX14Key(char *value, int size) {
    hidl_array<int32_t, 4096> result;
    int32_t len;
    int j;
    mSysCtrl->readHdcpRX14Key(size, [&result, &len](const Result &ret, const hidl_array<int32_t, 4096> v, const int32_t& l) {
        if (Result::OK == ret) {
            for (int i = 0; i < l; ++i) {
                result[i] = v[i];
            }
            len = l;
        }
    });

    for (j = 0; j < len; ++j) {
        value[j] = result[j];
    }

    return len;
}

bool SystemControlClient::writeHdcpRX14Key(const char *value, const int size) {
    int i;
    hidl_array<int32_t, 4096> result;
    for (i = 0; i < size; ++i) {
        result[i] = value[i];
    }

    for (; i < 4096; ++i) {
        result[i] = 0;
    }
    Result rtn = mSysCtrl->writeHdcpRX14Key(result, size);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::writeHdcpRXImg(const std::string& path) {
    Result rtn = mSysCtrl->writeHdcpRXImg(path);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::getBootEnv(const std::string& key, std::string& value) {
    mSysCtrl->getBootEnv(key, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return true;
}

void SystemControlClient::setBootEnv(const std::string& key, const std::string& value) {
    mSysCtrl->setBootEnv(key, value);
}

void SystemControlClient::setHdrStrategy(const std::string& value) {
    mSysCtrl->setHdrStrategy(value);
}

bool SystemControlClient::getModeSupportDeepColorAttr(const std::string& mode, const std::string& color) {
    Result rtn;
    rtn = mSysCtrl->getModeSupportDeepColorAttr(mode,color);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

void SystemControlClient::getDroidDisplayInfo(int &type __unused, std::string& socType __unused, std::string& defaultUI __unused,
    int &fb0w __unused, int &fb0h __unused, int &fb0bits __unused, int &fb0trip __unused,
    int &fb1w __unused, int &fb1h __unused, int &fb1bits __unused, int &fb1trip __unused) {

    /*mSysCtrl->getDroidDisplayInfo([&](const Result &ret, const DroidDisplayInfo& info) {
        if (Result::OK == ret) {
            type = info.type;
            socType = info.socType;
            defaultUI = info.defaultUI;
            fb0w = info.fb0w;
            fb0h = info.fb0h;
            fb0bits = info.fb0bits;
            fb0trip = info.fb0trip;
            fb1w = info.fb1w;
            fb1h = info.fb1h;
            fb1bits = info.fb1bits;
            fb1trip = info.fb1trip;
        }
    });*/
}

void SystemControlClient::loopMountUnmount(const bool &isMount, const std::string& path)  {
    mSysCtrl->loopMountUnmount((int)isMount, path);
}

void SystemControlClient::setMboxOutputMode(const std::string& mode) {
    mSysCtrl->setSourceOutputMode(mode);
}

void SystemControlClient::setSinkOutputMode(const std::string& mode) {
    mSysCtrl->setSinkOutputMode(mode);
}

void SystemControlClient::setDigitalMode(const std::string& mode) {
    mSysCtrl->setDigitalMode(mode);
}

void SystemControlClient::setOsdMouseMode(const std::string& mode) {
    mSysCtrl->setOsdMouseMode(mode);
}

void SystemControlClient::setOsdMousePara(int x, int y, int w, int h) {
    mSysCtrl->setOsdMousePara(x, y, w, h);
}

void SystemControlClient::setPosition(int left, int top, int width, int height)  {
    mSysCtrl->setPosition(left, top, width, height);
}

void SystemControlClient::getPosition(const std::string& mode, int &outx, int &outy, int &outw, int &outh) {
    mSysCtrl->getPosition(mode, [&outx, &outy, &outw, &outh](const Result &ret,
        const int32_t& x, const int32_t& y, const int32_t& w, const int32_t& h) {
        if (Result::OK == ret) {
            outx = x;
            outy = y;
            outw = w;
            outh = h;
        }
    });
}

void SystemControlClient::saveDeepColorAttr(const std::string& mode, const std::string& dcValue) {
    mSysCtrl->saveDeepColorAttr(mode, dcValue);
}

void SystemControlClient::getDeepColorAttr(const std::string& mode, std::string& value) {
    mSysCtrl->getDeepColorAttr(mode, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
}

void SystemControlClient::initDolbyVision(int state) {
    mSysCtrl->initDolbyVision(state);
}

void SystemControlClient::setDolbyVisionEnable(int state) {
    mSysCtrl->setDolbyVisionState(state);
}

bool SystemControlClient::isTvSupportDolbyVision(std::string& mode) {
    bool supported = false;
    mSysCtrl->sinkSupportDolbyVision([&mode, &supported](const Result &ret, const hidl_string& sinkMode, const bool &isSupport) {
        if (Result::OK == ret) {
            mode = sinkMode;
            supported = isSupport;
        }
    });

    return supported;
}

int32_t SystemControlClient::getDolbyVisionType() {
    int32_t result;
    mSysCtrl->getDolbyVisionType([&result](const Result &ret, const int32_t& v) {
        if (Result::OK == ret) {
            result = v;
        }
    });
    return result;
}

void SystemControlClient::setGraphicsPriority(const std::string& mode) {
   mSysCtrl->setGraphicsPriority(mode);
}

void SystemControlClient::getGraphicsPriority(std::string& mode) {
    mSysCtrl->getGraphicsPriority([&mode](const Result &ret, const hidl_string& tempmode) {
        if (Result::OK == ret)
            mode = tempmode.c_str();
        else
            mode.clear();
    });

    if (mode.empty()) {
        LOG(ERROR) << "system control client getGraphicsPriority FAIL.";
    }
}

int64_t SystemControlClient::resolveResolutionValue(const std::string& mode) {
    int64_t value = 0;
    mSysCtrl->resolveResolutionValue(mode, [&value](const Result &ret, const int64_t &v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return value;
}

void SystemControlClient::setHdrMode(const std::string& mode) {
    mSysCtrl->setHdrMode(mode);
}

void SystemControlClient::setSdrMode(const std::string& mode) {
    mSysCtrl->setSdrMode(mode);
}

void SystemControlClient::setAppInfo(const std::string& pkg, const std::string& cls, const std::vector<std::string> &proc) {
    hidl_vec< hidl_string> procList;
    size_t size = proc.size();
    procList.resize(size);
    for (int i = 0; i< (int) proc.size(); ++i) {
        procList[i] = proc[i];
    }
    mSysCtrl->setAppInfo(pkg, cls, procList);
}

/*
void SystemControlClient::setListener(const sp<ISystemControlCallback> callback) {
    Return<void> ret = mSysCtrl->setCallback(callback);
}*/

bool SystemControlClient::getSupportDispModeList(std::vector<std::string>& supportDispModes) {
    mSysCtrl->getSupportDispModeList([&supportDispModes](const Result &ret, const hidl_vec<hidl_string> list) {
        if (Result::OK == ret) {
            for (size_t i = 0; i < list.size(); i++) {
                supportDispModes.push_back(list[i]);
            }
        } else {
            supportDispModes.clear();
        }
    });

    if (supportDispModes.empty()) {
        LOG(ERROR) << "syscontrol::readEdidList FAIL.";
        return false;
    }

    return true;
}

bool SystemControlClient::getActiveDispMode(std::string& activeDispMode) {
    mSysCtrl->getActiveDispMode([&activeDispMode](const Result &ret, const hidl_string& mode) {
        if (Result::OK == ret)
            activeDispMode = mode.c_str();
        else
            activeDispMode.clear();
    });

    if (activeDispMode.empty()) {
        LOG(ERROR) << "system control client getActiveDispMode FAIL.";
        return false;
    }

    return true;
}

bool SystemControlClient::setActiveDispMode(std::string& activeDispMode) {
    Result rtn = mSysCtrl->setActiveDispMode(activeDispMode);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

void SystemControlClient::isHDCPTxAuthSuccess(int &status) {
    Result rtn = mSysCtrl->isHDCPTxAuthSuccess();
    if (rtn == Result::OK) {
        status = 1;
    }
    else {
        status = 0;
    }
}

//3D
int32_t SystemControlClient::set3DMode(const std::string& mode3d) {
    mSysCtrl->set3DMode(mode3d);
    return 0;
}

void SystemControlClient::init3DSetting(void) {
    mSysCtrl->init3DSetting();
}

int SystemControlClient::getVideo3DFormat(void) {
    int32_t value = 0;
    mSysCtrl->getVideo3DFormat([&value](const Result &ret, const int32_t &v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return value;
}

int SystemControlClient::getDisplay3DTo2DFormat(void) {
    int32_t value = 0;
    mSysCtrl->getDisplay3DTo2DFormat([&value](const Result &ret, const int32_t &v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return value;
}

bool SystemControlClient::setDisplay3DTo2DFormat(int format) {
    mSysCtrl->setDisplay3DTo2DFormat(format);
    return true;
}

bool SystemControlClient::setDisplay3DFormat(int format) {
    mSysCtrl->setDisplay3DFormat(format);
    return true;
}

int SystemControlClient::getDisplay3DFormat(void) {
    int32_t value = 0;
    mSysCtrl->getDisplay3DFormat([&value](const Result &ret, const int32_t &v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return value;
}

bool SystemControlClient::setOsd3DFormat(int format) {
    mSysCtrl->setOsd3DFormat(format);
    return true;
}

bool SystemControlClient::switch3DTo2D(int format) {
    mSysCtrl->switch3DTo2D(format);
    return true;
}

bool SystemControlClient::switch2DTo3D(int format) {
    mSysCtrl->switch2DTo3D(format);
    return true;
}

void SystemControlClient::autoDetect3DForMbox() {
    mSysCtrl->autoDetect3DForMbox();
}
//3D end
//PQ
int SystemControlClient::loadPQSettings(source_input_param_t srcInputParam) {
    SourceInputParam hidlSrcInput;
    memcpy(&hidlSrcInput, &srcInputParam, sizeof(source_input_param_t));
    return mSysCtrl->loadPQSettings(hidlSrcInput);
}

int SystemControlClient::setPQmode(int mode, int isSave, int is_autoswitch) {
    return mSysCtrl->setPQmode(mode, isSave, is_autoswitch);
}

int SystemControlClient::getPQmode(void) {
    return mSysCtrl->getPQmode();
}

int SystemControlClient::savePQmode(int mode) {
    return mSysCtrl->savePQmode(mode);
}

int SystemControlClient::setColorTemperature(int mode, int isSave) {
    return mSysCtrl->setColorTemperature(mode, isSave);
}

int SystemControlClient::getColorTemperature(void) {
    return mSysCtrl->getColorTemperature();
}

int SystemControlClient::saveColorTemperature(int mode) {
    return mSysCtrl->saveColorTemperature(mode);
}

int SystemControlClient::setColorTemperatureUserParam(int mode, int isSave, int param_type, int value) {
    return mSysCtrl->setColorTemperatureUserParam(mode, isSave, param_type, value);
}

tcon_rgb_ogo_t SystemControlClient::getColorTemperatureUserParam(void) {
    tcon_rgb_ogo_t ColorTemperatureParam;
    memset(&ColorTemperatureParam, 0, sizeof(tcon_rgb_ogo_t));
    mSysCtrl->getColorTemperatureUserParam([&](const WhiteBalanceParam& param) {
        ColorTemperatureParam.r_gain = param.r_gain;
        ColorTemperatureParam.g_gain = param.g_gain;
        ColorTemperatureParam.b_gain = param.b_gain;
        ColorTemperatureParam.r_post_offset = param.r_post_offset;
        ColorTemperatureParam.g_post_offset = param.g_post_offset;
        ColorTemperatureParam.b_post_offset = param.b_post_offset;
    });

    return ColorTemperatureParam;
}

int SystemControlClient::setBrightness(int value, int isSave) {
    return mSysCtrl->setBrightness(value, isSave);
}

int SystemControlClient::getBrightness(void) {
    return mSysCtrl->getBrightness();
}

int SystemControlClient::saveBrightness(int value) {
    return mSysCtrl->saveBrightness(value);
}

int SystemControlClient::setContrast(int value, int isSave) {
    return mSysCtrl->setContrast(value, isSave);
}

int SystemControlClient::getContrast(void) {
    return mSysCtrl->getContrast();
}

int SystemControlClient::saveContrast(int value) {
    return mSysCtrl->saveContrast(value);
}

int SystemControlClient::setSaturation(int value, int isSave) {
    return mSysCtrl->setSaturation(value, isSave);
}

int SystemControlClient::getSaturation(void) {
    return mSysCtrl->getSaturation();
}

int SystemControlClient::saveSaturation(int value) {
    return mSysCtrl->saveSaturation(value);
}

int SystemControlClient::setHue(int value, int isSave) {
    return mSysCtrl->setHue(value, isSave);
}

int SystemControlClient::getHue(void) {
    return mSysCtrl->getHue();
}

int SystemControlClient::saveHue(int value) {
    return mSysCtrl->saveHue(value);
}

int SystemControlClient::setSharpness(int value, int is_enable, int isSave) {
    return mSysCtrl->setSharpness(value, is_enable, isSave);
}

int SystemControlClient::getSharpness(void) {
    return mSysCtrl->getSharpness();
}

int SystemControlClient::saveSharpness(int value) {
    return mSysCtrl->saveSharpness(value);
}

int SystemControlClient::setNoiseReductionMode(int nr_mode, int isSave) {
    return mSysCtrl->setNoiseReductionMode(nr_mode, isSave);
}

int SystemControlClient::getNoiseReductionMode(void) {
    return mSysCtrl->getNoiseReductionMode();
}

int SystemControlClient::saveNoiseReductionMode(int nr_mode) {
    return mSysCtrl->saveNoiseReductionMode(nr_mode);
}

int SystemControlClient::setEyeProtectionMode(int source_input, int enable, int isSave) {
    return mSysCtrl->setEyeProtectionMode(source_input, enable, isSave);
}

int SystemControlClient::getEyeProtectionMode(int source_input) {
    return mSysCtrl->getEyeProtectionMode(source_input);
}

int SystemControlClient::setGammaValue(int gamma_curve, int isSave) {
    return mSysCtrl->setGammaValue(gamma_curve, isSave);
}

int SystemControlClient::getGammaValue(void) {
    return mSysCtrl->getGammaValue();
}

int SystemControlClient::setDisplayMode(int source_input, int mode, int isSave)
{
    return mSysCtrl->setDisplayMode(source_input, mode, isSave);
}

int SystemControlClient::getDisplayMode(int source_input) {
    return mSysCtrl->getDisplayMode(source_input);
}

int SystemControlClient::saveDisplayMode(int source_input, int mode)
{
    return mSysCtrl->saveDisplayMode(source_input, mode);
}

int SystemControlClient::setBacklight(int value, int isSave)
{
    return mSysCtrl->setBacklight(value, isSave);
}

int SystemControlClient::getBacklight(void)
{
    return mSysCtrl->getBacklight();
}

int SystemControlClient::saveBacklight(int value)
{
    return mSysCtrl->saveBacklight(value);
}

int SystemControlClient::setDynamicBacklight(int mode, int isSave)
{
    return mSysCtrl->setDynamicBacklight(mode, isSave);
}

int SystemControlClient::getDynamicBacklight(void)
{
    return mSysCtrl->getDynamicBacklight();
}

bool SystemControlClient::checkLdimExist(void)
{
    int ret = mSysCtrl->checkLdimExist();
    if (ret == 0) {
        return false;
    } else {
        return true;
    }
}
int SystemControlClient::setLocalContrastMode(int mode, int isSave)
{
    return mSysCtrl->setLocalContrastMode(mode, isSave);
}

int SystemControlClient::getLocalContrastMode()
{
    return mSysCtrl->getLocalContrastMode();
}

int SystemControlClient::setColorBaseMode(int mode, int isSave)
{
    return mSysCtrl->setColorBaseMode(mode, isSave);
}

int SystemControlClient::getColorBaseMode()
{
    return mSysCtrl->getColorBaseMode();
}

int SystemControlClient::factoryResetPQMode(void) {
    return mSysCtrl->factoryResetPQMode();
}
int SystemControlClient::factorySetPQMode_Brightness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value) {
    return mSysCtrl->factorySetPQMode_Brightness(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
}

int SystemControlClient::factoryGetPQMode_Brightness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode) {
    return mSysCtrl->factoryGetPQMode_Brightness(inputSrc, sig_fmt, trans_fmt, pq_mode);
}

int SystemControlClient::factorySetPQMode_Contrast(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value) {
    return mSysCtrl->factorySetPQMode_Contrast(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
}

int SystemControlClient::factoryGetPQMode_Contrast(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode) {
    return mSysCtrl->factoryGetPQMode_Contrast(inputSrc, sig_fmt, trans_fmt, pq_mode);
}

int SystemControlClient::factorySetPQMode_Saturation(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value) {
    return mSysCtrl->factorySetPQMode_Saturation(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
}

int SystemControlClient::factoryGetPQMode_Saturation(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode) {
    return mSysCtrl->factoryGetPQMode_Saturation(inputSrc, sig_fmt, trans_fmt, pq_mode);
}

int SystemControlClient::factorySetPQMode_Hue(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value) {
    return mSysCtrl->factorySetPQMode_Hue(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
}

int SystemControlClient::factoryGetPQMode_Hue(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode) {
    return mSysCtrl->factoryGetPQMode_Hue(inputSrc, sig_fmt, trans_fmt, pq_mode);
}

int SystemControlClient::factorySetPQMode_Sharpness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value) {
    return mSysCtrl->factorySetPQMode_Sharpness(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
}
int SystemControlClient::factoryGetPQMode_Sharpness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode) {
    return mSysCtrl->factoryGetPQMode_Sharpness(inputSrc, sig_fmt, trans_fmt, pq_mode);
}

int SystemControlClient::factoryResetColorTemp(void) {
    return mSysCtrl->factoryResetColorTemp();
}

int SystemControlClient::factorySetOverscan(int inputSrc, int sigFmt, int transFmt, int he_value, int hs_value, int ve_value, int vs_value) {
    return mSysCtrl->factorySetOverscan(inputSrc, sigFmt, transFmt, he_value, hs_value, ve_value, vs_value);
}

tvin_cutwin_t SystemControlClient::factoryGetOverscan(int inputSrc, int sigFmt, int transFmt) {
    tvin_cutwin_t overscanParam;
    memset(&overscanParam, 0, sizeof(tvin_cutwin_t));
    mSysCtrl->factoryGetOverscan(inputSrc, sigFmt, transFmt, [&](const OverScanParam& param) {
        overscanParam.he = param.he;
        overscanParam.hs = param.hs;
        overscanParam.ve = param.ve;
        overscanParam.vs = param.vs;
    });

    return overscanParam;
}

int SystemControlClient::factorySetNolineParams(int inputSrc, int sigFmt, int transFmt, int type, int osd0_value, int osd25_value,
                            int osd50_value, int osd75_value, int osd100_value) {
    return mSysCtrl->factorySetNolineParams(inputSrc, sigFmt, transFmt, type, osd0_value, osd25_value,
                                            osd50_value, osd75_value, osd100_value);
}

noline_params_t SystemControlClient::factoryGetNolineParams(int inputSrc, int sigFmt, int transFmt, int type) {
    noline_params_t nolineParam;
    memset(&nolineParam, 0, sizeof(noline_params_t));
    mSysCtrl->factoryGetNolineParams(inputSrc, sigFmt, transFmt, type, [&](const NolineParam& param) {
        nolineParam.osd0 = param.osd0;
        nolineParam.osd25 = param.osd25;
        nolineParam.osd50 = param.osd50;
        nolineParam.osd75 = param.osd75;
        nolineParam.osd100 = param.osd100;
    });

    return nolineParam;
}

int SystemControlClient::factoryfactoryGetColorTemperatureParams(int colorTemp_mode){
    return mSysCtrl->factoryfactoryGetColorTemperatureParams(colorTemp_mode);
}

int SystemControlClient::factorySetParamsDefault(void) {
    return mSysCtrl->factorySetParamsDefault();
}

int SystemControlClient::factorySSMRestore(void) {
    return mSysCtrl->factorySSMRestore();
}

int SystemControlClient::factoryResetNonlinear(void) {
    return mSysCtrl->factoryResetNonlinear();
}

int SystemControlClient::factorySetGamma(int gamma_r, int gamma_g, int gamma_b) {
    return mSysCtrl->factorySetGamma(gamma_r, gamma_g, gamma_b);
}

int SystemControlClient::sysSSMReadNTypes(int id, int data_len, int offset) {
    return mSysCtrl->sysSSMReadNTypes(id, data_len, offset);
}

int SystemControlClient::sysSSMWriteNTypes(int id, int data_len, int data_buf, int offset) {
    return mSysCtrl->sysSSMWriteNTypes(id, data_len, data_buf, offset);
}

int SystemControlClient::getActualAddr(int id) {
    return mSysCtrl->getActualAddr(id);
}

int SystemControlClient::getActualSize(int id) {
    return mSysCtrl->getActualSize(id);
}

int SystemControlClient::SSMRecovery(void) {
    return mSysCtrl->SSMRecovery();
}

int SystemControlClient::setPLLValues(source_input_param_t srcInputParam) {
    SourceInputParam hidlSrcInput;
    memcpy(&hidlSrcInput, &srcInputParam, sizeof(source_input_param_t));
    return mSysCtrl->setPLLValues(hidlSrcInput);
}

int SystemControlClient::setCVD2Values(void) {
    return mSysCtrl->setCVD2Values();
}

int SystemControlClient::getSSMStatus(void) {
    return mSysCtrl->getSSMStatus();
}

int SystemControlClient::setCurrentHdrInfo(int32_t hdrInfo) {
    return mSysCtrl->setCurrentHdrInfo(hdrInfo);
}

int SystemControlClient::setCurrentSourceInfo(int32_t sourceInput, int32_t sigFmt, int32_t transFmt) {
    return mSysCtrl->setCurrentSourceInfo(sourceInput, sigFmt, transFmt);
}

void SystemControlClient::getCurrentSourceInfo(int32_t &sourceInput, int32_t &sigFmt, int32_t &transFmt)
{
    mSysCtrl->getCurrentSourceInfo([&](const Result &ret, const SourceInputParam &hidlSrcInput) {
        if (Result::OK == ret) {
            sourceInput = hidlSrcInput.sourceInput;
            sigFmt = hidlSrcInput.sigFmt;
            transFmt = hidlSrcInput.transFmt;
        }
    });
}

int SystemControlClient::setwhiteBalanceGainRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value) {
    return mSysCtrl->setwhiteBalanceGainRed(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
}

int SystemControlClient::setwhiteBalanceGainGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value) {
    return mSysCtrl->setwhiteBalanceGainGreen(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
}

int SystemControlClient::setwhiteBalanceGainBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value) {
    return mSysCtrl->setwhiteBalanceGainBlue(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
}

int SystemControlClient::setwhiteBalanceOffsetRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value) {
    return mSysCtrl->setwhiteBalanceOffsetRed(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
}

int SystemControlClient::setwhiteBalanceOffsetGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value) {
    return mSysCtrl->setwhiteBalanceOffsetGreen(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
}

int SystemControlClient::setwhiteBalanceOffsetBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value) {
    return mSysCtrl->setwhiteBalanceOffsetBlue(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
}

int SystemControlClient::getwhiteBalanceGainRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode) {
    return mSysCtrl->getwhiteBalanceGainRed(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
}

int SystemControlClient::getwhiteBalanceGainGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode) {
    return mSysCtrl->getwhiteBalanceGainGreen(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
}

int SystemControlClient::getwhiteBalanceGainBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode) {
    return mSysCtrl->getwhiteBalanceGainBlue(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
}

int SystemControlClient::getwhiteBalanceOffsetRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode) {
    return mSysCtrl->getwhiteBalanceOffsetRed(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
}

int SystemControlClient::getwhiteBalanceOffsetGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode) {
    return mSysCtrl->getwhiteBalanceOffsetGreen(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
}

int SystemControlClient::getwhiteBalanceOffsetBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode) {
    return mSysCtrl->getwhiteBalanceOffsetBlue(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
}

int SystemControlClient:: saveWhiteBalancePara(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colorTemp_mode,
                                                       int32_t r_gain, int32_t g_gain, int32_t b_gain, int32_t r_offset, int32_t g_offset, int32_t b_offset) {
    return mSysCtrl->saveWhiteBalancePara(inputSrc, sig_fmt, trans_fmt, colorTemp_mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
}

int SystemControlClient::getRGBPattern() {
    return mSysCtrl->getRGBPattern();
}

int SystemControlClient::setRGBPattern(int32_t r, int32_t g, int32_t b) {
    return mSysCtrl->setRGBPattern(r, g, b);
}

int SystemControlClient::factorySetDDRSSC(int32_t step) {
    return mSysCtrl->factorySetDDRSSC(step);
}

int SystemControlClient::factoryGetDDRSSC() {
    return mSysCtrl->factoryGetDDRSSC();
}

int SystemControlClient::factorySetLVDSSSC(int32_t step) {
    return mSysCtrl->factorySetLVDSSSC(step);
}

int SystemControlClient::factoryGetLVDSSSC() {
    return mSysCtrl->factoryGetLVDSSSC();
}

int SystemControlClient::whiteBalanceGrayPatternClose() {
    return mSysCtrl->whiteBalanceGrayPatternClose();
}

int SystemControlClient::whiteBalanceGrayPatternOpen() {
    return mSysCtrl->whiteBalanceGrayPatternOpen();
}

int SystemControlClient::whiteBalanceGrayPatternSet(int32_t value) {
    return mSysCtrl->whiteBalanceGrayPatternSet(value);
}

int SystemControlClient::whiteBalanceGrayPatternGet() {
    return mSysCtrl->whiteBalanceGrayPatternGet();
}

int SystemControlClient::factorySetHdrMode(int mode) {
    return mSysCtrl->factorySetHdrMode(mode);
}

int SystemControlClient::factoryGetHdrMode(void) {
    return mSysCtrl->factoryGetHdrMode();
}

int SystemControlClient::setDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level) {
    return mSysCtrl->setDnlpParams(inputSrc, sigFmt, transFmt, level);
}

int SystemControlClient::getDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt) {
    return mSysCtrl->getDnlpParams(inputSrc, sigFmt, transFmt);
}

int SystemControlClient::factorySetDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level, int final_gain) {
    return mSysCtrl->factorySetDnlpParams(inputSrc, sigFmt, transFmt, level, final_gain);
}

int SystemControlClient::factoryGetDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level) {
    return mSysCtrl->factoryGetDnlpParams(inputSrc, sigFmt, transFmt, level);
}

int SystemControlClient::factorySetBlackExtRegParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int val) {
    return mSysCtrl->factorySetBlackExtRegParams(inputSrc, sigFmt, transFmt, val);
}

int SystemControlClient::factoryGetBlackExtRegParams(int inputSrc, int32_t sigFmt, int32_t transFmt) {
    return mSysCtrl->factoryGetBlackExtRegParams(inputSrc, sigFmt, transFmt);
}

int SystemControlClient::factorySetColorParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int color_type, int color_param, int val) {
    return mSysCtrl->factorySetColorParams(inputSrc, sigFmt, transFmt, color_type, color_param, val);
}
int SystemControlClient::factoryGetColorParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int color_type, int color_param) {
    return mSysCtrl->factoryGetColorParams(inputSrc, sigFmt, transFmt, color_type, color_param);
}

int SystemControlClient::factorySetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type, int val) {
    return mSysCtrl->factorySetNoiseReductionParams(inputSrc, sig_fmt, trans_fmt, nr_mode, param_type, val);
}

int SystemControlClient::factoryGetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type) {
    return mSysCtrl->factoryGetNoiseReductionParams(inputSrc, sig_fmt, trans_fmt, nr_mode, param_type);
}

int SystemControlClient::factorySetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val) {
    return mSysCtrl->factorySetCTIParams(inputSrc, sig_fmt, trans_fmt, param_type, val);
}

int SystemControlClient::factoryGetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type) {
    return mSysCtrl->factoryGetCTIParams(inputSrc, sig_fmt, trans_fmt, param_type);
}

int SystemControlClient::factorySetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val) {
    return mSysCtrl->factorySetDecodeLumaParams(inputSrc, sig_fmt, trans_fmt, param_type, val);
}
int SystemControlClient::factoryGetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type) {
    return mSysCtrl->factoryGetDecodeLumaParams(inputSrc, sig_fmt, trans_fmt, param_type);
}

int SystemControlClient::factorySetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD, int param_type, int val) {
    return mSysCtrl->factorySetSharpnessParams(inputSrc, sig_fmt, trans_fmt, isHD, param_type, val);
}

int SystemControlClient::factoryGetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD,int param_type) {
    return mSysCtrl->factoryGetSharpnessParams(inputSrc, sig_fmt, trans_fmt, isHD, param_type);
}

PQDatabaseInfo SystemControlClient::getPQDatabaseInfo(int32_t dataBaseName) {
    PQDatabaseInfo pqdatabaseinfo;
    mSysCtrl->getPQDatabaseInfo(dataBaseName, [&](const Result &ret, const PQDatabaseInfo& Info) {
        if (Result::OK == ret) {
            pqdatabaseinfo.ToolVersion = Info.ToolVersion;
            pqdatabaseinfo.ProjectVersion = Info.ProjectVersion;
            pqdatabaseinfo.GenerateTime = Info.GenerateTime;
        }
   });
   return pqdatabaseinfo;
}

int SystemControlClient::setDtvKitSourceEnable(int isEnable) {
    return mSysCtrl->setDtvKitSourceEnable(isEnable);
}

//PQ end

//FBC
int SystemControlClient::StartUpgradeFBC(const std::string&file_name, int mode, int upgrade_blk_size) {
    return mSysCtrl->StartUpgradeFBC(file_name, mode, upgrade_blk_size);
}

int SystemControlClient::UpdateFBCUpgradeStatus(int state, int param)
{
    return mSysCtrl->UpdateFBCUpgradeStatus(state, param);
}


void SystemControlClient::setListener(const sp<SysCtrlListener> &listener)
{
    mListener = listener;
}

// callback from systemcontrol service
Return<void> SystemControlClient::SystemControlHidlCallback::notifyCallback(const int event)
{
    ALOGI("notifyCallback event type:%d", event);
    sp<SysCtrlListener> listener;
    {
        Mutex::Autolock _l(mLock);
        listener = SysCtrlClient->mListener;
    }


    if (listener != NULL) {
        listener->notify(event);
    }
    return Void();
}

Return<void> SystemControlClient::SystemControlHidlCallback::notifyFBCUpgradeCallback(int state, int param) {
    ALOGI("notifyFBCUpgradeCallback state:%d, param:%d", state, param);
    sp<SysCtrlListener> listener;
    {
        Mutex::Autolock _l(mLock);
        listener = SysCtrlClient->mListener;
    }


    if (listener != NULL) {
        listener->notifyFBCUpgrade(state, param);
    }

    return Void();
}

void SystemControlClient::setDisplayModeListener(const sp<SysCtrlListener> &listener)
{
    mDisplayListener = listener;
}

Return<void> SystemControlClient::SystemControlHidlCallback::notifySetDisplayModeCallback(int mode) {
    ALOGI("notifySetDisplayModeCallback mode:%d", mode);
    sp<SysCtrlListener> listener;
    {
        Mutex::Autolock _l(mLock);
        listener = SysCtrlClient->mDisplayListener;
    }


    if (listener != NULL) {
        listener->onSetDisplayMode(mode);
    }

    return Void();
}

void SystemControlClient::SystemControlDeathRecipient::serviceDied(uint64_t cookie,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
    LOG(ERROR) << "system control service died. need release some resources";
    Mutex::Autolock _l(mLock);
    if (mClient != nullptr) {
        mClient->connect();
    }
}

}; // namespace android
