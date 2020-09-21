/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef PA_Decrypt_H_

#define PA_Decrypt_H_


#ifdef __cplusplus
extern "C" {
#endif

extern int  PA_DecryptContentData(unsigned char byEncryptFlag, unsigned char* pbyData, int* pnDataLen);
extern uint32_t  PA_Getsecmem(unsigned int  type );
extern uint32_t  PA_Tvpsecmen(void);
extern uint32_t  PA_Tvp4K_defaultsize(void);
extern uint32_t  PA_free_cma_buffer(void);


#ifdef __cplusplus
}
#endif
#endif

