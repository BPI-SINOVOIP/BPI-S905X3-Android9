#ifndef OMX_AUDIOMEDIASOURCE_H_
#define OMX_AUDIOMEDIASOURCE_H_

#include  "MediaSource.h"
#include  "DataSource.h"
#include  "MediaBufferGroup.h"
#include  "MetaData.h"
#include  "MediaBufferBase.h"

namespace android
{

class AudioMediaSource : public MediaSource
{
public:
    virtual status_t start(MetaData *params = NULL) = 0;
    virtual status_t stop() = 0;
    virtual sp<MetaData> getFormat() = 0;
    virtual status_t read(MediaBufferBase **buffer, const ReadOptions *options = NULL) = 0;

    virtual int  GetReadedBytes() = 0;
    virtual int GetSampleRate() = 0;
    virtual int SetSampleRate(int sample_rate __unused){
        return 0;
    };
    virtual int GetChNum() = 0;
    virtual int GetChNumOriginal() {
        return 0;
    };
    virtual int* Get_pStop_ReadBuf_Flag() = 0;
    virtual int Set_pStop_ReadBuf_Flag(int *pStop) = 0;

protected:

    AudioMediaSource();
    virtual ~AudioMediaSource();

private:

    AudioMediaSource(const AudioMediaSource &);
    AudioMediaSource &operator=(const AudioMediaSource &);
};


}

#endif
