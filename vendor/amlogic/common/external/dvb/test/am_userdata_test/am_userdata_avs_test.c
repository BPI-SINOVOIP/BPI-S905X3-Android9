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
#include <sys/ioctl.h>
#include <hamm-tables.h>
/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define AMSTREAM_IOC_MAGIC  'S'
//#define AMSTREAM_IOC_UD_PICTYPE _IOR(AMSTREAM_IOC_MAGIC, 0x55, unsigned long)
#define AMSTREAM_IOC_UD_LENGTH _IOR(AMSTREAM_IOC_MAGIC, 0x54, unsigned long)
#define AMSTREAM_IOC_UD_POC _IOR(AMSTREAM_IOC_MAGIC, 0x55, unsigned long)
#define AMSTREAM_IOC_UD_FLUSH_USERDATA _IOR(AMSTREAM_IOC_MAGIC, 0x56, int)


#define DMA_BLOCK_LEN 8
#define INVALID_POC 0xFFFFFFF

#define USE_MAX_CACHE 1
#define MAX_POC_CACHE 5
typedef struct
{
     unsigned int poc_info;
     unsigned int poc_number;
}userdata_poc_info_t;
static void dump_user_data(const uint8_t *data, int len)
{
	int i;

	fprintf(stderr, "-------------------------------------------\n");
	fprintf(stderr, "read %d bytes data:\n", len);
	for (i=0;i<len;i++)
	{
		fprintf(stderr, "%02x ", data[i]);
		if (((i+1)%16) == 0)
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
int am_vbi_to_ascii(int c)
{
     if (c < 0)
        return '?';

     c &= 0x7F;

     if (c < 0x20 || c >= 0x7F)
         return '.';

     return c;
}
extern const int8_t     _vbi_hamm24_inv_par [3][256];
int am_vbi_unpar8(unsigned int	c)
{
/* Disabled until someone finds a reliable way
   to test for cmov support at compile time. */
	if (_vbi_hamm24_inv_par[0][(uint8_t) c] & 32) {
		return c & 127;
	} else {
		/* The idea is to OR results together to find a parity
		   error in a sequence, rather than a test and branch on
		   each byte. */
		return -1;
	}
}
void
am_vbi_decode_caption(int line, uint8_t *buf, int poc)
{
	char c1 = buf[0] & 0x7F;
	int field2 = 1, i;

	switch (line) {
	case 21:	/* NTSC */
	case 22:	/* PAL */
		field2 = 0;
		break;

	case 335:	/* PAL, hardly XDS */
		break;

	case 284:	/* NTSC */
		//printf("Line 284, poc:%d, finish\n", poc);
		goto finish;

	default:
		//printf("Line %d, poc:%d, finish\n", line, poc);
		goto finish;
	}

	//if ( (buf[0]) < 0) {  //vbi_unpar8 (buf[0]) < 0
	if ( am_vbi_unpar8 (buf[0])  < 0) {  //
		c1 = 127;
		buf[0] = c1; /* traditional 'bad' glyph, ccfont has */
		buf[1] = c1; /*  room, design a special glyph? */
	}

	switch (c1) {

	case 0x01 ... 0x0F:
		//printf("c1:%#x, poc:%d, finish\n", c1, poc);
		break; /* XDS field 1?? */

	case 0x10 ... 0x1F:
		//cmd?
		//printf("c1:%#x, poc:%d, finish\n", c1, poc);
		break;

	default:
		printf( "prefree--@dump_cc: poc:%d, %c\n", poc, am_vbi_to_ascii (buf[0]));
		printf( "prefree--@dump_cc: poc:%d, %c\n", poc, am_vbi_to_ascii (buf[1]));
		break;
	}

 finish:
	return;
}
static void dump_cc_ascii(const uint8_t *buf, int poc)
{
    int cc_flag;
    int cc_count;
    int i;

    cc_flag = buf[1] & 0x40;
	if (!cc_flag)
    {
        printf( "### cc_flag is invalid\n");
        return;
    }
    /*
    if (buf[0] != 0x03)
    {
		printf("type code error\n");
		return;
    }
    */
    cc_count = buf[1] & 0x1f;

	for (i = 0; i < cc_count; ++i)
    {
        unsigned int b0;
        unsigned int cc_valid;
        unsigned int cc_type;
        unsigned char cc_data1;
        unsigned char cc_data2;

        b0 = buf[3 + i * 3];
        cc_valid = b0 & 4;
        cc_type = b0 & 3;
        cc_data1 = buf[4 + i * 3];
        cc_data2 = buf[5 + i * 3];


		if (cc_type == 0 || cc_type == 1) //NTSC pair, Line 21
        {
            //printf( "### get poc:%d, cc data: %c %c\n", poc, vbi_to_ascii(cc_data1), vbi_to_ascii(cc_data2));
			if (!cc_valid || i >= 3)
                break;
            am_vbi_decode_caption((0==cc_type)?21:284, &buf[4+i*3], poc);
        }
    }
}
int main(int argc, char **argv)
{
	int fd, poc;
	struct pollfd fds;
	uint8_t buf[5*1024];
	uint8_t *p;
	uint8_t cc_data[256];/*In fact, 99 is enough*/
	uint8_t cc_data_cnt;
	unsigned int cnt, left, min_bytes_valid = 0;
	userdata_poc_info_t poc_block;
	int first_check = 0;
	int flush = 0;

	left = 0;
	fd = open("/dev/amstream_userdata", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open amstream_userdata: %s\n", strerror(errno));
		return -1;
	}

	fds.events = POLLIN | POLLERR;
	fds.fd = fd;

	while (1) {
		if (poll(&fds, 1, 1000) > 0) {
			cnt = read(fd, buf+left, sizeof(buf)-left);
			if (flush == 0)
			{
				int ret = ioctl(fd, AMSTREAM_IOC_UD_FLUSH_USERDATA, NULL);
				printf("@=try to flush userdata cache:%d, cache_cnt\n", ret);
				flush = 1;
				continue;
			}

			cnt += left;
			left = 0;
			aml_swap_data(buf, cnt);
			p = buf;
			while (cnt > 0)
			{
				//if(p[0] == 0x47 && p[1] == 0x41 && p[2] == 0x39 && p[3] == 0x34)//avs
					if (p[0] == 0xb5 && p[1] == 0x0 && p[2] == 0x2f) //directv
				{
					if (cnt > DMA_BLOCK_LEN)
					{
						min_bytes_valid = 8 + (buf[5] & 0x1F)*3;
						if (cnt >= min_bytes_valid)
						{
							memset(&poc_block, 0, sizeof(userdata_poc_info_t));
							int ret = ioctl(fd, AMSTREAM_IOC_UD_POC, &poc_block);
							//printf("ioctl_get, ret=%d, poc_info:%#x, poc:%d\n", ret, poc_block.poc_info&0xFFFF, poc_block.poc_number);
							if (cnt >= min_bytes_valid) {
								dump_cc_ascii(p+4, poc_block.poc_number);//avs
								//dump_cc_ascii(p+4, poc_block.poc_number);//directv
							}

							while (min_bytes_valid%DMA_BLOCK_LEN)
								min_bytes_valid++;
							cnt -= min_bytes_valid;
							p += min_bytes_valid;
							continue;
						}
						else
						{
							left = cnt;
							memmove(buf, p, left);
							aml_swap_data(buf, left);
							printf( "####cnt:%d, min_bytes:%d, memmove %d buf\n", cnt, min_bytes_valid, left);
							break;
						}
					}
					else if(cnt > 0)
					{
						left = cnt;
						memmove(buf, p, left);
						aml_swap_data(buf, left);
						printf( "@@@@memmove %d buf\n", left);
						break;
					}
				}
				cnt -= DMA_BLOCK_LEN;
				p += DMA_BLOCK_LEN;
			}
		}else{
			fprintf(stderr, "no userdata available, is the video playing?\n");
		}
	}

	close(fd);
	return 0;
}
