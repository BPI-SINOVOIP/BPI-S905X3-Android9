#ifndef OMXDECODER
#define OMXDECODER
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Video.h>
#include <utils/Log.h>
//#include <media/openmax/OMX_Component.h>
#include <OMX_Component.h>
#include <OMX_IVCommon.h>
#include <OMX_Core.h>
#include <OMX_Index.h>

//#include <binder/IPCThreadState.h>
#include <utils/Errors.h>
#include <utils/Thread.h>
#include <utils/Timers.h>
#include <utils/Mutex.h>
#include <utils/List.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>
#include <dlfcn.h>

#include <nativebase/nativebase.h>
//#include <android/native_window.h>
//#include <gui/Surface.h>
//#include <HardwareAPI.h>


using namespace android;
#define ROUND_16(X)     ((X + 0xF) & (~0xF))
#define ROUND_32(X)     ((X + 0x1F) & (~0x1F))
#define YUV_SIZE(W, H)   ((W) * (H) * 3 >> 1)
#define ZTE_BUF_ADDR_ALIGNMENT_VALUE 512
#define OMX_IndexVendorZteOmxDecNormalYUVMode 0x7F10000C
#define LOG_LINE() ALOGD("%s:%d", __FUNCTION__, __LINE__);
typedef OMX_ERRORTYPE (*InitFunc)();
typedef OMX_ERRORTYPE (*DeinitFunc)();
typedef OMX_ERRORTYPE (*GetHandleFunc)(OMX_HANDLETYPE *, OMX_STRING, OMX_PTR, OMX_CALLBACKTYPE *);
typedef OMX_ERRORTYPE (*FreeHandleFunc)(OMX_HANDLETYPE *);

struct GrallocBufInfo {
    uint32_t width;
    uint32_t height;
    int format;
    uint32_t stride;
};

class OMXDecoder
{
public:
    OMXDecoder();
    OMXDecoder(bool useDMABuffer, bool keepOriginalSize);
    ~OMXDecoder();
    bool setParameters(uint32_t width, uint32_t height, uint32_t out_buffer_count);
    bool initialize(const char* name);
    bool prepareBuffers();
    void start();
    void freeBuffers();
    void deinitialize();
    //void saveNativeBufferHdr(void *buffer, int index, int bufferNum, struct GrallocBufInfo info, bool status);//omx zero-copy
    OMX_BUFFERHEADERTYPE* dequeueInputBuffer();
    void queueInputBuffer(OMX_BUFFERHEADERTYPE* pBufferHdr);
    //ANativeWindowBuffer * dequeueOutputBuffer();
    //native_handle_t * dequeueOutputBuffer();
    OMX_BUFFERHEADERTYPE* dequeueOutputBuffer();
    //void releaseOutputBuffer(ANativeWindowBuffer * pBufferHdr);
    //void releaseOutputBuffer(native_handle_t * pBufferHdr);
    void releaseOutputBuffer(OMX_BUFFERHEADERTYPE* pBufferHdr);

    template<class T> void InitOMXParams(T *params);

    OMX_ERRORTYPE OnEvent(
            OMX_IN OMX_EVENTTYPE eEvent,
            OMX_IN OMX_U32 nData1,
            OMX_IN OMX_U32 nData2,
            OMX_IN OMX_PTR pEventData);

    OMX_ERRORTYPE emptyBufferDone(OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);
    OMX_ERRORTYPE fillBufferDone(OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);

  static OMX_ERRORTYPE OnEvent(
          OMX_IN OMX_HANDLETYPE hComponent,
          OMX_IN OMX_PTR pAppData,
          OMX_IN OMX_EVENTTYPE eEvent,
          OMX_IN OMX_U32 nData1,
          OMX_IN OMX_U32 nData2,
          OMX_IN OMX_PTR pEventData);

  static OMX_ERRORTYPE OnEmptyBufferDone(
          OMX_IN OMX_HANDLETYPE hComponent,
          OMX_IN OMX_PTR pAppData,
          OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);

  static OMX_ERRORTYPE OnFillBufferDone(
          OMX_IN OMX_HANDLETYPE hComponent,
          OMX_IN OMX_PTR pAppData,
          OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);

    static OMX_CALLBACKTYPE kCallbacks;
    int Decode(uint8_t*src, size_t src_size,
                                  int dst_fd,uint8_t *dst_buf,
                                  size_t dst_w, size_t dst_h);
private:
    OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE hComponent, OMX_STATETYPE eTestState, OMX_STATETYPE eTestState2);
    OMX_U32 mWidth;
    OMX_U32 mHeight;
    int mFormat;
    uint32_t mStride;
    int mIonFd;
    bool mUseDMABuffer;
    bool mKeepOriginalSize;
    OMX_PARAM_PORTDEFINITIONTYPE mVideoInputPortParam;
    OMX_PARAM_PORTDEFINITIONTYPE mVideoOutputPortParam;
    OMX_BUFFERHEADERTYPE mInOutPutBufferParam;
    OMX_HANDLETYPE mVDecoderHandle;
    Mutex mInputBufferLock;
    List<OMX_BUFFERHEADERTYPE*> mListOfInputBufferHeader;
    Mutex mOutputBufferLock;
    List<OMX_BUFFERHEADERTYPE*> mListOfOutputBufferHeader;

  struct out_buffer_t {
      int index;
      int fd;
      int mIonFd;
      void *fd_ptr;
      struct ion_handle *ion_hnd;
      OMX_BUFFERHEADERTYPE * pBuffer;
  };

    struct out_buffer_t *mOutBuffer;
    int mNoFreeFlag;
    OMX_BUFFERHEADERTYPE **mppBuffer;

    //native buffer
    uint32_t mOutBufferCount;
  struct out_buffer_native {
      //ANativeWindowBuffer *mOutputNativeBuffer;
      native_handle_t* handle;
      OMX_BUFFERHEADERTYPE *pBuffer;
      bool isQueued;
  };
    struct out_buffer_native *mOutBufferNative;

    void* mLibHandle;
    InitFunc mInit;
    DeinitFunc mDeinit;
    GetHandleFunc mGetHandle;
    FreeHandleFunc mFreeHandle;
    OMX_STRING mDecoderComponentName;
    OMX_VERSIONTYPE mSpecVersion;
    int mDequeueFailNum;
    uint8_t* mTempFrame[3];
private:
    void QueueBuffer(uint8_t* src, size_t size);
    int DequeueBuffer(int dst_fd ,uint8_t* dst_buf,
                                size_t dst_w, size_t dst_h);
    bool normal_buffer_init(int buffer_size);
    bool ion_buffer_init();
    void free_ion_buffer();
    void free_normal_buffer();
};
#endif
