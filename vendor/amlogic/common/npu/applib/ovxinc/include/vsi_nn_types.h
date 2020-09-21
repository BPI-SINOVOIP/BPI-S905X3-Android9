/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
/** @file */
#ifndef _VSI_NN_TYPES_H_
#define _VSI_NN_TYPES_H_

#include <stdint.h>
#include "vsi_nn_platform.h"
#include "vsi_nn_feature_config.h"

#if defined(__cplusplus)
extern "C"{
#endif

#ifdef _WIN32
#define inline __inline
#endif

/** Enumuration type */
typedef int32_t  vsi_enum;
/** Status type */
typedef int32_t  vsi_status;
/** Bool type */
typedef int32_t   vsi_bool;
/** Half */
typedef uint16_t vsi_float16;
/** Truncate float16 */
typedef uint16_t vsi_bfloat16;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/** Status enum */
typedef enum
{
    VSI_FAILURE = VX_FAILURE,
    VSI_SUCCESS = VX_SUCCESS,
}vsi_nn_status_e;

/** Pad enum */
typedef enum
{
    VSI_NN_PAD_AUTO,
    VSI_NN_PAD_VALID,
    VSI_NN_PAD_SAME
} vsi_nn_pad_e;

/**
 * @deprecated  Platform enum
 * @see vsi_nn_dim_fmt_e
 */
typedef enum
{
    VSI_NN_PLATFORM_CAFFE,
    VSI_NN_PLATFORM_TENSORFLOW
} vsi_nn_platform_e;

/** Round type enum */
typedef enum
{
    VSI_NN_ROUND_CEIL,
    VSI_NN_ROUND_FLOOR
} vsi_nn_round_type_e;

/** Optimize driction */
typedef enum
{
    VSI_NN_OPTIMIZE_FORWARD,
    VSI_NN_OPTIMIZE_BACKWARD
} vsi_nn_opt_direction_e;

/** Type enum */
typedef enum
{
    VSI_NN_TYPE_NONE = VX_TYPE_INVALID,
    VSI_NN_TYPE_INT8 = VX_TYPE_INT8,
    VSI_NN_TYPE_INT16 = VX_TYPE_INT16,
    VSI_NN_TYPE_INT32 = VX_TYPE_INT32,
    VSI_NN_TYPE_INT64 = VX_TYPE_INT64,
    VSI_NN_TYPE_UINT8 = VX_TYPE_UINT8,
    VSI_NN_TYPE_UINT16 = VX_TYPE_UINT16,
    VSI_NN_TYPE_UINT32 = VX_TYPE_UINT32,
    VSI_NN_TYPE_UINT64 = VX_TYPE_UINT64,
    VSI_NN_TYPE_FLOAT16 = VX_TYPE_FLOAT16,
    VSI_NN_TYPE_FLOAT32 = VX_TYPE_FLOAT32,
    VSI_NN_TYPE_FLOAT64 = VX_TYPE_FLOAT64,
#ifdef VSI_BOOL8_SUPPORT
    VSI_NN_TYPE_BOOL8 = VX_TYPE_BOOL8,
#else
    VSI_NN_TYPE_BOOL8 = 0x011,
#endif
#ifdef VSI_BFLOAT16_SUPPORT
    VSI_NN_TYPE_BFLOAT16 = VX_TYPE_BFLOAT16,
#else
    VSI_NN_TYPE_BFLOAT16 = 0x81A,
#endif
    VSI_NN_TYPE_VDATA = VX_TYPE_USER_STRUCT_START + 0x1,
}vsi_nn_type_e;

typedef int32_t vsi_nn_activation_e; enum
{
    VSI_NN_ACT_NONE    = 0,
    VSI_NN_ACT_RELU    = 1,
    VSI_NN_ACT_RELU1   = 2,
    VSI_NN_ACT_RELU6   = 3,
    VSI_NN_ACT_TANH    = 4,
    VSI_NN_ACT_SIGMOID = 6,

    VSI_NN_ACT_HARD_SIGMOID = 31, /* temporary use 31*/

    //Deprecated enum, reversed only for old code
    VSI_NN_LSTMUNIT_ACT_NONE    = 0,
    VSI_NN_LSTMUNIT_ACT_RELU    = 1,
    VSI_NN_LSTMUNIT_ACT_RELU6   = 3,
    VSI_NN_LSTMUNIT_ACT_TANH    = 4,
    VSI_NN_LSTMUNIT_ACT_SIGMOID = 6,

    VSI_NN_LSTMUNIT_ACT_HARD_SIGMOID = 31,

    VSI_NN_GRU_ACT_NONE    = 0,
    VSI_NN_GRU_ACT_RELU    = 1,
    VSI_NN_GRU_ACT_RELU6   = 3,
    VSI_NN_GRU_ACT_TANH    = 4,
    VSI_NN_GRU_ACT_SIGMOID = 6,

    VSI_NN_GRU_ACT_HARD_SIGMOID = 31
};


/** Deprecated */
typedef uint32_t vsi_nn_size_t;

/** Tensor id type */
typedef uint32_t vsi_nn_tensor_id_t;

/** Node id type */
typedef uint32_t vsi_nn_node_id_t;

/** @see _vsi_nn_graph */
typedef struct _vsi_nn_graph vsi_nn_graph_t;

/** @see _vsi_nn_node */
typedef struct _vsi_nn_node vsi_nn_node_t;

/** @see _vsi_nn_tensor */
typedef struct _vsi_nn_tensor vsi_nn_tensor_t;

#if defined(__cplusplus)
}
#endif

#endif
