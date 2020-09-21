#define LOG_TAG "ctc_player"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <utils/Log.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <android/native_window.h>

#include <gui/Surface.h>
#include <ui/DisplayInfo.h>


#include <cutils/properties.h>
#include <CTC_MediaControl.h>
#define kReadSize (64*1024)
#define LOG_LINE() ALOGD("[%s:%d]", __FUNCTION__, __LINE__);

using namespace android;

unsigned char* mBuf[9];// = (unsigned char*) malloc(sizeof(unsigned char) * kReadSize);

//VIDEO_PARA_T vidPara = {0, 640, 480, 24, VFORMAT_H264, 0};
char filename[256];
int active_bitmask = 0x0;
pthread_t tid[9];

//int x = 100, y = 100, w = 640, h = 360;
int last_active_index = 0;
ITsPlayer* player[9];
int mSourceFD[9];
int my_idx = 0;
int idx[9];
#define DEBUG_VIDEO_INFO
#ifdef DEBUG_VIDEO_INFO

#define MAX_WAY atoi(argv[1])

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

#ifdef __cplusplus
};
#endif


/**
 * Get the base information for video and audio.
 *
 * @param filename the name of video to be parsered
 * @param vpamr pointer to the struct of video information defined in MediaCodec.h
 * @param apamr pointer to the struct of audio information defined in MediaCodec.h
 * @return whether get the base information of the video or not
 */
