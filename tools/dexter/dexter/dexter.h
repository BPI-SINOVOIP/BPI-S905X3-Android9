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

#pragma once

#include "dissasembler.h"
#include "slicer/dex_ir.h"

#include <stdlib.h>
#include <memory>
#include <vector>

// Encapsulates the state (command line switches, stats, ...) and
// the interface of the command line .dex manipulation tool.
class Dexter {
  static constexpr const char* VERSION = "v1.1";

 public:
  Dexter(int argc, char* argv[]) : argc_(argc), argv_(argv) {}

  Dexter(const Dexter&) = delete;
  Dexter& operator=(const Dexter&) = delete;

  // main entry point, returns an appropriate proces exit code
  int Run();

 private:
  int ProcessDex();
  void PrintHelp();
  bool CreateNewImage(std::shared_ptr<ir::DexFile> dex_ir);

 private:
  // command line
  int argc_ = 0;
  char** argv_ = nullptr;

  // parsed options
  const char* extract_class_ = nullptr;
  bool stats_ = false;
  bool verbose_ = false;
  bool list_classes_ = false;
  const char* out_dex_filename_ = nullptr;
  const char* dex_filename_ = nullptr;
  bool dissasemble_ = false;
  bool print_map_ = false;
  std::vector<const char*> experiments_;
  DexDissasembler::CfgType cfg_type_ = DexDissasembler::CfgType::None;

  // basic timing stats
  double reader_time_ = 0;
  double writer_time_ = 0;
  double experiments_time_ = 0;
};
