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
 */



#ifndef ANDROID_VIRTUAL_CAMERA_HARDWARE_H
#define ANDROID_VIRTUAL_CAMERA_HARDWARE_H
#include "CameraHal.h"

#define MIN_WIDTH           640
#define MIN_HEIGHT          480
#define PICTURE_WIDTH   3264 /* 5mp - 2560. 8mp - 3280 */ /* Make sure it is a multiple of 16. */
#define PICTURE_HEIGHT  2448 /* 5mp - 2048. 8mp - 2464 */ /* Make sure it is a multiple of 16. */
#define PREVIEW_WIDTH 176
#define PREVIEW_HEIGHT 144
//#define PIXEL_FORMAT           V4L2_PIX_FMT_UYVY

#define VIDEO_FRAME_COUNT_MAX    8 //NUM_OVERLAY_BUFFERS_REQUESTED
#define MAX_CAMERA_BUFFERS    8 //NUM_OVERLAY_BUFFERS_REQUESTED
#define MAX_ZOOM        3
#define THUMB_WIDTH     80
#define THUMB_HEIGHT    60
#define PIX_YUV422I 0
#define PIX_YUV420P 1

#define SATURATION_OFFSET 100
#define SHARPNESS_OFFSET 100
#define CONTRAST_OFFSET 100

#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                             GRALLOC_USAGE_HW_RENDER | \
                             GRALLOC_USAGE_SW_READ_RARELY | \
                             GRALLOC_USAGE_SW_WRITE_NEVER

//Enables Absolute PPM measurements in logcat
#ifndef PPM_INSTRUMENTATION_ABS
#define PPM_INSTRUMENTATION_ABS 1
#endif

#define LOCK_BUFFER_TRIES 5
//TODO this is wrong. fix this:
#define HAL_PIXEL_FORMAT_NV12 HAL_PIXEL_FORMAT_YCrCb_420_SP

//sensor listener is useless now, camera don't need to knwo the orientation now
//disable it now
//#define ENABLE_SENSOR_LISTENER 1 

//#define AMLOGIC_CAMERA_OVERLAY_SUPPORT
//#define AMLOGIC_USB_CAMERA_SUPPORT

#define NONNEG_ASSIGN(x,y) \
    if(x > -1) \
        y = x

namespace android {

#define PARAM_BUFFER            6000

///Forward declarations
class VirtualCamHal;
class CameraFrame;
class VirtualCamHalEvent;
class DisplayFrame;

/**
  * Class for handling data and notify callbacks to application
  */
class   AppCbNotifier: public ErrorNotifier , public virtual RefBase
{

public:

    ///Constants
    static const int NOTIFIER_TIMEOUT;
    static const int32_t MAX_BUFFERS = 8;

    enum NotifierCommands
        {
        NOTIFIER_CMD_PROCESS_EVENT,
        NOTIFIER_CMD_PROCESS_FRAME,
        NOTIFIER_CMD_PROCESS_ERROR
        };

    enum NotifierState
        {
        NOTIFIER_STOPPED,
        NOTIFIER_STARTED,
        NOTIFIER_EXITED
        };

public:

    ~AppCbNotifier();

    ///Initialzes the callback notifier, creates any resources required
    status_t initialize();

    ///Starts the callbacks to application
    status_t start();

    ///Stops the callbacks from going to application
    status_t stop();

    void setEventProvider(int32_t eventMask, MessageNotifier * eventProvider);
    void setFrameProvider(FrameNotifier *frameProvider);

    //All sub-components of Camera HAL call this whenever any error happens
    virtual void errorNotify(int error);

    status_t startPreviewCallbacks(CameraParameters &params, void *buffers, uint32_t *offsets, int fd, size_t length, size_t count);
    status_t stopPreviewCallbacks();

    status_t enableMsgType(int32_t msgType);
    status_t disableMsgType(int32_t msgType);

    //API for enabling/disabling measurement data
    void setMeasurements(bool enable);

    //thread loops
    bool notificationThread();

    ///Notification callback functions
    static void frameCallbackRelay(CameraFrame* caFrame);
    static void eventCallbackRelay(CameraHalEvent* chEvt);
    void frameCallback(CameraFrame* caFrame);
    void eventCallback(CameraHalEvent* chEvt);
    void flushAndReturnFrames();

    void setCallbacks(VirtualCamHal *cameraHal,
                        camera_notify_callback notify_cb,
                        camera_data_callback data_cb,
                        camera_data_timestamp_callback data_cb_timestamp,
                        camera_request_memory get_memory,
                        void *user);

