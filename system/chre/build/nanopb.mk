#
# Nanoapp NanoPB Makefile
#
# Include this file in your nanoapp Makefile to produce pb.c and pb.h for .proto
# files specified in the NANOPB_SRCS variable. The produced pb.c files are
# automatically be added to the COMMON_SRCS variable for the nanoapp build.
#
# NANOPB_FLAGS can be used to supply additional command line arguments to the
# nanopb compiler. Note that this is global and applies to all protobuf
# generated source.

# Environment Checks ###########################################################

ifneq ($(NANOPB_SRCS),)
ifeq ($(NANOPB_PREFIX),)
$(error "NANOPB_SRCS is non-empty. You must supply a NANOPB_PREFIX environment \
         variable containing a path to the nanopb project. Example: \
         export NANOPB_PREFIX=$$HOME/path/to/nanopb/nanopb-c")
endif
endif

# Generated Source Files #######################################################

NANOPB_GEN_PATH = $(OUT)/nanopb_gen

NANOPB_GEN_SRCS += $(patsubst %.proto, $(NANOPB_GEN_PATH)/%.pb.c, \
                              $(NANOPB_SRCS))

ifneq ($(NANOPB_GEN_SRCS),)
COMMON_CFLAGS += -I$(NANOPB_PREFIX)
COMMON_CFLAGS += -I$(NANOPB_GEN_PATH)
COMMON_CFLAGS += $(addprefix -I$(NANOPB_GEN_PATH)/, $(NANOPB_INCLUDES))

COMMON_SRCS += $(NANOPB_PREFIX)/pb_common.c
COMMON_SRCS += $(NANOPB_PREFIX)/pb_decode.c
COMMON_SRCS += $(NANOPB_PREFIX)/pb_encode.c
endif

# NanoPB Compiler Flags ########################################################

ifneq ($(NANOPB_GEN_SRCS),)
COMMON_CFLAGS += -DPB_NO_PACKED_STRUCTS=1
endif

# NanoPB Generator Setup #######################################################

NANOPB_GENERATOR_SRCS = $(NANOPB_PREFIX)/generator/proto/nanopb_pb2.py
NANOPB_GENERATOR_SRCS += $(NANOPB_PREFIX)/generator/proto/plugin_pb2.py

$(NANOPB_GENERATOR_SRCS):
	cd $(NANOPB_PREFIX)/generator/proto && make

# Generate NanoPB Sources ######################################################

COMMON_SRCS += $(NANOPB_GEN_SRCS)

NANOPB_PROTOC = $(NANOPB_PREFIX)/generator/protoc-gen-nanopb

$(NANOPB_GEN_PATH)/%.pb.c $(NANOPB_GEN_PATH)/%.pb.h: %.proto \
                                                     %.options \
                                                     $(NANOPB_GENERATOR_SRCS)
	mkdir -p $(dir $@)
	protoc --plugin=protoc-gen-nanopb=$(NANOPB_PROTOC) $(NANOPB_FLAGS) \
	  --nanopb_out="--options-file=$(basename $<).options:$(NANOPB_GEN_PATH)/$(NANOPB_PROTO_PATH)" \
	  $<

$(NANOPB_GEN_PATH)/%.pb.c $(NANOPB_GEN_PATH)/%.pb.h: %.proto \
                                                     $(NANOPB_GENERATOR_SRCS)
	mkdir -p $(dir $@)
	protoc --plugin=protoc-gen-nanopb=$(NANOPB_PROTOC) $(NANOPB_FLAGS)\
	  --nanopb_out="$(NANOPB_GEN_PATH)" $<
