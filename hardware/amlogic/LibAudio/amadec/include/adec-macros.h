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
 *  DESCRIPTION
 *      Some Macros for Audio Dec
 *
 */

#ifndef ADEC_MACROS_H
#define ADEC_MACROS_H


#ifdef  __cplusplus
#define ADEC_BEGIN_DECLS    extern "C" {
#define ADEC_END_DECLS  }
#else
#define ADEC_BEGIN_DECLS
#define ADEC_END_DECLS
#endif


#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

typedef unsigned int    adec_bool_t;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))
#endif

#define MAX(X, Y)    ((X) > (Y)) ? (X) : (Y)
#define MIN(X, Y)    ((X) < (Y)) ? (X) : (Y)

#endif