    //Set Burst mode
    void setBurst(bool burst);

    //Notifications from CameraHal for video recording case
    status_t startRecording();
    status_t stopRecording();
    status_t initSharedVideoBuffers(void *buffers, uint32_t *offsets, int fd, size_t length, size_t count, void *vidBufs);
    status_t releaseRecordingFrame(const void *opaque);

	status_t useMetaDataBufferMode(bool enable);

    void EncoderDoneCb(void*, void*, CameraFrame::FrameType type, void* cookie1, void* cookie2);

    void useVideoBuffers(bool useVideoBuffers);

    bool getUseVideoBuffers();
    void setVideoRes(int width, int height);

    void flushEventQueue();

    //Internal class definitions
    class NotificationThread : public Thread {
        AppCbNotifier* mAppCbNotifier;
        MSGUTILS::MessageQueue mNotificationThreadQ;
    public:
        enum NotificationThreadCommands
        {
        NOTIFIER_START,
        NOTIFIER_STOP,
        NOTIFIER_EXIT,
        };
    public:
        NotificationThread(AppCbNotifier* nh)
            : Thread(false), mAppCbNotifier(nh) { }
        virtual bool threadLoop() {
            return mAppCbNotifier->notificationThread();
        }

        MSGUTILS::MessageQueue &msgQ() { return mNotificationThreadQ;}
    };

    //Friend declarations
    friend class NotificationThread;

private:
    void notifyEvent();
    void notifyFrame();
    bool processMessage();
    void releaseSharedVideoBuffers();
    status_t dummyRaw();
    void copyAndSendPictureFrame(CameraFrame* frame, int32_t msgType);
    void copyAndSendPreviewFrame(CameraFrame* frame, int32_t msgType);

private:
    mutable Mutex mLock;
    mutable Mutex mBurstLock;
    VirtualCamHal* mCameraHal;
    camera_notify_callback mNotifyCb;
    camera_data_callback   mDataCb;
    camera_data_timestamp_callback mDataCbTimestamp;
    camera_request_memory mRequestMemory;
    void *mCallbackCookie;

    //Keeps Video MemoryHeaps and Buffers within
    //these objects
    KeyedVector<unsigned int, unsigned int> mVideoHeaps;
    KeyedVector<unsigned int, unsigned int> mVideoBuffers;
    KeyedVector<unsigned int, unsigned int> mVideoMap;

    //Keeps list of Gralloc handles and associated Video Metadata Buffers
    KeyedVector<uint32_t, uint32_t> mVideoMetadataBufferMemoryMap;
    KeyedVector<uint32_t, uint32_t> mVideoMetadataBufferReverseMap;

    bool mBufferReleased;

    sp< NotificationThread> mNotificationThread;
    EventProvider *mEventProvider;
    FrameProvider *mFrameProvider;
    MSGUTILS::MessageQueue mEventQ;
    MSGUTILS::MessageQueue mFrameQ;
    NotifierState mNotifierState;

    bool mPreviewing;
    camera_memory_t* mPreviewMemory;
    unsigned char* mPreviewBufs[MAX_BUFFERS];
    int mPreviewBufCount;
    const char *mPreviewPixelFormat;
    KeyedVector<unsigned int, sp<MemoryHeapBase> > mSharedPreviewHeaps;
    KeyedVector<unsigned int, sp<MemoryBase> > mSharedPreviewBuffers;

    //Burst mode active
    bool mBurst;
    mutable Mutex mRecordingLock;
    bool mRecording;
    bool mMeasurementEnabled;

    bool mUseMetaDataBufferMode;
    bool mRawAvailable;

    bool mUseVideoBuffers;

    int mVideoWidth;
    int mVideoHeight;

};
static void releaseImageBuffers(void *userData);

static void endImageCapture(void *userData);

 /**
    Implementation of the Android Camera hardware abstraction layer

*/
class VirtualCamHal

{

public:
    ///Constants
    static const int NO_BUFFERS_PREVIEW;
    static const int NO_BUFFERS_IMAGE_CAPTURE;
    static const uint32_t VFR_SCALE = 1000;


    /*--------------------Interface Methods---------------------------------*/

     //@{
public:

    /** Set the notification and data callbacks */
    void setCallbacks(camera_notify_callback notify_cb,
                        camera_data_callback data_cb,
                        camera_data_timestamp_callback data_cb_timestamp,
                        camera_request_memory get_memory,
                        void *user);

