#define LOG_NDEBUG 0
#define LOG_TAG "ffextractor"

#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Mutex.h>
#include <utils/List.h>
#include <utils/Timers.h>
#include <cutils/properties.h>

// #ifdef __cplusplus
extern "C"
{
// #endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include <libavfilter/avfiltergraph.h>
#endif
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
// #ifdef __cplusplus
}
// #endif
#include "ffextractor.h"
#define USE_AVFILTER 0
#define LOG_LINE() ALOGV("[%s:%d] >", __FUNCTION__, __LINE__);

using namespace android;
unsigned char   *aviobuffer;
AVIOContext     *avio;
uint8_t         *buffer = NULL;
AVFormatContext *pFormatCtx = NULL;
AVBitStreamFilterContext* h264bsfc;
AVCodecContext  *pCodecCtx = NULL;
AVCodec         *pCodec = NULL;
AVFrame         *pFrame = NULL;
AVFrame         *pDiFrame = NULL;
AVDictionary    *optionsDict = NULL;
int stream_changed = 0;
int inited = 0;
int videoStream;

List<AVPacket> AVPacketQueue;
Mutex AVPacketQueueLock;

#if USE_AVFILTER
AVFilterGraph *filter_graph=avfilter_graph_alloc();
AVFilterContext *filter_buffer_ctx;
AVFilterContext *filter_yadif_ctx;
AVFilterContext *filter_buffersink_ctx;
#endif

AVRational time_base = {1, 1000000};

static int s_extrator_frame_cnt = 0;
static int s_decode_frame_cnt = 0;
static int s_test_frame_num = 0;
static int s_writefrom_iframe = 0;
static int s_first_i_comming = 0;
static int s_nDumpEs = 0;
static FILE* mFp = NULL;
static int mEsNo = 0;
static int mYuvNo = 0;
static bool mOpenLog = false;
int ffextractor_init(int (*read_cb)(void *opaque, uint8_t *buf, int size), MediaInfo *pMi) {
	if (inited)
		return 0;
	char args[512];

	av_register_all();
#if USE_AVFILTER
	avfilter_register_all();
#endif
	char value[PROPERTY_VALUE_MAX];
	int ffmpeg_rawbufsize = 4*1024;
	if (property_get("iptv.soft.ffmpegbufsize",value, "4096")>0) {
		ffmpeg_rawbufsize = atoi(value);
		ALOGI("iptv.soft.ffmpegbufsize:%d", ffmpeg_rawbufsize);
	}

	memset(value, 0x0, sizeof(value));
	if (property_get("iptv.soft.iwrite",value, "0")>0) {
		s_writefrom_iframe = atoi(value);
		ALOGI("iptv.soft.iwrite:%d", s_writefrom_iframe);
	}
	s_first_i_comming = 0;
    mEsNo = 0;
    mYuvNo = 0;
	memset(value, 0x0, sizeof(value));
    property_get("iptv.omx.dumpes", value, "0");
    s_nDumpEs = atoi(value);
    if (s_nDumpEs == 1) {
        char tmpfilename[1024]="";
        static int s_nDumpCnt=0;
        sprintf(tmpfilename,"/storage/external_storage/sda1/Live%d.es",s_nDumpCnt);
        s_nDumpCnt++;
        mFp = fopen(tmpfilename, "wb+");
    }
    memset(value, 0, PROPERTY_VALUE_MAX);
    int open_log = 0;
    if (property_get("iptv.soft.log", value, NULL)>0)
        sscanf(value, "%d", &open_log);
    if (open_log)
        mOpenLog = true;
	aviobuffer = (unsigned char *) av_malloc(ffmpeg_rawbufsize);
	if (!aviobuffer) {
		ALOGE("aviobuffer malloc failed");
		goto fail1;
	}
	avio = avio_alloc_context(aviobuffer, ffmpeg_rawbufsize, 0, NULL, read_cb, NULL, NULL);
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
	pFormatCtx->probesize = 128 * 1024;
	memset(value, 0x0, sizeof(value));
	if (property_get("iptv.soft.noprobe",value, NULL) > 0) {
		ALOGI("iptv.soft.noprobe:%d", value);
		if (atoi(value) == 1) {
			pFormatCtx->iformat = av_find_input_format("mpegts");
		}
	}
	if (avformat_open_input(&pFormatCtx, NULL, NULL, NULL) != 0) {
		ALOGE("Couldn't open input stream.\n");
		goto fail2;
	} else
		ALOGD("avformat_open_input success");

	/*
	if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
		return -1; // Couldn't find stream information
	}
	*/

	videoStream = -1;

	unsigned int i;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	ALOGD("nb_streams=%u, videoStream=%d", pFormatCtx->nb_streams, videoStream);
	if (videoStream == -1)
	{
		ALOGE("Cannot find a video stream");
		goto fail3;
	}

	pCodecCtx=pFormatCtx->streams[videoStream]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);

	if (pCodec == NULL) {
		ALOGE("Unsupported codec!\n");
		goto fail3;
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0) {
		ALOGE("open codec failed");
		goto fail3;
	}

	pFrame = av_frame_alloc();
	if (pFrame == NULL) {
		ALOGE("av_frame_alloc failed");
		goto fail4;
	}

	ALOGI("pix_fmt:%d, width:%d, height:%d", pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	pMi->width = pCodecCtx->width;
	pMi->height = pCodecCtx->height;
	AVPacketQueue.clear();

#if USE_AVFILTER
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->time_base.num, pCodecCtx->time_base.den,
		pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den);
	avfilter_graph_create_filter(&filter_buffer_ctx,
				avfilter_get_by_name("buffer"), "in",
				args, NULL, filter_graph);
	avfilter_graph_create_filter(&filter_yadif_ctx,
				avfilter_get_by_name("yadif"), "yadif",
				"mode=send_frame:parity=auto:deint=interlaced",
				NULL, filter_graph);
	avfilter_graph_create_filter(&filter_buffersink_ctx,
								avfilter_get_by_name("buffersink"), "out",
								NULL, NULL,filter_graph);
	enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
	av_opt_set_int_list(filter_buffersink_ctx,
						"pix_fmts", pix_fmts,
						AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	avfilter_link(filter_buffer_ctx, 0, filter_yadif_ctx, 0);
	avfilter_link(filter_yadif_ctx, 0, filter_buffersink_ctx, 0);
	int ret;
	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) {
		printf("avfilter_graph_config:%d\n",ret);
		return -1;
	}
