/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

#ifndef _HDMI_CEC_CONTROL_CPP_
#define _HDMI_CEC_CONTROL_CPP_

#include <pthread.h>
#include <HdmiCecBase.h>
#include <CMsgQueue.h>
#include "SystemControlClient.h"
#include "TvServerHidlClient.h"
#include <utils/StrongPointer.h>
#include "EARCSupportMonitor.h"

#define CEC_FILE        "/dev/cec"
#define MAX_PORT        32
#define MESSAGE_SET_MENU_LANGUAGE 0x32

#define CEC_IOC_MAGIC                   'C'
#define CEC_IOC_GET_PHYSICAL_ADDR       _IOR(CEC_IOC_MAGIC, 0x00, uint16_t)
#define CEC_IOC_GET_VERSION             _IOR(CEC_IOC_MAGIC, 0x01, int)
#define CEC_IOC_GET_VENDOR_ID           _IOR(CEC_IOC_MAGIC, 0x02, uint32_t)
#define CEC_IOC_GET_PORT_INFO           _IOR(CEC_IOC_MAGIC, 0x03, int)
#define CEC_IOC_GET_PORT_NUM            _IOR(CEC_IOC_MAGIC, 0x04, int)
#define CEC_IOC_GET_SEND_FAIL_REASON    _IOR(CEC_IOC_MAGIC, 0x05, uint32_t)
#define CEC_IOC_SET_OPTION_WAKEUP       _IOW(CEC_IOC_MAGIC, 0x06, uint32_t)
#define CEC_IOC_SET_OPTION_ENALBE_CEC   _IOW(CEC_IOC_MAGIC, 0x07, uint32_t)
#define CEC_IOC_SET_OPTION_SYS_CTRL     _IOW(CEC_IOC_MAGIC, 0x08, uint32_t)
#define CEC_IOC_SET_OPTION_SET_LANG     _IOW(CEC_IOC_MAGIC, 0x09, uint32_t)
#define CEC_IOC_GET_CONNECT_STATUS      _IOR(CEC_IOC_MAGIC, 0x0A, uint32_t)
#define CEC_IOC_ADD_LOGICAL_ADDR        _IOW(CEC_IOC_MAGIC, 0x0B, uint32_t)
#define CEC_IOC_CLR_LOGICAL_ADDR        _IOW(CEC_IOC_MAGIC, 0x0C, uint32_t)
#define CEC_IOC_SET_DEV_TYPE            _IOW(CEC_IOC_MAGIC, 0x0D, uint32_t)
#define CEC_IOC_SET_ARC_ENABLE          _IOW(CEC_IOC_MAGIC, 0x0E, uint32_t)
#define CEC_IOC_SET_AUTO_DEVICE_OFF     _IOW(CEC_IOC_MAGIC, 0x0F, uint32_t)
#define CEC_IOC_GET_BOOT_ADDR           _IOW(CEC_IOC_MAGIC, 0x10, uint32_t)
#define CEC_IOC_GET_BOOT_REASON         _IOW(CEC_IOC_MAGIC, 0x11, uint32_t)
#define CEC_IOC_GET_BOOT_PORT           _IOW(CEC_IOC_MAGIC, 0x13, uint32_t)

#define CEC_WAKEUP      8

#define DEV_TYPE_TV                     0
#define DEV_TYPE_RECORDER               1
#define DEV_TYPE_RESERVED               2
#define DEV_TYPE_TUNER                  3
#define DEV_TYPE_PLAYBACK               4
#define DEV_TYPE_AUDIO_SYSTEM           5
#define DEV_TYPE_PURE_CEC_SWITCH        6
#define DEV_TYPE_VIDEO_PROCESSOR        7


#define ADDR_BROADCAST  15
#define DELAY_TIMEOUT_MS  5000

