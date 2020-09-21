/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**************************************************
* example based on multi instance frame check code
**************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <vcodec.h>
#include <dec_slt_res.h>

#define READ_SIZE       (64 * 1024)
#define EXTERNAL_PTS	(1)
#define SYNC_OUTSIDE	(2)
#define UNIT_FREQ       96000
#define PTS_FREQ        90000
#define AV_SYNC_THRESH	PTS_FREQ*30


#define TEST_CASE_HEVC 0
#define TEST_CASE_VDEC 1
#define RETRY_TIME     3


#define LPRINT0
#define LPRINT1(...)		printf(__VA_ARGS__)

#define ERRP(con, rt, p, ...) do {	\
	if (con) {						\
		LPRINT##p(__VA_ARGS__);	\
		rt;							\
	}								\
} while(0)

static vcodec_para_t v_codec_para;
static vcodec_para_t *pcodec, *vpcodec;

int set_tsync_enable(int enable)
{
    int fd;
    char *path = "/sys/class/tsync/enable";
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", enable);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

int set_cmd(const char *str, const char *path)
{
    int fd;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, str, strlen(str));
        close(fd);
        printf("[success]: %s > %s\n", str, path);
        return 0;
    }
    printf("[failed]: %s > %s\n", str, path);
    return -1;
}

/*
 * set tee load disable for some code tee is not updated;
 * set double write for hevc;
 * set frame check enable;
 * set check when 4 frame ready.
 */
static int slt_test_set(int tcase, int enable)
{
    if (enable) {
        set_cmd("1",      "/sys/module/tee/parameters/disable_flag");
        if (tcase == TEST_CASE_HEVC) {
            set_cmd("0x8000000", "/sys/module/amvdec_h265/parameters/debug");
            set_cmd("1",         "/sys/module/amvdec_h265/parameters/double_write_mode");
        }
    } else {
        set_cmd("0",      "/sys/module/tee/parameters/disable_flag");
        if (tcase == TEST_CASE_HEVC) {
            set_cmd("0",  "/sys/module/amvdec_h265/parameters/debug");
            set_cmd("0",  "/sys/module/amvdec_h265/parameters/double_write_mode");
        }
    }
    printf("%s test %s\n", (tcase == 11)?"hevc":"vdec",
        (enable == 0)?"end":"start");
    return 0;
}


static int do_video_decoder(int tcase)
{
    dec_sysinfo_t slt_sysinfo[2] = {
        [0] = {
            .format = VIDEO_DEC_FORMAT_HEVC,
            .width = 192,
            .height = 200,
            .rate = 96000/30,
            .param = (void *)(EXTERNAL_PTS | SYNC_OUTSIDE),
        },
        [1] = {
            .format = VIDEO_DEC_FORMAT_H264,
            .width = 80,
            .height = 96,
            .rate = 96000/30,
            .param = (void *)(EXTERNAL_PTS | SYNC_OUTSIDE),
        }
    };
    int ret = CODEC_ERROR_NONE;
    char buffer[READ_SIZE];

    unsigned int read_len, isize, pading_size, rest_size;
    unsigned int wait_cnt = 0;
    char *vstream = hevc_stream;

    vpcodec = &v_codec_para;
    memset(vpcodec, 0, sizeof(vcodec_para_t));
    vpcodec->has_video = 1;
    vpcodec->stream_type = STREAM_TYPE_ES_VIDEO;
    vpcodec->am_sysinfo = slt_sysinfo[tcase];
    vpcodec->video_type = (tcase == TEST_CASE_HEVC)?11:2;    //hevc ? vdec

    ret = vcodec_init(vpcodec);
    ERRP((ret != CODEC_ERROR_NONE), goto tst_error_0,
        1, "codec init failed, ret=-0x%x", -ret);

    set_tsync_enable(0);
    printf("vcodec init ok, set cmp crc !\n");

    if (tcase == TEST_CASE_HEVC) {
        vstream = hevc_stream;
        rest_size = sizeof(hevc_stream);
        vcodec_set_frame_cmp_crc(vpcodec, hevc_crc,
            sizeof(hevc_crc)/(sizeof(int)*2));
    } else {
        vstream = h264_stream;
        rest_size = sizeof(h264_stream);
        vcodec_set_frame_cmp_crc(vpcodec, h264_crc,
            sizeof(h264_crc)/(sizeof(int)*2));
    }

    pcodec = vpcodec;
    pading_size = 4096;
    while (1) {
        if (pading_size) {
            if (rest_size <= READ_SIZE) {
                memcpy(buffer, vstream, rest_size);
                read_len = rest_size;
                rest_size = 0;
            } else {
                memcpy(buffer, vstream, READ_SIZE);
                read_len = READ_SIZE;
                rest_size -= read_len;
            }
        }
        isize = 0;
        wait_cnt = 0;
        do {
            ret = vcodec_write(pcodec, buffer + isize, read_len);
            //printf("write ret = %d, isize = %d, read_len = %d\n", ret, isize, read_len);
            if (ret <= 0) {
                if (errno != EAGAIN) {
                    printf("write data failed, errno %d\n", errno);
                    goto tst_error_1;
                } else {
                    usleep(1000);
                    if (++wait_cnt > 255) {
                      break;
                    }
                    continue;
                }
            } else {
                isize += ret;    /*all write size*/
                wait_cnt = 0;
            }
        } while (isize < read_len);

        if ((!rest_size) && (!pading_size))
            break;

        if (!rest_size) {
            memset(buffer, 0, pading_size);
            read_len = pading_size;
            pading_size = 0;
        }
    }

    /* wait cmp crc to end */
    wait_cnt = 0;
    do {
        usleep(10000);
        ret = is_crc_cmp_ongoing(vpcodec, 0);
        if (ret < 0)
            goto tst_error_1;
        if (++wait_cnt > 99) {
            printf("wait decode end timeout.\n");
            ret = -1;
            goto tst_error_1;
        }
    } while (ret);

    ret = vcodec_get_crc_check_result(vpcodec, 0);

    vcodec_close(vpcodec);

    return ret;

tst_error_1:
    vcodec_close(vpcodec);
tst_error_0:
    return ((ret < 0)?ret:-1);
}

int main(int argc, char *argv[])
{
    int tcase = TEST_CASE_HEVC;
    int ret = 0;
    int retry = 0;

    if (argc > 1) {
        ret = atoi(argv[1]);
        if (ret == 1)
            tcase = TEST_CASE_VDEC;
    }
    printf("Decoder SLT Test [%s]\n", (tcase == 1)?"VDEC":"HEVC");

    /* set cmd for test */
    slt_test_set(tcase, 1);

    retry = 0;
    do {
        set_cmd("0 1", "/sys/class/vdec/frame_check");
        if (retry > 0)
            usleep(10000);

        retry++;
        ret = do_video_decoder(tcase);
        if (ret < 0) {
            if (retry > (RETRY_TIME - 1))
                break;
            continue;
        }
    } while ((ret != 0) && (retry < RETRY_TIME));

    if (retry > 1)
        printf("ret: %d, retry: %d \n\n", ret, retry);

    /* set restore */
    slt_test_set(tcase, 0);

    printf("\n\n{\"result\": %s, \"item\": %s}\n\n\n",
        ret?"false":"true", (tcase == TEST_CASE_HEVC)?"hevc":"vdec");

    return 0;
}

