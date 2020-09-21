/******************************************************************************
 *
 *  Copyright 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <base/logging.h>

#include "bt_common.h"
#include "buffer_allocator.h"

static void* buffer_alloc(size_t size) {
  CHECK(size <= BT_DEFAULT_BUFFER_SIZE);
  return osi_malloc(size);
}

static const allocator_t interface = {buffer_alloc, osi_free};

const allocator_t* buffer_allocator_get_interface() { return &interface; }
