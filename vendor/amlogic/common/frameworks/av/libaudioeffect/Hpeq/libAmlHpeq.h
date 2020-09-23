/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_HPEQ_API_H_
#define ANDROID_HPEQ_API_H_
extern int HPEQ_init_api(void *data);
extern int HPEQ_release_api(void);
extern int HPEQ_reset_api(void);
extern int HPEQ_process_api(short *in, short *out, int framecount);
extern int HPEQ_setBand_api(int band, int index);
extern int HPEQ_getBand_api(int *band, int index);
#endif
