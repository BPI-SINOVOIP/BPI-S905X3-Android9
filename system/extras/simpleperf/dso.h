/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef SIMPLE_PERF_DSO_H_
#define SIMPLE_PERF_DSO_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <android-base/logging.h>
#include <android-base/test_utils.h>

#include "build_id.h"
#include "read_elf.h"

struct Symbol {
  uint64_t addr;
  // TODO: make len uint32_t.
  uint64_t len;

  Symbol(const std::string& name, uint64_t addr, uint64_t len);
  const char* Name() const { return name_; }

  const char* DemangledName() const;

  bool HasDumpId() const {
    return dump_id_ != UINT_MAX;
  }

  bool GetDumpId(uint32_t* pdump_id) const {
    if (!HasDumpId()) {
      return false;
    }
    *pdump_id = dump_id_;
    return true;
  }

  static bool CompareByDumpId(const Symbol* s1, const Symbol* s2) {
    uint32_t id1 = UINT_MAX;
    s1->GetDumpId(&id1);
    uint32_t id2 = UINT_MAX;
    s2->GetDumpId(&id2);
    return id1 < id2;
  }

  static bool CompareByAddr(const Symbol* s1, const Symbol* s2) {
    return s1->addr < s2->addr;
  }

  static bool CompareValueByAddr(const Symbol& s1, const Symbol& s2) {
    return s1.addr < s2.addr;
  }

 private:
  const char* name_;
  mutable const char* demangled_name_;
  mutable uint32_t dump_id_;

  friend class Dso;
};

enum DsoType {
  DSO_KERNEL,
  DSO_KERNEL_MODULE,
  DSO_ELF_FILE,
};

struct KernelSymbol;
struct ElfFileSymbol;

class Dso {
 public:
  static void SetDemangle(bool demangle);
  static std::string Demangle(const std::string& name);
  static bool SetSymFsDir(const std::string& symfs_dir);
  static void SetVmlinux(const std::string& vmlinux);
  static void SetKallsyms(std::string kallsyms) {
    if (!kallsyms.empty()) {
      kallsyms_ = std::move(kallsyms);
    }
  }
  static void ReadKernelSymbolsFromProc() {
    read_kernel_symbols_from_proc_ = true;
  }
  static void SetBuildIds(
      const std::vector<std::pair<std::string, BuildId>>& build_ids);
  static BuildId FindExpectedBuildIdForPath(const std::string& path);
  static void SetVdsoFile(const std::string& vdso_file, bool is_64bit);

  static std::unique_ptr<Dso> CreateDso(DsoType dso_type, const std::string& dso_path,
                                        bool force_64bit = false);

  ~Dso();

  DsoType type() const { return type_; }

  // Return the path recorded in perf.data.
  const std::string& Path() const { return path_; }
  // Return the path containing symbol table and debug information.
  const std::string& GetDebugFilePath() const { return debug_file_path_; }
  // Return the file name without directory info.
  const std::string& FileName() const { return file_name_; }

  bool HasDumpId() {
    return dump_id_ != UINT_MAX;
  }

  bool GetDumpId(uint32_t* pdump_id) {
    if (!HasDumpId()) {
      return false;
    }
    *pdump_id = dump_id_;
    return true;
  }

  uint32_t CreateDumpId();
  uint32_t CreateSymbolDumpId(const Symbol* symbol);

  // Return the minimum virtual address in program header.
  uint64_t MinVirtualAddress();
  void SetMinVirtualAddress(uint64_t min_vaddr) { min_vaddr_ = min_vaddr; }

  const Symbol* FindSymbol(uint64_t vaddr_in_dso);

  const std::vector<Symbol>& GetSymbols();
  void SetSymbols(std::vector<Symbol>* symbols);

  // Create a symbol for a virtual address which can't find a corresponding
  // symbol in symbol table.
  void AddUnknownSymbol(uint64_t vaddr_in_dso, const std::string& name);

 private:
  static bool demangle_;
  static std::string symfs_dir_;
  static std::string vmlinux_;
  static std::string kallsyms_;
  static bool read_kernel_symbols_from_proc_;
  static std::unordered_map<std::string, BuildId> build_id_map_;
  static size_t dso_count_;
  static uint32_t g_dump_id_;
  static std::string vdso_64bit_;
  static std::string vdso_32bit_;

  Dso(DsoType type, const std::string& path, bool force_64bit);
  void Load();
  bool LoadKernel();
  bool LoadKernelModule();
  bool LoadElfFile();
  bool LoadEmbeddedElfFile();
  void FixupSymbolLength();
  BuildId GetExpectedBuildId();
  bool CheckReadSymbolResult(ElfStatus result, const std::string& filename);

  const DsoType type_;
  // path of the shared library used by the profiled program
  const std::string path_;
  // path of the shared library having symbol table and debug information
  // It is the same as path_, or has the same build id as path_.
  std::string debug_file_path_;
  // File name of the shared library, got by removing directories in path_.
  std::string file_name_;
  uint64_t min_vaddr_;
  std::vector<Symbol> symbols_;
  // unknown symbols are like [libc.so+0x1234].
  std::unordered_map<uint64_t, Symbol> unknown_symbols_;
  bool is_loaded_;
  // Used to identify current dso if it needs to be dumped.
  uint32_t dump_id_;
  // Used to assign dump_id for symbols in current dso.
  uint32_t symbol_dump_id_;
  android::base::LogSeverity symbol_warning_loglevel_;
};

const char* DsoTypeToString(DsoType dso_type);

#endif  // SIMPLE_PERF_DSO_H_
