/*
 * Copyright (C) 2012 The Android Open Source Project
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
 */

/**
 * This class is a simple simulation of a typical CMOS cellphone imager chip,
 * which outputs 12-bit Bayer-mosaic raw images.
 *
 * Unlike most real image sensors, this one's native color space is linear sRGB.
 *
 * The sensor is abstracted as operating as a pipeline 3 stages deep;
 * conceptually, each frame to be captured goes through these three stages. The
 * processing step for the sensor is marked off by vertical sync signals, which
 * indicate the start of readout of the oldest frame. The interval between
 * processing steps depends on the frame duration of the frame currently being
 * captured. The stages are 1) configure, 2) capture, and 3) readout. During
 * configuration, the sensor's registers for settings such as exposure time,
 * frame duration, and gain are set for the next frame to be captured. In stage
 * 2, the image data for the frame is actually captured by the sensor. Finally,
 * in stage 3, the just-captured data is read out and sent to the rest of the
 * system.
 *
 * The sensor is assumed to be rolling-shutter, so low-numbered rows of the
 * sensor are exposed earlier in time than larger-numbered rows, with the time
 * offset between each row being equal to the row readout time.
 *
 * The characteristics of this sensor don't correspond to any actual sensor,
 * but are not far off typical sensors.
 *
 * Example timing diagram, with three frames:
 *  Frame 0-1: Frame duration 50 ms, exposure time 20 ms.
 *  Frame   2: Frame duration 75 ms, exposure time 65 ms.
 * Legend:
 *   C = update sensor registers for frame
 *   v = row in reset (vertical blanking interval)
 *   E = row capturing image data
 *   R = row being read out
 *   | = vertical sync signal
 *time(ms)|   0          55        105       155            230     270
 * Frame 0|   :configure : capture : readout :              :       :
 *  Row # | ..|CCCC______|_________|_________|              :       :
 *      0 |   :\          \vvvvvEEEER         \             :       :
 *    500 |   : \          \vvvvvEEEER         \            :       :
 *   1000 |   :  \          \vvvvvEEEER         \           :       :
 *   1500 |   :   \          \vvvvvEEEER         \          :       :
 *   2000 |   :    \__________\vvvvvEEEER_________\         :       :
 * Frame 1|   :           configure  capture      readout   :       :
 *  Row # |   :          |CCCC_____|_________|______________|       :
 *      0 |   :          :\         \vvvvvEEEER              \      :
 *    500 |   :          : \         \vvvvvEEEER              \     :
 *   1000 |   :          :  \         \vvvvvEEEER              \    :
 *   1500 |   :          :   \         \vvvvvEEEER              \   :
 *   2000 |   :          :    \_________\vvvvvEEEER______________\  :
 * Frame 2|   :          :          configure     capture    readout:
 *  Row # |   :          :         |CCCC_____|______________|_______|...
 *      0 |   :          :         :\         \vEEEEEEEEEEEEER       \
 *    500 |   :          :         : \         \vEEEEEEEEEEEEER       \
 *   1000 |   :          :         :  \         \vEEEEEEEEEEEEER       \
 *   1500 |   :          :         :   \         \vEEEEEEEEEEEEER       \
 *   2000 |   :          :         :    \_________\vEEEEEEEEEEEEER_______\
 */

#ifndef HW_EMULATOR_CAMERA2_SENSOR_H
#define HW_EMULATOR_CAMERA2_SENSOR_H

#include "utils/Thread.h"
#include "utils/Mutex.h"
#include "utils/Timers.h"
#include <utils/String8.h>

#include "Scene.h"
//#include "Base.h"
#include "camera_hw.h"
#include <cstdlib>

