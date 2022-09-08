#define __STDC_FORMAT_MACROS
#include "OMXDecoder.h"
#include <linux/ion.h>
#include <ion/ion.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <inttypes.h>

#include <ui/GraphicBuffer.h>
#include <media/hardware/HardwareAPI.h>

#include <binder/IPCThreadState.h>
#ifdef GE2D_ENABLE
#include "fake-pipeline2/ge2d_stream.h"
#endif
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "OMXDecoder"
#endif


using namespace android;
OMX_CALLBACKTYPE OMXDecoder::kCallbacks = {
    &OnEvent, &OnEmptyBufferDone, &OnFillBufferDone
};

OMXDecoder::OMXDecoder(bool useDMABuffer, bool keepOriginalSize) {
    LOG_LINE();
    mUseDMABuffer        = useDMABuffer;
    mKeepOriginalSize    = keepOriginalSize;
    mOutBufferNative = NULL;
    mLibHandle = NULL;
    mFreeHandle = NULL;
    mDeinit = NULL;
    mVDecoderHandle = NULL;
    mDequeueFailNum = 0;
}

OMXDecoder::~OMXDecoder() {

    ALOGD("%s\n", __FUNCTION__);

}

//Please don't use saveNativeBufferHdr() again if you want to use setParameters().
bool OMXDecoder::setParameters(uint32_t width, uint32_t height,
        uint32_t out_buffer_count) {
    if (!width || !height || !out_buffer_count) {
        ALOGD("Error parameters!!! width %u  height %u  out_buffer_count %u",
                width, height, out_buffer_count);
        return false;
    }
    mWidth = width;
    mHeight = height;
    mOutBufferCount = out_buffer_count;
    ALOGD("width %u  height %u  out_buffer_count %u",
            width, height, out_buffer_count);
    return true;
}

