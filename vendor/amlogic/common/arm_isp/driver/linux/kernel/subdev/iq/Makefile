#
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2011-2018 ARM or its affiliates
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2.
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

obj-m += iv009_isp_iq.o

M_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

FW_SRC := $(wildcard $(M_PATH)/src/*.c  $(M_PATH)/src/*/*.c \
                $(M_PATH)/src/*/*/*.c  $(M_PATH)/app/*.c  $(M_PATH)/app/*/*.c)
M_FW_SRC := $(FW_SRC:$(M_PATH)/%=%)
FW_SRC_OBJ := $(M_FW_SRC:.c=.o)

iv009_isp_iq-objs += $(FW_SRC_OBJ)

EXTRA_CFLAGS := -I$(M_PATH)/app -I$(M_PATH)/inc -I$(M_PATH)/app/control \
                -I$(M_PATH)/inc/api -I$(M_PATH)/inc/isp -I$(M_PATH)/inc/sys \
                -I$(M_PATH)/src/platform -I$(M_PATH)/src/fw \
                -I$(M_PATH)/src/fw_lib -I$(M_PATH)/src/calibration \
                -I$(M_PATH)/src/driver/sensor -I$(M_PATH)/src/driver/lens

EXTRA_CFLAGS += -Wno-declaration-after-statement -Idrivers/staging/android/ \
		-Wno-error

