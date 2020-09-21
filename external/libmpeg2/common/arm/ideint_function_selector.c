/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
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
/**
*******************************************************************************
* @file
*  ideint_function_selector.c
*
* @brief
*  This file contains the function selector related code
*
* @author
*  Ittiam
*
* @par List of Functions:
* ih264e_init_function_ptr
*
* @remarks
*  None
*
*******************************************************************************
*/
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
/* System include files */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


/* User include files */
#include "icv_datatypes.h"
#include "icv_macros.h"
#include "icv.h"
#include "icv_variance.h"
#include "icv_sad.h"
#include "ideint.h"

#include "ideint_defs.h"
#include "ideint_structs.h"
#include "ideint_utils.h"
#include "ideint_cac.h"
#include "ideint_debug.h"
#include "ideint_function_selector.h"

/**
*******************************************************************************
*
* @brief
*  Call corresponding function pointer initialization function
*
* @par Description
*  Call corresponding function pointer initialization function
*
* @param[in] ps_ctxt
*  Context
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
void ideint_init_function_ptr(ctxt_t *ps_ctxt)
{
    ideint_init_function_ptr_generic(ps_ctxt);

    switch(ps_ctxt->s_params.e_arch)
    {
#if defined(ARMV8)
        default:
            ideint_init_function_ptr_av8(ps_ctxt);
            break;
#elif !defined(DISABLE_NEON)
        case ICV_ARM_NONEON:
            break;
        case ICV_ARM_A5:
        case ICV_ARM_A7:
        case ICV_ARM_A9:
        case ICV_ARM_A15:
        case ICV_ARM_A9Q:
        default:
            ideint_init_function_ptr_a9(ps_ctxt);
            break;
#else
        default:
            break;
#endif
    }

}

/**
*******************************************************************************
*
* @brief Determine the architecture of the encoder executing environment
*
* @par Description: This routine returns the architecture of the enviro-
* ment in which the current encoder is being tested
*
* @param[in] void
*
* @returns  IV_ARCH_T
*  architecture
*
* @remarks none
*
*******************************************************************************
*/
ICV_ARCH_T ideint_default_arch(void)
{
    return ICV_ARM_A9Q;
}

