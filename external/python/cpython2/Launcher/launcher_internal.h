// Copyright 2017 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ANDROID_CPYTHON2_PYTHON_LAUNCHER_INTERNAL_H
#define ANDROID_CPYTHON2_PYTHON_LAUNCHER_INTERNAL_H

#include <Python.h>

#include <string>

namespace android {
namespace cpython2 {
namespace python_launcher {

namespace internal{
#define ENTRYPOINT_FILE "entry_point.txt"
#define RUNFILES "runfiles"

// Use "runpy" module to locate and run Python script using Python module
// namespace rather than the filesystem.
// The caller owns "module" pointer, which cannot be NULL.
int RunModule(const char *module, int set_argv0);

// Get valid entrypoint file path.
// The caller owns "launcher_path" pointer, which cannot be NULL.
// Return non-empty string as success. Otherwise, return empty string.
std::string GetEntryPointFilePath(const char *launcher_path);

// Run the Python script embedded in ENTRYPOINT_FILE.
// The caller owns "launcher_path" pointer, which cannot be NULL.
int RunModuleNameFromEntryPoint(const char *launcher_path, std::string entrypoint);

// Add path to Python sys.path list.
// The caller owns "path" pointer, which cannot be NULL.
int AddPathToPythonSysPath(const char *path);

// Run __main__ module within the hermetic .par file.
// The caller owns "launcher_path" pointer, which cannot be NULL.
// Return 0 as success. Otherwise, return -1.
int RunMainFromImporter(const char *launcher_path);
}  // namespace internal

// Try to run the Python script embedded in ENTRYPOINT_FILE. Otherwise,
// run __main__ module as fallback.
// The caller owns "launcher_path" pointer, which cannot be NULL.
int RunEntryPointOrMainModule(const char *launcher_path);
}  // namespace python_launcher
}  // namespace cpython2
}  // namespace android

#endif  // ANDROID_CPYTHON2_PYTHON_LAUNCHER_INTERNAL_H
