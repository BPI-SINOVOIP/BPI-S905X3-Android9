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
 */

// $Id: DTSTranscode1m5.h,v 1.14 2007-08-09 01:41:03 dyee Exp $
#ifndef DTSTRANSCODE1M5_H_INCLUDED
#define DTSTRANSCODE1M5_H_INCLUDED

/*typedef struct pcm51_encoded_info_s
{
  unsigned int InfoValidFlag;
    unsigned int SampFs;
    unsigned int NumCh;
    unsigned int AcMode;
    unsigned int LFEFlag;
    unsigned int BitsPerSamp;
}pcm51_encoded_info_t;*/

int dts_transenc_init();
int dts_transenc_process_frame();
int dts_transenc_deinit();

#endif // DTSTRANSCODE1M5_H_INCLUDED