#define HDMIRX_SYSFS                   "/sys/class/hdmirx/hdmirx0/cec"
#define AUTO_WAKEUP_OFF_CEC_ON_BOOT    "2" //0x02 0xAB: tv_auto_power_on, B: hdmi_cec_en
#define AUTO_WAKEUP_OFF_CEC_ON         "1" //0x01
#define AUTO_WAKEUP_OFF_CEC_OFF        "0" //0x00
#define AUTO_WAKEUP_ON_CEC_ON_BOOT     "18" //0x12
#define AUTO_WAKEUP_ON_CEC_ON          "17" //0x11
#define AUTO_WAKEUP_ON_CEC_OFF         "16" //0x00

namespace android {

//must sync with hardware\libhardware\include\hardware\Hdmi_cec.h
enum {
    /* Option 4 not used */
    HDMI_OPTION_CEC_AUTO_DEVICE_OFF = 4,
};

/*
 * HDMI CEC messages para value
 */
enum cec_message_para_value{
    CEC_KEYCODE_UP = 0x01,
    CEC_KEYCODE_DOWN = 0x02,
    CEC_KEYCODE_LEFT = 0x03,
    CEC_KEYCODE_RIGHT = 0x04,
    CEC_KEYCODE_POWER = 0x40,
    CEC_KEYCODE_ROOT_MENU = 0x09,
    CEC_KEYCODE_POWER_ON_FUNCTION = 0x6D
};

enum send_message_result{
    SUCCESS = 0,
    NACK = 1, // not acknowledged
    BUSY = 2, // bus is busy
    FAIL = 3,
};

enum power_status{
    POWER_STATUS_UNKNOWN = -1,
    POWER_STATUS_ON = 0,
    POWER_STATUS_STANDBY = 1,
    POWER_STATUS_TRANSIENT_TO_ON = 2,
    POWER_STATUS_TRANSIENT_TO_STANDBY = 3,
};

enum hdmi_cec_option{
    HDMI_CONTROL_AUTO_WAKEUP_ENABLED = 1,
    HDMI_CONTROL_ENABLED = 2,
    HDMI_SYSTEM_AUDIO_CONTROL_ENABLED = 3,
    HDMI_CONTROL_AUTO_DEVICE_OFF_ENABLED =4,
    HDMI_OPTION_SET_LANG = 5,
    HDMI_CONTROL_AUTO_POWER_ON = 8,
    HDMI_OPTION_SERVICE_FLAG = 16,
};

/**
 * struct for information of cec device.
 * @mDeviceType      Indentify type of cec device, such as TV or BOX.
 * @mAddrBitmap      Bit maps for each valid logical address
 * @mFd              File descriptor for global read/write
 * @isCecEnabled     Flag of HDMI_OPTION_ENABLE_CEC,
 * can't do anything when value is false.
 * @isCecControlled  Flag of HDMI_OPTION_SYSTEM_CEC_CONTROL,
 * android system will stop handling CEC service when vaule is false.
 * @mTotalPort       Total ports of HDMI
 * @mpPortData       Array of HDMI ports
 * @mThreadId        pthread for polling cec rx message
 * @Run              Flag for rx polling thread
 * @mExited          Whether the thread is exited
 * @mExtendControl   Flag for extend cec device
 */
typedef struct hdmi_device {
    int                         *mDeviceTypes;
    int                         *mAddedPhyAddrs;
    int                         mTotalDevice;
    bool                        isTvDeviceType;
    bool                        isPlaybackDeviceType;
    bool                        isAvrDeviceType;
    int                         mAddrBitmap;
    int                         mLogicalAddress;
    int                         mFd;
    bool                        isCecEnabled;
    bool                        isCecControlled;
    unsigned int                mConnectStatus;
    int                         mTotalPort;
    hdmi_port_info_t            *mpPortData;
    pthread_t                   mThreadId;
    bool                        mRun;
    bool                        mExited;
    int                         mExtendControl;
    bool                        mFilterOtpEnabled;
    bool                        mWaitEdidUpdatedFinished;
    int                         mSelectedDeviceId;
    int                         mTvOsdName;
    int                         mTvDeviceVendorId;
    int                         mActiveLogicalAddr;
    int                         mActiveRoutingPath;
} hdmi_device_t;

class HdmiCecControl : public HdmiCecBase {
public:
    HdmiCecControl();
    ~HdmiCecControl();

