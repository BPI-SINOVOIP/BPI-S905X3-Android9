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

#ifndef ANDROID_HARDWARE_V1_0_HEXAGON_CONTROLLER_H
#define ANDROID_HARDWARE_V1_0_HEXAGON_CONTROLLER_H

#include <android-base/logging.h>
#include "HexagonUtils.h"
#include "dlfcn.h"
#include "hexagon_nn_controller/hexagon_nn_controller.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {
namespace hexagon {

// interface wrapper
class Controller {
    // methods
   private:
    Controller();
    ~Controller();
    Controller(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller& operator=(Controller&&) = delete;

    template <typename Function>
    Function loadFunction(const char* name) {
        void* fn = dlsym(mHandle, name);
        if (fn == nullptr) {
            LOG(ERROR) << "loadFunction -- failed to load function " << name;
        }
        return reinterpret_cast<Function>(fn);
    }

    bool openNnlib();
    bool closeNnlib();

   public:
    static Controller& getInstance();
    bool resetNnlib();

    int init(hexagon_nn_nn_id* g);

    int getlog(hexagon_nn_nn_id id, unsigned char* buf, uint32_t length);

    int snpprint(hexagon_nn_nn_id id, unsigned char* buf, uint32_t length);

    int set_debug_level(hexagon_nn_nn_id id, int level);

    int prepare(hexagon_nn_nn_id id);

    int append_node(hexagon_nn_nn_id id, uint32_t node_id, op_type operation,
                    hexagon_nn_padding_type padding, const hexagon_nn_input* inputs,
                    uint32_t num_inputs, const hexagon_nn_output* outputs, uint32_t num_outputs);

    int append_const_node(hexagon_nn_nn_id id, uint32_t node_id, uint32_t batches, uint32_t height,
                          uint32_t width, uint32_t depth, const uint8_t* data, uint32_t data_len);

    int execute_new(hexagon_nn_nn_id id, const hexagon_nn_tensordef* inputs, uint32_t n_inputs,
                    hexagon_nn_tensordef* outputs, uint32_t n_outputs);

    int execute(hexagon_nn_nn_id id, uint32_t batches_in, uint32_t height_in, uint32_t width_in,
                uint32_t depth_in, const uint8_t* data_in, uint32_t data_len_in,
                uint32_t* batches_out, uint32_t* height_out, uint32_t* width_out,
                uint32_t* depth_out, uint8_t* data_out, uint32_t data_out_max,
                uint32_t* data_out_size);

    int teardown(hexagon_nn_nn_id id);

    int get_perfinfo(hexagon_nn_nn_id id, hexagon_nn_perfinfo* info_out, unsigned int info_out_len,
                     unsigned int* n_items_out);

    int reset_perfinfo(hexagon_nn_nn_id id, uint32_t event);

    int version(int* ver);

    int last_execution_cycles(hexagon_nn_nn_id id, unsigned int* cycles_lo,
                              unsigned int* cycles_hi);

    int GetHexagonBinaryVersion(int* ver);

    int PrintLog(const uint8_t* data_in, unsigned int data_in_len);

    int op_name_to_id(const char* name, unsigned int* id);

    int op_id_to_name(const unsigned int id, char* name, int name_len);

    int disable_dcvs();

    int set_powersave_level(unsigned int level);

    int config();

    unsigned int get_dsp_offset();

    int boost(int bus_usage);

    int slow();

    // members
   private:
    static const char kFilename[];
    void* mHandle;
    hexagon_nn_controller_init_fn mFn_init;
    hexagon_nn_controller_getlog_fn mFn_getlog;
    hexagon_nn_controller_snpprint_fn mFn_snpprint;
    hexagon_nn_controller_set_debug_level_fn mFn_set_debug_level;
    hexagon_nn_controller_prepare_fn mFn_prepare;
    hexagon_nn_controller_append_node_fn mFn_append_node;
    hexagon_nn_controller_append_const_node_fn mFn_append_const_node;
    hexagon_nn_controller_execute_new_fn mFn_execute_new;
    hexagon_nn_controller_execute_fn mFn_execute;
    hexagon_nn_controller_teardown_fn mFn_teardown;
    hexagon_nn_controller_get_perfinfo_fn mFn_get_perfinfo;
    hexagon_nn_controller_reset_perfinfo_fn mFn_reset_perfinfo;
    hexagon_nn_controller_version_fn mFn_version;
    hexagon_nn_controller_last_execution_cycles_fn mFn_last_execution_cycles;
    hexagon_nn_controller_GetHexagonBinaryVersion_fn mFn_GetHexagonBinaryVersion;
    hexagon_nn_controller_PrintLog_fn mFn_PrintLog;
    hexagon_nn_controller_op_name_to_id_fn mFn_op_name_to_id;
    hexagon_nn_controller_op_id_to_name_fn mFn_op_id_to_name;
    hexagon_nn_controller_disable_dcvs_fn mFn_disable_dcvs;
    hexagon_nn_controller_set_powersave_level_fn mFn_set_powersave_level;
    hexagon_nn_controller_config_fn mFn_config;
    hexagon_nn_controller_get_dsp_offset_fn mFn_get_dsp_offset;
    hexagon_nn_controller_boost_fn mFn_boost;
    hexagon_nn_controller_slow_fn mFn_slow;
};

}  // namespace hexagon
}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_V1_0_HEXAGON_CONTROLLER_H
