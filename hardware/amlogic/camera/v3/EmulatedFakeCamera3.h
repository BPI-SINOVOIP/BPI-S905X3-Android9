/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef HW_EMULATOR_CAMERA_EMULATED_FAKE_CAMERA3_H
#define HW_EMULATOR_CAMERA_EMULATED_FAKE_CAMERA3_H

/**
 * Contains declaration of a class EmulatedCamera that encapsulates
 * functionality of a fake camera that implements version 3 of the camera device
 * interace.
 */

#include "EmulatedCamera3.h"
#include "fake-pipeline2/Base.h"
#include "fake-pipeline2/Sensor.h"
#include "fake-pipeline2/JpegCompressor.h"
#include <CameraMetadata.h>
#include <utils/List.h>
#include <utils/Mutex.h>

using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

namespace android {

/**
 * Encapsulates functionality for a v3 HAL camera which produces synthetic data.
 *
 * Note that EmulatedCameraFactory instantiates an object of this class just
 * once, when EmulatedCameraFactory instance gets constructed. Connection to /
 * disconnection from the actual camera device is handled by calls to
 * connectDevice(), and closeCamera() methods of this class that are invoked in
 * response to hw_module_methods_t::open, and camera_device::close callbacks.
 */
struct jpegsize {
    uint32_t width;
    uint32_t height;
};

class EmulatedFakeCamera3 : public EmulatedCamera3,
        private Sensor::SensorListener {
public:

    EmulatedFakeCamera3(int cameraId, struct hw_module_t* module);

    virtual ~EmulatedFakeCamera3();

    /****************************************************************************
     * EmulatedCamera3 virtual overrides
     ***************************************************************************/

public:

    virtual status_t Initialize();

    /****************************************************************************
     * Camera module API and generic hardware device API implementation
     ***************************************************************************/

public:
    virtual status_t connectCamera(hw_device_t** device);

    virtual status_t plugCamera();
    virtual status_t unplugCamera();
    virtual camera_device_status_t getHotplugStatus();

    virtual status_t closeCamera();

    virtual status_t getCameraInfo(struct camera_info *info);
    virtual bool getCameraStatus();

    /****************************************************************************
     * EmulatedCamera3 abstract API implementation
     ***************************************************************************/

protected:

    virtual status_t configureStreams(
        camera3_stream_configuration *streamList);

    virtual status_t registerStreamBuffers(
        const camera3_stream_buffer_set *bufferSet) ;

    virtual const camera_metadata_t* constructDefaultRequestSettings(
        int type);

    virtual status_t processCaptureRequest(camera3_capture_request *request);

    /** Debug methods */

    virtual void dump(int fd);
    virtual int  flush_all_requests();

    /** Tag query methods */
    virtual const char *getVendorSectionName(uint32_t tag);

    virtual const char *getVendorTagName(uint32_t tag);

    virtual int getVendorTagType(uint32_t tag);

private:

    /**
     * Build the static info metadata buffer for this device
     */
    status_t constructStaticInfo();
    int getAvailableChKeys(CameraMetadata *info, uint8_t level);
    void updateCameraMetaData(CameraMetadata *info);
    camera_metadata_ro_entry_t staticInfo(const CameraMetadata *info, uint32_t tag,
            size_t minCount=0, size_t maxCount=0, bool required=true) const;
    void                        getStreamConfigurationDurations(CameraMetadata *info);

    void getStreamConfigurationp(CameraMetadata *info);
    void getValidJpegSize(uint32_t picSizes[], uint32_t availablejpegsize[], int count);
    status_t checkValidJpegSize(uint32_t width, uint32_t height);

    //HW levels worst<->best, 0 = worst, 2 = best */
    //compareHardwareLevel
    //cts/tests/tests/hardware/src/android/hardware/camera2/cts/ExtendedCameraCharacteristicsTest.java
    typedef enum hardware_level_e {
        LEGACY,
        LIMITED,
        FULL,
        OPT = 0xFF, //max of uint8_t
    } hardware_level_t;

    typedef enum available_capabilities_e {
        NONE = 0,
        BC =  0x01,
        MANUAL_SENSOR = 0x02,
        MANUAL_POST_PROCESSING = 0x04,
        RAW = 0x08,
        ZSL = 0x10,
    }available_capabilities_t;

    struct KeyInfo_s{
        int32_t key;
        uint8_t level;
        uint8_t capmask;
    }KeyInfo_t;

    static const struct KeyInfo_s sKeyInfo[];
    static const struct KeyInfo_s sKeyInfoReq[];
    static const struct KeyInfo_s sKeyInfoResult[];
    static const struct KeyInfo_s sKeyBackwardCompat[];
    jpegsize maxJpegResolution;
    jpegsize getMaxJpegResolution(uint32_t picSizes[],int count);
    ssize_t getJpegBufferSize(int width, int height);
    /**
     * Run the fake 3A algorithms as needed. May override/modify settings
     * values.
     */
    status_t process3A(CameraMetadata &settings);

    status_t doFakeAE(CameraMetadata &settings);
    status_t doFakeAF(CameraMetadata &settings);
    status_t doFakeAWB(CameraMetadata &settings);
    void     update3A(CameraMetadata &settings);

    /** Signal from readout thread that it doesn't have anything to do */
    void     signalReadoutIdle();

    /** Handle interrupt events from the sensor */
    void     onSensorEvent(uint32_t frameNumber, Event e, nsecs_t timestamp);

    /****************************************************************************
     * Static configuration information
     ***************************************************************************/
private:
    static const uint32_t kMaxRawStreamCount = 1;
    static const uint32_t kMaxProcessedStreamCount = 3;
    static const uint32_t kMaxJpegStreamCount = 1;
    static const uint32_t kMaxReprocessStreamCount = 2;
    static const uint32_t kMaxBufferCount = 4;
    // We need a positive stream ID to distinguish external buffers from
    // sensor-generated buffers which use a nonpositive ID. Otherwise, HAL3 has
    // no concept of a stream id.
    static const uint32_t kGenericStreamId = 1;
    static const int32_t  kAvailableFormats[];
    static const uint32_t kAvailableRawSizes[];
    static const uint64_t kAvailableRawMinDurations[];
    static const uint32_t kAvailableProcessedSizesBack[];
    static const uint32_t kAvailableProcessedSizesFront[];
    static const uint64_t kAvailableProcessedMinDurations[];
    static const uint32_t kAvailableJpegSizesBack[];
    static const uint32_t kAvailableJpegSizesFront[];
    static const uint64_t kAvailableJpegMinDurations[];

    static const int64_t  kSyncWaitTimeout     = 10000000; // 10 ms
    static const int32_t  kMaxSyncTimeoutCount = 300; // 1000 kSyncWaitTimeouts
    static const uint32_t kFenceTimeoutMs      = 2000; // 2 s

    /****************************************************************************
     * Data members.
     ***************************************************************************/

    /* HAL interface serialization lock. */
    Mutex              mLock;

    /* Facing back (true) or front (false) switch. */
    bool               mFacingBack;

    /* Full mode (true) or limited mode (false) switch */
    bool               mFullMode;

    enum sensor_type_e mSensorType;

    /**
     * Cache for default templates. Once one is requested, the pointer must be
     * valid at least until close() is called on the device
     */
    camera_metadata_t *mDefaultTemplates[CAMERA3_TEMPLATE_COUNT];

    /**
     * Private stream information, stored in camera3_stream_t->priv.
     */
    struct PrivateStreamInfo {
        bool alive;
        bool registered;
    };

    // Shortcut to the input stream
    camera3_stream_t*  mInputStream;

    typedef List<camera3_stream_t*>           StreamList;
    typedef List<camera3_stream_t*>::iterator StreamIterator;
    typedef Vector<camera3_stream_buffer>     HalBufferVector;

    uint32_t mAvailableJpegSize[64 * 8];

    struct ExifInfo info;

    // All streams, including input stream
    StreamList         mStreams;

    // Cached settings from latest submitted request
    CameraMetadata     mPrevSettings;

    /** Fake hardware interfaces */
    sp<Sensor>         mSensor;
    sp<JpegCompressor> mJpegCompressor;
    friend class       JpegCompressor;
    unsigned int mSupportCap;
    unsigned int mSupportRotate;
    camera_status_t   mCameraStatus;
    bool mFlushTag;
    /** Processing thread for sending out results */

    class ReadoutThread : public Thread, private JpegCompressor::JpegListener {
      public:
        ReadoutThread(EmulatedFakeCamera3 *parent);
        ~ReadoutThread();

        struct Request {
            uint32_t         frameNumber;
            CameraMetadata   settings;
            HalBufferVector *buffers;
            Buffers         *sensorBuffers;
            bool             havethumbnail;
        };

        /**
         * Interface to parent class
         */

        // Place request in the in-flight queue to wait for sensor capture
        void     queueCaptureRequest(const Request &r);
        // Test if the readout thread is idle (no in-flight requests, not
        // currently reading out anything
        bool     isIdle();

        // Wait until isIdle is true
        status_t waitForReadout();
        status_t setJpegCompressorListener(EmulatedFakeCamera3 *parent);
        status_t startJpegCompressor(EmulatedFakeCamera3 *parent);
        status_t shutdownJpegCompressor(EmulatedFakeCamera3 * parent);
        void sendExitReadoutThreadSignal(void);
        status_t flushAllRequest(bool flag);
        void setFlushFlag(bool flag);
        void sendFlushSingnal(void);
      private:
        static const nsecs_t kWaitPerLoop  = 10000000L; // 10 ms
        static const nsecs_t kMaxWaitLoops = 1000;
        static const size_t  kMaxQueueSize = 2;

        EmulatedFakeCamera3 *mParent;
        Mutex mLock;

        List<Request> mInFlightQueue;
        Condition     mInFlightSignal;
        bool          mThreadActive;

        virtual bool threadLoop();

        // Only accessed by threadLoop

        Request mCurrentRequest;

        // Jpeg completion callbacks
        bool                  mExitReadoutThread;
        Mutex                 mJpegLock;
        bool                  mJpegWaiting;
        camera3_stream_buffer mJpegHalBuffer;
        //uint32_t              mJpegFrameNumber;
        bool mFlushFlag;
        Condition mFlush;
        virtual void onJpegDone(const StreamBuffer &jpegBuffer, bool success, CaptureRequest &r);
        virtual void onJpegInputDone(const StreamBuffer &inputBuffer);
    };

    sp<ReadoutThread> mReadoutThread;

    /** Fake 3A constants */

    static const nsecs_t kNormalExposureTime;
    static const nsecs_t kFacePriorityExposureTime;
    static const int     kNormalSensitivity;
    static const int     kFacePrioritySensitivity;
    // Rate of converging AE to new target value, as fraction of difference between
    // current and target value.
    static const float   kExposureTrackRate;
    // Minimum duration for precapture state. May be longer if slow to converge
    // to target exposure
    static const int     kPrecaptureMinFrames;
    // How often to restart AE 'scanning'
    static const int     kStableAeMaxFrames;
    // Maximum stop below 'normal' exposure time that we'll wander to while
    // pretending to converge AE. In powers of 2. (-2 == 1/4 as bright)
    static const float   kExposureWanderMin;
    // Maximum stop above 'normal' exposure time that we'll wander to while
    // pretending to converge AE. In powers of 2. (2 == 4x as bright)
    static const float   kExposureWanderMax;

    /** Fake 3A state */

    uint8_t mControlMode;
    bool    mFacePriority;
    uint8_t mAeState;
    uint8_t mAfState;
    uint8_t mAwbState;
    uint8_t mAeMode;
    uint8_t mAfMode;
    uint8_t mAwbMode;
    int     mAfTriggerId;
    int     mZoomMin;
    int     mZoomMax;
    int     mZoomStep;
    int     mFrameDuration;

    int     mAeCounter;
    nsecs_t mAeCurrentExposureTime;
    nsecs_t mAeTargetExposureTime;
    int     mAeCurrentSensitivity;

};

} // namespace android

#endif // HW_EMULATOR_CAMERA_EMULATED_CAMERA3_H
