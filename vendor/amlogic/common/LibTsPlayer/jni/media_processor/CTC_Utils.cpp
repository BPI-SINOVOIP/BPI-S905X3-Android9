#define LOG_NDEBUG 0
#define LOG_TAG "CTC_Utils"
#include <string.h>
#include <stdlib.h>
#include "CTC_Log.h"
#include "CTC_Utils.h"

extern "C" {
#include <libavformat/avformat.h>
}

OUTPUT_MODE get_display_mode()
{
    int fd;
    char mode[16] = {0};
    char path[64] = {"/sys/class/display/mode"};
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(mode, 0, 16); // clean buffer and read 15 byte to avoid strlen > 15	
        read(fd, mode, 15);
        mode[strlen(mode)] = '\0';
        close(fd);
        if(!strncmp(mode, "480i", 4) || !strncmp(mode, "480cvbs", 7)) {
            return OUTPUT_MODE_480I;
        } else if(!strncmp(mode, "480p", 4)) {
            return OUTPUT_MODE_480P;
        } else if(!strncmp(mode, "576i", 4) || !strncmp(mode, "576cvbs", 7)) {
            return OUTPUT_MODE_576I;
        } else if(!strncmp(mode, "576p", 4)) {
            return OUTPUT_MODE_576P;
        } else if(!strncmp(mode, "720p", 4)) {
            return OUTPUT_MODE_720P;
        } else if(!strncmp(mode, "1080i", 5)) {
            return OUTPUT_MODE_1080I;
        } else if(!strncmp(mode, "1080p", 5)) {
            return OUTPUT_MODE_1080P;
        } else if(!strncmp(mode, "2160p", 5)) {
            return OUTPUT_MODE_4K2K;
        } else if(!strncmp(mode, "smpte", 5)) {
            return OUTPUT_MODE_4K2KSMPTE;
        }
    } else {
        LOGE("get_display_mode open file %s error\n", path);
    }
    return OUTPUT_MODE_720P;
}

void getPosition(OUTPUT_MODE output_mode, int *x, int *y, int *width, int *height)
{
    char vaxis_newx_str[PROPERTY_VALUE_MAX] = {0};
    char vaxis_newy_str[PROPERTY_VALUE_MAX] = {0};
    char vaxis_width_str[PROPERTY_VALUE_MAX] = {0};
    char vaxis_height_str[PROPERTY_VALUE_MAX] = {0};

    switch(output_mode) {
    case OUTPUT_MODE_480I:
        property_get("ubootenv.var.480i_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.480i_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.480i_w", vaxis_width_str, "720");
        property_get("ubootenv.var.480i_h", vaxis_height_str, "480");
        break;
    case OUTPUT_MODE_480P:
        property_get("ubootenv.var.480p_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.480p_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.480p_w", vaxis_width_str, "720");
        property_get("ubootenv.var.480p_h", vaxis_height_str, "480");
        break;
    case OUTPUT_MODE_576I:
        property_get("ubootenv.var.576i_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.576i_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.576i_w", vaxis_width_str, "720");
        property_get("ubootenv.var.576i_h", vaxis_height_str, "576");
        break;
    case OUTPUT_MODE_576P:
        property_get("ubootenv.var.576p_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.576p_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.576p_w", vaxis_width_str, "720");
        property_get("ubootenv.var.576p_h", vaxis_height_str, "576");
        break;
    case OUTPUT_MODE_720P:
        property_get("ubootenv.var.720p_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.720p_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.720p_w", vaxis_width_str, "1280");
        property_get("ubootenv.var.720p_h", vaxis_height_str, "720");
        break;
    case OUTPUT_MODE_1080I:
        property_get("ubootenv.var.1080i_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.1080i_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.1080i_w", vaxis_width_str, "1920");
        property_get("ubootenv.var.1080i_h", vaxis_height_str, "1080");
        break;
    case OUTPUT_MODE_1080P:
        property_get("ubootenv.var.1080p_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.1080p_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.1080p_w", vaxis_width_str, "1920");
        property_get("ubootenv.var.1080p_h", vaxis_height_str, "1080");
        break;
    case OUTPUT_MODE_4K2K:
        property_get("ubootenv.var.4k2k_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.4k2k_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.4k2k_w", vaxis_width_str, "3840");
        property_get("ubootenv.var.4k2k_h", vaxis_height_str, "2160");
        break;
    case OUTPUT_MODE_4K2KSMPTE:
        property_get("ubootenv.var.4k2ksmpte_x", vaxis_newx_str, "0");
        property_get("ubootenv.var.4k2ksmpte_y", vaxis_newy_str, "0");
        property_get("ubootenv.var.4k2ksmpte_w", vaxis_width_str, "4096");
        property_get("ubootenv.var.4k2ksmpte_h", vaxis_height_str, "2160");
        break;
    default:
    	  *x = 0;
    	  *y = 0;
    	  *width = 1280;
    	  *height = 720;
        LOGW("UNKNOW MODE:%d", output_mode);
        return;
    }
    *x = atoi(vaxis_newx_str);
    *y = atoi(vaxis_newy_str);
    *width = atoi(vaxis_width_str);
    *height = atoi(vaxis_height_str);
}

