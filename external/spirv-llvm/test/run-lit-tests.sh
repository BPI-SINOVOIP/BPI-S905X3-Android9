#!/bin/bash -e

LIT_PATH=$ANDROID_BUILD_TOP/external/spirv-llvm/test/llvm-lit
LIBSPIRV_TESTS=$ANDROID_BUILD_TOP/external/spirv-llvm/test/SPIRV

$LIT_PATH $LIBSPIRV_TESTS $@
