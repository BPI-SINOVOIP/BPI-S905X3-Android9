/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define CITADEL_IOC_MAGIC           'c'
#define CITADEL_IOC_RESET           _IO(CITADEL_IOC_MAGIC, 2)

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
  int fd, r;

  fd = open("/dev/citadel0", O_RDWR);
  if (fd < 0) {
    perror("can't open /dev/citadel0");
    fprintf(stderr, "did you run \"setprop ctl.stop vendor.citadeld\" ?\n");
    return 1;
  }

  r = ioctl(fd, CITADEL_IOC_RESET);
  close(fd);

  if (r) {
    perror("ioctl failed");
    return 1;
  }

  fprintf(stderr, "Citadel should have been reset, AFAICT\n");

  return 0;
}
