/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvSetting"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cutils/properties.h>

#include <netinet/ether.h>
#include <netinet/if_ether.h>

#include <netutils/ifc.h>

#include "CTvSetting.h"

#include <tvconfig.h>
#include <tvutils.h>
#include <CTvLog.h>
#include "../vpp/CVpp.h"

#define CC_DEF_CHARACTER_CHAR_VAL                   (0x8A)

pthread_mutex_t ssm_r_w_op_mutex = PTHREAD_MUTEX_INITIALIZER;

/************************ Start APIs For UI ************************/
int TVSSMWriteNTypes(int id, int data_len, int data_buf, int offset)
{
    return tvSSMWriteNTypes(id, data_len, data_buf, offset);
}

int TVSSMReadNTypes(int id, int data_len, int *data_buf, int offset)
{
    return tvSSMReadNTypes(id, data_len, data_buf, offset);
}

/************************ Start APIs For UI ************************/
int MiscSSMRestoreDefault()
{
    SSMSaveFactoryBurnMode(0);
    SSMSavePowerOnOffChannel(1);
    SSMSaveSystemLanguage(0);
    SSMSaveAgingMode(0);
    SSMSavePanelType(0);
    SSMSavePowerOnMusicSwitch(0);
    SSMSavePowerOnMusicVolume(20);
    SSMSaveSystemSleepTimer(0xFFFFFFFF);
    SSMSaveInputSourceParentalControl(0, 0);
    SSMSaveParentalControlSwitch(0);
    SSMSaveBlackoutEnable(0);
    return 0;
}

int MiscSSMFacRestoreDefault()
{
    SSMSaveSystemLanguage(0);
    SSMSavePowerOnMusicSwitch(1);
    SSMSavePowerOnMusicVolume(20);
    SSMSaveSystemSleepTimer(0xFFFFFFFF);
    SSMSaveInputSourceParentalControl(0, 0);
    SSMSaveParentalControlSwitch(0);
    SSMSaveSearchNavigateFlag(1);
    SSMSaveInputNumLimit(2);
    SSMSaveLocalDimingOnOffFlg(0);

    return 0;
}

int ReservedSSMRestoreDefault()
{
    return SSMSaveBurnWriteCharaterChar(CC_DEF_CHARACTER_CHAR_VAL);
}

int SSMSaveBurnWriteCharaterChar(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RSV_W_CHARACTER_CHAR_START, 1, rw_val);
}

int SSMReadBurnWriteCharaterChar()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RSV_W_CHARACTER_CHAR_START, 1, &tmp_val) < 0) {
        return -1;
    }

    return tmp_val;
}

int SSMSaveFactoryBurnMode(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_FBMF_START, 1, rw_val);
}

int SSMReadFactoryBurnMode()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_FBMF_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val != 0) {
        return 1;
    }

    return 0;
}

int SSMSavePowerOnOffChannel(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_POWER_CHANNEL_START, 1, rw_val);
}

int SSMReadPowerOnOffChannel()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_POWER_CHANNEL_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSaveLastSelectSourceInput(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_LAST_SOURCE_INPUT_START, 1, rw_val);
}

int SSMReadLastSelectSourceInput()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_LAST_SOURCE_INPUT_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSaveSystemLanguage(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_SYS_LANGUAGE_START, 1, rw_val);
}

int SSMReadSystemLanguage()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_SYS_LANGUAGE_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSaveAgingMode(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_AGING_MODE_START, 4, rw_val);
}

int SSMReadAgingMode()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_AGING_MODE_START, 4, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSavePanelType(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_PANEL_TYPE_START, 1, rw_val);
}

int SSMReadPanelType()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_PANEL_TYPE_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSavePowerOnMusicSwitch(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_POWER_ON_MUSIC_SWITCH_START, 1, rw_val);
}

int SSMReadPowerOnMusicSwitch()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_POWER_ON_MUSIC_SWITCH_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSavePowerOnMusicVolume(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_POWER_ON_MUSIC_VOL_START, 1, rw_val);
}

int SSMReadPowerOnMusicVolume()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_POWER_ON_MUSIC_VOL_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSaveSystemSleepTimer(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_SYS_SLEEP_TIMER_START, 1, rw_val);
}

