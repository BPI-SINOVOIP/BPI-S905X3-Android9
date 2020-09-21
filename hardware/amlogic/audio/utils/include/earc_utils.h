/*
 * Copyright (C) 2019 Amlogic Corporation.
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

#ifndef _EARC_UTILS_H_
#define _EARC_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cutils/log.h>
#include <aml_alsa_mixer.h>

int earcrx_config_latency(struct mixer *pMixer, int latency);
int earctx_fetch_latency(struct mixer * pMixer);
int earcrx_config_cds(struct mixer *pMixer, char *cds_str);
int earcrx_fetch_cds(struct mixer *pMixer, char *cds_str);
int earctx_fetch_cds(struct mixer *pMixer, char *cds_str);

#endif
