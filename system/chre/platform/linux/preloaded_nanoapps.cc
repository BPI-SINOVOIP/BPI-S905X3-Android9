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

#include "chre/platform/linux/preloaded_nanoapps.h"

#include "chre/core/event_loop_manager.h"
#include "chre/platform/fatal_error.h"
#include "chre/util/macros.h"
#include "chre/util/unique_ptr.h"

namespace chre {

void loadPreloadedNanoapps() {
  struct PreloadedNanoappDescriptor {
    const char *filename;
    UniquePtr<Nanoapp> nanoapp;
  };

  // The list of nanoapps to be loaded from the filesystem of the device.
  // TODO: allow these to be overridden by target-specific build configuration
  static PreloadedNanoappDescriptor preloadedNanoapps[] = {
    { "activity.so", MakeUnique<Nanoapp>() },
  };

  EventLoop& eventLoop = EventLoopManagerSingleton::get()->getEventLoop();
  for (size_t i = 0; i < ARRAY_SIZE(preloadedNanoapps); i++) {
    if (preloadedNanoapps[i].nanoapp.isNull()) {
      FATAL_ERROR("Couldn't allocate memory for preloaded nanoapp");
    } else {
      preloadedNanoapps[i].nanoapp->loadFromFile(preloadedNanoapps[i].filename);
      eventLoop.startNanoapp(preloadedNanoapps[i].nanoapp);
    }
  }
}

}  // namespace chre
