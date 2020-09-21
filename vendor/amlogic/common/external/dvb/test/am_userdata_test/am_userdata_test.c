#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief USER DATA 测试程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-10: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define USERDATA_POLL_TIMEOUT 100
#define MAX_CC_NUM			64
#define MAX_CC_DATA_LEN  (1024*5 + 4)
#define IS_H264(p)	((p[0] == 0xb5 && p[3] == 0x47 && p[4] == 0x41 && p[5] == 0x39 && p[6] == 0x34))
#define IS_DIRECTV(p) ((p[0] == 0xb5 && p[1] == 0x00 && p[2] == 0x2f))
#define IS_AVS(p)	((p[0] == 0x47) && (p[1] == 0x41) && (p[2] == 0x39) && (p[3] == 0x34))
#define IS_ATSC(p)	((p[0] == 0x47) && (p[1] == 0x41) && (p[2] == 0x39) && (p[3] == 0x34) && (p[4] == 0x3))
#define IS_SCTE(p)  ((p[0]==0x3) && ((p[1]&0x7f) == 1))


#define AMSTREAM_IOC_MAGIC  'S'
#define AMSTREAM_IOC_UD_LENGTH _IOR(AMSTREAM_IOC_MAGIC, 0x54, unsigned long)
#define AMSTREAM_IOC_UD_POC _IOR(AMSTREAM_IOC_MAGIC, 0x55, int)
#define AMSTREAM_IOC_UD_FLUSH_USERDATA _IOR(AMSTREAM_IOC_MAGIC, 0x56, int)
#define AMSTREAM_IOC_UD_BUF_READ _IOR(AMSTREAM_IOC_MAGIC, 0x57, int)
#define AMSTREAM_IOC_UD_AVAIBLE_VDEC      _IOR(AMSTREAM_IOC_MAGIC, 0x5c, unsigned int)


/**************************************************************************** * Type definitions ***************************************************************************/
typedef enum {
	INVALID_TYPE 	= 0,
	MPEG_CC_TYPE 	= 1,
	H264_CC_TYPE 	= 2,
	DIRECTV_CC_TYPE = 3,
	AVS_CC_TYPE 	= 4,
	SCTE_CC_TYPE = 5
} userdata_type;

typedef enum {
	/* 0 forbidden */
	I_TYPE = 1,
	P_TYPE = 2,
	B_TYPE = 3,
	D_TYPE = 4,
	/* 5 ... 7 reserved */
} picture_coding_type;


struct userdata_meta_info_t {
	uint32_t poc_number;
	/************ flags bit defination ***********/
	/*
	 * bit 0:		//used for mpeg2
	 *	1, group start
	 *	0, not group start
	 * bit 1-2:	//used for mpeg2
	 *	0, extension_and_user_data( 0 )
	 *	1, extension_and_user_data( 1 )
	 *	2, extension_and_user_data( 2 )
	 * bit 3-6:	//video format
	 *	0,	VFORMAT_MPEG12
	 *	1,	VFORMAT_MPEG4
	 *	2,	VFORMAT_H264
	 *	3,	VFORMAT_MJPEG
	 *	4,	VFORMAT_REAL
	 *	5,	VFORMAT_JPEG
	 *	6,	VFORMAT_VC1
	 *	7,	VFORMAT_AVS
	 *	8,	VFORMAT_SW
	 *	9,	VFORMAT_H264MVC
	 *	10, VFORMAT_H264_4K2K
	 *	11, VFORMAT_HEVC
	 *	12, VFORMAT_H264_ENC
	 *	13, VFORMAT_JPEG_ENC
	 *	14, VFORMAT_VP9
	 * bit 7-9:	//frame type
	 *	0, Unknown Frame Type
	 *	1, I Frame
	 *	2, B Frame
	 *	3, P Frame
	 *	4, D_Type_MPEG2
	 * bit 10:  //top_field_first_flag valid
	 *	0: top_field_first_flag is not valid
	 *	1: top_field_first_flag is valid
	 * bit 11: //top_field_first bit val
	 bit 12-13: //picture_struct, used for H264
		0: Invalid
		1: TOP_FIELD_PICTURE
		2: BOT_FIELD_PICTURE
		3: FRAME_PICTURE
	 */
	uint32_t flags;
	uint32_t vpts;			/*video frame pts*/
	/*
	 * 0: pts is invalid, please use duration to calcuate
	 * 1: pts is valid
	 */
	uint32_t vpts_valid;
	/*duration for frame*/
	uint32_t duration;
	/* how many records left in queue waiting to be read*/
	uint32_t records_in_que;
	unsigned long long priv_data;
	uint32_t padding_data[4];
};