    /** Receives orientation events from SensorListener **/
    void onOrientationEvent(uint32_t orientation, uint32_t tilt);

    /**
     * The following three functions all take a msgtype,
     * which is a bitmask of the messages defined in
     * include/ui/Camera.h
     */

    /**
     * Enable a message, or set of messages.
     */
    void        enableMsgType(int32_t msgType);

    /**
     * Disable a message, or a set of messages.
     */
    void        disableMsgType(int32_t msgType);

    /**
     * Query whether a message, or a set of messages, is enabled.
     * Note that this is operates as an AND, if any of the messages
     * queried are off, this will return false.
     */
    int        msgTypeEnabled(int32_t msgType);

    /**
     * Start preview mode.
     */
    int    startPreview();

    /**
     * Only used if overlays are used for camera preview.
     */
    int setPreviewWindow(struct preview_stream_ops *window);

    /**
     * Stop a previously started preview.
     */
    void        stopPreview();

    /**
     * Returns true if preview is enabled.
     */
    bool        previewEnabled();

    /**
     * Start record mode. When a record image is available a CAMERA_MSG_VIDEO_FRAME
     * message is sent with the corresponding frame. Every record frame must be released
     * by calling releaseRecordingFrame().
     */
    int    startRecording();

    /**
     * Stop a previously started recording.
     */
    void        stopRecording();

    /**
     * Returns true if recording is enabled.
     */
    int        recordingEnabled();

    /**
     * Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.
     */
    void        releaseRecordingFrame(const void *opaque);

    /**
     * Start auto focus, the notification callback routine is called
     * with CAMERA_MSG_FOCUS once when focusing is complete. autoFocus()
     * will be called again if another auto focus is needed.
     */
    int    autoFocus();

    /**
     * Cancels auto-focus function. If the auto-focus is still in progress,
     * this function will cancel it. Whether the auto-focus is in progress
     * or not, this function will return the focus position to the default.
     * If the camera does not support auto-focus, this is a no-op.
     */
    int    cancelAutoFocus();

    /**
     * Take a picture.
     */
    int    takePicture();

    /**
     * Cancel a picture that was started with takePicture.  Calling this
     * method when no picture is being taken is a no-op.
     */
    int    cancelPicture();

    /** Set the camera parameters. */
    int    setParameters(const char* params);
    int    setParameters(const CameraParameters& params);

    /** Return the camera parameters. */
    char*  getParameters();
    void putParameters(char *);

    /**
     * Send command to camera driver.
     */
    int sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);

    /**
     * Release the hardware resources owned by this object.  Note that this is
     * *not* done in the destructor.
     */
    void release();

    /**
     * Dump state of the camera hardware
     */
    int dump(int fd) const;


		status_t storeMetaDataInBuffers(bool enable);

     //@}

/*--------------------Internal Member functions - Public---------------------------------*/

public:
 /** @name internalFunctionsPublic */
  //@{

    /** Constructor of VirtualCamHal */
    VirtualCamHal(int cameraId);

    // Destructor of VirtualCamHal
    ~VirtualCamHal();

    /** Initialize VirtualCamHal */
    status_t initialize(CameraProperties::Properties*);

    /** Deinitialize VirtualCamHal */
    void deinitialize();

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Uses the constructor timestamp as a reference to calcluate the
    // elapsed time
    static void PPM(const char *);
    //Uses a user provided timestamp as a reference to calcluate the
    // elapsed time
    static void PPM(const char *, struct timeval*, ...);

#endif

    /** Free image bufs */
    status_t freeImageBufs();

    //Signals the end of image capture
    status_t signalEndImageCapture();

    //Events
    static void eventCallbackRelay(CameraHalEvent* event);
    void eventCallback(CameraHalEvent* event);
    void setEventProvider(int32_t eventMask, MessageNotifier * eventProvider);

/*--------------------Internal Member functions - Private---------------------------------*/
private:

    /** @name internalFunctionsPrivate */
    //@{

    /**  Set the camera parameters specific to Video Recording. */
    bool        setVideoModeParameters(const CameraParameters&);

    /** Reset the camera parameters specific to Video Recording. */
    bool       resetVideoModeParameters();

    /** Restart the preview with setParameter. */
    status_t        restartPreview();

    status_t parseResolution(const char *resStr, int &width, int &height);

    void insertSupportedParams();

    /** Allocate preview data buffers */
    status_t allocPreviewDataBufs(size_t size, size_t bufferCount);

    /** Free preview data buffers */
    status_t freePreviewDataBufs();