///////////////////////////////////////////////////////////////////////////////
static aformat_t convertToAFormat(AVCodecID codecId);
static vformat_t convertToVFormat(AVCodecID codecId);
static SUBTITLE_TYPE convertToSubFormat(AVCodecID codecId);

int getMediaInfo(const char* url, CTC_MediaInfo* mediaInfo)
{
    av_register_all();

    memset(mediaInfo, 0, sizeof(*mediaInfo));

    AVFormatContext* ctx = avformat_alloc_context();

    AVInputFormat* inputFormat = av_find_input_format("mpegts");
    int ret = avformat_open_input(&ctx, url, inputFormat, nullptr);
    if (ret != 0) {
        ALOGE("avformat open failed! ret=%d, %s", ret, av_err2str(ret));
        avformat_free_context(ctx);
        return -1;
    }

    ret = avformat_find_stream_info(ctx, nullptr);
    if (ret < 0) {
        ALOGE("find stream info failed! ret:%d, %s", ret, av_err2str(ret));
        avformat_close_input(&ctx);
        return -1;
    }

    mediaInfo->bitrate = ctx->bit_rate;
    printf("bitrate:%lld KB/s, duration:%.2f s\n", mediaInfo->bitrate>>13, ctx->duration*1.0/AV_TIME_BASE);

    av_dump_format(ctx, -1, nullptr, 0);

    for (size_t i = 0; i < ctx->nb_streams; ++i) {
        AVStream* st = ctx->streams[i];
        if (mediaInfo->vPara.pid == 0 && st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mediaInfo->vPara.pid = st->id;
            mediaInfo->vPara.vFmt = convertToVFormat(st->codecpar->codec_id);
            mediaInfo->vPara.nVideoWidth = st->codecpar->width;
            mediaInfo->vPara.nVideoHeight = st->codecpar->height;
            mediaInfo->vPara.nFrameRate = av_q2d(st->avg_frame_rate);
            mediaInfo->vPara.pExtraData = (uint8_t*)malloc(st->codecpar->extradata_size);
            memcpy(mediaInfo->vPara.pExtraData, st->codecpar->extradata, st->codecpar->extradata_size);
            mediaInfo->vPara.nExtraSize = st->codecpar->extradata_size;
            ALOGI("video pid:%d, fmt:%d, frame_rate:%d\n", mediaInfo->vPara.pid, mediaInfo->vPara.vFmt, mediaInfo->vPara.nFrameRate);
        } else if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AUDIO_PARA_T *aPara = &mediaInfo->aParas[mediaInfo->aParaCount++];
            aPara->pid = st->id;
            aPara->aFmt = convertToAFormat(st->codecpar->codec_id);
            aPara->nChannels = st->codecpar->channels;
            aPara->nSampleRate = st->codecpar->sample_rate;
            aPara->pExtraData = (uint8_t*)malloc(st->codecpar->extradata_size);
            mempcpy(aPara->pExtraData, st->codecpar->extradata, st->codecpar->extradata_size);
            aPara->nExtraSize = st->codecpar->extradata_size;
            ALOGI("audio pid:%d, fmt:%d\n", aPara->pid, aPara->aFmt);
        } else if (st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            SUBTITLE_PARA_T* sPara = &mediaInfo->sParas[mediaInfo->sParaCcount++];
            sPara->pid = st->id;
            sPara->sub_type = convertToSubFormat(st->codecpar->codec_id);
            ALOGI("subtitle pid:%d, fmt:%d\n", sPara->pid, sPara->sub_type);
        }
    }

    avformat_close_input(&ctx);
    return 0;
}

