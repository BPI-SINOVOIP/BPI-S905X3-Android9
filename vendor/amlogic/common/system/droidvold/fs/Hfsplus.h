/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_VOLD_HFSPLUS_H
#define ANDROID_VOLD_HFSPLUS_H

#include <unistd.h>
#include <string>

namespace android {
namespace droidvold {
namespace hfsplus {

int Check(const char *fsPath);
int Mount(const char *fsPath, const char *mountPoint, bool ro,
               bool remount, int ownerUid, int ownerGid, int permMask,
               bool createLost);
int Format(const char *fsPath, unsigned int numSectors);

}  // namespace hfsplus
}  // namespace vold
}  // namespace android

#endif
