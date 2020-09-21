#define LOG_NDEBUG 0
#define LOG_TAG "am-ffextractor"

#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Mutex.h>
#include <utils/List.h>
#include <utils/Timers.h>
#include <cutils/properties.h>
#define INT64_0     INT64_C(0x8000000000000000)
// #ifdef __cplusplus
extern "C"
{
// #endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libavfilter/avfiltergraph.h>
//#include <libavfilter/buffersink.h>
//#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
// #ifdef __cplusplus
}
// #endif
#include "amffextractor.h"
#ifdef USE_OPTEEOS
#include <PA_Decrypt.h>
#endif

#define USE_AVFILTER 0
#define PTS_FREQ 90000
#define LOG_LINE() ALOGV("[%s:%d] >", __FUNCTION__, __LINE__);

using namespace android;
unsigned char   *aviobuffer;
AVIOContext     *avio;
uint8_t         *buffer = NULL;
AVFormatContext *pFormatCtx = NULL;
AVDictionary    *optionsDict = NULL;
AVStream 		*pStream;
int stream_changed = 0;
int inited = 0;
int videoStream;
int audioStream;
#ifdef USE_OPTEEOS
int prop_useDeCrypt = 0;
#endif
float a_time_base_ratio = 0.00;
float v_time_base_ratio = 0.00;
#ifdef USE_OPTEEOS
int codec_write_mode = 0;
uint32_t  mTVPaddr=0;
int prop_tvpdrm = 1;

typedef enum {
    DRM_LEVEL1     = 1,
    DRM_LEVEL2     = 2,
    DRM_LEVEL3     = 3,
    DRM_NONE       = 4,
} drm_level_t;

typedef struct drm_info {
    drm_level_t drm_level;
    uint32_t drm_flag;
    uint32_t drm_hasesdata;
    uint32_t drm_priv;
    uint32_t drm_pktsize;
    uint32_t drm_pktpts;
    uint32_t drm_phy;
    uint32_t drm_vir;
    uint32_t drm_remap;
    uint32_t data_offset;
    uint32_t extpad[8];
} drminfo_t;
#endif

AVRational time_base = {1, 1000000};
FILE* am_fp;
static int amprop_dumpfile;
int am_debug = 0;
FILE *de_fp;
#ifdef USE_OPTEEOS
static int deprop_dumpfile;

int drm_stronedrminfo(uint8_t *outpktdata, uint8_t *addr,int size,  int isdrminfo)
{
    drminfo_t  chinadrminfo;
    #define TYPE_DRMINFO   0x80

    chinadrminfo.drm_level = DRM_LEVEL1;
    chinadrminfo.drm_pktsize = size;
    int dsize;
    if (size < sizeof(drminfo_t)) {
        ALOGE("++++packet size %d < sizeof(drminfo_t) %d++++\n", size, sizeof(drminfo_t));
    }
    else if (isdrminfo == 1) { //infoonly
        chinadrminfo.drm_hasesdata = 0;
        chinadrminfo.drm_phy = (uint32_t)addr;
        chinadrminfo.drm_flag = TYPE_DRMINFO;
        memcpy(outpktdata, &chinadrminfo, sizeof(drminfo_t));
        //ALOGE(" ######## phyaddr = drminfo.drm_phy [0x%x] type[%d]\n",drminfo.drm_phy,type);
    } else { //info+es;
        chinadrminfo.drm_hasesdata = 1;
        chinadrminfo.drm_flag = 0;
        memcpy(outpktdata, &chinadrminfo, sizeof(drminfo_t));
        memcpy(outpktdata + sizeof(drminfo_t), addr, chinadrminfo.drm_pktsize);
    }
    return 0;
}
#endif

