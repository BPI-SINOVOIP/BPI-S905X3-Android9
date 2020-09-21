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
/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define AMSTREAM_IOC_MAGIC  'S'
//#define AMSTREAM_IOC_UD_PICTYPE _IOR(AMSTREAM_IOC_MAGIC, 0x55, unsigned long)
#define AMSTREAM_IOC_UD_LENGTH _IOR(AMSTREAM_IOC_MAGIC, 0x54, unsigned long)
#define AMSTREAM_IOC_UD_POC _IOR(AMSTREAM_IOC_MAGIC, 0x55, unsigned long)

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
int vbi_to_ascii(int c)
{
     if (c < 0)
        return '?';

     c &= 0x7F;

     if (c < 0x20 || c >= 0x7F)
         return '.';

     return c;
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


		if (cc_type == 0 ) //NTSC pair, Line 21
        {
            printf( "### get poc:%d, cc data: %c %c\n", poc, vbi_to_ascii(cc_data1), vbi_to_ascii(cc_data2));
			if (!cc_valid || i >= 3)
                break;
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
			cnt += left;
					left = 0;
					aml_swap_data(buf, cnt);
					p = buf;
					while (cnt > 0)
					{
						if(p[0] == 0xb5 && p[3] == 0x47 && p[4] == 0x41
							&& p[5] == 0x39 && p[6] == 0x34)
						{
							if (cnt > DMA_BLOCK_LEN)
							{
								min_bytes_valid = 11 + (buf[8] & 0x1F)*3;
								if (cnt >= min_bytes_valid)
								{
									memset(&poc_block, 0, sizeof(userdata_poc_info_t));
									min_bytes_valid = 11 + (buf[8] & 0x1F)*3;
									ioctl(fd, AMSTREAM_IOC_UD_POC, &poc_block);
									/*
									 *	通常本地播放从pos=0开始播放, 都能产生正确的poc-userdata配对, 但是如果Seek, 或者DTV播放, 这样的场景
									 *	我们的驱动会产生错误的poc, 需要丢弃当前的数据以及poc, 否则cc会乱码. poc_info 可以计算出当前的poc是
									 *  kernel送出来的第几帧poc, 如果发现开始播放第一次取出来的poc并不是第一帧poc, 那么就要丢弃这个poc,
									 * 	并且cc数据也要一起丢弃.
									 */
									if (first_check == 0)
									{
										unsigned int driver_userdata_pkt_len = min_bytes_valid;
										while (driver_userdata_pkt_len%DMA_BLOCK_LEN)
											driver_userdata_pkt_len++;
										printf( "read cc cnt:%d, min_bytes_valid:%d, pkt_len:%d\n", cnt, min_bytes_valid, driver_userdata_pkt_len);
										printf("@@@ check_first pocinfo:%#x, poc:%d ,jump %d package\n",
											poc_block.poc_info&0xFFFF, poc_block.poc_number,
											(poc_block.poc_info&0xFFFF)/min_bytes_valid);
										if (cnt > driver_userdata_pkt_len && (poc_block.poc_info&0xFFFF) > min_bytes_valid)
										{

											printf("break, pocinfo:%#x, poc:%d ,jump %d package\n",
												poc_block.poc_info&0xFFFF, poc_block.poc_number,
												(poc_block.poc_info&0xFFFF)/min_bytes_valid);
											cnt = 0;
											break;
										}
										first_check = 1;
									}

									if (cnt >= min_bytes_valid) {
										dump_cc_ascii(p+3+4, poc_block.poc_number);
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
									printf( "memmove %d buf\n", left);
									break;
								}
							}
							else if(cnt > 0)
							{
								left = cnt;
								memmove(buf, p, left);
								aml_swap_data(buf, left);
								printf( "memmove %d buf\n", left);
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
