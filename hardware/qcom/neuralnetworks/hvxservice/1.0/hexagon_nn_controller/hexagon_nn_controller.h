/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef HEXAGON_NN_CONTROLLER_H
#define HEXAGON_NN_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

// includes
#include "hexagon_nn_ops.h"

// hexagon types

typedef struct hexagon_nn_input {
    unsigned int src_id;
    unsigned int output_idx;
} hexagon_nn_input;

typedef struct hexagon_nn_output {
    unsigned int rank;
    unsigned int max_sizes[8];
    unsigned int elementsize;
    int zero_offset;
    float stepsize;
} hexagon_nn_output;

typedef struct hexagon_nn_perfinfo {
    unsigned int node_id;
    unsigned int executions;
    unsigned int counter_lo;
    unsigned int counter_hi;
} hexagon_nn_perfinfo;

typedef int hexagon_nn_nn_id;

typedef enum hexagon_nn_padding_type {
    NN_PAD_NA,
    NN_PAD_SAME,
    NN_PAD_VALID,
    NN_PAD_MIRROR_REFLECT,
    NN_PAD_MIRROR_SYMMETRIC,
    NN_PAD_SAME_CAFFE,
    _32BIT_PLACEHOLDER_hexagon_nn_padding_type = 0x7fffffff
} hexagon_nn_padding_type;

typedef struct hexagon_nn_tensordef {
    unsigned int batches;
    unsigned int height;
    unsigned int width;
    unsigned int depth;
    unsigned char* data;
    int dataLen;
    unsigned int data_valid_len;
    unsigned int unused;
} hexagon_nn_tensordef;

// interface types

typedef int (*hexagon_nn_controller_init_fn)(hexagon_nn_nn_id* g);

typedef int (*hexagon_nn_controller_getlog_fn)(hexagon_nn_nn_id id, unsigned char* buf,
                                               unsigned int length);

typedef int (*hexagon_nn_controller_snpprint_fn)(hexagon_nn_nn_id id, unsigned char* buf,
                                                 unsigned int length);

typedef int (*hexagon_nn_controller_set_debug_level_fn)(hexagon_nn_nn_id id, int level);

typedef int (*hexagon_nn_controller_prepare_fn)(hexagon_nn_nn_id id);

typedef int (*hexagon_nn_controller_append_node_fn)(
    hexagon_nn_nn_id id, unsigned int node_id, op_type operation, hexagon_nn_padding_type padding,
    const hexagon_nn_input* inputs, unsigned int num_inputs, const hexagon_nn_output* outputs,
    unsigned int num_outputs);

typedef int (*hexagon_nn_controller_append_const_node_fn)(hexagon_nn_nn_id id, unsigned int node_id,
                                                          unsigned int batches, unsigned int height,
                                                          unsigned int width, unsigned int depth,
                                                          const unsigned char* data,
                                                          unsigned int data_len);

typedef int (*hexagon_nn_controller_execute_new_fn)(hexagon_nn_nn_id id,
                                                    const hexagon_nn_tensordef* inputs,
                                                    unsigned int n_inputs,
                                                    hexagon_nn_tensordef* outputs,
                                                    unsigned int n_outputs);

typedef int (*hexagon_nn_controller_execute_fn)(hexagon_nn_nn_id id, unsigned int batches_in,
                                                unsigned int height_in, unsigned int width_in,
                                                unsigned int depth_in, const unsigned char* data_in,
                                                unsigned int data_len_in, unsigned int* batches_out,
                                                unsigned int* height_out, unsigned int* width_out,
                                                unsigned int* depth_out, unsigned char* data_out,
                                                unsigned int data_out_max,
                                                unsigned int* data_out_size);

typedef int (*hexagon_nn_controller_teardown_fn)(hexagon_nn_nn_id id);

typedef int (*hexagon_nn_controller_get_perfinfo_fn)(hexagon_nn_nn_id id,
                                                     hexagon_nn_perfinfo* info_out,
                                                     unsigned int info_out_len,
                                                     unsigned int* n_items_out);

typedef int (*hexagon_nn_controller_reset_perfinfo_fn)(hexagon_nn_nn_id id, unsigned int event);

typedef int (*hexagon_nn_controller_version_fn)(int* ver);

typedef int (*hexagon_nn_controller_last_execution_cycles_fn)(hexagon_nn_nn_id id,
                                                              unsigned int* cycles_lo,
                                                              unsigned int* cycles_hi);

typedef int (*hexagon_nn_controller_GetHexagonBinaryVersion_fn)(int* ver);

typedef int (*hexagon_nn_controller_PrintLog_fn)(const unsigned char* data_in,
                                                 unsigned int data_in_len);

typedef int (*hexagon_nn_controller_op_name_to_id_fn)(const char* name, unsigned int* id);

typedef int (*hexagon_nn_controller_op_id_to_name_fn)(const unsigned int id, char* name,
                                                      int name_len);

typedef int (*hexagon_nn_controller_disable_dcvs_fn)();

typedef int (*hexagon_nn_controller_set_powersave_level_fn)(unsigned int level);

typedef int (*hexagon_nn_controller_config_fn)();

typedef unsigned int (*hexagon_nn_controller_get_dsp_offset_fn)();

typedef int (*hexagon_nn_controller_boost_fn)(int bus_usage);

typedef int (*hexagon_nn_controller_slow_fn)();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // HEXAGON_NN_CONTROLLER_H
