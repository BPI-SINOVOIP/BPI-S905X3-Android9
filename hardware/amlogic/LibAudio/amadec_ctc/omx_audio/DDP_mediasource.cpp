#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>
#include "DDP_mediasource.h"

extern "C" int read_buffer(unsigned char *buffer, int size);

//#define LOG_TAG "DDP_Medissource"
//#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace android
{


DDPerr DDP_MediaSource::ddbs_init(DDPshort * buf, DDPshort bitptr, DDP_BSTRM *p_bstrm)
{
    p_bstrm->buf = buf;
    p_bstrm->bitptr = bitptr;
    p_bstrm->data = *buf;
    return 0;
}


DDPerr DDP_MediaSource::ddbs_unprj(DDP_BSTRM    *p_bstrm, DDPshort *p_data,  DDPshort numbits)
{
    DDPushort data;
    *p_data = (DDPshort)((p_bstrm->data << p_bstrm->bitptr) & msktab[numbits]);
    p_bstrm->bitptr += numbits;
    if (p_bstrm->bitptr >= BITSPERWRD) {
        p_bstrm->buf++;
        p_bstrm->data = *p_bstrm->buf;
        p_bstrm->bitptr -= BITSPERWRD;
        data = (DDPushort)p_bstrm->data;
        *p_data |= ((data >> (numbits - p_bstrm->bitptr)) & msktab[numbits]);
    }
    *p_data = (DDPshort)((DDPushort)(*p_data) >> (BITSPERWRD - numbits));
    return 0;
}


int DDP_MediaSource::Get_ChNum_DD(void *buf)//at least need:56bit(=7 bytes)
{
    int numch = 0;
    DDP_BSTRM bstrm = {0, 0, 0};
    DDP_BSTRM *p_bstrm = &bstrm;
    short tmp = 0, acmod, lfeon, fscod, frmsizecod;
    ddbs_init((short*)buf, 0, p_bstrm);

    ddbs_unprj(p_bstrm, &tmp, 16);
    if (tmp != SYNCWRD) {
        ALOGI("Invalid synchronization word");
        return 0;
    }
    ddbs_unprj(p_bstrm, &tmp, 16);
    ddbs_unprj(p_bstrm, &fscod, 2);
    if (fscod == MAXFSCOD) {
        ALOGI("Invalid sampling rate code");
        return 0;
    }

    if (fscod == 0) {
        sample_rate = 48000;
    } else if (fscod == 1) {
        sample_rate = 44100;
    } else if (fscod == 2) {
        sample_rate = 32000;
    }

    ddbs_unprj(p_bstrm, &frmsizecod, 6);
    if (frmsizecod >= MAXDDDATARATE) {
        ALOGI("Invalid frame size code");
        return 0;
    }

    frame_size = frmsizetab[fscod][frmsizecod];

    ddbs_unprj(p_bstrm, &tmp, 5);
    if (!ISDD(tmp)) {
        ALOGI("Unsupported bitstream id");
        return 0;
    }

    ddbs_unprj(p_bstrm, &tmp, 3);
    ddbs_unprj(p_bstrm, &acmod, 3);

    if ((acmod != MODE10) && (acmod & 0x1)) {
        ddbs_unprj(p_bstrm, &tmp, 2);
    }
    if (acmod & 0x4) {
        ddbs_unprj(p_bstrm, &tmp, 2);
    }

    if (acmod == MODE20) {
        ddbs_unprj(p_bstrm, &tmp, 2);
    }
    ddbs_unprj(p_bstrm, &lfeon, 1);


    numch = chanary[acmod];
    //numch+=lfeon;
    ChNumOriginal = numch;
    if (0) {
        if (numch >= 3) {
            numch = 8;
        } else {
            numch = 2;
        }
    } else {
        numch = 2;
    }
    ChNum = numch;
    //ALOGI("DEBUG:numch=%d sample_rate=%d %p [%s %d]",ChNum,sample_rate,this,__FUNCTION__,__LINE__);
    return numch;

}
int DDP_MediaSource::Get_ChNum_DDP(void *buf)//at least need:40bit(=5 bytes)
{

    int numch = 0;
    DDP_BSTRM bstrm = {0, 0, 0};
    DDP_BSTRM *p_bstrm = &bstrm;
    short tmp = 0, acmod, lfeon, strmtyp;

    ddbs_init((short*)buf, 0, p_bstrm);

    ddbs_unprj(p_bstrm, &tmp, 16);
    if (tmp != SYNCWRD) {
        ALOGI("Invalid synchronization word");
        return 0;
    }

    ddbs_unprj(p_bstrm, &strmtyp, 2);
    ddbs_unprj(p_bstrm, &tmp, 3);
    ddbs_unprj(p_bstrm, &tmp, 11);
    frame_size = tmp + 1;
    //---------------------------
    if (strmtyp != 0 && strmtyp != 2) {
        return 0;
    }
    //---------------------------
    ddbs_unprj(p_bstrm, &tmp, 2);

    if (tmp == 0x3) {
        ALOGI("Half sample rate unsupported");
        return 0;
    } else {
        if (tmp == 0) {
            sample_rate = 48000;
        } else if (tmp == 1) {
            sample_rate = 44100;
        } else if (tmp == 2) {
            sample_rate = 32000;
        }

        ddbs_unprj(p_bstrm, &tmp, 2);
    }

    ddbs_unprj(p_bstrm, &acmod, 3);
    ddbs_unprj(p_bstrm, &lfeon, 1);

    numch = chanary[acmod];
    //numch+=lfeon;

    ChNumOriginal = numch;
    if (0) {
        if (numch >= 3) {
            numch = 8;
        } else {
            numch = 2;
        }
    } else {
        numch = 2;
    }
    ChNum = numch;
    //ALOGI("DEBUG:numch=%d sample_rate=%d %p [%s %d]",ChNum,sample_rate,this,__FUNCTION__,__LINE__);
    return numch;

}


DDPerr DDP_MediaSource::ddbs_skip(DDP_BSTRM     *p_bstrm, DDPshort    numbits)
{
    p_bstrm->bitptr += numbits;
    while (p_bstrm->bitptr >= BITSPERWRD) {
        p_bstrm->buf++;
        p_bstrm->data = *p_bstrm->buf;
        p_bstrm->bitptr -= BITSPERWRD;
    }

    return 0;
}


DDPerr DDP_MediaSource::ddbs_getbsid(DDP_BSTRM *p_inbstrm,    DDPshort *p_bsid)
{
    DDP_BSTRM    bstrm;

    ddbs_init(p_inbstrm->buf, p_inbstrm->bitptr, &bstrm);
    ddbs_skip(&bstrm, BS_BITOFFSET);
    ddbs_unprj(&bstrm, p_bsid, 5);
    if (!ISDDP(*p_bsid) && !ISDD(*p_bsid)) {
        ALOGI("Unsupported bitstream id");
    }

    return 0;
}


int DDP_MediaSource::Get_ChNum_AC3_Frame(void *buf)
{
    DDP_BSTRM bstrm = {0, 0, 0};
    DDP_BSTRM *p_bstrm = &bstrm;
    DDPshort    bsid;
    int chnum = 0;
    uint8_t ptr8[PTR_HEAD_SIZE];

    memcpy(ptr8, buf, PTR_HEAD_SIZE);


    //ALOGI("LZG->ptr_head:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
    //       ptr8[0],ptr8[1],ptr8[2], ptr8[3],ptr8[4],ptr8[5] );
    if ((ptr8[0] == 0x0b) && (ptr8[1] == 0x77)) {
        int i;
        uint8_t tmp;
        for (i = 0; i < PTR_HEAD_SIZE; i += 2) {
            tmp = ptr8[i];
            ptr8[i] = ptr8[i + 1];
            ptr8[i + 1] = tmp;
        }
    }


    ddbs_init((short*)ptr8, 0, p_bstrm);
    ddbs_getbsid(p_bstrm, &bsid);
    //ALOGI("LZG->bsid=%d \n", bsid );
    if (ISDDP(bsid)) {
        Get_ChNum_DDP(ptr8);
    } else if (ISDD(bsid)) {
        Get_ChNum_DD(ptr8);
    }

    return chnum;
}

//#####################################################

DDP_MediaSource::DDP_MediaSource(void *read_buffer)
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    mStarted = false;
    mMeta = new MetaData;
    mDataSource = NULL;
    mGroup = NULL;
    mBytesReaded = 0;
    mCurrentTimeUs = 0;
    pStop_ReadBuf_Flag = NULL;
    fpread_buffer = (fp_read_buffer)read_buffer;
    sample_rate = 0;
    ChNum = 0;
    frame_size = 0;
    bytes_readed_sum_pre = 0;
    bytes_readed_sum = 0;
    extractor_cost_bytes = 0;
    extractor_cost_bytes_last = 0;
    mMeta->setInt32(kKeyChannelCount, 2);
    mMeta->setInt32(kKeySampleRate, /*audec->samplerate*/48000);
    memset(frame.rawbuf, 0, 6144);
    memset(frame_length_his, 0, sizeof(frame_length_his));
    frame.len = 0;
    ChNumOriginal = 0;
}