namespace android {

typedef enum camera_mirror_flip_e {
    MF_NORMAL = 0,
    MF_MIRROR,
    MF_FLIP,
    MF_MIRROR_FLIP,
}camera_mirror_flip_t;


typedef enum camera_wb_flip_e {
    CAM_WB_AUTO = 0,
    CAM_WB_CLOUD,
    CAM_WB_DAYLIGHT,
    CAM_WB_INCANDESCENCE,
    CAM_WB_TUNGSTEN,
    CAM_WB_FLUORESCENT,
    CAM_WB_MANUAL,
    CAM_WB_SHADE,
    CAM_WB_TWILIGHT,
    CAM_WB_WARM_FLUORESCENT,
}camera_wb_flip_t;

typedef enum camera_effect_flip_e {
        CAM_EFFECT_ENC_NORMAL = 0,
        CAM_EFFECT_ENC_GRAYSCALE,
        CAM_EFFECT_ENC_SEPIA,
        CAM_EFFECT_ENC_SEPIAGREEN,
        CAM_EFFECT_ENC_SEPIABLUE,
        CAM_EFFECT_ENC_COLORINV,
}camera_effect_flip_t;

typedef enum camera_night_mode_flip_e {
    CAM_NM_AUTO = 0,
        CAM_NM_ENABLE,
}camera_night_mode_flip_t;

typedef enum camera_banding_mode_flip_e {
        CAM_ANTIBANDING_DISABLED= V4L2_CID_POWER_LINE_FREQUENCY_DISABLED,
        CAM_ANTIBANDING_50HZ    = V4L2_CID_POWER_LINE_FREQUENCY_50HZ,
        CAM_ANTIBANDING_60HZ    = V4L2_CID_POWER_LINE_FREQUENCY_60HZ,
        CAM_ANTIBANDING_AUTO,
        CAM_ANTIBANDING_OFF,
}camera_banding_mode_flip_t;

typedef enum camera_flashlight_status_e{
        FLASHLIGHT_AUTO = 0,
        FLASHLIGHT_ON,
        FLASHLIGHT_OFF,
        FLASHLIGHT_TORCH,
        FLASHLIGHT_RED_EYE,
}camera_flashlight_status_t;

typedef enum camera_focus_mode_e {
    CAM_FOCUS_MODE_RELEASE = 0,
    CAM_FOCUS_MODE_FIXED,
    CAM_FOCUS_MODE_INFINITY,
    CAM_FOCUS_MODE_AUTO,
    CAM_FOCUS_MODE_MACRO,
    CAM_FOCUS_MODE_EDOF,
    CAM_FOCUS_MODE_CONTI_VID,
    CAM_FOCUS_MODE_CONTI_PIC,
}camera_focus_mode_t;

typedef enum sensor_type_e{
    SENSOR_MMAP = 0,
    SENSOR_ION,
    SENSOR_ION_MPLANE,
    SENSOR_DMA,
    SENSOR_CANVAS_MODE,
    SENSOR_USB,
    SENSOR_SHARE_FD,
}sensor_type_t;

typedef enum sensor_face_type_e{
    SENSOR_FACE_NONE= 0,
    SENSOR_FACE_FRONT,
    SENSOR_FACE_BACK,
}sensor_face_type_t;

typedef struct usb_frmsize_discrete {
    uint32_t width;
    uint32_t height;
} usb_frmsize_discrete_t;

#define IOCTL_MASK_ROTATE	(1<<0)

class Sensor: private Thread, public virtual RefBase {
  public:

    Sensor();
    ~Sensor();

    /*
     * Power control
     */
    void sendExitSingalToSensor();
    status_t startUp(int idx);
    status_t shutDown();

    int getOutputFormat();
    int halFormatToSensorFormat(uint32_t pixelfmt);
    status_t setOutputFormat(int width, int height, int pixelformat, bool isjpeg);
    void setPictureRotate(int rotate);
    int getPictureRotate();
    uint32_t getStreamUsage(int stream_type);

    status_t streamOn();
    status_t streamOff();

    int getPictureSizes(int32_t picSizes[], int size, bool preview);
    int getStreamConfigurations(uint32_t picSizes[], const int32_t kAvailableFormats[], int size);
    int64_t getMinFrameDuration();
    int getStreamConfigurationDurations(uint32_t picSizes[], int64_t duration[], int size, bool flag);
    bool isStreaming();
    bool isNeedRestart(uint32_t width, uint32_t height, uint32_t pixelformat);
    status_t IoctlStateProbe(void);
    void dump(int fd);
    /*
     * Access to scene
     */
    Scene &getScene();

    /*
     * Controls that can be updated every frame
     */