int SSMReadSystemSleepTimer()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_SYS_SLEEP_TIMER_START, 4, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val < 0) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSaveInputSourceParentalControl(int source_index, unsigned char ctl_flag)
{
    int tmp_val = 0;

    if (source_index < 0 || source_index > 31) {
        return -1;
    }

    if (ctl_flag != 0 && ctl_flag != 1) {
        return -1;
    }

    if (TVSSMReadNTypes(SSM_RW_INPUT_SRC_PARENTAL_CTL_START, 4, &tmp_val) < 0) {
        return -1;
    }

    tmp_val = (tmp_val & (~(1 << source_index))) | (ctl_flag << source_index);

    return TVSSMWriteNTypes(SSM_RW_INPUT_SRC_PARENTAL_CTL_START, 4, tmp_val);
}

int SSMReadInputSourceParentalControl(int source_index)
{
    int tmp_val = 0;

    if (SSMReadParentalControlSwitch() == 0) {
        return 0;
    }

    if (TVSSMReadNTypes(SSM_RW_INPUT_SRC_PARENTAL_CTL_START, 4, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val & (1 << source_index)) {
        return 1;
    }

    return 0;
}

int SSMSaveParentalControlSwitch(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_PARENTAL_CTL_SWITCH_START, 1, rw_val);
}

int SSMReadParentalControlSwitch()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_PARENTAL_CTL_SWITCH_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val != 0) {
        tmp_val = 1;
    }

    return tmp_val;
}

int SSMGetCustomerDataStart()
{
    return tvGetActualAddr(CUSTOMER_DATA_POS_HDMI1_EDID_START);
}

int SSMGetCustomerDataLen()
{
    int LastAddr = tvGetActualAddr(CUSTOMER_DATA_POS_HDMI_HDCP_SWITCHER_START);
    int LastSize = tvGetActualSize(CUSTOMER_DATA_POS_HDMI_HDCP_SWITCHER_START);
    int len = LastAddr - SSMGetCustomerDataStart() + LastSize;

    return len;
}

int SSMGetATVDataStart()
{
    return 0;
}

int SSMGetATVDataLen()
{
    return 0;
}

int SSMGetVPPDataStart()
{
    return tvGetActualAddr(VPP_DATA_POS_COLOR_DEMO_MODE_START);
}

int SSMGetVPPDataLen()
{
    int LastAddr = tvGetActualAddr(SSM_RSV3);
    int LastSize = tvGetActualSize(SSM_RSV3);
    int len = LastAddr - SSMGetVPPDataStart() + LastSize;

    return len;
}

int SSMSaveSearchNavigateFlag(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_SEARCH_NAVIGATE_FLAG_START, 1, rw_val);
}

int SSMReadSearchNavigateFlag()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_SEARCH_NAVIGATE_FLAG_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveInputNumLimit(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_INPUT_NUMBER_LIMIT_START, 1, rw_val);
}

int SSMReadInputNumLimit()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_INPUT_NUMBER_LIMIT_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveLocalDimingOnOffFlg(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_LOCAL_DIMING_START, 1, rw_val);
}

int SSMReadLocalDimingOnOffFlg()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_LOCAL_DIMING_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveVDac2DValue(unsigned short rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_VDAC_2D_START, 1, rw_val);
}

int SSMReadVDac2DValue()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_VDAC_2D_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveVDac3DValue(unsigned short rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_VDAC_3D_START, 1, rw_val);
}

