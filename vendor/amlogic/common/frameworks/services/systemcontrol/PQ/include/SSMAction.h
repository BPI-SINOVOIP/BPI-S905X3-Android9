/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __SSM_ACTION_H__
#define __SSM_ACTION_H__

#include "PQType.h"
#include "SSMHandler.h"
#include "PQSettingCfg.h"

#define SSM_DATA_PATH             "/mnt/vendor/param/pq/ssm_data"
#define SSM_RGBOGO_FILE_PATH      "/dev/block/cri_data"
#define SSM_RGBOGO_FILE_OFFSET    (0)

#define SSM_CR_RGBOGO_LEN                           (256)
#define SSM_CR_RGBOGO_CHKSUM_LEN                    (2)
#define DEFAULT_BACKLIGHT_BRIGHTNESS                (10)

#define CRI_DATE_GAMMA_OFFSET                       (512)
#define CRI_DATE_GAMMA_RGB_SIZE                     (257)
#define CRI_DATE_GAMMA_R_index                      (1)
#define CRI_DATE_GAMMA_G_index                      (2)
#define CRI_DATE_GAMMA_B_index                      (3)
#define CRI_DATE_GAMMA_COOL                         (1)
#define CRI_DATE_GAMMA_STANDARD                     (2)
#define CRI_DATE_GAMMA_WARM                         (3)

class SSMAction {
public:
    SSMAction();
    ~SSMAction();
    void init();
    int SaveBurnWriteCharaterChar(int rw_val);
    int ReadBurnWriteCharaterChar();
    int DeviceMarkCheck();
    int RestoreDeviceMarkValues();
    static SSMAction *getInstance();
    bool isFileExist(const char *file_name);
    int WriteBytes(int offset, int size, int *buf);
    int ReadBytes(int offset, int size, int *buf);
    int EraseAllData(void);
    int GetSSMActualAddr(int id);
    int GetSSMActualSize(int id);
    int GetSSMStatus(void);
    int SSMReadNTypes(int id, int data_len, int *data_buf, int offset = 0);
    int SSMWriteNTypes(int id, int data_len, int *data_buf, int offset = 0);

    bool SSMRecovery();
    int  SSMRestoreDefault(int id, bool resetAll = true);