DDP_MediaSource::~DDP_MediaSource()
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    if (mStarted) {
        stop();
    }
}

int DDP_MediaSource::GetSampleRate()
{
    return sample_rate;
}
int DDP_MediaSource::GetChNumOriginal()
{
    return ChNumOriginal;

}
int DDP_MediaSource::GetChNum()
{
    return ChNum;
}

int* DDP_MediaSource::Get_pStop_ReadBuf_Flag()
{
    return pStop_ReadBuf_Flag;
}

int DDP_MediaSource::Set_pStop_ReadBuf_Flag(int *pStop)
{
    pStop_ReadBuf_Flag = pStop;

    return 0;
}

int DDP_MediaSource::SetReadedBytes(int size)
{
    mBytesReaded = size;
    return 0;
}

int DDP_MediaSource::GetReadedBytes()
{
    int bytes_used;
#if 0
    bytes_used = bytes_readed_sum - bytes_readed_sum_pre;
    if (bytes_used < 0) {
        ALOGI("[%s]bytes_readed_sum(%lld) < bytes_readed_sum_pre(%lld) \n", __FUNCTION__, bytes_readed_sum, bytes_readed_sum_pre);
        bytes_used = 0;
    }
    bytes_readed_sum_pre = bytes_readed_sum;
#endif
    bytes_used = extractor_cost_bytes;
    bytes_used -= extractor_cost_bytes_last;
    extractor_cost_bytes_last += bytes_used;
    return bytes_used;
}

