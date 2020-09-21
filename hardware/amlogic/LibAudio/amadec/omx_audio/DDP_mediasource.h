/*
 * Copyright (C) 2018 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef MEDIA_DDPMEDIASOURCE_H_
#define MEDIA_DDPMEDIASOURCE_H_

#include  "MediaSource.h"
#include  "DataSource.h"
#include  "MediaBufferGroup.h"
#include  "MetaData.h"
#include  "audio_mediasource.h"
#include  "MediaBufferBase.h"

namespace android {

#define     DDPshort short
#define     DDPerr   short
#define     DDPushort unsigned short
#define     BYTESPERWRD         2
#define     BITSPERWRD          (BYTESPERWRD*8)
#define     SYNCWRD             ((DDPshort)0x0b77)
#define     MAXFSCOD            3
#define     MAXDDDATARATE       38
#define     BS_STD              8
#define     ISDD(bsid)          ((bsid) <= BS_STD)
#define     MAXCHANCFGS         8
#define     BS_AXE            16
#define     ISDDP(bsid)         ((bsid) <= BS_AXE && (bsid) > 10)
#define     BS_BITOFFSET      40
#define     PTR_HEAD_SIZE 7	//20
#define     FRAME_RECORD_NUM   40
	typedef struct {
		DDPshort *buf;
		DDPshort bitptr;
		DDPshort data;
	} DDP_BSTRM;

	typedef struct {
		unsigned char rawbuf[6144];
		int len;
	} BUF_T;

	const DDPshort chanary[MAXCHANCFGS] = { 2, 1, 2, 3, 3, 4, 4, 5 };
	enum { MODE11 = 0, MODE_RSVD = 0, MODE10, MODE20,
		MODE30, MODE21, MODE31, MODE22, MODE32
	};

	const DDPushort msktab[] = {
		0x0000, 0x8000, 0xc000, 0xe000,
		0xf000, 0xf800, 0xfc00, 0xfe00,
		0xff00, 0xff80, 0xffc0, 0xffe0,
		0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff
	};

	const DDPshort frmsizetab[MAXFSCOD][MAXDDDATARATE] = {
		/* 48kHz */
		{
		 64, 64, 80, 80, 96, 96, 112, 112,
		 128, 128, 160, 160, 192, 192, 224, 224,
		 256, 256, 320, 320, 384, 384, 448, 448,
		 512, 512, 640, 640, 768, 768, 896, 896,
		 1024, 1024, 1152, 1152, 1280, 1280},
		/* 44.1kHz */
		{
		 69, 70, 87, 88, 104, 105, 121, 122,
		 139, 140, 174, 175, 208, 209, 243, 244,
		 278, 279, 348, 349, 417, 418, 487, 488,
		 557, 558, 696, 697, 835, 836, 975, 976,
		 1114, 1115, 1253, 1254, 1393, 1394},
		/* 32kHz */
		{
		 96, 96, 120, 120, 144, 144, 168, 168,
		 192, 192, 240, 240, 288, 288, 336, 336,
		 384, 384, 480, 480, 576, 576, 672, 672,
		 768, 768, 960, 960, 1152, 1152, 1344, 1344,
		 1536, 1536, 1728, 1728, 1920, 1920}
	};

	typedef int (*fp_read_buffer) (unsigned char *, int);

	class DDP_MediaSource:public AudioMediaSource {
 public:
		DDP_MediaSource(void *read_buffer);

		status_t start(MetaData * params = NULL);
		status_t stop();
		 sp < MetaData > getFormat();
		status_t read(MediaBufferBase ** buffer,
			      const ReadOptions * options = NULL);

		int GetReadedBytes();
		int GetSampleRate();
		int GetChNum();
		virtual int GetChNumOriginal();
		int *Get_pStop_ReadBuf_Flag();
		int Set_pStop_ReadBuf_Flag(int *pStop);

		int SetReadedBytes(int size);
		int MediaSourceRead_buffer(unsigned char *buffer, int size);

		fp_read_buffer fpread_buffer;

		//----------------------------------------
		DDPerr ddbs_init(DDPshort * buf, DDPshort bitptr,
				 DDP_BSTRM * p_bstrm);
		DDPerr ddbs_unprj(DDP_BSTRM * p_bstrm, DDPshort * p_data,
				  DDPshort numbits);
		int Get_ChNum_DD(void *buf);
		int Get_ChNum_DDP(void *buf);
		DDPerr ddbs_skip(DDP_BSTRM * p_bstrm, DDPshort numbits);
		DDPerr ddbs_getbsid(DDP_BSTRM * p_inbstrm, DDPshort * p_bsid);
		int Get_ChNum_AC3_Frame(void *buf);
                int get_frame_size(void);
                void store_frame_size(int lastFrameLen);
                //---------------------------------------

		int sample_rate;
		int ChNum;
		int frame_size;
		BUF_T frame;
		int64_t bytes_readed_sum_pre;
		int64_t bytes_readed_sum;
		int extractor_cost_bytes;
		int extractor_cost_bytes_last;
		int *pStop_ReadBuf_Flag;
		int ChNumOriginal;
                int frame_length_his[FRAME_RECORD_NUM];
protected:
		 virtual ~ DDP_MediaSource();

 private:
		 bool mStarted;
		 sp < DataSource > mDataSource;
		 sp < MetaData > mMeta;
		MediaBufferGroup *mGroup;
		int64_t mCurrentTimeUs;
		int mBytesReaded;

		 DDP_MediaSource(const DDP_MediaSource &);
		 DDP_MediaSource & operator=(const DDP_MediaSource &);
	};

}

#endif
