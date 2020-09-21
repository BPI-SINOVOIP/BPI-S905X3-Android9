#
# Copyright (C) 2017 The Android Open Source Project
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
#
# This lets us build simple apps that talk to a Citadel dev board.
#

PROGS = test_citadel

# Assumes a full Nugget OS source tree
NOS_ROOT = $(realpath ../../..)
HOST_GENERIC = $(NOS_ROOT)/host/generic
HOST_LINUX = $(NOS_ROOT)/host/linux
MPSSE = $(NOS_ROOT)/core/nugget/util/libmpsse/src

INCDIRS = . \
	$(HOST_GENERIC)/nugget/include \
	$(HOST_GENERIC)/libnos_datagram/include \
	$(HOST_GENERIC)/libnos_transport/include \
	$(MPSSE)

EXT_SRCS = \
	$(HOST_LINUX)/citadel/libnos_datagram/citadel.c \
	$(HOST_GENERIC)/libnos_transport/transport.c \
	$(MPSSE)/mpsse.c \
	$(MPSSE)/support.c

ifneq ($(DEBUG),)
CFLAGS += -DDEBUG=$(DEBUG)
endif
CFLAGS += -Wall -Werror
CFLAGS += $(foreach I,$(INCDIRS),-I$(I))

CFLAGS += $(shell pkg-config --cflags libftdi libcrypto)
LDFLAGS += $(shell pkg-config --libs libftdi libcrypto)

% : %.c $(EXT_SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

.PHONY: all
all: $(PROGS)

.PHONY: clean
clean:
	rm -f $(PROGS)