int am_ffextractor_init(int (*read_cb)(void *opaque, uint8_t *buf, int size), MediaInfo *pMi)
{
	if (inited)
		return 0;
	static int tmpfileindex = 0;
	char args[512];
	char tmpfilename[1024] = "";
	char tmpfilename1[1024] = "";
	char value[PROPERTY_VALUE_MAX] = {0};

	property_get("iptv.dumppath", value, "/storage/external_storage/sda");
	sprintf(tmpfilename, "%s/am_Live%d.ts", value, tmpfileindex);
	sprintf(tmpfilename1, "%s/de_Live%d.ts", value,tmpfileindex);
	tmpfileindex++;
	memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("amiptv.dumpfile", value, "0");
    amprop_dumpfile = atoi(value);
	if (amprop_dumpfile) {
	am_fp = fopen(tmpfilename, "wb+");
	    de_fp=fopen(tmpfilename1, "wb+");
	    //de_fp=fopen("/storage/external_storage/sdcard1/am_Livedec.ts", "wb+");
	}
	memset(value, 0, PROPERTY_VALUE_MAX);
	property_get("iptv.shouldshowlog", value, "0");
	am_debug = atoi(value);
	ALOGV("am_debug = %d\n", am_debug);
#ifdef USE_OPTEEOS
	memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("iptv.usedecrypt", value, "1");
    prop_useDeCrypt = atoi(value);
	ALOGV("prop_useDeCrypt :%d\n",prop_useDeCrypt);
	memset(value, 0, PROPERTY_VALUE_MAX);
	property_get("deiptv.dumpfile", value, "0");
	deprop_dumpfile = atoi(value);
	if (deprop_dumpfile)
		de_fp = fopen(tmpfilename1, "wb+");

	memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("iptv.tvpdrm", value, "1");
    prop_tvpdrm = atoi(value);
	ALOGV("prop_tvpdrm :%d, 1 tvp and 0 is no tvp debug \n",prop_tvpdrm);

#endif

	av_register_all();

	aviobuffer = (unsigned char *) av_malloc(32768);
	if (!aviobuffer) {
		ALOGE("aviobuffer malloc failed");
		goto fail1;
	}
	avio = avio_alloc_context(aviobuffer, 32768, 0, NULL, read_cb, NULL, NULL);
	if (!avio) {
		ALOGE("avio_alloc_context failed");
		av_free(aviobuffer);
		goto fail1;
	}
	pFormatCtx = avformat_alloc_context();
	if (pFormatCtx == NULL) {
		ALOGD("avformat_alloc_context failed");
		goto fail2;
	}

	pFormatCtx->pb = avio;
	memset(value, 0, PROPERTY_VALUE_MAX);
	property_get("iptv.softprobesize", value, "2048000");
	pFormatCtx->probesize = atoi(value);
	ALOGD("soft demux probesize :%d\n", pFormatCtx->probesize);

	if (avformat_open_input(&pFormatCtx, NULL, NULL, NULL) != 0) {
		ALOGE("Couldn't open input stream.\n");
		goto fail2;
	} else
		ALOGD("avformat_open_input success");

	videoStream = -1;
	audioStream = -1;
	unsigned int i;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	ALOGD("nb_streams=%u, videoStream=%d", pFormatCtx->nb_streams, videoStream);
	if (videoStream != -1) {
		pStream = pFormatCtx->streams[videoStream];
		ALOGV("has video,den:%d\n",pStream->time_base.den);
		ALOGV("num:%d\n", pStream->time_base.num);
		if (0 != pStream->time_base.den)
			v_time_base_ratio = PTS_FREQ * ((float)pStream->time_base.num / pStream->time_base.den);
			ALOGV("v_time_base_ratio:%f\n",v_time_base_ratio);
	}

	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStream = i;
			break;
		}
	}
	ALOGD("nb_streams=%u, audioStream=%d", pFormatCtx->nb_streams, audioStream);
	if (audioStream != -1) {
		pStream = pFormatCtx->streams[audioStream];
		ALOGV("has audio,den:%d\n",pStream->time_base.den);
		ALOGV("num:%d\n", pStream->time_base.num);
		if (0 != pStream->time_base.den)
			a_time_base_ratio = PTS_FREQ * ((float)pStream->time_base.num / pStream->time_base.den);
			ALOGV("a_time_base_ratio:%f\n",a_time_base_ratio);
	}


	/*
	if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
		return -1; // Couldn't find stream information
	}
	*/