struct userdata_param_t {
	uint32_t version;
	uint32_t instance_id; /*input, 0~9*/
	uint32_t buf_len; /*input*/
	uint32_t data_size; /*output*/
	void     *pbuf_addr; /*input*/
	struct userdata_meta_info_t meta_info; /*output*/
};

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

static void dump_user_data(const uint8_t *data, int len)
{
	int i;
	
	fprintf(stderr, "-------------------------------------------\n");
	fprintf(stderr, "read %d bytes data:\n", len);	
	for(i=0;i<len;i++)
	{
		fprintf(stderr, "%02x ", data[i]);
		if(((i+1)%16)==0) 
			fprintf(stderr, "\n");
	}
	
	fprintf(stderr, "\n-------------------------------------------\n");
	fprintf(stderr, "\n");
}

static void aml_swap_data(uint8_t *user_data, int ud_size)
{
	int swap_blocks, i, j, k, m;
	unsigned char c_temp;

	/* swap byte order */
	swap_blocks = ud_size / 8;
	for (i=0; i<swap_blocks; i++) {
		j = i * 8;
		k = j + 7;
		for (m=0; m<4; m++) {
			c_temp = user_data[j];
			user_data[j++] = user_data[k];
			user_data[k--] = c_temp;
		}
	}
}

static char display_buffer[10*1024];
int main(int argc, char **argv)
{
	int fd, cnt;
	struct pollfd fds;
	uint8_t buf[5*1024];
	int flush  = 1;
	int ret;
	int r, i;
	uint8_t data[MAX_CC_DATA_LEN];
	uint8_t *pd = data;
	int left = 0;
	int vdec_ids;
	struct userdata_param_t user_para_info;


	fd = open("/dev/amstream_userdata", O_RDONLY);
	if (fd < 0){
		fprintf(stderr, "Cannot open amstream_userdata: %s\n", strerror(errno));
		return -1;
	}

	fds.events = POLLIN | POLLERR;
	fds.fd = fd;

	while(1){
		ret = poll(&fds, 1, USERDATA_POLL_TIMEOUT);
		if (ret != 1)
			continue;
		if (!(fds.revents & POLLIN))
			continue;

		//For multi-instances support
		vdec_ids = 0;
		if (-1 == ioctl(fd, AMSTREAM_IOC_UD_AVAIBLE_VDEC, &vdec_ids)) {
			printf("get avaible vdec failed");
		} else {
			printf("get avaible vdec OK: 0x%x\n", vdec_ids);
		}

		if (!(vdec_ids & 1))
			continue;

		if (flush) {
			ioctl(fd, AMSTREAM_IOC_UD_FLUSH_USERDATA, 0);
			flush = 0;
			continue;
		}

		memset(&user_para_info, 0, sizeof(struct userdata_param_t));
		user_para_info.pbuf_addr= (void*)data;
		user_para_info.buf_len = sizeof(data);
		if (-1 == ioctl(fd, AMSTREAM_IOC_UD_BUF_READ, &user_para_info))
			printf("call AMSTREAM_IOC_UD_BUF_READ failed\n");
		printf("ioctl left data: %d\n",user_para_info.meta_info.records_in_que);

		r = user_para_info.data_size;
		r = (r > MAX_CC_DATA_LEN) ? MAX_CC_DATA_LEN : r;
		if (r <= 0)
			continue;
//			aml_swap_data(data + left, r);
//			left += r;

		aml_swap_data(data, r);
		pd = data;

#if 1
		printf("poc_number %d, flags %x, vpts %x valid %d\n",
				user_para_info.meta_info.poc_number,
				user_para_info.meta_info.flags,
				user_para_info.meta_info.vpts,
				user_para_info.meta_info.vpts_valid);

{
		int count;
		uint8_t *pcc_data;

		count = r;
		pcc_data = data;
		while (count >= 16) {
			for (i=0; i<16; i++)
				sprintf(&display_buffer[i*3], " %02x", pcc_data[i]);
			printf("%s\n", display_buffer);
			count -= 16;
			pcc_data += 16;
		}
		if (count > 0) {
			for (i=0; i<count; i++)
				sprintf(&display_buffer[i*3], " %02x", pcc_data[i]);
			printf("%s\n", display_buffer);
		}
}

#endif
	}

	close(fd);
	return 0;
}