int SSMReadVDac3DValue()
{
    unsigned short tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_VDAC_3D_START, 1, (int *)&tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveChromaStatus(int mode)
{
    int fd = -1, ret = -1;
    char value[20] = "";

    sprintf(value, "%d", mode);

    fd = open("/sys/class/tvafe/tvafe0/cvd_reg8a", O_RDWR);

    if (fd < 0) {
        LOGE("open /sys/class/tvafe/tvafe0/cvd_reg8a ERROR(%s)!!\n",
             strerror(errno));
        return -1;
    }

    ret = write(fd, value, strlen(value));

    close(fd);

    return ret;
}

int SSMSaveNonStandardValue(unsigned short rw_val)
{
    LOGD("%s, save NonStandard_value = %d", CFG_SECTION_TV, rw_val);

    return TVSSMWriteNTypes(SSM_RW_NON_STANDARD_START, 2, rw_val);
}

int SSMReadNonStandardValue(void)
{
    int value = 0;

    if (TVSSMReadNTypes(SSM_RW_NON_STANDARD_START, 2, &value) < 0) {
        LOGE("%s, read NonStandard_value error.", CFG_SECTION_TV);
        return 0;
    }

    LOGD("%s, read NonStandard_value = %d.", CFG_SECTION_TV, value);

    return value;
}

int SSMSaveAdbSwitchValue(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_ADB_SWITCH_START, 1, rw_val);
}

int SSMReadAdbSwitchValue(void)
{
    int switch_val = 0;

    if (TVSSMReadNTypes(SSM_RW_ADB_SWITCH_START, 1, &switch_val) < 0) {
        LOGD("%s, read switch value error", CFG_SECTION_TV);
        return -1;
    }

    LOGD("%s, read switch value = %d", CFG_SECTION_TV, switch_val);

    return switch_val;
}

int SSMSaveNoiseGateThresholdValue(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_NOISE_GATE_THRESHOLD_START, 1, rw_val);
}

int SSMReadNoiseGateThresholdValue(void)
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_NOISE_GATE_THRESHOLD_START, 1, &tmp_val) < 0) {
        LOGD("%s, read NoiseGateThreshold error", CFG_SECTION_TV);
        return -1;
    }

    LOGD("%s, read NoiseGateThreshold = %d", CFG_SECTION_TV, tmp_val);

    return tmp_val;
}

int SSMSaveGraphyBacklight(int rw_val)
{
    if (rw_val < 0 || rw_val > 100) {
        return -1;
    }

    return TVSSMWriteNTypes(SSM_RW_UI_GRHPHY_BACKLIGHT_START, 1, rw_val);
}

int SSMReadGraphyBacklight(void)
{
    int value = 0;

    if (TVSSMReadNTypes(SSM_RW_UI_GRHPHY_BACKLIGHT_START, 1, &value) < 0) {
        LOGD("%s, read graphybacklight error.\n", CFG_SECTION_TV);
        return -1;
    }

    if (value > 100) {
        LOGD("%s, range of graphybacklight (%d) is not between 0-100.\n",
             CFG_SECTION_TV, value);
        return -1;
    }

    return value;
}

int SSMSaveFastSuspendFlag(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_FASTSUSPEND_FLAG_START, 1, rw_val);
}

int SSMReadFastSuspendFlag(void)
{
    int value = 0;

    if (TVSSMReadNTypes(SSM_RW_FASTSUSPEND_FLAG_START, 1, &value) < 0) {
        LOGD("%s, read FastSuspendFlag error.\n", CFG_SECTION_TV);
        return -1;
    }

    return value;
}

int SSMSaveCABufferSizeValue(unsigned short rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_CA_BUFFER_SIZE_START, 2, rw_val);
}

int SSMReadCABufferSizeValue(void)
{
    int data = 0;

    if (TVSSMReadNTypes(SSM_RW_CA_BUFFER_SIZE_START, 2, &data) < 0) {
        LOGE("%s, read ca_buffer_size error", CFG_SECTION_TV);
        return 0;
    }

    return data;
}

int SSMSaveStandbyMode(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_STANDBY_MODE_FLAG_START, 1, rw_val);
}

int SSMReadStandbyMode()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_STANDBY_MODE_FLAG_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveHDMIEQMode(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_HDMIEQ_MODE_START, 1, rw_val);
}

int SSMReadHDMIEQMode()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_HDMIEQ_MODE_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveLogoOnOffFlag(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_LOGO_ON_OFF_FLAG_START, 1, rw_val);
}

int SSMReadLogoOnOffFlag()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_LOGO_ON_OFF_FLAG_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveHDMIInternalMode(unsigned int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_HDMIINTERNAL_MODE_START, 4, rw_val);
}

int SSMReadHDMIInternalMode()
{
    int value;
    if (TVSSMReadNTypes(SSM_RW_HDMIINTERNAL_MODE_START, 4, &value) < 0) {
        return 0;
    }

    return value;
}

