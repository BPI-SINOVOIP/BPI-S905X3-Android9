#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

ifeq ($(ARCH_ARM_HAVE_NEON),true)

LOCAL_PATH := $(call my-dir)

# List of validated intrinsics (copy-pasted from Makefile)
ARM_NEON_TESTS_REFNAMES =                                                \
           vld1 vadd vld1_lane vld1_dup vdup vget_high vget_low          \
           vqdmlal_lane vqdmlsl_lane vext vshrn_n vset_lane vget_lane    \
           vqsub vqdmulh_lane vqdmull vqdmlal vqdmlsl vceq vcge vcle     \
           vcgt vclt vbsl vshl vdup_lane vrshrn_n vqdmull_lane           \
           vst1_lane vqshl vqshl_n vqrshrn_n vsub vqadd vabs vqabs       \
           vcombine vmax vmin vneg vqneg vmlal vmlal_lane vmlsl          \
           vmlsl_lane vmovl vmovn vmull vmull_lane vrev vrshl vshl_n     \
           vshr_n vsra_n vtrn vuzp vzip vreinterpret vqdmulh vqrdmulh    \
           vqrdmulh_lane vqrshl vaba vabal vabd vabdl vand vorr vorn     \
           veor vbic vcreate vldX_lane vmla vmls vmul                    \
           vmul_lane vmul_n vmull_n vqdmulh_n vqdmull_n vqrdmulh_n       \
           vmla_lane vmls_lane vmla_n vmls_n vmlal_n vmlsl_n vqdmlal_n   \
           vqdmlsl_n vsri_n vsli_n vtst vaddhn vraddhn vaddl vaddw       \
           vhadd vrhadd vhsub vsubl vsubw vsubhn vrsubhn vmvn vqmovn     \
           vqmovun vrshr_n vrsra_n vshll_n vpaddl vpadd vpadal           \
           vqshlu_n vclz vcls vcnt vqshrn_n vpmax vpmin vqshrun_n        \
           vqrshrun_n vstX_lane vtbX vrecpe vrsqrte vcage vcagt vcale    \
           vcalt vrecps vrsqrts vcvt

ARM_NEON_TESTS_REFLIST = $(addprefix ref_, $(ARM_NEON_TESTS_REFNAMES))

ARM_NEON_TESTS_SOURCES = compute_ref.c \
                         $(addsuffix .c, $(ARM_NEON_TESTS_REFLIST))

ARM_NEON_TESTS_REFGCCARM = stm-arm-neon.gccarm
ARM_NEON_TESTS_EXPECTED_INPUT = expected_input4gcc.txt

ARM_NEON_TESTS_CFLAGS = -DREFFILE=\"$(ARM_NEON_TESTS_REFGCCARM)\" \
                        -DGCCTESTS_FILE=\"$(ARM_NEON_TESTS_EXPECTED_INPUT)\"

ARM_NEON_TESTS_CFLAGS += \
     -Wall \
     -Werror \
     -Wno-format \
     -Wno-ignored-qualifiers \
     -Wno-uninitialized \
     -Wno-unused-function \
     -Wno-unused-variable \

include $(CLEAR_VARS)
LOCAL_MODULE := arm_neon_tests_arm
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_CFLAGS := $(ARM_NEON_TESTS_CFLAGS)
LOCAL_SRC_FILES := $(ARM_NEON_TESTS_SOURCES)
LOCAL_CXX_STL := none
include $(BUILD_NATIVE_TEST)

include $(CLEAR_VARS)
LOCAL_MODULE := arm_neon_tests_thumb
LOCAL_ARM_MODE := thumb
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_CFLAGS := $(ARM_NEON_TESTS_CFLAGS)
LOCAL_SRC_FILES := $(ARM_NEON_TESTS_SOURCES)
LOCAL_CXX_STL := none
include $(BUILD_NATIVE_TEST)

endif
