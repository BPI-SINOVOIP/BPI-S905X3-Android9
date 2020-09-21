/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AIDL_IMPORT_RESOLVER_H_
#define AIDL_IMPORT_RESOLVER_H_

#include <string>
#include <vector>

#include <android-base/macros.h>

#include "io_delegate.h"

namespace android {
namespace aidl {

class ImportResolver {
 public:
  ImportResolver(const IoDelegate& io_delegate,
                 const std::vector<std::string>& import_paths);
  virtual ~ImportResolver() = default;

  // Resolve the canonical name for a class to a file that exists
  // in one of the import paths given to the ImportResolver.
  std::string FindImportFile(const std::string& canonical_name) const;

 private:
  const IoDelegate& io_delegate_;
  std::vector<std::string> import_paths_;

  DISALLOW_COPY_AND_ASSIGN(ImportResolver);
};

}  // namespace android
}  // namespace aidl

#endif // AIDL_IMPORT_RESOLVER_H_