int SSMSaveParentalControlPassWord(unsigned char *password, int size)
{
    return TVSSMWriteNTypes(SSM_RW_PARENTAL_CTL_PASSWORD_START, size, (int)*password);
}

int SSMReadParentalControlPassWord(unsigned short  *password)
{

    if (TVSSMReadNTypes(SSM_RW_PARENTAL_CTL_PASSWORD_START, 16, (int *)password) < 0) {
        return -1;
    }

    return 0;
}

int SSMSaveDisable3D(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_DISABLE_3D_START, 1, rw_val);
}

int SSMReadDisable3D()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_DISABLE_3D_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSaveGlobalOgoEnable(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_GLOBAL_OGO_ENABLE_START, 1, rw_val);
}

int SSMReadGlobalOgoEnable()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(SSM_RW_GLOBAL_OGO_ENABLE_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

//Mark r/w values
static const int SSM_MARK_01_VALUE = 0xDD;
static const int SSM_MARK_02_VALUE = 0x88;
static const int SSM_MARK_03_VALUE = 0xCC;

int SSMDeviceMarkCheck()
{
    int i = 0, failed_count = 0;
    int mark_offset[3] = { 0, 0, 0 };
    unsigned char mark_values[3] = { 0, 0, 0 };
    int tmp_ch = 0;

    //read temp one byte
    TVSSMReadNTypes(0, 1, &tmp_ch);

    mark_offset[0] = SSM_MARK_01_START;
    mark_offset[1] = SSM_MARK_02_START;
    mark_offset[2] = SSM_MARK_03_START;

    mark_values[0] = SSM_MARK_01_VALUE;
    mark_values[1] = SSM_MARK_02_VALUE;
    mark_values[2] = SSM_MARK_03_VALUE;

    if (SSMReadBurnWriteCharaterChar() != CC_DEF_CHARACTER_CHAR_VAL) {
        SSMSaveBurnWriteCharaterChar(CC_DEF_CHARACTER_CHAR_VAL);
    }

    failed_count = 0;
    for (i = 0; i < 3; i++) {
        tmp_ch = 0;
        if (TVSSMReadNTypes(mark_offset[i], 1, &tmp_ch) < 0) {
            LOGE("%s, SSMDeviceMarkCheck Read Mark failed!!!\n", CFG_SECTION_TV);
            break;
        }

        if ((unsigned char)tmp_ch != mark_values[i]) {
            failed_count += 1;
            LOGE(
                "%s, SSMDeviceMarkCheck Mark[%d]'s offset = %d, Mark[%d]'s Value = %d, read value = %d.\n",
                CFG_SECTION_TV, i, mark_offset[i], i, mark_values[i], tmp_ch);
        }
    }

    if (failed_count >= 3) {
        return -1;
    }

    return 0;
}

int SSMRestoreDeviceMarkValues()
{
    int i;
    int mark_offset[3] = {
        (int) SSM_MARK_01_START, //
        (int) SSM_MARK_02_START, //
        (int) SSM_MARK_03_START, //
    };

    int mark_values[3] = {SSM_MARK_01_VALUE, SSM_MARK_02_VALUE, SSM_MARK_03_VALUE};

    for (i = 0; i < 3; i++) {
        if (TVSSMWriteNTypes(mark_offset[i], 1, mark_values[i]) < 0) {
            LOGD("SSMRestoreDeviceMarkValues Write Mark failed.\n");
            break;
        }
    }

    if (i < 3) {
        return -1;
    }

    return 0;
}

static int SSMGetPreCopyingEnableCfg()
{
    const char *prop_value;

    prop_value = config_get_str(CFG_SECTION_TV, CFG_SSM_PRECOPY_ENABLE, "null");
    if (strcmp(prop_value, "null") == 0 || strcmp(prop_value, "0") == 0
            || strcmp(prop_value, "disable") == 0) {
        return 0;
    }

    return 1;
}

static int SSMGetPreCopyingDevicePathCfg(char dev_path[])
{
    const char *prop_value;

    if (dev_path == NULL) {
        return -1;
    }

    prop_value = config_get_str(CFG_SECTION_TV, CFG_SSM_PRECOPY_FILE_PATH, "null");
    if (strcmp(prop_value, "null") == 0) {
        return 1;
    }

    strcpy(dev_path, prop_value);

    return 0;
}

static unsigned char gTempDataBuf[4096] = { 0 };
int SSMHandlePreCopying()
{
    int device_fd = -1;
    int i = 0, tmp_size = 0, ret = 0, remove_ret = 0;
    int tmp_ch = 0;
    char tmpPreCopyingDevicePath[256] = { '\0' };

    if (SSMGetPreCopyingEnableCfg() == 0) {
        LOGD("%s, Pre copying is disable now.\n", CFG_SECTION_TV);
        return 0;
    }

    //read temp one byte
    TVSSMReadNTypes(0, 1, &tmp_ch);

    SSMGetPreCopyingDevicePathCfg(tmpPreCopyingDevicePath);

    device_fd = open(tmpPreCopyingDevicePath, O_RDONLY);
    if (device_fd < 0) {
        LOGE("%s, Open device file \"%s\" error: %s.\n", CFG_SECTION_TV,
             tmpPreCopyingDevicePath, strerror(errno));
        return -1;
    }

    tmp_size = lseek(device_fd, 0, SEEK_END);
    if (tmp_size == 4096) {
        lseek(device_fd, 0, SEEK_SET);
        ret = read(device_fd, gTempDataBuf, tmp_size);
        if (ret > 0)
        {
            for (i=0;i<tmp_size;i++) {
                TVSSMWriteNTypes(0, 1, gTempDataBuf[i]);
            }
        }
    }

    close(device_fd);

    remove_ret = remove(tmpPreCopyingDevicePath);
    if (remove_ret == -1)
    {
        return -1;
    }

    return 0;
}

int SSMSaveDTVType(int rw_val)
{
    return TVSSMWriteNTypes(SSM_RW_DTV_TYPE_START, 1, rw_val);
}

int SSMReadDTVType(int *rw_val)
{
    int tmp_ret = 0;

    tmp_ret = TVSSMReadNTypes(SSM_RW_DTV_TYPE_START, 1, rw_val);

    return tmp_ret;
}

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

/************************ End APIs For UI ************************/

// other api
int GetSSMCfgBufferData(const char *key_str, int *buf_item_count, int radix,
                        unsigned char data_buf[])
{
    int cfg_item_count = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    const char *config_value;
    char data_str[CC_CFG_VALUE_STR_MAX_LEN] = { 0 };

    config_value = config_get_str(CFG_SECTION_TV, key_str, "null");
    if (strcasecmp(config_value, "null") == 0) {
        LOGE("%s, can't get config \"%s\"!!!\n", CFG_SECTION_TV, key_str);
        return -1;
    }

    cfg_item_count = 0;

    memset((void *)data_str, 0, sizeof(data_str));
    strncpy(data_str, config_value, sizeof(data_str) - 1);

    char *pSave;
    token = strtok_r(data_str, strDelimit, &pSave);
    while (token != NULL) {
        if (cfg_item_count < *buf_item_count) {
            data_buf[cfg_item_count] = strtol(token, NULL, radix);

            token = strtok_r(NULL, strDelimit, &pSave);
            cfg_item_count += 1;
        } else {
            LOGE("%s, we get data count more than desire count (%d)!!!\n",
                 CFG_SECTION_TV, *buf_item_count);
            return -1;
        }
    }

    *buf_item_count = cfg_item_count;

    return 0;
}

int SSMSaveSourceInput(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(TVIN_DATA_POS_SOURCE_INPUT_START, 1, tmp_val);
}

int SSMReadSourceInput()
{
    int tmp_val = 0;

    if (TVSSMReadNTypes(TVIN_DATA_POS_SOURCE_INPUT_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSaveCVBSStd(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(TVIN_DATA_CVBS_STD_START, 1, tmp_val);
}

int SSMReadCVBSStd(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(TVIN_DATA_CVBS_STD_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSave3DMode(unsigned char rw_val)
{
    int tmp_val = rw_val;

    return TVSSMWriteNTypes(TVIN_DATA_POS_3D_MODE_START, 1, tmp_val);
}

int SSMRead3DMode(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(TVIN_DATA_POS_3D_MODE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSave3DLRSwitch(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(TVIN_DATA_POS_3D_LRSWITCH_START, 1, tmp_val);
}

int SSMRead3DLRSwitch(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(TVIN_DATA_POS_3D_LRSWITCH_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSave3DDepth(unsigned char rw_val)
{
    int tmp_val = rw_val;

    return TVSSMWriteNTypes(TVIN_DATA_POS_3D_DEPTH_START, 1, tmp_val);
}

int SSMRead3DDepth(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(TVIN_DATA_POS_3D_DEPTH_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSave3DTO2D(unsigned char rw_val)
{
    int tmp_val = rw_val;

    return TVSSMWriteNTypes(TVIN_DATA_POS_3D_TO2D_START, 1, tmp_val);
}

int SSMRead3DTO2D(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(TVIN_DATA_POS_3D_TO2D_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveSceneMode(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_SCENE_MODE_START, 1, rw_val);
}

int SSMReadSceneMode(int *rw_val)
{
    int tmp_ret = 0;
    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_SCENE_MODE_START, 1, rw_val);

    return tmp_ret;
}

int SSMSaveFBCN360BackLightVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_BACKLIGHT_START , 1, rw_val);
}

int SSMReadFBCN360BackLightVal(int *rw_val)
{
    int tmp_ret = 0;

    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_FBC_BACKLIGHT_START, 1, rw_val);

    return tmp_ret;
}

int SSMSaveFBCN360ColorTempVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_COLORTEMP_START , 1, rw_val);
}

int SSMReadFBCN360ColorTempVal(int *rw_val)
{
    int tmp_ret = 0;
    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_FBC_COLORTEMP_START, 1, rw_val);

    return tmp_ret;
}


int SSMSaveFBCELECmodeVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_ELECMODE_START , 1, rw_val);
}

int SSMReadFBCELECmodeVal(int *rw_val)
{
    int tmp_ret = 0;
    tmp_ret =  TVSSMReadNTypes(VPP_DATA_POS_FBC_ELECMODE_START, 1, rw_val);

    return tmp_ret;
}

int SSMSaveDBCStart(unsigned char rw_val)
{
    int tmp_val = rw_val;

    return TVSSMWriteNTypes(VPP_DATA_POS_DBC_START, 1, tmp_val);
}

int SSMReadDBCStart(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_POS_DBC_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveDnlpStart(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_POS_DNLP_START, 1, tmp_val);
}

int SSMReadDnlpStart(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_POS_DNLP_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSavePanoramaStart(int offset, unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_POS_PANORAMA_START, 1, tmp_val, offset);
}

int SSMReadPanoramaStart(int offset, unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_POS_PANORAMA_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveTestPattern(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_POS_TEST_PATTERN_START, 1, tmp_val);
}

int SSMReadTestPattern(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = TVSSMReadNTypes(VPP_DATA_POS_TEST_PATTERN_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveAPL(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_APL_START, 1, tmp_val);
}

int SSMReadAPL(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_APL_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveAPL2(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_APL2_START, 1, tmp_val);
}

int SSMReadAPL2(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_APL2_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveBD(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_BD_START, 1, tmp_val);
}

int SSMReadBD(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_BD_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveBP(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_BP_START, 1, tmp_val);
}

int SSMReadBP(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_BP_START, 1, &tmp_val);\
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveDDRSSC(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_POS_DDR_SSC_START, 1, tmp_val);
}

int SSMReadDDRSSC(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_POS_DDR_SSC_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveLVDSSSC(unsigned char *rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_LVDS_SSC_START, 1, *rw_val);
}

int SSMReadLVDSSSC(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_POS_LVDS_SSC_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveDreamPanel(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_POS_DREAM_PANEL_START, 1, tmp_val);
}

int SSMReadDreamPanel(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_POS_DREAM_PANEL_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveUserNatureLightSwitch(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_USER_NATURE_SWITCH_START, 1, tmp_val);
}

int SSMReadUserNatureLightSwitch(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_USER_NATURE_SWITCH_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveDBCBacklightEnable(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_DBC_BACKLIGHT_START, 1, tmp_val);
}

int SSMReadDBCBacklightEnable(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_DBC_BACKLIGHT_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveDBCBacklightStd(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_DBC_STANDARD_START, 1, tmp_val);
}

int SSMReadDBCBacklightStd(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_DBC_STANDARD_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveDBCEnable(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_DBC_ENABLE_START, 1, tmp_val);
}

int SSMReadDBCEnable(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_DBC_ENABLE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveBackLightReverse(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return TVSSMWriteNTypes(VPP_DATA_POS_BACKLIGHT_REVERSE_START, 1, tmp_val);
}

int SSMReadBackLightReverse(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(VPP_DATA_POS_BACKLIGHT_REVERSE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveBlackoutEnable(int8_t enable)
{
    int tmp_val = enable;

    return TVSSMWriteNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, tmp_val);
}

int SSMReadBlackoutEnable(int8_t *enable)
{
    int tmp_val = 0;
    int ret = 0;

    ret = TVSSMReadNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, &tmp_val);
    *enable = tmp_val;

    return ret;
}

int SSMSaveFBCN310BackLightVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_N310_BACKLIGHT_START , 1, rw_val);
}

int SSMReadFBCN310BackLightVal(int *rw_val)
{
    int tmp_ret = 0;

    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_FBC_N310_BACKLIGHT_START, 1, rw_val);

    return tmp_ret;
}

int SSMSaveFBCN310ColorTempVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_N310_COLORTEMP_START , 1, rw_val);
}

