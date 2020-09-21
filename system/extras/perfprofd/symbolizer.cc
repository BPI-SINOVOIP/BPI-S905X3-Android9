/*
 *
 * Copyright 2018, The Android Open Source Project
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

#include "symbolizer.h"

#include <map>
#include <memory>
#include <unordered_map>

#include <android-base/logging.h>

#include "build_id.h"
#include "read_elf.h"

namespace perfprofd {

namespace {

struct SimpleperfSymbolizer : public Symbolizer {
  // For simplicity, we assume non-overlapping symbols.
  struct Symbol {
    std::string name;
    uint64_t length;
  };
  using SymbolMap = std::map<uint64_t, Symbol>;

  std::string Decode(const std::string& dso, uint64_t address) override {
    auto it = dsos.find(dso);
    if (it == dsos.end()) {
      LoadDso(dso);
      it = dsos.find(dso);
      DCHECK(it != dsos.end());
    }

    const SymbolMap& map = it->second;
    if (map.empty()) {
      return "";
    }
    auto upper_bound = map.upper_bound(address);
    if (upper_bound == map.begin()) {
      // Nope, not in the map.
      return "";
    }

    upper_bound--;
    if (upper_bound->first + upper_bound->second.length > address) {
      // This element covers the given address, return its name.
      return upper_bound->second.name;
    }

    return "";
  }

  void LoadDso(const std::string& dso) {
    SymbolMap data;
    auto callback = [&data](const ElfFileSymbol& sym) {
      Symbol symbol;
      symbol.name = sym.name;
      symbol.length = sym.len;
      data.emplace(sym.vaddr, std::move(symbol));
    };
    ElfStatus status = ParseSymbolsFromElfFile(dso, BuildId(), callback);
    if (status != ElfStatus::NO_ERROR) {
      LOG(WARNING) << "Could not parse dso " << dso << ": " << status;
    }
    dsos.emplace(dso, std::move(data));
  }

  bool GetMinExecutableVAddr(const std::string& dso, uint64_t* addr) override {
    ElfStatus status = ReadMinExecutableVirtualAddressFromElfFile(dso, BuildId(), addr);
    return status == ElfStatus::NO_ERROR;
  }

  std::unordered_map<std::string, SymbolMap> dsos;
};

}  // namespace

std::unique_ptr<Symbolizer> CreateELFSymbolizer() {
  return std::unique_ptr<Symbolizer>(new SimpleperfSymbolizer());
}

}  // namespace perfprofd

