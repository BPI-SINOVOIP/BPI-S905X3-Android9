/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <errno.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <ion/ion.h>

#define VDIN1_DEV "/dev/vdin1"
struct vdin_v4l2_param_s {
	int width;
	int height;
	int fps;
};

struct vdin_set_canvas_s {
	int fd;
	int index;
};

#define _TM_T 'T'
#define TVIN_IOC_S_VDIN_V4L2START  _IOW(_TM_T, 0x25, struct vdin_v4l2_param_s)
#define TVIN_IOC_S_CANVAS_ADDR  _IOW(_TM_T, 0x4f,\
	struct vdin_set_canvas_s)
#define TVIN_IOC_S_VDIN_V4L2STOP   _IO(_TM_T, 0x26)
#define TVIN_IOC_S_CANVAS_RECOVERY  _IO(_TM_T, 0x0a)


int main() {
    int dev = open(VDIN1_DEV, O_RDONLY);
    if (dev < 0) {
        return 0;
    }

    int ion_dev = ion_open();
    int ret;
    vdin_set_canvas_s canvas[6];
    int cavnsfd[6];

    for (int i = 0;i < 6; i++) {
        canvas[i].index = i;

        ion_user_handle_t ion_hnd = -1;
        ret = ion_alloc(ion_dev, 1920*1080*3, 64, 1 << ION_HEAP_TYPE_DMA, 0, &ion_hnd);
        ret = ion_share(ion_dev, ion_hnd, &cavnsfd[i]);
        canvas[i].fd = ::dup(cavnsfd[i]);
        printf("ion alloc %d - %d\n",i, canvas[i].fd);
        ion_free(ion_dev, ion_hnd);
    }

    printf("ion alloc end \n");

    ioctl(dev, TVIN_IOC_S_CANVAS_ADDR, canvas);
    printf("TVIN_IOC_S_CANVAS_ADDR \n");

    vdin_v4l2_param_s param;
    param.width = 1920;
    param.height = 1080;
    param.fps = 60;
    ioctl(dev, TVIN_IOC_S_VDIN_V4L2START, &param);
	printf("TVIN_IOC_S_VDIN_V4L2START \n");

	int savefd[6];
    savefd[0] = open("/sdcard/0.bin", O_CREAT | O_RDWR , 0644);
	savefd[1] = open("/sdcard/1.bin", O_CREAT | O_RDWR , 0644);
	savefd[2] = open("/sdcard/2.bin", O_CREAT | O_RDWR , 0644);
	savefd[3] = open("/sdcard/3.bin", O_CREAT | O_RDWR , 0644);
	savefd[4] = open("/sdcard/4.bin", O_CREAT | O_RDWR , 0644);
	savefd[5] = open("/sdcard/5.bin", O_CREAT | O_RDWR , 0644);

    //char idxstr[32];
    int id;

    struct pollfd fds[1];
    fds[0].fd = dev;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

  //  int i = 6;
    while (1) {
        int rtn = poll(fds, 1, 100000);
        if (rtn > 0 && fds[0].revents == POLLIN) {
            printf("poll ok\n");
            //memset(idxstr, 0, 32);
            if(read(dev, &id, sizeof(int)) >= 0) {
                int idx = id;//atoi(idxstr);
                printf("read ok, idx=%d\n", idx);
                #if 0
                if (idx < 6) {
                    //HANDLE BUFFER.
                    char * img = (char *)mmap(NULL, 1920*1080*3, PROT_READ, MAP_SHARED, cavnsfd[idx], 0);
                    printf("mmap buf %p\n", img);

                    if (write(savefd[idx], img, 1920*1080*3) <= 0) {
                        printf("write error return %d \n",-errno);
                    }
                    close(savefd[idx]);
                }
                #endif
                printf("TVIN_IOC_S_CANVAS_RECOVERY \n");

                ioctl(dev, TVIN_IOC_S_CANVAS_RECOVERY);
            } else {
                printf("read fail \n");
            }
        } else {
            printf("poll fail \n");
        }
    }

    for (int i = 0;i < 6; i++) {
        close(canvas[i].fd);
    }


    ioctl(dev, TVIN_IOC_S_VDIN_V4L2STOP);
    return 0;
}