int SSMReadFBCN310ColorTempVal(int *rw_val)
{
    int tmp_ret = 0;
    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_FBC_N310_COLORTEMP_START, 1, rw_val);


    return tmp_ret;
}

int SSMSaveFBCN310LightsensorVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START , 1, rw_val);
}

int SSMReadFBCN310LightsensorVal(int *rw_val)
{
    int tmp_ret = 0;
    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START, 1, rw_val);

    return tmp_ret;
}

int SSMSaveFBCN310Dream_PanelVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_N310_DREAMPANEL_START , 1, rw_val);
}

int SSMReadFBCN310Dream_PanelVal(int *rw_val)
{
    int tmp_ret = 0;

    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_FBC_N310_DREAMPANEL_START, 1, rw_val);
    return tmp_ret;
}

int SSMSaveFBCN310MULT_PQVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_N310_MULTI_PQ_START , 1, rw_val);
}

int SSMReadFBCN310MULT_PQVal(int *rw_val)
{
    int tmp_ret = 0;
    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_FBC_N310_MULTI_PQ_START, 1, rw_val);

    return tmp_ret;
}

int SSMSaveFBCN310MEMCVal(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_FBC_N310_MEMC_START , 1, rw_val);
}

int SSMReadFBCN310MEMCVal(int *rw_val)
{
    int tmp_ret = 0;
    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_FBC_N310_MEMC_START, 1, rw_val);

    return tmp_ret;
}

