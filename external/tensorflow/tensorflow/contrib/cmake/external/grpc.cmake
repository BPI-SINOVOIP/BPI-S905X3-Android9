# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
include (ExternalProject)

set(GRPC_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/include)
set(GRPC_URL https://github.com/grpc/grpc.git)
set(GRPC_BUILD ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc)
set(GRPC_TAG 730b778632e79cc3c96ad237f282d687ee325ce7)

if(WIN32)
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    set(grpc_STATIC_LIBRARIES
        ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/Release/grpc++_unsecure.lib
        ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/Release/grpc_unsecure.lib
        ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/Release/gpr.lib)
  else()
    set(grpc_STATIC_LIBRARIES
        ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/grpc++_unsecure.lib
        ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/grpc_unsecure.lib
        ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/gpr.lib)
  endif()
else()
  set(grpc_STATIC_LIBRARIES
      ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/libgrpc++_unsecure.a
      ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/libgrpc_unsecure.a
      ${CMAKE_CURRENT_BINARY_DIR}/grpc/src/grpc/libgpr.a)
endif()

add_definitions(-DGRPC_ARES=0)

ExternalProject_Add(grpc
    PREFIX grpc
    DEPENDS protobuf zlib
    GIT_REPOSITORY ${GRPC_URL}
    GIT_TAG ${GRPC_TAG}
    DOWNLOAD_DIR "${DOWNLOAD_LOCATION}"
    BUILD_IN_SOURCE 1
    BUILD_BYPRODUCTS ${grpc_STATIC_LIBRARIES}
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release --target grpc++_unsecure
    COMMAND ${CMAKE_COMMAND} --build . --config Release --target grpc_cpp_plugin
    INSTALL_COMMAND ""
    CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=Release
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
        -DPROTOBUF_INCLUDE_DIRS:STRING=${PROTOBUF_INCLUDE_DIRS}
        -DPROTOBUF_LIBRARIES:STRING=${protobuf_STATIC_LIBRARIES}
        -DZLIB_ROOT:STRING=${ZLIB_INSTALL}
	-DgRPC_SSL_PROVIDER:STRING=NONE
)

# grpc/src/core/ext/census/tracing.c depends on the existence of openssl/rand.h.
ExternalProject_Add_Step(grpc copy_rand
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/patches/grpc/rand.h ${GRPC_BUILD}/include/openssl/rand.h
    DEPENDEES patch
    DEPENDERS build
)