sp<MetaData> DDP_MediaSource::getFormat()
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    return mMeta;
}

status_t DDP_MediaSource::start(MetaData *params __unused)
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(MediaBufferBase::Create(4096));
    mStarted = true;
    return OK;
}

status_t DDP_MediaSource::stop()
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    delete mGroup;
    mGroup = NULL;
    mStarted = false;
    return OK;
}

/*
static int calc_dd_frame_size(int code)
{
    static const int FrameSize32K[] = { 96, 96, 120, 120, 144, 144, 168, 168, 192, 192, 240, 240, 288, 288, 336, 336, 384, 384, 480, 480, 576, 576, 672, 672, 768, 768, 960, 960, 1152, 1152, 1344, 1344, 1536, 1536, 1728, 1728, 1920, 1920 };
    static const int FrameSize44K[] = { 69, 70, 87, 88, 104, 105, 121, 122, 139, 140, 174, 175, 208, 209, 243, 244, 278, 279, 348, 349, 417, 418, 487, 488, 557, 558, 696, 697, 835, 836, 975, 976, 114, 1115, 1253, 1254, 1393, 1394 };
    static const int FrameSize48K[] = { 64, 64, 80, 80, 96, 96, 112, 112, 128, 128, 160, 160, 192, 192, 224, 224, 256, 256, 320, 320, 384, 384, 448, 448, 512, 512, 640, 640, 768, 768, 896, 896, 1024, 1024, 1152, 1152, 1280, 1280 };

    int fscod = (code >> 6) & 0x3;
    int frmsizcod = code & 0x3f;

    if (fscod == 0) return 2 * FrameSize48K[frmsizcod];
    if (fscod == 1) return 2 * FrameSize44K[frmsizcod];
    if (fscod == 2) return 2 * FrameSize32K[frmsizcod];

    return 0;
}
*/

