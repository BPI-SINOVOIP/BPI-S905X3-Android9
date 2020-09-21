/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <keymaster/new>
#include <stdlib.h>

namespace std {
struct nothrow_t {};
}

const std::nothrow_t __attribute__((weak)) std::nothrow = {};

void* __attribute__((weak)) operator new(size_t __sz, const std::nothrow_t&) {
    return malloc(__sz);
}
void* __attribute__((weak)) operator new[](size_t __sz, const std::nothrow_t&) {
    return malloc(__sz);
}

void __attribute__((weak)) operator delete(void* ptr) {
    if (ptr)
        free(ptr);
}

void __attribute__((weak)) operator delete[](void* ptr) {
    if (ptr)
        free(ptr);
}

extern "C" {
void __attribute__((weak)) __cxa_pure_virtual() {
    abort();
}
}