static int get_info_av(const char *filename, VIDEO_PARA_T* vpamr, AUDIO_PARA_T* apamr, int index)
{
	ALOGD("TS_PARSER : Beginning to do get_info_av()......");

	AVFormatContext *pFormatCtx;
    int             i, videoindex;
	int             audioindex;
	double          fps;
    AVCodecContext  *pCodecCtx, *aCodecCtx;
    AVCodec         *pCodec, *aCodec;
	AVStream        *st_video, *st_audio;
	const char      *vfmt_name, *afmt_name;
	bool hasAudio;

	av_register_all();
    avformat_network_init();

	pFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        ALOGD("TS_PARSER : Couldn't open input stream.\n");
        return -1;
    }

	if(avformat_find_stream_info(pFormatCtx, NULL)<0){
        ALOGD("TS_PARSER : Couldn't find stream information.\n");
        return -1;
    }

	videoindex = -1;
	audioindex = -1;
	for (i=0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
		} else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
		    audioindex = i;
			hasAudio = true;
		}
	}

	if (videoindex == -1) {
		ALOGD("TS_PARSER : Didn't find a video stream.\n");
		return -1;
	} else if (audioindex == -1) {
	    ALOGD("TS_PARSER : Didn't find a audio stream.");
		hasAudio = false;
		//return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (hasAudio) {
	    aCodecCtx = pFormatCtx->streams[audioindex]->codec;
	    aCodec = avcodec_find_encoder(aCodecCtx->codec_id);
    }

    /*if(pCodec == NULL){
        ALOGD("TS_PARSER : Video Codec not found.\n");
        //return -1;
    } else if (aCodec == NULL) {
        ALOGD("TS_PARSER : Audio Codec not found.\n");
        //return -1;
    }*/

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		ALOGD("TS_PARSER : Could not open video codec ! \n");
		//return -1;
	} else if (hasAudio && avcodec_open2(aCodecCtx, aCodec, NULL) < 0) {
		ALOGD("TS_PARSER : Could not open audio codec ! \n");
		//return -1;
	}

	av_dump_format(pFormatCtx, 0, filename, 0);

    st_video = pFormatCtx->streams[videoindex];
	if (hasAudio)
	    st_audio = pFormatCtx->streams[audioindex];

    vfmt_name = st_video->codec->codec->name;
	fps = av_q2d(st_video->avg_frame_rate);
    ALOGD("codec= %s, width= %d, height= %d, pid = %d, fps = %f,",
               vfmt_name, st_video->codec->coded_width,
               st_video->codec->coded_height, st_video->id, fps);

    /* pass video information */
	if (strcmp(vfmt_name, "h264") == 0) {
        vpamr->vFmt = VFORMAT_H264;
	} else if (strcmp(vfmt_name, "hevc") == 0) {
	    vpamr->vFmt = VFORMAT_HEVC;
	} else if (strcmp(vfmt_name, "mpeg12") == 0) {
	    vpamr->vFmt = VFORMAT_MPEG12;
	} else if (strcmp(vfmt_name, "mpeg4") == 0) {
	    vpamr->vFmt = VFORMAT_MPEG4;
	} else if (strcmp(vfmt_name, "vc1") == 0) {
	    vpamr->vFmt = VFORMAT_VC1;
	} else if (strcmp(vfmt_name, "avs") == 0) {
	    vpamr->vFmt = VFORMAT_AVS;
	} else if (strcmp(vfmt_name, "h264mvc") == 0) {
	    vpamr->vFmt = VFORMAT_H264MVC;
	} else if (strcmp(vfmt_name, "h264_4K2K") == 0) {
	    vpamr->vFmt = VFORMAT_H264_4K2K;
	}

	vpamr->pid = st_video->id;
	vpamr->nVideoWidth = st_video->codec->coded_width;
	vpamr->nVideoHeight = st_video->codec->coded_height;
	vpamr->nFrameRate = fps;
	vpamr->nExtraSize = 0;
	vpamr->pExtraData = NULL;

    /* pass audio information */
	if (hasAudio) {
        afmt_name = avcodec_get_name(aCodecCtx->codec_id);
	    ALOGD("codec = %s, sample_rate = %d, channels = %d, pid = %d, extradata_size = %d, block_align = %d, bit_rate = %d",
               afmt_name, st_audio->codec->sample_rate,st_audio->codec->channels, st_audio->id,
               st_audio->codec->extradata_size, st_audio->codec->block_align, st_audio->codec->bit_rate/1000);

        if (strcmp(afmt_name, "mp2") == 0) {
		    apamr->aFmt = AFORMAT_MPEG2;
        } else if (strcmp(afmt_name, "aac") == 0) {
            apamr->aFmt = AFORMAT_AAC;
        } else if (strcmp(afmt_name, "ac3") == 0) {
            apamr->aFmt = AFORMAT_AC3;
        } else if (strcmp(afmt_name, "mpeg") == 0) {
            apamr->aFmt = AFORMAT_MPEG;
        } else if (strcmp(afmt_name, "alaw") == 0) {
            apamr->aFmt = AFORMAT_ALAW;
        } else if (strcmp(afmt_name, "dts") == 0) {
            apamr->aFmt = AFORMAT_DTS;
        } else if (strcmp(afmt_name, "mpeg1") == 0) {
            apamr->aFmt = AFORMAT_MPEG1;
        } else if (strcmp(afmt_name, "eac3") == 0) {
            apamr->aFmt = AFORMAT_EAC3;
        } else if (strcmp(afmt_name, "alac") == 0) {
            apamr->aFmt = AFORMAT_ALAC;
        } else if (strcmp(afmt_name, "pcm_bluray") == 0) {
            apamr->aFmt = AFORMAT_PCM_BLURAY;
        }

	    apamr->pid = st_audio->id;
	    apamr->nChannels = st_audio->codec->channels;
	    apamr->nSampleRate = st_audio->codec->sample_rate;
	    //apamr->block_align = st_audio->codec->block_align;
	    //apamr->bit_per_sample = (st_audio->codec->bit_rate) /1000;
	    apamr->nExtraSize = st_audio->codec->extradata_size;
        if (apamr->nExtraSize != 0) {
            for(int i = 0; i < apamr->nExtraSize; i++)
                ALOGD("apamr[%d]->pExtraData:%02x", index, *(st_audio->codec->extradata + i));
            if ((0 == index) && (apamr->aFmt == AFORMAT_PCM_BLURAY)) {
                uint8_t array[apamr->nExtraSize];
                memset(array, 0, apamr->nExtraSize);
                memcpy(array, st_audio->codec->extradata, apamr->nExtraSize);
                uint8_t tmp;
                for(int i = 0; i <= (apamr->nExtraSize -1) / 2; i++) {
                    tmp = array[i];
                    array[i] = array[apamr->nExtraSize - i - 1];
                    array[apamr->nExtraSize - i - 1] = tmp;
                }
                apamr->pExtraData = array;
            } else
                apamr->pExtraData = st_audio->codec->extradata;
            for(int i = 0; i <= apamr->nExtraSize -1; i++)
                ALOGD("apamr[%d]->pExtraData: %02x", index, *(apamr->pExtraData + i));
        } else
            apamr->pExtraData = NULL;
	}

	ALOGD("TS_PARSER : get_info_av() end......\n");

    /* free memory for format and codec */
    avcodec_close(pCodecCtx);
	if (hasAudio)
	    avcodec_close(aCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
#endif

int amsysfs_set_sysfs_int(const char *path, int val)
{
    int fd;
    int bytes;
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
        ALOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return -1;
}

void* doWrite(void *arg) {
    int i = *(static_cast<int*>(arg));
    while (true) {
        int ret = read(mSourceFD[i], mBuf[i], kReadSize);
        if (ret == 0) {
            ALOGD("player %d rewind", i);
            lseek(mSourceFD[i], 0, SEEK_SET);
            continue;
        }
		int dataSizeWritten;
		while ((dataSizeWritten = player[i]->WriteData(mBuf[i], ret)) == -1)
		{
			usleep(5 * 1000);
		}

        usleep(5 * 1000);
    }
    return NULL;
}
sp<SurfaceControl> mSoftControl[9];
sp<Surface> mSoftSurface[9];
sp<SurfaceComposerClient> mSoftComposerClient[9];

void createSurface(int mInstanceNo, int x, int y, int weight, int high) {
    mSoftComposerClient[mInstanceNo] = new SurfaceComposerClient;
    mSoftComposerClient[mInstanceNo]->initCheck();

//    sp<IBinder> display(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
//    DisplayInfo info;
//    SurfaceComposerClient::getDisplayInfo(display, &info);
//    ALOGI("CTsHwOmxPlayer::mInstanceNo=%d,createWindowSurface: %d, %d, %d, %d\n", mInstanceNo, mSoftNativeX, mSoftNativeY, mSoftNativeWidth, mSoftNativeHeight);

    mSoftControl[mInstanceNo] = mSoftComposerClient[mInstanceNo]->createSurface(
                String8("ctc"),
                weight,
                high,
                PIXEL_FORMAT_RGB_565,
                0);
	ALOGD("createSurface:x = %d, y = %d, weight = %d,high = %d", x, y, weight,high);
    if (mSoftControl[mInstanceNo] == NULL)
		return;
    mSoftControl[mInstanceNo]->isValid();

#if ANDROID_PLATFORM_SDK_VERSION <= 27

    SurfaceComposerClient::openGlobalTransaction();

    mSoftControl[mInstanceNo]->setLayer(INT_MAX);
    mSoftControl[mInstanceNo]->show();
    mSoftControl[mInstanceNo]->setPosition(x, y);
    SurfaceComposerClient::closeGlobalTransaction();

#else
    SurfaceComposerClient::Transaction{}
        .hide(mSoftControl[mInstanceNo])
        .setLayer(mSoftControl[mInstanceNo], INT_MAX)
        .setPosition(mSoftControl[mInstanceNo], x, y)
        .setSize(mSoftControl[mInstanceNo], weight, y)
        .show(mSoftControl[mInstanceNo])
        .apply();
#endif
    mSoftSurface[mInstanceNo] = mSoftControl[mInstanceNo]->getSurface();
    if (mSoftSurface[mInstanceNo] == NULL)
		return;
	player[mInstanceNo]->SetSurface(mSoftSurface[mInstanceNo].get());
}

int main(int argc, char* argv[]) {
    ALOGD("start ctc_test");
	amsysfs_set_sysfs_int("/sys/module/amvideo/parameters/ctsplayer_exist", 0);

    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("media.decoder.omx", value, "0");
    int prop_use_omxdecoder = atoi(value);


    int instNum = atoi(argv[1]);
    fprintf(stderr, "instNum=%d\n", instNum);
    if (instNum > 1)
        property_set("media.ctcplayer.enable", "1");
    else
        property_set("media.ctcplayer.enable", "0");
    int x, y, w, h, column_number, line_number;
    if (instNum > 6) {
        column_number = ceil(sqrt((double)instNum));
        line_number = ceil(((double)instNum) /column_number);
        w = 1280 / column_number,h = 720 / line_number;
    } else {
        x = 0, y = 0, w = 1280, h = 720;
    }
    int type_mp4 = atoi(argv[2]);
    int gap=0;
    char video_dir[256]={'\n'};
    property_get("media.ctc.gap", video_dir, NULL);
    sscanf(video_dir, "%d", &gap);
    fprintf(stderr, "gap=%d\n", gap);

    property_get("media.ctc.video-dir", video_dir, NULL);
    ALOGD("media.ctc.video-dir: %s", video_dir);
    fprintf(stderr, "media.ctc.video-dir: %s\n", video_dir);
    VIDEO_PARA_T *vidPara;
	vidPara = (VIDEO_PARA_T *)malloc(sizeof(VIDEO_PARA_T) * instNum);
    AUDIO_PARA_T *audPara;
	audPara = (AUDIO_PARA_T *)malloc(sizeof(AUDIO_PARA_T) * instNum);
    for (int i=0;i<instNum;i++) {
        LOG_LINE();
        usleep(1000);
        if (instNum > 6) {
            x = w * (i % column_number);
            y = h * (i / column_number);
        } else {
            w = 1280;
            h = 720;
            if (0 != i) {
                x = 2 + w - w / 5;
                y = 1 + ((i - 1) * h) / 5;
                w = w / 5 - 3;
                h = h / 5 - 2;
            }
        }
        if (0 == i && !prop_use_omxdecoder) {
            if (0 == type_mp4)
                sprintf(filename, "/storage/external_storage/sda1/demo_video/test%d.ts", i);
            else
                sprintf(filename, "/storage/external_storage/sda1/demo_video_mp4/test%d.ts", i);
            mSourceFD[i] = open(filename, O_RDONLY);
            get_info_av(filename, vidPara+i, audPara+i, i);
            player[i] = GetMediaControl(0);
            player[i]->InitVideo(vidPara+i);
            player[i]->InitAudio(audPara+i);
            createSurface(i, x, y, w, h);
            player[i]->SetVideoWindow(x, y, w, h);
            player[i]->StartPlay();
            LOG_LINE();

            mBuf[i] = (unsigned char*) malloc(sizeof(unsigned char) * kReadSize);
            if (mBuf[i] == NULL) {
                ALOGD("mBuf[%d] == NULL", i);
                return 0;
            }

            idx[i] = i;
            pthread_create(&(tid[i]), NULL, doWrite, (void *)(&idx[i]));
        } else {
            player[i] = GetMediaControl(2);
            if (0 == type_mp4) {
                sprintf(filename, "/storage/external_storage/sda1/demo_video/test%d.ts", i);
                mSourceFD[i] = open(filename, O_RDONLY);
                createSurface(i, x, y, w, h);
                player[i]->SetVideoWindow(x, y, w, h);
                player[i]->StartPlay();
                LOG_LINE();

                mBuf[i] = (unsigned char*) malloc(sizeof(unsigned char) * kReadSize);
                if (mBuf[i] == NULL) {
                    ALOGD("mBuf[%d] == NULL", i);
                    return 0;
                }

                idx[i] = i;
                pthread_create(&(tid[i]), NULL, doWrite, (void *)(&idx[i]));
            } else {
                sprintf(filename, "/storage/external_storage/sda1/demo_video_mp4/test%d.mp4", i);
                LOG_LINE();
                //player[i]->InitVideo(vidPara+i);
                //player[i]->InitAudio(audPara+i);
                player[i]->setDataSource(filename);
                createSurface(i, x, y, w, h);
                player[i]->SetVideoWindow(x, y, w, h);
                player[i]->StartPlay();
            }
        }
        usleep(100 * 1000);
    }

    ALOGD("[%s:%d]", __FUNCTION__, __LINE__);
	int current_audio_idx = 0;

    while (1) {
        char levels_value[92];
        int i=0;

        if (property_get("media.ctc.audio_idx", levels_value, NULL) > 0)
            sscanf(levels_value, "%d", &i);

		if (i == current_audio_idx || i < 0 || i > instNum)
			;
		else {
		    fprintf(stderr, "enable audio_idx %d\n", i);
			player[current_audio_idx]->ExecuteCmd("pauseAudio");
			player[i]->ExecuteCmd("resumeAudio");
			current_audio_idx = i;
		}

		usleep(100 * 1000);
	}
    free(vidPara);
    free(audPara);
    for (int i = 0; i < 9; i++) {
        if (mBuf[i] != NULL)
            free(mBuf[i]);
        else
            break;
	}
    return 0;
}
