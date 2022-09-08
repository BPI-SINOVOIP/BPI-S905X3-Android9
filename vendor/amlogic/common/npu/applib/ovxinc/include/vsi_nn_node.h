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
#ifndef _VSI_NN_NODE_H
#define _VSI_NN_NODE_H

/*------------------------------------
               Includes
  -----------------------------------*/
#include "vsi_nn_platform.h"
#include "vsi_nn_ops.h"
#include "vsi_nn_tensor.h"
#include "vsi_nn_types.h"
#include "vsi_nn_node_type.h"

#if defined(__cplusplus)
extern "C"{
#endif

/*------------------------------------
                Macros
  -----------------------------------*/

/** Node invalid id */
#define VSI_NN_NODE_ID_NA           ((uint32_t)-1)
/** Node invalid uid */
#define VSI_NN_NODE_UID_NA          ((uint32_t)-1)

/*------------------------------------
                Types
  -----------------------------------*/
typedef enum _vsi_nn_cache_const_tensor_e
{
    VSI_NN_CACHE_CONST_TENSOR_DISABLED,
    VSI_NN_CACHE_CONST_TENSOR_CURRENT_GRAPH,
    VSI_NN_CACHE_CONST_TENSOR_PERMANENT,

    VSI_NN_CACHE_CONST_TENSOR_CNT
} vsi_nn_cache_const_tensor_e;

typedef struct _vsi_nn_node_attr_t
{
    int32_t cache_const_tensor_type;
    int32_t reserved[7];
} vsi_nn_node_attr_t;

/** Node structure */
struct _vsi_nn_node
{
    /**
     * Graph handle
     * @see vsi_nn_graph_t
     */
    vsi_nn_graph_t * graph;
    /** OpenVX node */
    vx_node          n;
    /**
     * Operation type
     * @see vsi_nn_op_t
     */
    vsi_nn_op_t      op;
    /** Node inputs */
    struct
    {
        vsi_nn_tensor_id_t * tensors;
        uint32_t            num;
    } input;
    /** Node outputs */
    struct
    {
        vsi_nn_tensor_id_t * tensors;
        uint32_t            num;
    } output;
    /** Operation parameters */
    vsi_nn_nn_param_t nn_param;
    /** Vx parameters */
    vsi_nn_vx_param_t vx_param;
    /**
     * User specific ID
     * This is for debug only.
     */
    uint32_t uid;
    /** Node's internal node wksp */
    void* internal_node_wksp;
    vsi_nn_node_attr_t attr;
};

/*------------------------------------
              Functions
  -----------------------------------*/
/**
 * New node
 * Create a new node with given input and output number.
 *
 * @param[in] graph Graph handle.
 * @param[in] op Operation type.
 * @param[in] input_num Input tensor number, set to 0 to use default value.
 * @param[in] output_num Output tensor number, set to 0 to use default value.
 * @see vei_nn_op_t
 *
 * @return Node handle on success, or NULL otherwise.
 */
OVXLIB_API vsi_nn_node_t * vsi_nn_NewNode
    (
    vsi_nn_graph_t * graph,
    vsi_nn_op_t      op,
    uint32_t         input_num,
    uint32_t         output_num
    );

/**
 * @deprecated
 * @see vsi_nn_NewNode
 */
OVXLIB_API vsi_nn_node_t * vsi_nn_CreateNode
    (
    vsi_nn_graph_t * graph,
    vsi_nn_op_t      op
    );

/**
 * Release node
 * Release a node and set the handle to NULL.
 *
 * param[in] node Node handle.
 */
OVXLIB_API void vsi_nn_ReleaseNode
    (
    vsi_nn_node_t ** node
    );

/**
 * Print node
 * Print brief info of a node.
 *
 * @param[in] node Node handle.
 * @param[in] id Node id.
 */
OVXLIB_API void vsi_nn_PrintNode
    (
    vsi_nn_node_t * node,
    vsi_nn_node_id_t id
    );

#if defined(__cplusplus)
}
#endif

#endif