    //PQ mode
    int SSMSavePictureMode(int offset, int rw_val);
    int SSMReadPictureMode(int offset, int *rw_val);
    //Color Temperature
    int SSMSaveColorTemperature(int offset, int rw_val);
    int SSMReadColorTemperature(int offset, int *rw_val);
    int SSMSaveColorDemoMode(unsigned char rw_val);
    int SSMReadColorDemoMode(unsigned char *rw_val);
    int SSMSaveColorBaseMode(unsigned char rw_val);
    int SSMReadColorBaseMode(unsigned char *rw_val);
    int SSMSaveRGBGainRStart(int offset, unsigned int rw_val);
    int SSMReadRGBGainRStart(int offset, unsigned int *rw_val);
    int SSMSaveRGBGainGStart(int offset, unsigned int rw_val);
    int SSMReadRGBGainGStart(int offset, unsigned int *rw_val);
    int SSMSaveRGBGainBStart(int offset, unsigned int rw_val);
    int SSMReadRGBGainBStart(int offset, unsigned int *rw_val);
    int SSMSaveRGBPostOffsetRStart(int offset, int rw_val);
    int SSMReadRGBPostOffsetRStart(int offset, int *rw_val);
    int SSMSaveRGBPostOffsetGStart(int offset, int rw_val);
    int SSMReadRGBPostOffsetGStart(int offset, int *rw_val);
    int SSMSaveRGBPostOffsetBStart(int offset, int rw_val);
    int SSMReadRGBPostOffsetBStart(int offset, int *rw_val);
    int SSMReadRGBOGOValue(int offset, int size, unsigned char data_buf[]);
    int SSMSaveRGBOGOValue(int offset, int size, unsigned char data_buf[]);
    int SSMSaveRGBValueStart(int offset, int8_t rw_val);
    int SSMReadRGBValueStart(int offset, int8_t *rw_val);
    int SSMSaveColorSpaceStart(unsigned char rw_val);
    int SSMReadColorSpaceStart(unsigned char *rw_val);
    int ReadDataFromFile(const char *file_name, int offset, int nsize, unsigned char data_buf[]);
    int SaveDataToFile(const char *file_name, int offset, int nsize, unsigned char data_buf[]);
    //Brightness
    int SSMSaveBrightness(int offset, int rw_val);
    int SSMReadBrightness(int offset, int *rw_val);
    //constract
    int SSMSaveContrast(int offset, int rw_val);
    int SSMReadContrast(int offset, int *rw_val);
    //saturation
    int SSMSaveSaturation(int offset, int rw_val);
    int SSMReadSaturation(int offset, int *rw_val);
    //hue
    int SSMSaveHue(int offset, int rw_val);
    int SSMReadHue(int offset, int *rw_val);
    //Sharpness
    int SSMSaveSharpness(int offset, int rw_val);
    int SSMReadSharpness(int offset, int *rw_val);
    //NoiseReduction
    int SSMSaveNoiseReduction(int offset, int rw_val);
    int SSMReadNoiseReduction(int offset, int *rw_val);
    //Gamma
    int SSMSaveGammaValue(int offset, int rw_val);
    int SSMReadGammaValue(int offset, int *rw_val);
    //Edge enhance
    int SSMSaveEdgeEnhanceStatus(int offset, int rw_val);
    int SSMReadEdgeEnhanceStatus(int offset, int *rw_val);
    //mpeg NR
    int SSMSaveMpegNoiseReduction(int offset, int rw_val);
    int SSMReadMpegNoiseReduction(int offset, int *rw_val);
    //Dynamic contrast
    int SSMSaveDynamicContrast(int offset, int rw_val);
    int SSMReadDynamicContrast(int offset, int *rw_val);
    //Dynamic Backlight
    int SSMSaveDynamicBacklightMode(int rw_val);
    int SSMReadDynamicBacklightMode(int *rw_val);
    //backlight
    int SSMReadBackLightVal(int *rw_val);
    int SSMSaveBackLightVal(int rw_val);
    int SSMSaveDnlpMode(int offset, int rw_val);
    int SSMReadDnlpMode(int offset, int *rw_val);
    int SSMSaveDnlpGainValue(int offset, int rw_val);
    int SSMReadDnlpGainValue(int offset, int *rw_val);
    int SSMSaveEyeProtectionMode(int rw_val);
    int SSMReadEyeProtectionMode(int *rw_val);
    int SSMSaveDDRSSC(unsigned char rw_val);
    int SSMReadDDRSSC(unsigned char *rw_val);
    int SSMSaveLVDSSSC(int *rw_val);
    int SSMReadLVDSSSC(int *rw_val);

    int SSMSaveDisplayMode(int offset, int rw_val);
    int SSMReadDisplayMode(int offset, int *rw_val);
    int SSMSaveAutoAspect(int offset, int rw_val);
    int SSMReadAutoAspect(int offset, int *rw_val);
    int SSMSave43Stretch(int offset, int rw_val);
    int SSMRead43Stretch(int offset, int *rw_val);
    int SSMEdidRestoreDefault(int rw_val);
    int SSMHdcpSwitcherRestoreDefault(int rw_val);
    int SSMSColorRangeModeRestoreDefault(int rw_val);
    int SSMSaveLocalContrastMode(int offset, int rw_val);
    int SSMReadLocalContrastMode(int offset, int *rw_val);

    int m_dev_fd;
    static SSMAction *mInstance;
    static SSMHandler *mSSMHandler;
    class ISSMActionObserver {
        public:
            ISSMActionObserver() {};
            virtual ~ISSMActionObserver() {};
            virtual void resetAllUserSettingParam() {};
    };

    void setObserver (ISSMActionObserver *pOb)
    {
        mpObserver = pOb;
    };
private:
    ISSMActionObserver *mpObserver;
};
#endif