void freeMediaInfo(CTC_MediaInfo* mediaInfo)
{
    free(mediaInfo->vPara.pExtraData);

    for (size_t i = 0; i < mediaInfo->aParaCount; ++i) {
        free(mediaInfo->aParas[i].pExtraData);
    }
}


static aformat_t convertToAFormat(AVCodecID codecId)
{
    aformat_t aformat = AFORMAT_UNKNOWN;

    switch (codecId) {
    case AV_CODEC_ID_AAC:
        aformat = AFORMAT_AAC;
        break;

    case AV_CODEC_ID_AC3:
        aformat = AFORMAT_AC3;
        break;

    case AV_CODEC_ID_MP2:
        aformat = AFORMAT_MPEG2;
        break;

    case AV_CODEC_ID_MP3:
        aformat = AFORMAT_MPEG;
        break;

    default:
        break;
    }

    return aformat;
}

static vformat_t convertToVFormat(AVCodecID codecId)
{
    vformat_t vformat = VFORMAT_UNKNOWN;

    switch (codecId) {
    case AV_CODEC_ID_H264:
        vformat = VFORMAT_H264;
        break;

    case AV_CODEC_ID_HEVC:
        vformat = VFORMAT_HEVC;
        break;

    default:
        break;
    }

    return vformat;
}

static SUBTITLE_TYPE convertToSubFormat(AVCodecID codecId)
{
    SUBTITLE_TYPE subType = CTC_CODEC_ID_DVD_SUBTITLE;

    switch (codecId) {
    case AV_CODEC_ID_DVD_SUBTITLE:
        subType = CTC_CODEC_ID_DVD_SUBTITLE;
        break;

    case AV_CODEC_ID_DVB_SUBTITLE:
        subType = CTC_CODEC_ID_DVB_SUBTITLE;
        break;

    case AV_CODEC_ID_TEXT:
        subType = CTC_CODEC_ID_TEXT;
        break;

    case AV_CODEC_ID_XSUB:
        subType = CTC_CODEC_ID_XSUB;
        break;

    case AV_CODEC_ID_SSA:
        subType = CTC_CODEC_ID_SSA;
        break;

    case AV_CODEC_ID_MOV_TEXT:
        subType = CTC_CODEC_ID_MOV_TEXT;
        break;

    case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
        subType = CTC_CODEC_ID_HDMV_PGS_SUBTITLE;
        break;

    case AV_CODEC_ID_DVB_TELETEXT:
        subType = CTC_CODEC_ID_DVB_TELETEXT;
        break;

    case AV_CODEC_ID_SRT:
        subType = CTC_CODEC_ID_SRT;
        break;

    case AV_CODEC_ID_MICRODVD:
        subType = CTC_CODEC_ID_MICRODVD;
        break;

    default:
        break;
    }

    return subType;
}

