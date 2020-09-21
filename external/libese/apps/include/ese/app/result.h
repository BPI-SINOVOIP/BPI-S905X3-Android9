/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ESE_APP_RESULT_H_
#define ESE_APP_RESULT_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

// EseAppResult is structured with the low order bits being
// the enum below and the high order bits being a short
// from the applet when the code is ERROR_OS or ERROR_APPLET.
typedef enum {
  ESE_APP_RESULT_FALSE,
  ESE_APP_RESULT_OK = ESE_APP_RESULT_FALSE,
  ESE_APP_RESULT_TRUE = 1,
  ESE_APP_RESULT_ERROR_ARGUMENTS,
  ESE_APP_RESULT_ERROR_COMM_FAILED,
  ESE_APP_RESULT_ERROR_OS,
  ESE_APP_RESULT_ERROR_APPLET,
  ESE_APP_RESULT_ERROR_UNCONFIGURED,
  ESE_APP_RESULT_ERROR_COOLDOWN,
} EseAppResult;

#define EseAppResultValue(_res) (res & 0xffff)
#define EseAppResultAppValue(_res) (res >> 16)
#define ese_make_os_result(_app_hi, _app_lo) \
  ((((_app_hi) << 8 | (_app_lo)) << 16) | (ESE_APP_RESULT_ERROR_OS))
#define ese_make_app_result(_app_hi, _app_lo) \
  ((((_app_hi) << 8 | (_app_lo)) << 16) | (ESE_APP_RESULT_ERROR_APPLET))

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ESE_APP_RESULT_H_ */
