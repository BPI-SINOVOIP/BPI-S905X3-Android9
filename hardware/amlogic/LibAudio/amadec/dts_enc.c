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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <log-print.h>
#include <pthread.h>
#include "dts_enc.h"
#include "dts_transenc_api.h"
#include <cutils/properties.h>
#include <amthreadpool.h>

#include "Amsysfsutils.h"
#include "amconfigutils.h"
#include "spdif_api.h"

#define LOG_TAG "dts_enc"
typedef enum {
    IDLE,
    TERMINATED,
    STOPPED,
    INITTED,
    ACTIVE,
    PAUSED,
} dtsenc_state_t;

typedef struct {
    dtsenc_state_t  state;
    pthread_t       thread_pid;
    int raw_mode;
    int dts_flag;
} dtsenc_info_t;


//static dtsenc_info_t dtsenc_info;
//static void *dts_enc_loop();

#define DIGITAL_RAW_PATH             "sys/class/audiodsp/digital_raw"
#define FORMAT_PATH                        "/sys/class/astream/format"

extern int match_types(const char *filetypestr, const char *typesetting);
/*
static int get_dts_mode(void)
{
    int val = 0;
    char  bcmd[28];
    amsysfs_get_sysfs_str(DIGITAL_RAW_PATH, bcmd, 28);
    val = bcmd[21] & 0xf;
    return val;

}

static int get_dts_format(void)
{
    char format[21];
    //int len;

    format[0] = 0;

    amsysfs_get_sysfs_str(FORMAT_PATH, format, 21);
    if (strncmp(format, "NA", 2) == 0) {
        return 0;
    }
    adec_print("amadec format: %s", format);
    if (strncmp(format, "amadec_dts", 10) == 0) {
        return 1;
    }
    return 0;
}

static int get_cpu_type(void)
{
    char value[PROPERTY_VALUE_MAX];
    int ret = property_get("ro.board.platform", value, NULL);
    adec_print("ro.board.platform = %s\n", value);
    if (ret > 0 && match_types("meson6", value)) {
        return 1;
    }
    return 0;
}*/
int dtsenc_init()
{
    return 0;
    /*
    int ret;
    memset(&dtsenc_info, 0, sizeof(dtsenc_info_t));
    dtsenc_info.dts_flag = get_dts_format();
    if (!dtsenc_info.dts_flag) {
        return -1;
    }
    dtsenc_info.raw_mode = get_dts_mode();
    //dtsenc_info.raw_mode=1;//default open
    if (!dtsenc_info.raw_mode) {
        return -1;
    }
    if (!get_cpu_type()) { //if cpu !=m6 ,skip
        return -1;
    }

    //adec_print("====dts_flag:%d raw_mode:%d \n",dtsenc_info.dts_flag,dtsenc_info.raw_mode);

    ret = dts_transenc_init();
    if (ret != 1) {
        adec_print("====dts_trancenc init failed \n");
        return -1;
    }
    dtsenc_info.state = INITTED;

    pthread_t    tid;
    ret = amthreadpool_pthread_create(&tid, NULL, (void *)dts_enc_loop, NULL);
    if (ret != 0) {
        dtsenc_release();
        return -1;
    }
    pthread_setname_np(tid, "AmadecDtsEncLP");
    dtsenc_info.thread_pid = tid;
    adec_print("====dts_enc init success \n");
    return 0;
    */
}
int dtsenc_start()
{
    return 0;
    /*
    int ret;
    if (dtsenc_info.state != INITTED) {
        return -1;
    }
    dtsenc_info.state = ACTIVE;
    adec_print("====dts_enc thread start success \n");
    return 0;
    */
}
int dtsenc_pause()
{
    return 0;
    /*
    if (dtsenc_info.state == ACTIVE) {
        dtsenc_info.state = PAUSED;
    }
    return 0;*/
}
int dtsenc_resume()
{
    return 0;
    //if (dtsenc_info.state == PAUSED) {
    //    dtsenc_info.state = ACTIVE;
    //}
    //return 0;
}
int dtsenc_stop()
{
    return 0;
    /*
    if (dtsenc_info.state < INITTED) {
        return -1;
    }
    dtsenc_info.state = STOPPED;
    //jone the thread
    if (dtsenc_info.thread_pid <= 0) {
        return -1;
    }
    amthreadpool_pthread_join(dtsenc_info.thread_pid, NULL);
    dtsenc_info.thread_pid = 0;
    if (dtsenc_info.state != STOPPED) {
        return -1;
    }
    dts_transenc_deinit();
    adec_print("====dts_enc stop ok\n");
    return 0;*/
}
int dtsenc_release()
{
    return 0;
    /*
    memset(&dtsenc_info, 0, sizeof(dtsenc_info_t));
    // dtsenc_info.state=TERMINATED;
    adec_print("====dts_enc release ok\n");
    return 0;*/
}

//static void *dts_enc_loop()
//{
//    return 0;
    /*
    int ret;
    while (1) {
        switch (dtsenc_info.state) {
        case INITTED:
            amthreadpool_thread_usleep(10000);
            continue;
        case ACTIVE:
            break;
        case PAUSED:
            iec958buf_fill_zero();
            amthreadpool_thread_usleep(100000);
            continue;
        case STOPPED:
            goto quit_loop;
        default:
            goto err;
        }
        //shaoshuai --non_block
        ret = dts_transenc_process_frame();
        //amthreadpool_thread_usleep(100000);
        //adec_print("====dts_enc thread is running \n");
    }
quit_loop:
    adec_print("====dts_enc thread exit success \n");
    pthread_exit(NULL);
    return NULL;
err:
    adec_print("====dts_enc thread exit success err\n");
    pthread_exit(NULL);
    return NULL;
    */
//}