int DDP_MediaSource::get_frame_size(void)
{
    int i;
    unsigned sum = 0;
    unsigned valid_his_num = 0;
    for (i = 0; i < FRAME_RECORD_NUM; i++) {
        if (frame_length_his[i] > 0) {
            valid_his_num ++;
            sum += frame_length_his[i];
        }
    }

    if (valid_his_num == 0) {
        return 768;
    }
    return sum / valid_his_num;
}
void DDP_MediaSource::store_frame_size(int lastFrameLen)
{
    /* record the frame length into the history buffer */
    int i = 0;
    for (i = 0; i < FRAME_RECORD_NUM - 1; i++) {
        frame_length_his[i] = frame_length_his[i + 1];
    }
    frame_length_his[FRAME_RECORD_NUM - 1] = lastFrameLen;
}
int DDP_MediaSource::MediaSourceRead_buffer(unsigned char *buffer, int size)
{
    int readcnt = 0;
    int readsum = 0;
    if (fpread_buffer != NULL) {
        int sleep_time = 0;
        while ((readsum < size) && (*pStop_ReadBuf_Flag == 0)) {
            readcnt = fpread_buffer(buffer + readsum, size - readsum);
            if (readcnt < (size - readsum)) {
                sleep_time++;
                usleep(10000);
            }
            readsum += readcnt;
            if ((sleep_time > 0) && (sleep_time % 100 == 0)) {
                //wait for max 10s to get audio data
                ALOGE("[%s] Can't get data from audiobuffer,wait for %d ms\n ", __FUNCTION__, sleep_time * 10);
            }
        }
        bytes_readed_sum += readsum;
        if (*pStop_ReadBuf_Flag == 1) {
            ALOGI("[%s] End of Stream: *pStop_ReadBuf_Flag==1\n ", __FUNCTION__);
        }
        return readsum;
    } else {
        ALOGE("[%s]ERR: fpread_buffer=NULL\n ", __FUNCTION__);
        return 0;
    }
}

status_t DDP_MediaSource::read(MediaBufferBase **out, const ReadOptions *options __unused)
{
    *out = NULL;
    int readdiff = 0;
    int read_delta = 0;
    int read_size = 0;
    while (1) {
        frame_size = 0;
        read_size =     get_frame_size() + PTR_HEAD_SIZE + read_delta;
        if (read_size > frame.len) {
            read_size -= frame.len;
        }
        /*check if the read_size exceed buffer size*/
        if ((frame.len + read_size) > 6144) {
            read_size = 6144 - frame.len;
        }
        frame.len += MediaSourceRead_buffer(frame.rawbuf + frame.len, read_size/* - frame.len*/);
        if (frame.len < PTR_HEAD_SIZE) {
            ALOGI("WARNING: fpread_buffer read failed [%s %d]!\n", __FUNCTION__, __LINE__);
            return ERROR_END_OF_STREAM;
        }

        if (*pStop_ReadBuf_Flag == 1) {
            ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]", __FUNCTION__, __LINE__);
            return ERROR_END_OF_STREAM;
        }

        unsigned short head;
        head = frame.rawbuf[0] << 8 | frame.rawbuf[1];

        if (head == 0x0b77 || head == 0x770b) {
            Get_ChNum_AC3_Frame(frame.rawbuf);
            frame_size = frame_size * 2;

            if ((frame_size == 0) || (frame_size < PTR_HEAD_SIZE) || (frame_size > 4096)) {
                ALOGI("frame_size %d error\n", frame_size);
                memcpy((char*)(frame.rawbuf), (char *)(frame.rawbuf + 1), frame.len - 1);
                frame.len -= 1;
                readdiff ++;
                continue;
            }
            /* if framesize bigger than current frame size + syncword size.read more */
            if (frame_size > (frame.len - 2)) {
                ALOGI("frame size %d exceed cached size %d,read more\n", frame_size, frame.len);
                read_delta = frame_size - (frame.len - 2);
                continue;
            }
            head = (frame.rawbuf[frame_size] << 8) | frame.rawbuf[frame_size + 1];

            if (head == 0x0b77 || head == 0x770b) {
                ALOGI("next frame is ok,frame size %d\n", frame_size);
                break;
            } else {
                ALOGI("=====next frame sync word error %x,resync\n", head);
                memmove((char*)(frame.rawbuf), (char *)(frame.rawbuf + 1), frame.len - 1);
                frame.len -= 1;
                readdiff ++;
            }
        } else {
            memmove((char*)(frame.rawbuf), (char *)(frame.rawbuf + 1), frame.len - 1);
            frame.len -= 1;
            readdiff ++;
        }
    }
    read_delta = 0;
    MediaBufferBase *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);

    if (err != OK) {
        return err;
    }

    memcpy((unsigned char*)(buffer->data()), (unsigned char*)frame.rawbuf, frame_size);
    memmove((unsigned char*)frame.rawbuf, (unsigned char*)(frame.rawbuf + frame_size), frame.len - frame_size);
    frame.len -= frame_size;
    buffer->set_range(0, frame_size);
    buffer->meta_data().setInt64(kKeyTime, mCurrentTimeUs);
    buffer->meta_data().setInt32(kKeyIsSyncFrame, 1);

    *out = buffer;
    if (readdiff > 0) {
        extractor_cost_bytes += readdiff;
    }
    store_frame_size(frame_size);
    return OK;
}

}  // namespace android

