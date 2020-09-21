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
#ifndef __PCMENC_API_H
#define __PCMENC_API_H

typedef struct pcm51_encoded_info_s {
    unsigned int InfoValidFlag;
    unsigned int SampFs;
    unsigned int NumCh;
    unsigned int AcMode;
    unsigned int LFEFlag;
    unsigned int BitsPerSamp;
} pcm51_encoded_info_t;


#define AUDIODSP_PCMENC_GET_RING_BUF_SIZE      _IOR('l', 0x01, unsigned long)
#define AUDIODSP_PCMENC_GET_RING_BUF_CONTENT   _IOR('l', 0x02, unsigned long)
#define AUDIODSP_PCMENC_GET_RING_BUF_SPACE     _IOR('l', 0x03, unsigned long)
#define AUDIODSP_PCMENC_SET_RING_BUF_RPTR      _IOW('l', 0x04, unsigned long)
#define AUDIODSP_PCMENC_GET_PCMINFO            _IOR('l', 0x05, unsigned long)


extern int  pcmenc_init();
extern int  pcmenc_read_pcm(char *inputbuf, uint size);
extern int  pcmenc_deinit();
extern int  pcmenc_get_pcm_info(pcm51_encoded_info_t *info);
#endif