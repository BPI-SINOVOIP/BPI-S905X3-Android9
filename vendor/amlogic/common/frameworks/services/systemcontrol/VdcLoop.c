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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2014/12/02
 *  @par function description:
 *  - 1 connect to vold to use vdc cmd with socket
 */

#define LOG_TAG "SystemControl"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <utils/Log.h>
#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>

static int do_monitor(int sock, int stop_after_cmd);
static int do_cmd(int sock, int argc, char **argv);

int vdc_loop(int argc, char **argv) {
    int sock;

    int sleep_count = 0;
    while ((sock = socket_local_client("vold",
                                 ANDROID_SOCKET_NAMESPACE_RESERVED,
                                 SOCK_STREAM)) < 0) {
        ALOGE("Error connecting (%s)\n", strerror(errno));
        sleep(1);
        if (sleep_count++ > 2)
            return -1;
    }

    if (!strcmp(argv[1], "monitor")) {
        do_monitor(sock, 0);
    } else {
        do_cmd(sock, argc, argv);
    }

    close(sock);
    return 0;
}

static int do_cmd(int sock, int argc, char **argv) {
    char final_cmd[4196] = "0 "; /* 0 is a (now required) sequence number */
    char pathname[4096] = {'\0'};
    char *pathpoint = NULL;
    int i;
    size_t ret;

    if ((argc > 3) && (strcmp(argv[argc-2], "mount") == 0)) {
        if (strlen(argv[argc-1]) > 4096) {
            ALOGE("The Path name lengh[%ld] is too long, exceed Linux limit 4096 byte\n", strlen(argv[argc-1]));
            ALOGE("This is path:[%s]\n", argv[argc-1]);
            return errno;
        }
        pathpoint = argv[argc-1];
        while (*pathpoint != '\0') {
            if (*pathpoint == '/') {
                i = 0;
                pathpoint++;
                while (*pathpoint != '/' && *pathpoint != '\0')
                    pathname[i++] = *pathpoint++;
                if (strlen(pathname) > 255) {
                    ALOGE("The File name lengh [%ld] is too long, exceed Linux limit 255 byte\n", strlen(pathname));
                    ALOGE("This is file name:[%s]\n", pathname);
                    return errno;
                }
                memset(pathname, 0, 4096*sizeof(char));
            } else
                pathpoint++;
        }
    }

    for (i = 1; i < argc; i++) {
        char *cmp;

        if (!strchr(argv[i], ' '))
            asprintf(&cmp, "%s%s", argv[i], (i == (argc -1)) ? "" : " ");
        else
            asprintf(&cmp, "\"%s\"%s", argv[i], (i == (argc -1)) ? "" : " ");

        ret = strlcat(final_cmd, cmp, sizeof(final_cmd));
        if (ret >= sizeof(final_cmd))
            ALOGE(" strlcat ret:%d, sizeof(final_cmd):%d\n", ret, sizeof(final_cmd));
        free(cmp);
    }

    ALOGI("final_cmd: %s\n", final_cmd);

    if (write(sock, final_cmd, strlen(final_cmd) + 1) < 0) {
        ALOGE(" socket write error cmd:%s\n", final_cmd);
        return errno;
    }

    return do_monitor(sock, 1);
}

static int do_monitor(int sock, int stop_after_cmd) {
    char *buffer = malloc(4096);

    if (!stop_after_cmd)
        ALOGI("[Connected to Vold]\n");

    while (1) {
        fd_set read_fds;
        struct timeval to;
        int rc = 0;

        to.tv_sec = 10;
        to.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        if ((rc = select(sock +1, &read_fds, NULL, NULL, &to)) < 0) {
            ALOGE("Error in select (%s)\n", strerror(errno));
            free(buffer);
            return errno;
        } else if (!rc) {
            continue;
            ALOGE("[TIMEOUT]\n");
            return ETIMEDOUT;
        } else if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, 4096);
            if ((rc = read(sock, buffer, 4096)) <= 0) {
                if (rc == 0)
                    ALOGE("Lost connection to Vold - did it crash?\n");
                else
                    ALOGE("Error reading data (%s)\n", strerror(errno));
                free(buffer);
                if (rc == 0)
                    return ECONNRESET;
                return errno;
            }

            int offset = 0;
            int i = 0;

            for (i = 0; i < rc; i++) {
                if (buffer[i] == '\0') {
                    int code;
                    char tmp[4];

                    strncpy(tmp, buffer + offset, 3);
                    tmp[3] = '\0';
                    code = atoi(tmp);

                    ALOGI("%s\n", buffer + offset);
                    if (stop_after_cmd) {
                        if (code >= 200 && code < 600)
                            return 0;
                    }
                    offset = i + 1;
                }
            }
        }
    }
    free(buffer);
    return 0;
}

/*
static void usage(char *progname) {
    fprintf(stderr,
            "Usage: %s [--wait] <monitor>|<cmd> [arg1] [arg2...]\n", progname);
 }*/

