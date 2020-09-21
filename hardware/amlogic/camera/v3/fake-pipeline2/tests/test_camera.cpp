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
 *
 * Description:
 */

#include <camera_hw.h>

static const char short_options[] = "d:hmruofc:";

static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", no_argument,       NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
        int frame_count = 0;
        char *dev_name = "/dev/video0";
        struct VideoInfo *vinfo;
        int ret = 0;
        FILE* fp;

        uint8_t *src = NULL;
        uint8_t *dst = NULL;

        fp = fopen("/sdcard/raw.data", "ab+");

        vinfo = (struct VideoInfo *) calloc(1, sizeof(*vinfo));

        if (NULL == vinfo){
                CAMHAL_LOGDA("calloc failed\n");
                return -1;
        }

        for (;;) {
                int idx;
                int c;

                c = getopt_long(argc, argv,
                                short_options, long_options, &idx);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'd':
                        dev_name = optarg;
                        break;

                case 'h':

                case 'm':
                        break;

                case 'r':
                        break;

                case 'u':
                        break;

                case 'o':
                        break;

                case 'f':
                        break;

                case 'c':
                        errno = 0;
                        frame_count = strtol(optarg, NULL, 0);
                        break;

                default:
                        break;
                }
        }

        vinfo->idx = 0;
        vinfo->preview.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vinfo->preview.format.fmt.pix.width = 640;
        vinfo->preview.format.fmt.pix.height = 480;
        vinfo->preview.format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;

		vinfo->picture.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vinfo->picture.format.fmt.pix.width = 480;
        vinfo->picture.format.fmt.pix.height = 640;
        vinfo->picture.format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;//V4L2_PIX_FMT_NV21;
        ret = camera_open(vinfo);
        if (ret < 0) {
                return -1;
        }

        ret = setBuffersFormat(vinfo);
        if (ret < 0) {
                return -1;
        }

        ret = start_capturing(vinfo);
        if (ret < 0) {
                return -1;
        }

        while (frame_count > 0) {

                src = (uint8_t *)get_frame(vinfo);
                if (!src) {
                        usleep(30000);
                        continue;
                }

                fwrite( src, vinfo->preview.buf.length, 1, fp);

                putback_frame(vinfo);
                frame_count --;
        }

        stop_capturing(vinfo);
        camera_close(vinfo);

        if (vinfo){
                free(vinfo);
                vinfo = NULL;
        }

        if (fp){
                fclose(fp);
                fp = NULL;
        }


        return 0;
}