bool OMXDecoder::initialize(const char* name) {
    LOG_LINE();
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    for (int i = 0; i < 3; i++)
        mTempFrame[i] = (uint8_t*)malloc(mWidth*mHeight*3/2);

    if (0 == strcmp(name,"mjpeg"))
        mDecoderComponentName = (char *)"OMX.amlogic.mjpeg.decoder.awesome";
    mLibHandle = dlopen("libOmxCore.so", RTLD_NOW);
    if (mLibHandle != NULL) {
        mInit         =     (InitFunc) dlsym(mLibHandle, "OMX_Init");
        mDeinit     =     (DeinitFunc) dlsym(mLibHandle, "OMX_Deinit");
        mGetHandle  =     (GetHandleFunc) dlsym(mLibHandle, "OMX_GetHandle");
        mFreeHandle =     (FreeHandleFunc) dlsym(mLibHandle, "OMX_FreeHandle");
    } else {
        ALOGE("cannot open libOmxCore.so\n");
        return false;
    }

    if (OMX_ErrorNone != (*mInit)()) {
        ALOGE("OMX_Init fail!\n");
        return false;
    } else {
        ALOGD("OMX_Init success!\n");
    }

    eRet = (*mGetHandle)(&mVDecoderHandle, mDecoderComponentName, this, &kCallbacks);

    if (OMX_ErrorNone != eRet) {
        ALOGE("OMX_GetHandle fail!, eRet = %#x\n", eRet);
        return false;
    } else {
        ALOGD("OMX_GetHandle success!\n");
    }

    OMX_UUIDTYPE componentUUID;
    char pComponentName[128];
    OMX_VERSIONTYPE componentVersion;

    eRet = OMX_GetComponentVersion(mVDecoderHandle, pComponentName,
            &componentVersion, &mSpecVersion,
            &componentUUID);
    if (eRet != OMX_ErrorNone)
        ALOGE("OMX_GetComponentVersion failed!, eRet = %#x\n", eRet);
    else
        ALOGD("OMX_GetComponentVersion success!\n");

    OMX_PORT_PARAM_TYPE mPortParam;
    mPortParam.nSize = sizeof(OMX_PORT_PARAM_TYPE);
    mPortParam.nVersion = mSpecVersion;

    eRet = OMX_GetParameter(mVDecoderHandle, OMX_IndexParamVideoInit,
            &mPortParam);
    if (eRet != OMX_ErrorNone) {
        ALOGE("OMX_GetParameter failed!\n");
        return false;
    }
    else
        ALOGD("OMX_GetParameter succes!\n");

    /*configure input port*/
    mVideoInputPortParam.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    mVideoInputPortParam.nVersion = mSpecVersion;
    mVideoInputPortParam.nPortIndex = mPortParam.nStartPortNumber;
    eRet = OMX_GetParameter(mVDecoderHandle, OMX_IndexParamPortDefinition,
            &mVideoInputPortParam);
    if (eRet != OMX_ErrorNone) {
        ALOGE("[%s:%d]OMX_GetParameter OMX_IndexParamPortDefinition failed!! eRet = %#x\n",
                __FUNCTION__, __LINE__, eRet);
        return false;
    }

    ALOGD("[%s:%d]OMX_GetParameter mVideoInputPortParam.nBufferSize = %zu, mVideoInputPortParam.format.video.eColorFormat =%d\n",
            __FUNCTION__, __LINE__,
            mVideoInputPortParam.nBufferSize, mVideoInputPortParam.format.video.eColorFormat);

    mVideoInputPortParam.format.video.nFrameWidth = mWidth;
    mVideoInputPortParam.format.video.nFrameHeight = mHeight;
    if (strcmp(name,"mjpeg") == 0)
        mVideoInputPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingMJPEG;
    mVideoInputPortParam.format.video.xFramerate = (15 << 16);
    eRet = OMX_SetParameter(mVDecoderHandle, OMX_IndexParamPortDefinition, &mVideoInputPortParam);
    if (OMX_ErrorNone != eRet) {
        ALOGE("[%s:%d]OMX_SetParameter OMX_IndexParamPortDefinition error!! eRet = %#x\n",
                __FUNCTION__, __LINE__, eRet);
        return false;
    }

    ALOGD("[%s:%d]OMX_SetParameter mVideoInputPortParam.nBufferSize = %zu\n",
            __FUNCTION__, __LINE__,
            mVideoInputPortParam.nBufferSize);

    eRet = OMX_GetParameter(mVDecoderHandle, OMX_IndexParamPortDefinition, &mVideoInputPortParam);
    if (OMX_ErrorNone != eRet) {
        ALOGE("[%s:%d]OMX_GetParameter OMX_IndexParamPortDefinition error!! eRet = %#x\n",
                __FUNCTION__, __LINE__, eRet);
        return false;
    }

    ALOGD("[%s:%d]mVideoInputPortParam.nBufferCountActual = %zu, mVideoInputPortParam.nBufferSize = %zu, mVideoInputPortParam.format.video.eColorFormat =%d\n",
            __FUNCTION__, __LINE__,
            mVideoInputPortParam.nBufferCountActual,
            mVideoInputPortParam.nBufferSize,
            mVideoInputPortParam.format.video.eColorFormat);

    /*configure output port*/
    if (mKeepOriginalSize) {
        OMX_INDEXTYPE index;
        OMX_BOOL keepOriginalSize = OMX_TRUE;
        eRet = OMX_GetExtensionIndex(mVDecoderHandle, (char *)"OMX.amlogic.android.index.KeepOriginalSize", &index);
        if (eRet == OMX_ErrorNone) {
            ALOGD("OMX_GetExtensionIndex returned %#x", index);
            eRet = OMX_SetParameter(mVDecoderHandle, index, &keepOriginalSize);
            if (eRet != OMX_ErrorNone)
                ALOGW("setting keeporiginalsize returned error: %#x", eRet);
        } else
            ALOGW("OMX_GetExtensionIndex returned error: %#x", eRet);
    }

    if (mUseDMABuffer) {
        /*Enable DMA Buffer*/
        OMX_INDEXTYPE index;
        OMX_BOOL useDMABuffers = OMX_TRUE;
        eRet = OMX_GetExtensionIndex(mVDecoderHandle, (char *)"OMX.amlogic.android.index.EnableDMABuffers", &index);
        if (eRet == OMX_ErrorNone) {
            ALOGD("OMX_GetExtensionIndex returned %#x", index);
            eRet = OMX_SetParameter(mVDecoderHandle, index, &useDMABuffers);
            if (eRet != OMX_ErrorNone)
                ALOGW("setting enableDMABuffers returned error: %#x", eRet);
        } else
            ALOGW("OMX_GetExtensionIndex returned error: %#x", eRet);
    }

    mVideoOutputPortParam.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    mVideoOutputPortParam.nVersion = mSpecVersion;
    mVideoOutputPortParam.nPortIndex = mPortParam.nStartPortNumber + 1;
    eRet = OMX_GetParameter(mVDecoderHandle, OMX_IndexParamPortDefinition, &mVideoOutputPortParam);
    ALOGD("[%s:%d]mVideoOutputPortParam.format.video.eCompressionFormat = %d\n",
            __FUNCTION__, __LINE__,
            mVideoOutputPortParam.format.video.eCompressionFormat);

    if (OMX_ErrorNone != eRet)
        ALOGE("[%s:%d]mVDecoderHandle OMX_IndexParamPortDefinition error!! eRet = %#x\n",
                __FUNCTION__, __LINE__, eRet);

    mVideoOutputPortParam.nBufferCountActual = mOutBufferCount;
    mVideoOutputPortParam.format.video.nFrameWidth = mWidth;
    mVideoOutputPortParam.format.video.nFrameHeight = mHeight;
    mVideoOutputPortParam.format.video.nStride = ROUND_16(mWidth);
    mVideoOutputPortParam.format.video.nSliceHeight = ROUND_16(mHeight);
    mVideoOutputPortParam.format.video.eColorFormat = static_cast<OMX_COLOR_FORMATTYPE>(HAL_PIXEL_FORMAT_YCrCb_420_SP);//OMX_COLOR_FormatYUV420SemiPlanar;
    mVideoOutputPortParam.format.video.xFramerate = (15 << 16);
    mVideoOutputPortParam.nBufferSize = YUV_SIZE(mVideoOutputPortParam.format.video.nStride,
            mVideoOutputPortParam.format.video.nSliceHeight);

    eRet = OMX_SetParameter(mVDecoderHandle, OMX_IndexParamPortDefinition, &mVideoOutputPortParam);
    if (OMX_ErrorNone != eRet) {
        ALOGE("[%s:%d]OMX_SetParameter OMX_IndexParamPortDefinition error!! eRet = %#x\n",
                __FUNCTION__, __LINE__, eRet);
        return false;
    }

    eRet = OMX_GetParameter(mVDecoderHandle, OMX_IndexParamPortDefinition, &mVideoOutputPortParam);
    if (OMX_ErrorNone != eRet) {
        ALOGE("[%s:%d]OMX_GetParameter OMX_IndexParamPortDefinition error!! eRet = %#x\n",
                __FUNCTION__, __LINE__, eRet);
        return false;
    }

    ALOGD("[%s:%d]mVideoOutputPortParam.nBufferCountActual = %zu, mVideoOutputPortParam.nBufferSize = %zu\n",
            __FUNCTION__, __LINE__, mVideoOutputPortParam.nBufferCountActual, mVideoOutputPortParam.nBufferSize);

    OMX_SendCommand(mVDecoderHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

    return true;
}

template<class T>
void OMXDecoder::InitOMXParams(T *params) {
    memset(params, 0, sizeof(T));
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

void OMXDecoder::start()
{
    LOG_LINE();
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    AutoMutex l(mOutputBufferLock);
    while (!mListOfOutputBufferHeader.empty()) {
        pBufferHdr = *mListOfOutputBufferHeader.begin();
        OMX_FillThisBuffer(mVDecoderHandle, pBufferHdr);
        mListOfOutputBufferHeader.erase(mListOfOutputBufferHeader.begin());
    }
}

void OMXDecoder::deinitialize()
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_STATETYPE eState1, eState2;
    LOG_LINE();
    for (int i = 0; i < 3; i++)
            free(mTempFrame[i]);
    mNoFreeFlag = 1;
    if (mVDecoderHandle == NULL) {
        ALOGD("mVDecoderHandle is NULL, alread deinitialized or not initialized at all");
        return;
    }
    OMX_SendCommand(mVDecoderHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    do {
        eRet = OMX_GetState(mVDecoderHandle, &eState1);
    } while (OMX_StateIdle != eState1 && OMX_StateInvalid != eState1);

    if (eRet != OMX_ErrorNone) {
        ALOGE("Switch to StateIdle failed");
    }
    ALOGD("Switch to StateIdle successful");

    OMX_SendCommand(mVDecoderHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);

    while (mListOfInputBufferHeader.size() != mVideoInputPortParam.nBufferCountActual
            || mListOfOutputBufferHeader.size() != mVideoOutputPortParam.nBufferCountActual) {
        ALOGD("Input: %u/%u  Output: %u/%u",
                mListOfInputBufferHeader.size(), mVideoInputPortParam.nBufferCountActual,
                mListOfOutputBufferHeader.size(), mVideoOutputPortParam.nBufferCountActual);
        usleep(5000);
    }

    freeBuffers();

    do {
        eRet = OMX_GetState(mVDecoderHandle, &eState2);
    } while (OMX_StateLoaded != eState2 && OMX_StateInvalid != eState2);

    if (eRet != OMX_ErrorNone) {
        ALOGE("Switch to StateLoaded failed");
    }
    ALOGD("Switch to StateLoaded successful");

    (*mFreeHandle)(static_cast<OMX_HANDLETYPE *>(mVDecoderHandle));
    (*mDeinit)();
    if (mLibHandle != NULL) {
        ALOGD("dlclose lib handle at %p and null it", mLibHandle);
    }
    //dlclose(mLibHandle);
    //mLibHandle = NULL;
}

OMX_BUFFERHEADERTYPE* OMXDecoder::dequeueInputBuffer()
{
    AutoMutex l(mInputBufferLock);
    OMX_BUFFERHEADERTYPE *ret = NULL;
    if (!mListOfInputBufferHeader.empty()) {
        ret = *mListOfInputBufferHeader.begin();
        mListOfInputBufferHeader.erase(mListOfInputBufferHeader.begin());
    }
    return ret;
}

void OMXDecoder::queueInputBuffer(OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    if (pBufferHdr != NULL)
        if (mNoFreeFlag) {
            ALOGD("exiting!! return to input queue.");
            AutoMutex l(mInputBufferLock);
            mListOfInputBufferHeader.push_back(pBufferHdr);
        } else
            OMX_EmptyThisBuffer(mVDecoderHandle, pBufferHdr);
        else
            ALOGD("queueInputBuffer can't find pBufferHdr .\n");
}

OMX_BUFFERHEADERTYPE* OMXDecoder::dequeueOutputBuffer()
{
    AutoMutex l(mOutputBufferLock);
    OMX_BUFFERHEADERTYPE *ret = NULL;
    if (!mListOfOutputBufferHeader.empty()) {
        ret = *mListOfOutputBufferHeader.begin();
        mListOfOutputBufferHeader.erase(mListOfOutputBufferHeader.begin());
    }
    return ret;
}

void OMXDecoder::releaseOutputBuffer(OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    if (pBufferHdr != NULL)
        if (mNoFreeFlag) {
            ALOGD("exiting!! return to output queue.");
            AutoMutex l(mOutputBufferLock);
            mListOfOutputBufferHeader.push_back(pBufferHdr);
        } else
            OMX_FillThisBuffer(mVDecoderHandle, pBufferHdr);
        else
            ALOGD("releaseOutputBuffer can't find pBufferHdr .\n");
}


bool OMXDecoder::normal_buffer_init(int buffer_size){
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    for (uint32_t i = 0; i < mOutBufferCount; i++) {
        if (mUseDMABuffer) {
            OMX_BUFFERHEADERTYPE* bufferHdr;
            OMX_U8 *ptr = (OMX_U8 *)(malloc(buffer_size * sizeof(OMX_U8)));
            if (!ptr) {
                ALOGE("out of memory when allocation output buffers");
                return false;
            }
            eRet = OMX_UseBuffer(mVDecoderHandle, &bufferHdr,
                    mVideoOutputPortParam.nPortIndex, NULL,
                    buffer_size, ptr);
            if (OMX_ErrorNone != eRet) {
                ALOGE("OMX_UseBuffer on output port failed! eRet = %#x\n", eRet);
                return false;
            }
            ALOGD("OMX_UseBuffer output %p", bufferHdr);
            mListOfOutputBufferHeader.push_back(bufferHdr);
        } else {
            sp<GraphicBuffer> graphicBuffer(new GraphicBuffer(mOutBufferNative[i].handle,
                        GraphicBuffer::TAKE_HANDLE,
                        mWidth,
                        mHeight,
                        mFormat,
                        1,
                        GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK,
                        mStride));
            OMX_STRING nameEnable = const_cast<OMX_STRING>(
                    "OMX.google.android.index.enableAndroidNativeBuffers");
            OMX_INDEXTYPE indexEnable;
            OMX_ERRORTYPE err = OMX_GetExtensionIndex(mVDecoderHandle, nameEnable, &indexEnable);
            if (err == OMX_ErrorNone) {
                EnableAndroidNativeBuffersParams params;
                InitOMXParams(&params);
                params.nPortIndex = mVideoOutputPortParam.nPortIndex;
                params.enable = OMX_TRUE;

                err = OMX_SetParameter(mVDecoderHandle, indexEnable, &params);
                if (err != OMX_ErrorNone)
                    ALOGE("setParameter error 1.");
            } else {
                ALOGE("getExtensionIndex failed 1.");
            }

            OMX_STRING name = const_cast<OMX_STRING>(
                    "OMX.google.android.index.useAndroidNativeBuffer");
            OMX_INDEXTYPE index;
            err = OMX_GetExtensionIndex(mVDecoderHandle, name, &index);
            if (err != OMX_ErrorNone) {
                ALOGE("getExtensionIndex failed 2.");
            }
            OMX_BUFFERHEADERTYPE* header;
            OMX_VERSIONTYPE ver;
            ver.s.nVersionMajor = 1;
            ver.s.nVersionMinor = 0;
            ver.s.nRevision = 0;
            ver.s.nStep = 0;
            UseAndroidNativeBufferParams params = {
                sizeof(UseAndroidNativeBufferParams), ver, mVideoOutputPortParam.nPortIndex, NULL,
                &header, graphicBuffer
            };

            err = OMX_SetParameter(mVDecoderHandle, index, &params);
            if (err != OMX_ErrorNone)
                ALOGE("setParameter error 2.");

            mOutBufferNative[i].pBuffer = header;
            if (mOutBufferNative[i].isQueued)
                mListOfOutputBufferHeader.push_back(header);
        }
    }
    return true;
}

bool OMXDecoder::ion_buffer_init() {

    ion_user_handle_t ion_hnd;
    int shared_fd;
    int ret = 0;
    int buffer_size = mWidth * mHeight * 3 / 2 ;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned int ion_flag = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
    mIonFd = ion_open();
    if (mIonFd < 0) {
        ALOGE("ion open failed! fd= %d \n", mIonFd);
        return false;
    }
    OMX_U32 uAlignedBytes = (((mVideoOutputPortParam.nBufferSize
                    + ZTE_BUF_ADDR_ALIGNMENT_VALUE - 1)
                & ~(ZTE_BUF_ADDR_ALIGNMENT_VALUE - 1)));
    for (uint32_t i = 0; i < mVideoOutputPortParam.nBufferCountActual; i++) {
        OMX_BUFFERHEADERTYPE* bufferHdr;
        OMX_U8 *cpu_ptr;

        if (mUseDMABuffer) {
            ALOGD("try to allocate dma buffer %d", i);
            ret = ion_alloc(mIonFd, buffer_size, 0, 1 << ION_HEAP_TYPE_CUSTOM, ion_flag, &ion_hnd);
            if (ret) {
                ALOGE("ion alloc error, errno=%d",ret);
                ion_close(mIonFd);
                return false;
            } else
                ALOGD("allocating dma buffer %d success, handle %d", i, ion_hnd);
            ret = ion_share(mIonFd, ion_hnd, &shared_fd);
            if (ret) {
                ALOGE("ion share error!, errno=%d\n",ret);
                ion_free(mIonFd, ion_hnd);
                ion_close(mIonFd);
                return false;
            }
            cpu_ptr = (OMX_U8*)mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    shared_fd, 0);
            if (MAP_FAILED == cpu_ptr) {
                ALOGE("ion mmap error!\n");
                close(shared_fd);
                ion_free(mIonFd, ion_hnd);
                ion_close(mIonFd);
                return false;
            }
            if (cpu_ptr == NULL)
                ALOGD("cpu_ptr is NULL");
            ALOGD("AllocDmaBuffers ion_hnd=%d, shared_fd=%d, cpu_ptr=%p\n", ion_hnd, shared_fd, cpu_ptr);
        } else {
            cpu_ptr = (OMX_U8 *)(malloc(uAlignedBytes * sizeof(OMX_U8)));
            if (!cpu_ptr) {
                ALOGE("out of memory when allocation output buffers");
                return false;
            }
        }
        eRet = OMX_UseBuffer(mVDecoderHandle,
                &bufferHdr,
                mVideoOutputPortParam.nPortIndex,
                (OMX_PTR)shared_fd,
                mVideoOutputPortParam.nBufferSize,
                (OMX_U8*)cpu_ptr);
        if (OMX_ErrorNone != eRet) {
            ALOGE("OMX_UseBuffer on output port failed! eRet = %#x\n", eRet);
            return false;
        }
        bufferHdr->pAppPrivate = (OMX_PTR)ion_hnd;
        mListOfOutputBufferHeader.push_back(bufferHdr);
    }
    return true;
}

bool OMXDecoder::prepareBuffers()
{
    LOG_LINE();
    OMX_U32 uAlignedBytes = (((mVideoInputPortParam.nBufferSize + ZTE_BUF_ADDR_ALIGNMENT_VALUE - 1) & ~(ZTE_BUF_ADDR_ALIGNMENT_VALUE - 1)));
    mNoFreeFlag = 0;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    for (uint32_t i = 0; i < mVideoInputPortParam.nBufferCountActual; i++) {
        OMX_BUFFERHEADERTYPE* bufferHdr;
        OMX_U8 *ptr = (OMX_U8 *)(malloc(uAlignedBytes * sizeof(OMX_U8)));
        if (!ptr) {
            ALOGE("out of memory when allocation input buffers");
            return false;
        }
        eRet = OMX_UseBuffer(mVDecoderHandle, &bufferHdr,
                mVideoInputPortParam.nPortIndex, NULL,
                mVideoInputPortParam.nBufferSize, ptr);

        if (OMX_ErrorNone != eRet) {
            ALOGE("OMX_UseBuffer on input port failed! eRet = %#x\n", eRet);
            return false;
        }
        ALOGD("OMX_UseBuffer input %p", bufferHdr);
        mListOfInputBufferHeader.push_back(bufferHdr);
    }

    OMX_STATETYPE eState1, eState2;
    int buffer_size = mWidth * mHeight * 3 / 2 ;
    ALOGD("Allocating %u buffers from a native window of size %u on "
            "output port", mOutBufferCount, buffer_size);

    ion_buffer_init();
    //normal_buffer_init(buffer_size);

    do {
        OMX_GetState(mVDecoderHandle, &eState1);
    } while (OMX_StateIdle != eState1 && OMX_StateInvalid != eState1);
    ALOGD("STATETRANS FROM LOADED TO IDLE COMPLETED, eRet =%x\n", eRet);

    //swith to Excuting state
    OMX_SendCommand(mVDecoderHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

    do {
        OMX_GetState(mVDecoderHandle, &eState2);
    } while (OMX_StateExecuting != eState2 && OMX_StateInvalid != eState2);

    ALOGD("STATETRANS FROM IDLE TO EXECUTING COMPLETED, eRet =%x\n", eRet);

    return true;
}

void OMXDecoder::free_ion_buffer(void) {
    while (!mListOfOutputBufferHeader.empty()) {
        OMX_BUFFERHEADERTYPE* bufferHdr = *(mListOfOutputBufferHeader.begin());
        OMX_ERRORTYPE err;
        if (bufferHdr != NULL) {
            if (mUseDMABuffer) {
                munmap(bufferHdr->pBuffer, mWidth * mHeight * 3 / 2);
                int ret = close((int)bufferHdr->pPlatformPrivate);
                if (ret != 0) {
                    ALOGD("close ion shared fd failed for reason %s",strerror(errno));
                }
                ALOGD("bufferHdr->pAppPrivate: %p", bufferHdr->pAppPrivate);
                ret = ion_free(mIonFd, (ion_user_handle_t)(bufferHdr->pAppPrivate));
                if (ret != 0) {
                    ALOGD("ion_free failed for reason %s",strerror(errno));
                }
                err = OMX_FreeBuffer(mVDecoderHandle,mVideoOutputPortParam.nPortIndex,bufferHdr);
                if (OMX_ErrorNone != err) {
                    ALOGE("%d, OutPortIndex: %d\n",__LINE__,mVideoOutputPortParam.nPortIndex);
                }
            } else if (bufferHdr->pBuffer != NULL)
                free(bufferHdr->pBuffer);
        }
        mListOfOutputBufferHeader.erase(mListOfOutputBufferHeader.begin());
    }
    if (mIonFd != -1) {
        int ret = ion_close(mIonFd);
        ALOGD("free_ion_buffer:close ion device fd %s",ret==0 ? "sucess":strerror(errno));
        mIonFd = -1;
    }
}

void OMXDecoder::free_normal_buffer(void) {
    OMX_ERRORTYPE err;
    while (!mListOfOutputBufferHeader.empty()) {
        OMX_BUFFERHEADERTYPE* bufferHdr = *(mListOfOutputBufferHeader.begin());
        if (bufferHdr != NULL) {
            OMX_U8 *pOut = bufferHdr->pBuffer;
            ALOGD("OMX_FreeBuffer output %p", bufferHdr);
            err = OMX_FreeBuffer(mVDecoderHandle, mVideoOutputPortParam.nPortIndex, bufferHdr);
            if (OMX_ErrorNone != err) {
                ALOGE("%d, OutPortIndex: %d\n", __LINE__, mVideoOutputPortParam.nPortIndex);
            }
            if (pOut != NULL) {
                free(pOut);
                pOut = NULL;
            }
        }
        mListOfOutputBufferHeader.erase(mListOfOutputBufferHeader.begin());
    }
}

void OMXDecoder::freeBuffers() {
    OMX_ERRORTYPE err;
    unsigned int i;
    while (!mListOfInputBufferHeader.empty()) {
        OMX_BUFFERHEADERTYPE* bufferHdr = *(mListOfInputBufferHeader.begin());
        if (bufferHdr != NULL) {
            OMX_U8 *pIn = bufferHdr->pBuffer;
            ALOGD("OMX_FreeBuffer input %p", bufferHdr);
            err = OMX_FreeBuffer(mVDecoderHandle, mVideoInputPortParam.nPortIndex, bufferHdr);
            if (OMX_ErrorNone != err) {
                ALOGE("%d, InPortIndex: %d\n", __LINE__, mVideoInputPortParam.nPortIndex);
            }
            if (pIn != NULL) {
                free(pIn);
                pIn = NULL;
            }
        }
        mListOfInputBufferHeader.erase(mListOfInputBufferHeader.begin());
    }

    if (mUseDMABuffer) {
        free_ion_buffer();
        //free_normal_buffer();
    } else {
        for (i = 0; i < mOutBufferCount; i++) {
            OMX_BUFFERHEADERTYPE* bufferHdr = mOutBufferNative[i].pBuffer;
            if (bufferHdr != NULL) {
                err = OMX_FreeBuffer(mVDecoderHandle, mVideoOutputPortParam.nPortIndex, bufferHdr);
                if (OMX_ErrorNone != err) {
                    ALOGE("%d, OutPortIndex: %d\n", __LINE__, mVideoOutputPortParam.nPortIndex);
                }
            }
        }

        if (mOutBufferNative != NULL) {
            free(mOutBufferNative);
            mOutBufferNative = NULL;
        }
    }
}

OMX_ERRORTYPE OMXDecoder::WaitForState(OMX_HANDLETYPE hComponent, OMX_STATETYPE eTestState, OMX_STATETYPE eTestState2)
{
    LOG_LINE();
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_STATETYPE eState;

    eError = OMX_GetState(hComponent, &eState);

    while (eState != eTestState && eState != eTestState2)
    {
        sleep(1);
        eError = OMX_GetState(hComponent, &eState);
    }
    return eError;
}

OMX_ERRORTYPE OMXDecoder::OnEvent(
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR)
{
    LOG_LINE();
    ALOGD("data1 = %zu, data2 = %zu, event = %d\n", nData1, nData2, eEvent);

    if (eEvent == OMX_EventBufferFlag)
    {
        ALOGD("Got OMX_EventBufferFlag event\n");
    }
    else if (eEvent == OMX_EventError)
    {
        ALOGD("Got OMX_EventError event\n");
    }
    else if (eEvent == OMX_EventPortSettingsChanged)
    {
        OMX_ERRORTYPE omx_error_type = OMX_ErrorNone;
        ALOGD("Got OMX_EventPortSettingsChanged event\n");
        if (OMX_IndexParamPortDefinition == (OMX_INDEXTYPE) nData2)
        {
            mVideoOutputPortParam.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
            mVideoOutputPortParam.nVersion = mSpecVersion;
            mVideoOutputPortParam.nPortIndex = nData1;
            omx_error_type = OMX_GetParameter(mVDecoderHandle, OMX_IndexParamPortDefinition, &mVideoOutputPortParam);
            if (omx_error_type != OMX_ErrorNone)
            {
                ALOGD("OMX_GetParameter FAILED");
            }
            ALOGD("w= %zu, h= %zu\n", mVideoOutputPortParam.format.video.nFrameWidth, mVideoOutputPortParam.format.video.nFrameHeight);
            if (mWidth != mVideoOutputPortParam.format.video.nFrameWidth || mHeight != mVideoOutputPortParam.format.video.nFrameHeight)
            {
                ALOGD("Dynamic resolution changes triggered");
            }
        }
    }
    else
    {
        ALOGD("not support event!\n");
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXDecoder::emptyBufferDone(OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    AutoMutex l(mInputBufferLock);
    mListOfInputBufferHeader.push_back(pBuffer);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXDecoder::fillBufferDone(OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    AutoMutex l(mOutputBufferLock);
    mListOfOutputBufferHeader.push_back(pBuffer);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXDecoder::OnEvent(
        OMX_IN OMX_HANDLETYPE,/*ommit hComponent to avoid unused variable warning*/
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    OMXDecoder *instance = static_cast<OMXDecoder *>(pAppData);
    return instance->OnEvent(eEvent, nData1, nData2, pEventData);
}

OMX_ERRORTYPE OMXDecoder::OnEmptyBufferDone(
        OMX_IN OMX_HANDLETYPE,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMXDecoder *instance = static_cast<OMXDecoder *>(pAppData);
    return instance->emptyBufferDone(pBuffer);
}

OMX_ERRORTYPE OMXDecoder::OnFillBufferDone(
        OMX_IN OMX_HANDLETYPE,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMXDecoder *instance = static_cast<OMXDecoder *>(pAppData);
    return instance->fillBufferDone(pBuffer);
}

void OMXDecoder::QueueBuffer(uint8_t* src, size_t size) {
    static OMX_TICKS timeStamp = 0;
    OMX_BUFFERHEADERTYPE *pInPutBufferHdr = NULL;
    pInPutBufferHdr = dequeueInputBuffer();
    //ALOGD("omx pInPutBufferHdr = %p\n", pInPutBufferHdr);
    if (pInPutBufferHdr && pInPutBufferHdr->pBuffer) {
        memcpy(pInPutBufferHdr->pBuffer, src, size);
        pInPutBufferHdr->nFilledLen = size;
        pInPutBufferHdr->nOffset = 0;
        pInPutBufferHdr->nTimeStamp = timeStamp;
        pInPutBufferHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        queueInputBuffer(pInPutBufferHdr);
        timeStamp += 33 * 1000; //44
    }
}

int OMXDecoder::DequeueBuffer(int dst_fd ,uint8_t* dst_buf,
                                    size_t dst_w, size_t dst_h) {

        OMX_BUFFERHEADERTYPE *pOutPutBufferHdr = NULL;
         pOutPutBufferHdr = dequeueOutputBuffer();
        if (pOutPutBufferHdr == NULL) {
            //dequeue fail
            //ALOGD("%s:dequeue fail",__FUNCTION__);
            return 0;
        } else {
            //ALOGD("omx pOutPutBufferHdr = %p\n", pOutPutBufferHdr);
#ifdef GE2D_ENABLE
           if (dst_fd != -1) {
                //copy data using ge2d
                int omx_share_fd = (int)pOutPutBufferHdr->pPlatformPrivate;
                ge2dDevice::ge2d_copy(dst_fd,omx_share_fd,dst_w,dst_h);
            }
            else
                memcpy(dst_buf, pOutPutBufferHdr->pBuffer, pOutPutBufferHdr->nFilledLen);
#else
            //no ge2d support
            memcpy(dst_buf, pOutPutBufferHdr->pBuffer, pOutPutBufferHdr->nFilledLen);

#endif
            releaseOutputBuffer(pOutPutBufferHdr);
            return 1;
        }
        return 1;
}


int OMXDecoder::Decode(uint8_t*src, size_t src_size,
                          int dst_fd,uint8_t *dst_buf,
                          size_t dst_w, size_t dst_h) {
        QueueBuffer(src, src_size);
        int ret = DequeueBuffer(dst_fd,dst_buf,
                                dst_w,dst_h);
        if (!ret) {
            mDequeueFailNum ++;
            ALOGD("%s:FailNumber=%d",__FUNCTION__,mDequeueFailNum);
        }

        if (ret && mDequeueFailNum >= 1) {
            int r = 0;
            int i = 0;
            for (i = 0 ; i < 3; i++) {
                /*becase mTempFrame is not physical consist memory,
                 *if decode image to this buffer, we can't use ge2d to rotate
                 *the image.
                 */
                int value = DequeueBuffer(-1,mTempFrame[i],dst_w,dst_h);
                if (value) {
                    ALOGD("%s:read cached data.",__FUNCTION__);
                    mDequeueFailNum -= 1;
                    r = 1;
                    continue;
                }
                else
                    break;
            }
            if (r && ((i-1) >= 0)) {
                ALOGD("%s:i=%d",__FUNCTION__,i);
                memcpy(dst_buf,mTempFrame[i-1],mWidth*mHeight*3/2);
                ret = r;
            }
    }
    return ret;
}