    /** Allocate preview buffers */
    status_t allocPreviewBufs(int width, int height, const char* previewFormat, unsigned int bufferCount, unsigned int &max_queueable);

    /** Allocate video buffers */
    status_t allocVideoBufs(uint32_t width, uint32_t height, uint32_t bufferCount);

    /** Allocate image capture buffers */
    status_t allocImageBufs(unsigned int width, unsigned int height, size_t length, const char* previewFormat, unsigned int bufferCount);

    /** Free preview buffers */
    status_t freePreviewBufs();

    /** Free video bufs */
    status_t freeVideoBufs(void *bufs);

    //Check if a given resolution is supported by the current camera
    //instance
    bool isResolutionValid(unsigned int width, unsigned int height, const char *supportedResolutions);

    //Check if a given parameter is supported by the current camera
    // instance
    bool isParameterValid(const char *param, const char *supportedParams);
    bool isParameterValid(int param, const char *supportedParams);
    bool isParameterInRange(int param, const char *supportedParams);
    status_t doesSetParameterNeedUpdate(const char *new_param, const char *old_params, bool &update);

    /** Initialize default parameters */
    void initDefaultParameters();

    void dumpProperties(CameraProperties::Properties& cameraProps);

    status_t startImageBracketing();

    status_t stopImageBracketing();

    void setShutter(bool enable);

    void forceStopPreview();

    void selectFPSRange(int framerate, int *min_fps, int *max_fps);

    void setPreferredPreviewRes(int width, int height);
    void resetPreviewRes(CameraParameters *mParams, int width, int height);

    //@}


/*----------Member variables - Public ---------------------*/
public:
    int32_t mMsgEnabled;
    bool mRecordEnabled;
    nsecs_t mCurrentTime;
    bool mFalsePreview;
    bool mPreviewEnabled;
    uint32_t mTakePictureQueue;
    bool mBracketingEnabled;
    bool mBracketingRunning;
    //User shutter override
    bool mShutterEnabled;
    bool mMeasurementEnabled;
    //Google's parameter delimiter
    static const char PARAMS_DELIMITER[];

    CameraAdapter *mCameraAdapter;
    sp<AppCbNotifier> mAppCbNotifier;
    sp<DisplayAdapter> mDisplayAdapter;
    sp<MemoryManager> mMemoryManager;

    sp<IMemoryHeap> mPictureHeap;

    int* mGrallocHandles;
    bool mFpsRangeChangedByApp;





///static member vars

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Timestamp from the VirtualCamHal constructor
    static struct timeval ppm_start;
    //Timestamp of the autoFocus command
    static struct timeval mStartFocus;
    //Timestamp of the startPreview command
    static struct timeval mStartPreview;
    //Timestamp of the takePicture command
    static struct timeval mStartCapture;

#endif

/*----------Member variables - Private ---------------------*/
private:
    bool mDynamicPreviewSwitch;
    //keeps paused state of display
    bool mDisplayPaused;
    //Index of current camera adapter
    int mCameraIndex;

    mutable Mutex mLock;

#ifdef ENABLE_SENSOR_LISTENER
    sp<SensorListener> mSensorListener;
#endif
    void* mCameraAdapterHandle;

    CameraParameters mParameters;
    bool mPreviewRunning;
    bool mPreviewStateOld;
    bool mRecordingEnabled;
    EventProvider *mEventProvider;

    int32_t *mPreviewDataBufs;
    uint32_t *mPreviewDataOffsets;
    int mPreviewDataFd;
    int mPreviewDataLength;
    int32_t *mImageBufs;
    uint32_t *mImageOffsets;
    int mImageFd;
    int mImageLength;
    int32_t *mPreviewBufs;
    uint32_t *mPreviewOffsets;
    int mPreviewLength;
    int mPreviewFd;
    int32_t *mVideoBufs;
    uint32_t *mVideoOffsets;
    int mVideoFd;
    int mVideoLength;

    int mBracketRangePositive;
    int mBracketRangeNegative;

    ///@todo Rename this as preview buffer provider
    BufferProvider *mBufProvider;
    BufferProvider *mVideoBufProvider;


    CameraProperties::Properties* mCameraProperties;

    bool mPreviewStartInProgress;

    bool mSetPreviewWindowCalled;

    uint32_t mPreviewWidth;
    uint32_t mPreviewHeight;
    int32_t mMaxZoomSupported;

    int mVideoWidth;
    int mVideoHeight;

};


}; // namespace android

#endif