    int getZoom(int *zoomMin, int *zoomMax, int *zoomStep);
    int setZoom(int zoomValue);
    int getExposure(int *mamExp, int *minExp, int *def, camera_metadata_rational *step);
    status_t setExposure(int expCmp);
    status_t setEffect(uint8_t effect);
    int getAntiBanding(uint8_t *antiBanding, uint8_t maxCont);
    status_t setAntiBanding(uint8_t antiBanding);
    status_t setFocuasArea(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    int getAWB(uint8_t *awbMode, uint8_t maxCount);
    status_t setAWB(uint8_t awbMode);
    status_t setAutoFocuas(uint8_t afMode);
    int getAutoFocus(uint8_t *afMode, uint8_t maxCount);
    void setExposureTime(uint64_t ns);
    void setFrameDuration(uint64_t ns);
    void setSensitivity(uint32_t gain);
    // Buffer must be at least stride*height*2 bytes in size
    void setDestinationBuffers(Buffers *buffers);
    // To simplify tracking sensor's current frame
    void setFrameNumber(uint32_t frameNumber);
    void  setFlushFlag(bool flushFlag);
    status_t force_reset_sensor();
    bool get_sensor_status();
    /*
     * Controls that cause reconfiguration delay
     */

    void setBinning(int horizontalFactor, int verticalFactor);

    /*
     * Synchronizing with sensor operation (vertical sync)
     */

    // Wait until the sensor outputs its next vertical sync signal, meaning it
    // is starting readout of its latest frame of data. Returns true if vertical
    // sync is signaled, false if the wait timed out.
    status_t waitForVSync(nsecs_t reltime);

    // Wait until a new frame has been read out, and then return the time
    // capture started.  May return immediately if a new frame has been pushed
    // since the last wait for a new frame. Returns true if new frame is
    // returned, false if timed out.
    status_t waitForNewFrame(nsecs_t reltime,
            nsecs_t *captureTime);

    /*
     * Interrupt event servicing from the sensor. Only triggers for sensor
     * cycles that have valid buffers to write to.
     */
    struct SensorListener {
        enum Event {
            EXPOSURE_START, // Start of exposure
            ERROR_CAMERA_DEVICE,
        };

        virtual void onSensorEvent(uint32_t frameNumber, Event e,
                nsecs_t timestamp) = 0;
        virtual ~SensorListener();
    };

    void setSensorListener(SensorListener *listener);

    /**
     * Static sensor characteristics
     */
    static const unsigned int kResolution[2];

    static const nsecs_t kExposureTimeRange[2];
    static const nsecs_t kFrameDurationRange[2];
    static const nsecs_t kMinVerticalBlank;

    static const uint8_t kColorFilterArrangement;

    // Output image data characteristics
    static const uint32_t kMaxRawValue;
    static const uint32_t kBlackLevel;
    // Sensor sensitivity, approximate

    static const float kSaturationVoltage;
    static const uint32_t kSaturationElectrons;
    static const float kVoltsPerLuxSecond;
    static const float kElectronsPerLuxSecond;

    static const float kBaseGainFactor;

    static const float kReadNoiseStddevBeforeGain; // In electrons
    static const float kReadNoiseStddevAfterGain;  // In raw digital units
    static const float kReadNoiseVarBeforeGain;
    static const float kReadNoiseVarAfterGain;

    // While each row has to read out, reset, and then expose, the (reset +
    // expose) sequence can be overlapped by other row readouts, so the final
    // minimum frame duration is purely a function of row readout time, at least
    // if there's a reasonable number of rows.
    static const nsecs_t kRowReadoutTime;

    static const int32_t kSensitivityRange[2];
    static const uint32_t kDefaultSensitivity;

    sensor_type_e getSensorType(void);

    sensor_face_type_e mSensorFace;

  private:
    Mutex mControlMutex; // Lock before accessing control parameters
    // Start of control parameters
    Condition mVSync;
    bool      mGotVSync;
    uint64_t  mExposureTime;
    uint64_t  mFrameDuration;
    uint32_t  mGainFactor;
    Buffers  *mNextBuffers;
    uint8_t  *mKernelBuffer;
    uintptr_t mKernelPhysAddr;
    uint32_t  mFrameNumber;
    int  mRotateValue;
    // End of control parameters

    int mEV;

    Mutex mReadoutMutex; // Lock before accessing readout variables
    // Start of readout variables
    Condition mReadoutAvailable;
    Condition mReadoutComplete;
    Buffers  *mCapturedBuffers;
    nsecs_t   mCaptureTime;
    SensorListener *mListener;
    // End of readout variables

    uint8_t *mTemp_buffer;
    bool mExitSensorThread;

    // Time of sensor startup, used for simulation zero-time point
    nsecs_t mStartupTime;

    //store the v4l2 info
    struct VideoInfo *vinfo;

    struct timeval mTimeStart, mTimeEnd;
    struct timeval mTestStart, mTestEnd;

    uint32_t mFramecount;
    float mCurFps;

    enum sensor_type_e mSensorType;
    unsigned int mIoctlSupport;
    unsigned int msupportrotate;
    uint32_t mTimeOutCount;
    bool mWait;
    uint32_t mPre_width;
    uint32_t mPre_height;
    bool mFlushFlag;
    bool mSensorWorkFlag;
    /**
     * Inherited Thread virtual overrides, and members only used by the
     * processing thread
     */
  private:
    virtual status_t readyToRun();

    virtual bool threadLoop();

    nsecs_t mNextCaptureTime;
    Buffers *mNextCapturedBuffers;

    Scene mScene;

    int captureNewImageWithGe2d();
    int captureNewImage();
    void captureRaw(uint8_t *img, uint32_t gain, uint32_t stride);
    void captureRGBA(uint8_t *img, uint32_t gain, uint32_t stride);
    void captureRGB(uint8_t *img, uint32_t gain, uint32_t stride);
    void captureNV21(StreamBuffer b, uint32_t gain);
    void captureYV12(StreamBuffer b, uint32_t gain);
    void captureYUYV(uint8_t *img, uint32_t gain, uint32_t stride);
    void YUYVToNV21(uint8_t *src, uint8_t *dst, int width, int height);
    void YUYVToYV12(uint8_t *src, uint8_t *dst, int width, int height);
};

}

#endif // HW_EMULATOR_CAMERA2_SENSOR_H