    virtual int openCecDevice();
    virtual int closeCecDevice();

    virtual int getVersion(int* version);
    virtual int getVendorId(uint32_t* vendorId);
    virtual int getPhysicalAddress(uint16_t* addr);
    virtual int sendMessage(const cec_message_t* message, bool isExtend);

    virtual void getPortInfos(hdmi_port_info_t* list[], int* total);
    virtual int addLogicalAddress(cec_logical_address_t address);
    virtual void clearLogicaladdress();
    virtual void setOption(int flag, int value);
    virtual void setAudioReturnChannel(int port, bool flag);
    virtual bool isConnected(int port);
    virtual int getCecWakePort();

    void setEventObserver(const sp<HdmiCecEventListener> &eventListener);
protected:
    class MsgHandler: public CMsgQueueThread {
    public:
        static const int MSG_FILTER_OTP_TIMEOUT = 0;
        static const int GET_MENU_LANGUAGE = 1;
        static const int SET_MENU_LANGUAGE = 2;
        static const int GIVE_OSD_NAEM = 3;
        static const int SET_OSD_NAEM = 4;
        static const int MSG_WAIT_EDIDUPDATE_FINISHED_TIMEOUT = 5;
        static const int MSG_GIVE_PHYSICAL_ADDRESS = 6;
        static const int MSG_SET_STREAM_PATH = 7;
        static const int MSG_USER_CONTROL_PRESSED = 8;
        static const int MSG_TURN_ON_DEVICE = 9;
        static const int MSG_GIVE_DEVICE_VENDOR_ID = 11;

        MsgHandler(HdmiCecControl *hdmiControl);
        ~MsgHandler();
    private:
        virtual void handleMessage (CMessage &msg);
        HdmiCecControl *mControl;
    };
public:
    class TvEventListner: public TvListener {
    public:
        static const int TV_EVENT_SOURCE_SWITCH = 506;
        TvEventListner(HdmiCecControl *hdmiControl);
        ~TvEventListner();
        virtual void notify(const tv_parcel_t &parcel);
    private:
        HdmiCecControl *mControl;
    };
private:
    void init();
    void getDeviceTypes();
    void getBootConnectStatus();
    static void *__threadLoop(void *data);
    void threadLoop();

    int sendExtMessage(const cec_message_t* message);
    int sendMessage(int initiator, int destination, int length, cec_message_type type);
    int send(const cec_message_t* message);
    int readMessage(unsigned char *buf, int msgCount);
    void checkConnectStatus();

    bool assertHdmiCecDevice();
    bool hasHandledByExtend(const cec_message_t* message);
    int preHandleOfSend(const cec_message_t* message);
    int postHandleOfSend(const cec_message_t* message, int result);
    bool isWakeUpMsg(char *msgBuf, int len);
    void messageValidateAndHandle(hdmi_cec_event_t* event);
    void supportForEARC(hdmi_cec_event_t* event, cec_message_t* message, bool send);
    void handleOTPMsg(hdmi_cec_event_t* event);
    void handleSetMenuLanguage(hdmi_cec_event_t* event);
    void handleHotplug(int port, bool connected);
    void turnOnDevice(int logicalAddress);
    void getDeviceExtraInfo(int flag);
    void optionInit();
    void handleHdmiHpdStatus(int flag);

    hdmi_device_t mCecDevice;
    sp<HdmiCecEventListener> mEventListener;
    sp<SystemControlClient> mSystemControl;
    sp<TvServerHidlClient> mTvSession;
    sp<HdmiCecControl::TvEventListner> mTvEventListner;
    MsgHandler mMsgHandler;
    EARCSupportMonitor *pMonitor;
    mutable Mutex mLock;
    mutable Mutex mSendLock;
};


}; //namespace android

#endif /* _HDMI_CEC_CONTROL_CPP_ */
