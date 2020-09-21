/*
interface to call OMX codec
*/

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/SimpleDecodingSource.h>
#include "../adec_omx_brige.h"
#include "adec_omx.h"
#include "audio_mediasource.h"
#include "DDP_mediasource.h"
#include "ALAC_mediasource.h"
#include "MP3_mediasource.h"
#include "ASF_mediasource.h"
#include "DTSHD_mediasource.h"
#include "Vorbis_mediasource.h"
#include "THD_mediasource.h"
#include <android/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <cutils/properties.h>
#include <Amsysfsutils.h>
#include <media/IOMX.h>
#include "AmMetaDataExt.h"
#include <media/AudioSystem.h>
#include <system/audio.h>
#if ANDROID_PLATFORM_SDK_VERSION >= 25 //8.0
#include <system/audio-base.h>
#endif

//#define LOG_TAG "Adec_OMX"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"Adec_OMX",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"Adec_OMX",__VA_ARGS__)

namespace android
{

//#####################################################

AmlOMXCodec::AmlOMXCodec(int codec_type, void *read_buffer, int *exit, aml_audio_dec_t *audec)
{
    m_codec = NULL;
    status_t m_OMXClientConnectStatus = m_OMXClient.connect();
    lock_init();
    locked();
    buf_decode_offset = 0;
    buf_decode_offset_pre = 0;
    if (m_OMXClientConnectStatus != OK) {
        LOGE("Err:omx client connect error\n");
    } else {
        const char *mine_type = NULL;
        audec->data_width = AV_SAMPLE_FMT_S16;
        omx_codec_type = 0;
        if (audec->channels > 0) {
            audec->channels = (audec->channels > 2 ? 2 : audec->channels);
            audec->adec_ops->channels = audec->channels;
        } else {
            audec->adec_ops->channels = audec->channels = 2;
        }

        if (audec->samplerate > 0) {
            audec->adec_ops->samplerate = audec->samplerate;
        } else {
            audec->adec_ops->samplerate = audec->samplerate = 48000;
        }

        LOGI("Data_width:%d Samplerate:%d Channel:%d \n", audec->data_width, audec->samplerate, audec->channels);

        if (codec_type == OMX_ENABLE_CODEC_AC3) {
            mine_type = MEDIA_MIMETYPE_AUDIO_AC3;
            m_OMXMediaSource = new DDP_MediaSource(read_buffer);
        } else if (codec_type == OMX_ENABLE_CODEC_EAC3) {
            mine_type = MEDIA_MIMETYPE_AUDIO_EC3;
            m_OMXMediaSource = new DDP_MediaSource(read_buffer);
        } else if (codec_type == OMX_ENABLE_CODEC_ALAC) {
            mine_type = MEDIA_MIMETYPE_AUDIO_ALAC;
            m_OMXMediaSource = new ALAC_MediaSource(read_buffer, audec);
        } else if (codec_type == OMX_ENABLE_CODEC_MPEG_LAYER_II) {
            mine_type = MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II;
            //mine_type=MEDIA_MIMETYPE_AUDIO_MPEG;
            m_OMXMediaSource = new MP3_MediaSource(read_buffer, audec, exit);
        } else if (codec_type == OMX_ENABLE_CODEC_WMA) {
            mine_type = MEDIA_MIMETYPE_AUDIO_WMA;
            m_OMXMediaSource = new Asf_MediaSource(read_buffer, audec);
        } else if (codec_type == OMX_ENABLE_CODEC_WMAPRO) {
            mine_type = MEDIA_MIMETYPE_AUDIO_WMAPRO;
            m_OMXMediaSource = new Asf_MediaSource(read_buffer, audec);
        } else if (codec_type == OMX_ENABLE_CODEC_DTSHD) {
            mine_type = MEDIA_MIMETYPE_AUDIO_DTSHD;
            m_OMXMediaSource = new Dtshd_MediaSource(read_buffer);
        } else if (codec_type == OMX_ENABLE_CODEC_VORBIS) {
            mine_type = MEDIA_MIMETYPE_AUDIO_VORBIS;
            m_OMXMediaSource = new Vorbis_MediaSource(read_buffer, audec);
        } else if (codec_type == OMX_ENABLE_CODEC_TRUEHD) {
            mine_type = MEDIA_MIMETYPE_AUDIO_TRUEHD;
            m_OMXMediaSource = new THD_MediaSource(read_buffer);
        } else if (codec_type == OMX_ENABLE_CODEC_WMAVOI) {
            mine_type = MEDIA_MIMETYPE_AUDIO_FFMPEG;
            m_OMXMediaSource = new Asf_MediaSource(read_buffer, audec);
        }
        omx_codec_type = codec_type;
        LOGI("mine_type=%s %s %d \n", mine_type, __FUNCTION__, __LINE__);

        m_OMXMediaSource->Set_pStop_ReadBuf_Flag(exit);
        audio_format_t aformat = AUDIO_FORMAT_INVALID;
        if (audec->format == ACODEC_FMT_AC3) {
            aformat = AUDIO_FORMAT_AC3;
            if (AudioSystem::isPassthroughSupported(aformat)) {
                audec->raw_enable = 2;
            } else {
                audec->raw_enable = 0;
            }
        } else if (audec->format == ACODEC_FMT_EAC3) {
            aformat = AUDIO_FORMAT_E_AC3;
            if (AudioSystem::isPassthroughSupported(aformat)) {
                audec->raw_enable = 2;
            } else if (AudioSystem::isPassthroughSupported(AUDIO_FORMAT_AC3)) {
                audec->raw_enable = 1;
            } else {
                audec->raw_enable = 0;
            }
        } else {
            audec->raw_enable = 0;
        }
        LOGI("audec->raw_enable %d", audec->raw_enable);
        sp<MetaData> metadata = m_OMXMediaSource->getFormat();
        metadata->setCString(kKeyMIMEType, mine_type);
        //add 3 to distinguish amadec and mediacodec call
        metadata->setInt32(kKeyIsPtenable, audec->raw_enable + 3);

        m_codec = SimpleDecodingSource::Create(
                      m_OMXMediaSource/*,
                        0,
                        0*/);

        if (m_codec != NULL) {
            LOGI("OMXCodec::Create success %s %d \n", __FUNCTION__, __LINE__);
        } else {
            LOGE("Err: OMXCodec::Create failed %s %d \n", __FUNCTION__, __LINE__);
        }
    }
    unlocked();
}


AmlOMXCodec::~AmlOMXCodec()
{

    m_OMXMediaSource = NULL;
    m_codec = NULL;
}


status_t AmlOMXCodec::read(unsigned char *buf, unsigned *size, int *exit)
{
    if (m_codec == NULL) {
        LOGE("m_codec==NULL  %s %d failed!\n", __FUNCTION__, __LINE__);
        return !OK;
    }
    MediaBuffer *srcBuffer;
    status_t status;
    m_OMXMediaSource->Set_pStop_ReadBuf_Flag(exit);

    if (*exit) {
        LOGI("NOTE:exit flag enabled! [%s %d] \n", __FUNCTION__, __LINE__);
        *size = 0;
        return OK;
    }

    status =  m_codec->read((MediaBufferBase **)&srcBuffer, NULL);

    if (srcBuffer == NULL) {
        if (status == INFO_FORMAT_CHANGED) {
            ALOGI("format changed \n");
        }
        *size = 0;
        return OK;
    }
    if (*size > srcBuffer->range_length()) { //surpose buf is large enough
        *size = srcBuffer->range_length();
    }
    if (status == OK && (*size != 0)) {
        memcpy(buf, (void*)((unsigned long)srcBuffer->data() + srcBuffer->range_offset()), *size);
        srcBuffer->set_range(srcBuffer->range_offset() + (*size), srcBuffer->range_length() - (*size));
        srcBuffer->meta_data().findInt64(kKeyTime, &buf_decode_offset);
    }

    if (srcBuffer->range_length() == 0) {
        srcBuffer->release();
        srcBuffer = NULL;
    }
    return OK;
}

status_t AmlOMXCodec::start(aml_audio_dec_t *audec)
{
    LOGI("[%s %d] \n", __FUNCTION__, __LINE__);
    if (m_codec == NULL) {
        LOGE("m_codec==NULL  %s %d failed!\n", __FUNCTION__, __LINE__);
        return !OK;
    }
    status_t status = m_codec->start();
    if (omx_codec_type == OMX_ENABLE_CODEC_AC3 || omx_codec_type == OMX_ENABLE_CODEC_EAC3  \
        || omx_codec_type == OMX_ENABLE_CODEC_DTSHD || omx_codec_type == OMX_ENABLE_CODEC_TRUEHD) {
        android::sp<android::MetaData> output_format = m_codec->getFormat();
        int enable_flag = 0;
        output_format->findInt32(android::kKeyAudioFlag, &enable_flag);
        LOGI("dts/dolby audio enable flag %d \n", enable_flag);
        audec->audio_decoder_enabled = enable_flag;
    }
    if (status != OK) {
        LOGE("Err:OMX client can't start OMX decoder?! status=%d (0x%08x)\n", (int)status, (int)status);
        m_codec = NULL;
    }
    return status;
}

void  AmlOMXCodec::stop()
{
    LOGI("[%s %d] enter \n", __FUNCTION__, __LINE__);
    if (m_codec != NULL) {
        if (m_OMXMediaSource->Get_pStop_ReadBuf_Flag()) {
            *m_OMXMediaSource->Get_pStop_ReadBuf_Flag() = 1;
        }
        m_codec->pause();
        m_codec->stop();
        wp<MediaSource> tmp = m_codec;
        m_codec.clear();
        while (tmp.promote() != NULL) {
            LOGI("[%s %d]wait m_codec free OK!\n", __FUNCTION__, __LINE__);
            usleep(1000);
        }

        //m_OMXMediaSource->stop();//stop in omxcodec
        m_OMXClient.disconnect();
        m_OMXMediaSource = NULL;
        m_codec = NULL;
    } else {
        LOGE("m_codec==NULL m_codec->stop() failed! %s %d \n", __FUNCTION__, __LINE__);
    }
}

void AmlOMXCodec::pause()
{
    LOGI("[%s %d] \n", __FUNCTION__, __LINE__);
    if (m_codec != NULL) {
        m_codec->pause();
    } else {
        LOGE("m_codec==NULL m_codec->pause() failed! %s %d \n", __FUNCTION__, __LINE__);
    }
}

int AmlOMXCodec::GetDecBytes()
{
    int used_len = 0;

    if (omx_codec_type == OMX_ENABLE_CODEC_AC3 || omx_codec_type == OMX_ENABLE_CODEC_EAC3) {
        used_len += m_OMXMediaSource->GetReadedBytes();
    }

    if (omx_codec_type == OMX_ENABLE_CODEC_AC3 || omx_codec_type == OMX_ENABLE_CODEC_EAC3 || omx_codec_type == OMX_ENABLE_CODEC_DTSHD) {
        used_len += (buf_decode_offset - buf_decode_offset_pre);
        buf_decode_offset_pre = buf_decode_offset;
        return used_len;
    } else {
        return m_OMXMediaSource->GetReadedBytes();
    }
}

void AmlOMXCodec::lock_init()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&lock, &attr);
    pthread_mutexattr_destroy(&attr);
}
void AmlOMXCodec::locked()
{
    pthread_mutex_lock(&lock);
}
void AmlOMXCodec::unlocked()
{
    pthread_mutex_unlock(&lock);
}

}; // namespace android