#else
	ALOGD("USE_AVFILTER not defined");
#endif

	inited = 1;
	ALOGI("ffmpeg buf[start:0x%p, buf_ptr:0x%p, end:0x%p, buffer_size:%d]",
		pFormatCtx->pb->buffer, pFormatCtx->pb->buf_ptr, pFormatCtx->pb->buf_end, pFormatCtx->pb->buffer_size);
	av_dump_format(pFormatCtx, 0, "", 0);
	ALOGD("################ffextractor init successful################");
	s_extrator_frame_cnt = 0;
	s_decode_frame_cnt = 0;
	char levels_value1[92];
	if (property_get("iptv.soft.testframe",levels_value1, "60")>0) {
		s_test_frame_num = atoi(levels_value1);
	}
	return 0;

fail4:
	avcodec_close(pCodecCtx);
fail3:
	avformat_close_input(&pFormatCtx);
fail2:
	if (avio->buffer != NULL)
		av_free(avio->buffer);
	if(avio != NULL)
		av_free(avio);
	avio = NULL;
fail1:
	return -1;
}

void ffextractor_read_packet(size_t *queue_size) {
	AVPacket packet;
	if (!inited) {
		return;
	}
	for (;;) {
		int ret = av_read_frame(pFormatCtx, &packet);

		if (ret < 0) {
			ALOGD(">>>>>av_read_frame failed<<<<<");
			return;
		}

		if (packet.stream_index == videoStream) {
			if (s_test_frame_num > 0) {
				if (s_extrator_frame_cnt <= s_test_frame_num) {
					s_extrator_frame_cnt++;
					if (mOpenLog)
						ALOGE("v:read_frame[no:key:pts:pos](%d, %d, 0x%llx, %lld)",
							s_extrator_frame_cnt, (packet.flags& AV_PKT_FLAG_KEY), packet.pts, packet.pos);
				}
			}
			if (s_writefrom_iframe == 1) {
				if (s_first_i_comming == 0) {
					if ((packet.flags& AV_PKT_FLAG_KEY) == 1) {
						s_first_i_comming = 1;
						ALOGI("first i frame");
						ALOGI("ffmpeg buf[start:0x%p, buf_ptr:0x%p, end:0x%p, buffer_size:%d]",
							pFormatCtx->pb->buffer, pFormatCtx->pb->buf_ptr, pFormatCtx->pb->buf_end,
							pFormatCtx->pb->buffer_size);
					} else {
						av_free_packet(&packet);
						return;
					}
				}
			}
            ALOGD_IF(mOpenLog, "ffextractor_read_packet: packet.pts=%lld, packet.dts=%lld, packet.size=%d, packet.flags=%d, mEsNo=%d", packet.pts, packet.dts, packet.size, packet.flags, mEsNo++);
            if (mFp != NULL) {
                fwrite(packet.data, 1, packet.size, mFp);
            }
			AutoMutex l(AVPacketQueueLock);
			AVPacketQueue.push_back(packet);
			*queue_size = AVPacketQueue.size();
			return;
		}
		else//audio or subtitle packet, simply discard and re-read
			av_free_packet(&packet);
	}
}

