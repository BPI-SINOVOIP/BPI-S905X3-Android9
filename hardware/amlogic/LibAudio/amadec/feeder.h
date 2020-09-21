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
 *  DESCRIPTION:
 *      brief  Functions of Feeder.
 *
 */

#ifndef FEEDER_H
#define FEEDER_H

#include <audio-dec.h>

ADEC_BEGIN_DECLS

#define FORMAT_PATH             "/sys/class/astream/format"
#define CHANNUM_PATH            "/sys/class/astream/channum"
#define SAMPLE_RATE_PATH        "/sys/class/astream/samplerate"
#define DATAWIDTH_PATH      "/sys/class/astream/datawidth"
#define AUDIOPTS_PATH           "/sys/class/astream/pts"

ADEC_END_DECLS

#endif