//#####################################################
extern "C"
{

    android::AmlOMXCodec *arm_omx_codec = NULL;

    void arm_omx_codec_init(aml_audio_dec_t *audec, int codec_type, void *readbuffer, int *exit)
    {
        char value[128] = {0};
        int ret = 0;
        android::AmlOMXCodec *arm_omx_codec = NULL;
        //amsysfs_write_prop("media.libplayer.dtsopt0", "1");
        LOGI("property_set<media.libplayer.dtsopt0> ret/%d\n", ret);
        arm_omx_codec = new android::AmlOMXCodec(codec_type, readbuffer, exit, audec);
        if (arm_omx_codec == NULL) {
            property_set("media.libplayer.dtsopt0", "0");
            LOGE("Err:arm_omx_codec_init failed\n");
        }
        if (property_get("media.libplayer.dtsopt0", value, NULL) > 0) {
            LOGI("[%s %d]  media.libplayer.dtsopt0/%s \n", __FUNCTION__, __LINE__, value);
        } else {
            LOGE("[%s %d] property_set<media.libplayer.dtsopt0> failed\n", __FUNCTION__, __LINE__);
        }
        LOGI("[%s %d] arm_omx_codec=%p \n", __FUNCTION__, __LINE__, arm_omx_codec);
        audec->arm_omx_codec = arm_omx_codec;
    }

    void arm_omx_codec_start(aml_audio_dec_t *audec)
    {
        android::AmlOMXCodec *arm_omx_codec = (android::AmlOMXCodec *)(audec->arm_omx_codec);
        if (arm_omx_codec != NULL) {
            arm_omx_codec->locked();
            arm_omx_codec->start(audec);
            arm_omx_codec->unlocked();
        } else {
            LOGE("arm_omx_codec==NULL arm_omx_codec->start failed! %s %d \n", __FUNCTION__, __LINE__);
        }
    }

    void arm_omx_codec_pause(aml_audio_dec_t *audec)
    {
        android::AmlOMXCodec *arm_omx_codec = (android::AmlOMXCodec *)(audec->arm_omx_codec);
        if (arm_omx_codec != NULL) {
            arm_omx_codec->locked();
            arm_omx_codec->pause();
            arm_omx_codec->unlocked();
        } else {
            LOGE("arm_omx_codec==NULL  arm_omx_codec->pause failed! %s %d \n", __FUNCTION__, __LINE__);
        }
    }

    void arm_omx_codec_read(aml_audio_dec_t *audec, unsigned char *buf, unsigned *size, int *exit)
    {
        android::AmlOMXCodec *arm_omx_codec = (android::AmlOMXCodec *)(audec->arm_omx_codec);
        if (arm_omx_codec != NULL) {
            arm_omx_codec->locked();
            arm_omx_codec->read(buf, size, exit);
            arm_omx_codec->unlocked();
        } else {
            LOGE("arm_omx_codec==NULL  arm_omx_codec->read failed! %s %d \n", __FUNCTION__, __LINE__);
        }
    }

    void arm_omx_codec_close(aml_audio_dec_t *audec)
    {
        int ret = 0;
        char value[128] = {0};
        android::AmlOMXCodec *arm_omx_codec = (android::AmlOMXCodec *)(audec->arm_omx_codec);
        //amsysfs_write_prop("media.libplayer.dtsopt0", "0");
        LOGI("property_set<media.libplayer.dtsopt0> ret/%d\n", ret);
        if (arm_omx_codec != NULL) {
            arm_omx_codec->locked();
            arm_omx_codec->stop();
            arm_omx_codec->unlocked();
            delete arm_omx_codec;
            arm_omx_codec = NULL;
        } else {
            LOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_close() do nothing! %s %d \n", __FUNCTION__, __LINE__);
        }
        if (property_get("media.libplayer.dtsopt0", value, NULL) > 0) {
            LOGI("[%s %d]  media.libplayer.dtsopt0/%s \n", __FUNCTION__, __LINE__, value);
        } else {
            LOGE("[%s %d] property_set<media.libplayer.dtsopt0> failed\n", __FUNCTION__, __LINE__);
        }
    }

    int arm_omx_codec_get_declen(aml_audio_dec_t *audec)
    {
        int declen = 0;
        android::AmlOMXCodec *arm_omx_codec = (android::AmlOMXCodec *)(audec->arm_omx_codec);
        if (arm_omx_codec != NULL) {
            arm_omx_codec->locked();
            declen = arm_omx_codec->GetDecBytes();
            arm_omx_codec->unlocked();
        } else {
            LOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_declen() return 0! %s %d \n", __FUNCTION__, __LINE__);
        }

        return declen;

    }

#define DTSETC_DECODE_VERSION_CORE  350
#define DTSETC_DECODE_VERSION_M6_M8 380
    int arm_omx_codec_get_FS(aml_audio_dec_t *audec)
    {
        android::AmlOMXCodec *arm_omx_codec = (android::AmlOMXCodec *)(audec->arm_omx_codec);
        if (arm_omx_codec != NULL) {
            arm_omx_codec->locked();
            if (arm_omx_codec->omx_codec_type == OMX_ENABLE_CODEC_DTSHD) {
                int sampleRate = 0;
                android::sp<android::MetaData> output_format = arm_omx_codec->m_codec->getFormat();
                output_format->findInt32(android::kKeySampleRate, &sampleRate);
                if (audec->VersionNum == -1 || (audec->VersionNum == DTSETC_DECODE_VERSION_M6_M8 && audec->DTSHDIEC958_FS == 0)) {
                    output_format->findInt32(android::kKeyDtsDecoderVer, &audec->VersionNum);
                    output_format->findInt32(android::kKeyDts958Fs, &audec->DTSHDIEC958_FS);
                    output_format->findInt32(android::kKeyDts958PktSize, &audec->DTSHDIEC958_PktFrmSize);
                    output_format->findInt32(android::kKeyDts958PktType, &audec->DTSHDIEC958_PktType);
                    output_format->findInt32(android::kKeyDtsPcmSampsInFrmMaxFs, &audec->DTSHDPCM_SamsInFrmAtMaxSR);
                }
                arm_omx_codec->unlocked();
                arm_omx_codec->m_OMXMediaSource->SetSampleRate(sampleRate);
                return sampleRate;
            } else {
                arm_omx_codec->unlocked();
                return arm_omx_codec->m_OMXMediaSource->GetSampleRate();
            }
        } else {
            LOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_FS() return 0! %s %d \n", __FUNCTION__, __LINE__);
            return 0;
        }
    }

    int arm_omx_codec_get_Nch(aml_audio_dec_t *audec)
    {
        android::AmlOMXCodec *arm_omx_codec = (android::AmlOMXCodec *)(audec->arm_omx_codec);
        if (arm_omx_codec != NULL) {
            arm_omx_codec->locked();
            if (arm_omx_codec->omx_codec_type == OMX_ENABLE_CODEC_DTSHD) {
                int numChannels = 0;
                android::sp<android::MetaData> output_format = arm_omx_codec->m_codec->getFormat();
                output_format->findInt32(android::kKeyChannelCount, &numChannels);
                arm_omx_codec->unlocked();
                return numChannels;
            } else {
                arm_omx_codec->unlocked();
                audec->adec_ops->NchOriginal = arm_omx_codec->m_OMXMediaSource->GetChNumOriginal();
                return arm_omx_codec->m_OMXMediaSource->GetChNum();
            }
        } else {
            LOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_Nch() return 0! %s %d \n", __FUNCTION__, __LINE__);
            return 0;
        }
    }


} // namespace android
