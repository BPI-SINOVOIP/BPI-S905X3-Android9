#ifndef MEDIA_ADECOMX_H_
#define MEDIA_ADECOMX_H_

#include "OMX_Index.h"
#include "OMX_Core.h"
#include "OMXClient.h"
#include "audio_mediasource.h"
#include "../audio-dec.h"

namespace android
{

static const char *MEDIA_MIMETYPE_AUDIO_AC3 = "audio/ac3";
static const char *MEDIA_MIMETYPE_AUDIO_EC3 = "audio/eac3";
static const char *MEDIA_MIMETYPE_AUDIO_AMR_NB = "audio/3gpp";
static const char *MEDIA_MIMETYPE_AUDIO_AMR_WB = "audio/amr-wb";
static const char *MEDIA_MIMETYPE_AUDIO_MPEG = "audio/mpeg"; //mp3
static const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_I = "audio/mpeg-L1";
static const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II = "audio/mpeg-L2";
static const char *MEDIA_MIMETYPE_AUDIO_AAC = "audio/mp4a-latm";
static const char *MEDIA_MIMETYPE_AUDIO_QCELP = "audio/qcelp";
static const char *MEDIA_MIMETYPE_AUDIO_VORBIS = "audio/vorbis";
static const char *MEDIA_MIMETYPE_AUDIO_G711_ALAW = "audio/g711-alaw";
static const char *MEDIA_MIMETYPE_AUDIO_G711_MLAW = "audio/g711-mlaw";
static const char *MEDIA_MIMETYPE_AUDIO_RAW = "audio/raw";
static const char *MEDIA_MIMETYPE_AUDIO_ADPCM_IMA = "audio/adpcm-ima";
static const char *MEDIA_MIMETYPE_AUDIO_ADPCM_MS = "audio/adpcm-ms";
static const char *MEDIA_MIMETYPE_AUDIO_FLAC = "audio/flac";
static const char *MEDIA_MIMETYPE_AUDIO_AAC_ADTS = "audio/aac-adts";
static const char *MEDIA_MIMETYPE_AUDIO_ALAC = "audio/alac";
static const char *MEDIA_MIMETYPE_AUDIO_AAC_ADIF = "audio/aac-adif";
static const char *MEDIA_MIMETYPE_AUDIO_AAC_LATM = "audio/aac-latm";
static const char *MEDIA_MIMETYPE_AUDIO_ADTS_PROFILE = "audio/adts";
static const char *MEDIA_MIMETYPE_AUDIO_WMA = "audio/wma";
static const char *MEDIA_MIMETYPE_AUDIO_WMAPRO = "audio/wmapro";
static const char *MEDIA_MIMETYPE_AUDIO_DTSHD = "audio/dtshd";
static const char *MEDIA_MIMETYPE_AUDIO_TRUEHD = "audio/truehd";
static const char *MEDIA_MIMETYPE_AUDIO_FFMPEG = "audio/ffmpeg";

class AmlOMXCodec
{
public:

    AmlOMXCodec(int codec_type, void *read_buffer, int *exit, aml_audio_dec *audec);

    OMXClient             m_OMXClient;
    sp<AudioMediaSource>  m_OMXMediaSource;
    int                   read(unsigned char *buf, unsigned *size, int *exit);
    virtual status_t      start(aml_audio_dec_t *audec);
    void                  pause();
    void                  stop();
    int                   GetDecBytes();
    void                  lock_init();
    void                  locked();
    void                  unlocked();
    int                   started_flag;
    int                   omx_codec_type;
    //protected:
    virtual ~AmlOMXCodec();
    //private:
    sp<MediaSource>       m_codec;
    pthread_mutex_t       lock;
    int64_t buf_decode_offset;
    int64_t buf_decode_offset_pre;

};


}

#endif
