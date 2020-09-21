/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Brian Zhu
 *  @version  2.0
 *  @date     2017/05/04
 *  @par function description:
 *  - 1 DV Config file
 */

#define LOG_TAG "dv_config"
//#define LOG_NDEBUG 0

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>

#define DEVICE_NAME "/dev/amvecm"

int main(int argc, char* argv[])
{
    FILE *fp = NULL;
    int i;
    int dev_fd = -1;
    int ret = -1;
    unsigned int file_len = 0, temp_len;
    unsigned char *buff = NULL;

    for (i = 0; i < argc; i++)
        ALOGI("dv config parameter[%d] %s \n", i, argv[i]);

    if (argc < 2) {
        ALOGW("dv config file error");
        goto exit;
    }

    dev_fd = open(DEVICE_NAME, O_RDWR);
    if (dev_fd < 0) {
        ALOGW("dv config open %s error", DEVICE_NAME);
        goto exit;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        ALOGW("dv config open cfg file %s is failed!!!", argv[1]);
        goto exit;
    }

    fseek(fp, 0, SEEK_END);
    file_len =ftell(fp);
    ALOGI("dv cfg file len is %d", file_len);

    buff = malloc(file_len + 1);
    if (!buff) {
        ALOGW("dv config alloc buffer error");
        goto exit;
    }
    fseek(fp, 0, SEEK_SET);
    temp_len = fread(buff, 1, file_len, fp);
    if (temp_len != file_len) {
        ALOGW("dv config read cfg file len %d, want: %d", temp_len, file_len);
        goto exit;
    }
    ret = write(dev_fd, buff, file_len);
    ALOGI("dv config result is %d", ret);
    ret = 0;
exit:
    if (buff)
        free(buff);
    if (fp)
        fclose(fp);
    if (dev_fd >= 0)
        close(dev_fd);
    return ret;
}
