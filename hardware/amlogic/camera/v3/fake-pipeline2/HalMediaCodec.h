#pragma once
#include <media/NdkMediaCodec.h>
//class AMediaCodec;
namespace android {
class HalMediaCodec
{
public:
    int mWidth;
    int mHeight;
    int mOriFrameSize;
    int mFrameSize;
	AMediaCodec *mMediaCodec;

public:
    int init(int width, int height,const char* name);
    int fini();
    int decode(uint8_t*bufData, size_t bufSize, uint8_t*outBuf);
private:
    unsigned int timeGetTime();
    void NV12toNV21(uint8_t *buf, int width, int height);
	void QueueBuffer(uint8_t*bufData,size_t bufSize);
	int DequeueBuffer(uint8_t*outBuf);
private:
	int mDecoderFailNumber;
	uint8_t* mTempOutBuffer[3];
};
}
