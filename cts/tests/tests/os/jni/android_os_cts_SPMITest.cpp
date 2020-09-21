/**
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

#include <dirent.h>
#include <errno.h>
#include <jni.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

jboolean android_os_cts_SPMITest_leakPointer(JNIEnv *env, jobject thiz) {
  jboolean result = true;
  struct dirent **namelist = NULL;
  char *path = (char*)"/sys/kernel/debug";
  int n = scandir(path, &namelist, NULL, NULL);
  if (n <= 0) {
    printf("scandir %s failed %s\n", path, strerror(errno));
    return true;
  }
  int i = 0;
  for (i = 0; i < n; ++i) {
    if (strstr(namelist[i]->d_name, "ffffffc") != NULL) {
      result = false;
    }
  }
  while (n--) {
    free(namelist[n]);
  }
  free(namelist);
  return result;
}

static JNINativeMethod gMethods[] = {
    {"leakPointer", "()Z", (void *)android_os_cts_SPMITest_leakPointer},
};

int register_android_os_cts_SPMITest(JNIEnv *env) {
  jclass clazz = env->FindClass("android/os/cts/SPMITest");

  return env->RegisterNatives(clazz, gMethods,
                              sizeof(gMethods) / sizeof(JNINativeMethod));
}