YUVFrame *ff_decode_frame() {
	AVPacket packet;
	YUVFrame *frameOut = NULL;
	int frameFinished;
	int32_t width;
	int32_t height;
	AVFrame *tmpFrame = NULL;
	int tmpFormat;
	int do_di = 0;
	uint64_t start;
	if (!inited) {
		return frameOut;
	}

	{
		AutoMutex l(AVPacketQueueLock);
		if (!AVPacketQueue.empty())
		{
			packet = *AVPacketQueue.begin();
			AVPacketQueue.erase(AVPacketQueue.begin());
			if (s_test_frame_num > 0 && s_decode_frame_cnt == 0) {
				if (mOpenLog)
					ALOGE("start decoding first packet[no:key:pts:pos](%d, %d, 0x%llx, %lld)",
						s_decode_frame_cnt, (packet.flags& AV_PKT_FLAG_KEY), packet.pts, packet.pos);
			}
		}
		else
		{
			return frameOut;
		}
	}

	start = getCurrentTimeMs();
	int err = avcodec_decode_video2(pCodecCtx,
						  pFrame,
						  &frameFinished,
						  &packet);
	if (s_test_frame_num > 0) {
		if (s_decode_frame_cnt <= s_test_frame_num) {
			if (mOpenLog)
				ALOGE("decode err:%d,frameFinished:%d [no:key:pts:pos](%d, %d, 0x%llx, %lld)",
					err, frameFinished, s_decode_frame_cnt, (packet.flags& AV_PKT_FLAG_KEY), packet.pts, packet.pos);
			if (err > 0 && frameFinished) {
				s_decode_frame_cnt = s_test_frame_num + 1;
			}
		}
	}
	int pkt_key = (packet.flags& AV_PKT_FLAG_KEY);
	int64_t pkt_pos = packet.pos;
	int64_t pkt_pts = packet.pts;
	av_free_packet(&packet);
	if (err > 0 && frameFinished) {
		width = pFrame->width;
		height = pFrame->height;
		ALOGD_IF(mOpenLog, "pFrame->pkt_pts=%lld, pFrame->pkt_dts=%lld, pFrame->pts=%lld, mYuvNo=%d", pFrame->pkt_pts, pFrame->pkt_dts, pFrame->pts, mYuvNo++);
		int interlaced = pFrame->interlaced_frame;
		int64_t pts = av_frame_get_best_effort_timestamp(pFrame);
		pts = av_rescale_q(pts,
						   pFormatCtx->streams[videoStream]->time_base,
						   time_base);
		tmpFormat = pFrame->format;
		if(pFrame->format == 13)
			pFrame->format = 0;

		frameOut = (YUVFrame *)malloc(sizeof(YUVFrame));
		if (frameOut == NULL) {
			ALOGE("frameOut malloc failed");
			exit(1);
		}

		frameOut->data = (void *)malloc((width * height)*3/2);
		if (frameOut->data == NULL) {
			ALOGE("frameOut->data malloc failed");
			exit(1);
		}

		frameOut->pts = pts;
		frameOut->size = (width * height) * 3 / 2;
		frameOut->flags = 0;
		frameOut->width = width;
		frameOut->height = height;

		tmpFrame = pFrame;

		char levels_value[92];
		do_di = 0;
		if(property_get("iptv.soft.di",levels_value,NULL)>0)
			sscanf(levels_value,"%d",&do_di);

		if (do_di && interlaced) {
			start = getCurrentTimeMs();
#if USE_AVFILTER
			int ret = av_buffersrc_add_frame_flags(filter_buffer_ctx,
				pFrame, AV_BUFFERSRC_FLAG_KEEP_REF);

			if (ret < 0) {
				ALOGE("failed to feed filtergraph");
			} else {
				while(1) {
					ret = av_buffersink_get_frame(filter_buffersink_ctx, pDiFrame);

					if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
						break;
					if (ret < 0) {
						ALOGE("deinterlacing failed");
					}
					tmpFrame = pDiFrame;
					av_frame_unref(pDiFrame);
				}
			}
#else
				if (buffer == NULL || pDiFrame == NULL) {
					pDiFrame = av_frame_alloc();
					if (pDiFrame == NULL) {
						ALOGE("pDiFrame malloc failed");
						exit(1);
					}
					size_t numBytes=avpicture_get_size(pCodecCtx->pix_fmt,
														width,
														height);
					buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
					if (buffer == NULL) {
						ALOGE("di buffer malloc failed");
						exit(1);
					}
					avpicture_fill((AVPicture *)pDiFrame,
									buffer,
									pCodecCtx->pix_fmt,
									width,
									height);
				}
#if ANDROID_PLATFORM_SDK_VERSION <= 27
				avpicture_deinterlace((AVPicture*)pDiFrame, 
									(const AVPicture*)pFrame, 
									pCodecCtx->pix_fmt, 
									width,
									height);
#endif
			   tmpFrame = pDiFrame;
#endif
		}

		const uint8_t *srcLine = (const uint8_t *)tmpFrame->data[0];
		uint8_t *dst = (uint8_t *)frameOut->data;
		size_t i = 0;
		for (i = 0; i < height; ++i) {
			memcpy(dst, srcLine, width);
			srcLine += tmpFrame->linesize[0];
			dst += width;
		}

		srcLine = (const uint8_t *)tmpFrame->data[1];
		if(tmpFormat==13)
		{
			for (i = 0; i < height/2; i++) {
				memcpy(dst, srcLine , width/2);
				srcLine += tmpFrame->linesize[1];
				srcLine += tmpFrame->linesize[1];
				dst += width/2;
			}
		}
		else
		{
			for (i = 0; i < height / 2; ++i) {
				memcpy(dst, srcLine, width / 2);
				srcLine += tmpFrame->linesize[1];
				dst += width / 2;
			}
		}
		srcLine = (const uint8_t *)tmpFrame->data[2];
		if(tmpFormat==13)
		{
			for (i = 0; i < height/2; i++) {
				memcpy(dst, srcLine, width/2);
				srcLine += tmpFrame->linesize[1];
				srcLine += tmpFrame->linesize[1];
				dst += width/2;
			}
		}
		else
		{
			for (i = 0; i < height / 2; ++i) {
				memcpy(dst, srcLine, width / 2);
				srcLine += tmpFrame->linesize[2];
				dst += width / 2;
				}
		}

		if (s_test_frame_num > 0) {
			if (s_decode_frame_cnt <= s_test_frame_num) {
				if (mOpenLog)
					ALOGE("decode err:%d,frameFinished:%d [no:key:pts:pos:pts](%d, %d, %d, %d, 0x%llx, %lld, 0x%llx)", 
						err, frameFinished, s_decode_frame_cnt, pkt_key, pkt_pts, pkt_pos, frameOut->pts);
			}
			s_decode_frame_cnt++;
		}
		return frameOut;
	} else {
		ALOGE_IF(checkLogMask(), "Decode frame failed");
	}
	if (s_test_frame_num > 0) {
		if (s_decode_frame_cnt <= s_test_frame_num) {
			s_decode_frame_cnt++;
		}
	}
	return frameOut;
}

void ffextractor_deinit() {
	if (inited == 0) {
		ALOGW("ffextractor not inited, no need to deinit");
		return;
	}
	if (buffer != NULL)
		av_free(buffer);
	buffer = NULL;

	if (pDiFrame != NULL)
		av_free(pDiFrame);
	pDiFrame = NULL;

	if (pFrame != NULL)
		av_free(pFrame);
	pFrame = NULL;
	
#if USE_AVFILTER
	avfilter_graph_free(&filter_graph);
#endif
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	{
		AutoMutex l(AVPacketQueueLock);
		while (!AVPacketQueue.empty())
		{
			AVPacket packet = *AVPacketQueue.begin();
			av_free_packet(&packet);
			AVPacketQueue.erase(AVPacketQueue.begin());
		}
		AVPacketQueue.clear();
	}

	if(avio->buffer != NULL)
		av_free(avio->buffer);

	if(avio != NULL)
		av_free(avio);
	avio = NULL;
	inited = 0;
	ALOGD("################ffextractor de-init successful################");
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