/*	ALOGI("width:%d, height:%d", pCodecCtx->width, pCodecCtx->height);
	pMi->width = pCodecCtx->width;
	pMi->height = pCodecCtx->height;*/
#ifdef USE_OPTEEOS
	if (prop_tvpdrm == 1) {
	    mTVPaddr=PA_Getsecmem(1);
	}
#endif
	inited = 1;
	av_dump_format(pFormatCtx, 0, "", 0);
	ALOGD("################ffextractor init successful################");
	return 0;

fail3:
	avformat_close_input(&pFormatCtx);
fail2:
	if (avio->buffer != NULL)
		av_free(avio->buffer);
	if (avio != NULL)
		av_free(avio);
	avio = NULL;
fail1:
	return -1;
}

void am_ffextractor_read_packet(codec_para_t *vcodec, codec_para_t *acodec)
{
	AVPacket  packet;
	int temp_size = 0;
    int write_max = 20;
	int64_t pts;
	if (!inited) {
		return;
	}
#ifdef USE_OPTEEOS
	uint8_t* packetdatacpy=NULL;
	int packetsizecpy=0;
	uint8_t * outbuffer=NULL;
	if (codec_write_mode == 0 && vcodec&&acodec){
        codec_set_drmmode(vcodec,1);
        codec_set_drmmode(acodec,1);
	    codec_write_mode=1;
	}
#endif
	av_init_packet(&packet);
	int ret = av_read_frame(pFormatCtx, &packet);
	/*ALOGV("demux stream_index=%d,size :%d\n",
			packet.stream_index,packet.size);*/
	if (ret < 0) {
		ALOGD(">>>>>av_read_frame failed<<<<<");
		return;
	}
	/*ALOGV("stream_index:%d,pts:%lld,dts:%lld\n",
			packet.stream_index, packet.pts, packet.dts);*/
	if (packet.stream_index == videoStream) {

		if((am_fp != NULL) && (packet.size > 0)) {
			if(amprop_dumpfile){
                fwrite(packet.data, 1, packet.size, am_fp);
            	fflush(am_fp);
			}
		}
		if((int64_t)INT64_0 != packet.pts) {
			pts = (double)packet.pts * (double)v_time_base_ratio;
			if (codec_checkin_pts(vcodec, pts) != 0)
				ALOGE("ERROR video: check in pts error!\n");
		}else if((int64_t)INT64_0 != packet.dts) {
			pts = (double)packet.dts * (double)v_time_base_ratio;
			if (codec_checkin_pts(vcodec, pts) != 0)
				ALOGE("ERROR video: check in dts error!\n");
		}



#ifdef USE_OPTEEOS
	   if(prop_useDeCrypt != 0){
	    if(prop_tvpdrm==1){
			    char flag='f';
			    packetdatacpy=packet.data;
			    packetsizecpy=packet.size;
			    PA_DecryptContentData(flag, (unsigned char*)packet.data, &packet.size);
                drm_stronedrminfo(packet.data, (uint8_t*)mTVPaddr, packet.size,1);
			    packet.size=sizeof(drminfo_t);
			}else {
			    char flag='d';
			    PA_DecryptContentData(flag, (unsigned char*)packet.data, &packet.size);
			    if(de_fp){
			        fwrite(packet.data, 1, packet.size, de_fp);
			        fflush(de_fp);
			}
		}
      }
#endif

		for(int retry_count=0; retry_count < write_max; retry_count++) {
            ret = codec_write(vcodec, packet.data + temp_size, packet.size - temp_size);
            if((ret < 0) || (ret > packet.size)) {
                if(ret < 0 && errno == EAGAIN) {
                    usleep(10);
                    ALOGV("Read and write: codec_write return EAGAIN!\n");
                    continue;
                }
            } else {
                temp_size += ret;
				if(am_debug)
                	ALOGV("Video Read and write: data nSize is %d! temp_size=%d,retry_number:%d\n",
							packet.size, temp_size,retry_count);
                if(retry_count == write_max -1 && temp_size < packet.size) {
                    ALOGV("not write complete,increase write_max\n");
                    write_max += 10;
                }
                if(temp_size >= packet.size) {
                    temp_size = packet.size;
                    break;
                }
            }
		}

	}else if (packet.stream_index == audioStream){
		if((int64_t)INT64_0 != packet.pts) {
			pts = (double)packet.pts * (double)a_time_base_ratio;
			if (codec_checkin_pts(acodec, pts) != 0)
				ALOGE("ERROR audio: check in pts error!\n");
		}else if((int64_t)INT64_0 != packet.dts) {
			pts = (double)packet.dts * (double)a_time_base_ratio;
			if (codec_checkin_pts(acodec, pts) != 0)
				ALOGE("ERROR audio: check in dts error!\n");
		}
#ifdef USE_OPTEEOS
	        if(prop_tvpdrm==1){
	            outbuffer = (uint8_t *)av_malloc( packet.size+sizeof(drminfo_t));
                if (!outbuffer)
                    return ;
		    drm_stronedrminfo(outbuffer, packet.data, packet.size,0;
		    packetdatacpy=packet.data;
	        packetsizecpy=packet.size;
		    packet.data=outbuffer;
		    packet.size+=sizeof(drminfo_t);
	       	}
#endif
		for(int retry_count=0; retry_count < write_max; retry_count++) {
            ret = codec_write(acodec, packet.data + temp_size, packet.size-temp_size);
            if((ret < 0) || (ret > packet.size)) {
                if(ret < 0 && errno == EAGAIN) {
                    usleep(10);
                    ALOGV("Audio Read and write: codec_write return EAGAIN!\n");
                    continue;
                }
            } else {
                temp_size += ret;
				if(am_debug)
                	ALOGV("Audio Read and write: data nSize is %d! temp_size=%d,retry_count:%d\n",
							packet.size, temp_size,retry_count);
                if(retry_count == write_max - 1 && temp_size < packet.size) {
                    ALOGV("not write complete,increase write_max\n");
                    write_max += 10;
                }

                if(temp_size >= packet.size) {
                    temp_size = packet.size;
                    break;
                }
            }
		}

	}
#ifdef USE_OPTEEOS
    if(prop_tvpdrm==1){
	    packet.data=packetdatacpy;
	    packet.size=packetsizecpy;
	    if(packet.stream_index == audioStream&&outbuffer){
	        av_free(outbuffer);
		    outbuffer=NULL;
	     }
	 }
#endif
           av_free_packet(&packet);
}

void am_ffextractor_deinit() {
	if (inited == 0) {
		ALOGW("ffextractor not inited, no need to deinit");
		return;
	}
	if (buffer != NULL)
		av_free(buffer);
	buffer = NULL;
	if(am_fp != NULL) {
        fclose(am_fp);
        am_fp = NULL;
    }
#ifdef USE_OPTEEOS
	if(de_fp != NULL) {
		fclose(de_fp);
		de_fp = NULL;
    }
#endif
	avformat_close_input(&pFormatCtx);

	if(avio->buffer != NULL)
		av_free(avio->buffer);

	if(avio != NULL)
		av_free(avio);
	avio = NULL;
	inited = 0;
#ifdef USE_OPTEEOS
	codec_write_mode=0;
	mTVPaddr=0;
#endif
	ALOGD("################amffextractor de-init successful################");
}

int64_t getCurrentTimeMs()
{
	return systemTime(SYSTEM_TIME_MONOTONIC) / 1000000;
}
int64_t getCurrentTimeUs()
{
	return systemTime(SYSTEM_TIME_MONOTONIC) / 1000;
}
int checkLogMask() 
{
	char levels_value[92];
	int ret = 0;
	if(property_get("iptv.soft.logmask",levels_value,NULL)>0)
		sscanf(levels_value, "%d", &ret);
	return ret;
}
