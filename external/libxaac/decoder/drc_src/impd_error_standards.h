/******************************************************************************
 *
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
*/

#ifndef IMPD_ERROR_STANDARDS_H
#define IMPD_ERROR_STANDARDS_H

/*****************************************************************************/
/* File includes                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Type definitions                                                          */
/*****************************************************************************/
typedef WORD32 IA_ERRORCODE;

/*****************************************************************************/
/* Constant hash defines                                                     */
/*****************************************************************************/
#define IA_NO_ERROR 0x00000000
/* error handling 'AND' definition */
#define IA_FATAL_ERROR 0x80000000

#endif /* __IMPD_ERROR_STANDARDS_H__ */