int SSMSaveN311_VbyOne_Spread_Spectrum_Val(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START , 1, rw_val);
}

int SSMReadN311_VbyOne_Spread_Spectrum_Val(int *rw_val)
{
    int tmp_ret = 0;

    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START, 1, rw_val);
    return tmp_ret;
}
int SSMSaveN311_Bluetooth_Vol(int rw_val)
{
    return TVSSMWriteNTypes(VPP_DATA_POS_N311_BLUETOOTH_VAL_START , 1, rw_val);
}

int SSMReadN311_Bluetooth_Vol(void)
{
    int tmp_ret = 0;
    int tmp_val = 0;

    tmp_ret = TVSSMReadNTypes(VPP_DATA_POS_N311_BLUETOOTH_VAL_START, 1, &tmp_val);

    if (tmp_ret < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSave_PANEL_ID_Val(int rw_val)
{
    return  TVSSMWriteNTypes(SSM_RW_PANEL_ID_START , 1, rw_val);
}
int SSMRead_PANEL_ID_Val(void)
{
    int tmp_val = 0;
    int tmp_ret = 0;
    tmp_ret = TVSSMReadNTypes(SSM_RW_PANEL_ID_START, 1, &tmp_val);
    if (tmp_ret < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSaveHDMIEdidVersion(tv_hdmi_port_id_t port, tv_hdmi_edid_version_t rw_val)
{
    int ret = -1;
    int tmp_val = rw_val;
    switch (port) {
        case HDMI_PORT_1 :
            ret = TVSSMWriteNTypes(CUSTOMER_DATA_POS_HDMI1_EDID_START, 1, tmp_val);
            break;
        case HDMI_PORT_2 :
            ret = TVSSMWriteNTypes(CUSTOMER_DATA_POS_HDMI2_EDID_START, 1, tmp_val);
            break;
        case HDMI_PORT_3 :
            ret = TVSSMWriteNTypes(CUSTOMER_DATA_POS_HDMI3_EDID_START, 1, tmp_val);
            break;
        case HDMI_PORT_4 :
            ret = TVSSMWriteNTypes(CUSTOMER_DATA_POS_HDMI4_EDID_START, 1, tmp_val);
        default:
            break;
    }
    return ret;
}

tv_hdmi_edid_version_t SSMReadHDMIEdidVersion(tv_hdmi_port_id_t port)
{
    int tmp_val = 0;
    int tmp_ret = 0;
    switch (port) {
        case HDMI_PORT_1:
            tmp_ret = TVSSMReadNTypes(CUSTOMER_DATA_POS_HDMI1_EDID_START, 1, &tmp_val);
            if (tmp_ret < 0) {
                tmp_val = 0;
            }
            if (1 < tmp_val ) {
                tmp_val = 0;
            }
            break;
        case HDMI_PORT_2 :
            tmp_ret = TVSSMReadNTypes(CUSTOMER_DATA_POS_HDMI2_EDID_START, 1, &tmp_val);
            if (tmp_ret < 0) {
                tmp_val = 0;
            }
            if (1 < tmp_val ) {
                tmp_val = 0;
            }
            break;
        case HDMI_PORT_3 :
            tmp_ret = TVSSMReadNTypes(CUSTOMER_DATA_POS_HDMI3_EDID_START, 1, &tmp_val);
            if (tmp_ret < 0) {
                tmp_val = 0;
            }
            if (1 < tmp_val ) {
                tmp_val = 0;
            }
            break;
        case HDMI_PORT_4:
            tmp_ret = TVSSMReadNTypes(CUSTOMER_DATA_POS_HDMI4_EDID_START, 1, &tmp_val);
            if (tmp_ret < 0) {
                tmp_val = 0;
            }
            if (1 < tmp_val ) {
                tmp_val = 0;
            }
            break;
        default:
            tmp_val = 0;
            break;
    }
    return (tv_hdmi_edid_version_t)tmp_val;
}

int SSMHDMIEdidRestoreDefault(tv_hdmi_edid_version_t edid_version)
{
    int i = 0;
    for (i=1;i<=HDMI_PORT_4;i++) {
        SSMSaveHDMIEdidVersion((tv_hdmi_port_id_t)i, edid_version);
    }

    return 0;
}

int SSMSaveHDMIColorRangeMode(tvin_color_range_t rw_val)
{
    return TVSSMWriteNTypes(CUSTOMER_DATA_POS_HDMI_COLOR_RANGE_START, 1, rw_val);
}

int SSMReadHDMIColorRangeMode(int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = TVSSMReadNTypes(CUSTOMER_DATA_POS_HDMI_COLOR_RANGE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMSaveHDMIHdcpSwitcher(int rw_val)
{
    return TVSSMWriteNTypes(CUSTOMER_DATA_POS_HDMI_HDCP_SWITCHER_START, 1, rw_val);
}

int SSMReadHDMIHdcpSwitcher(void)
{
    int tmp_val = 0;
    int tmp_ret = 0;
    tmp_ret = TVSSMReadNTypes(CUSTOMER_DATA_POS_HDMI_HDCP_SWITCHER_START, 1, &tmp_val);
    if (tmp_ret < 0) {
        tmp_val = 0;
    }
    if (1 < tmp_val ) {
        tmp_val = 0;
    }
    return tmp_val;
}
