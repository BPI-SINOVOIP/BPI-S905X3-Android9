// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dso.h"

#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include <algorithm>
#include <vector>

#include <build_id.h>
#include <read_elf.h>

#include "base/logging.h"
#include "compat/string.h"
#include "file_reader.h"

namespace quipper {

void InitializeLibelf() {
  // Unnecessary.
}

bool ReadElfBuildId(const string &filename, string *buildid) {
  BuildId id;
  ElfStatus status = GetBuildIdFromElfFile(filename, &id);
  if (status == ElfStatus::NO_ERROR) {
    *buildid = id.ToString();
    return true;
  }
  return false;
}

bool ReadElfBuildId(int fd, string *buildid) {
  // TODO: Implement. b/74410255.
  return false;
}

// read /sys/module/<module_name>/notes/.note.gnu.build-id
bool ReadModuleBuildId(const string &module_name, string *buildid) {
  string note_filename =
      "/sys/module/" + module_name + "/notes/.note.gnu.build-id";

  FileReader file(note_filename);
  if (!file.IsOpen()) return false;

  return ReadBuildIdNote(&file, buildid);
}

bool ReadBuildIdNote(DataReader *data, string *buildid) {
  // Non-simpleperf implementation, as a reader is given.
  Elf64_Nhdr note_header;

  while (data->ReadData(sizeof(note_header), &note_header)) {
    size_t name_size = Align<4>(note_header.n_namesz);
    size_t desc_size = Align<4>(note_header.n_descsz);

    string name;
    if (!data->ReadString(name_size, &name)) return false;
    string desc;
    if (!data->ReadDataString(desc_size, &desc)) return false;
    if (note_header.n_type == NT_GNU_BUILD_ID && name == ELF_NOTE_GNU) {
      *buildid = desc;
      return true;
    }
  }
  return false;
}

bool IsKernelNonModuleName(string name) {
  // List from kernel: tools/perf/util/dso.c : __kmod_path__parse()
  static const std::vector<string> kKernelNonModuleNames{
      "[kernel.kallsyms]",
      "[guest.kernel.kallsyms",
      "[vdso]",
      "[vsyscall]",
  };

  for (const auto &n : kKernelNonModuleNames) {
    if (name.compare(0, n.size(), n) == 0) return true;
  }
  return false;
}

// Do the |DSOInfo| and |struct stat| refer to the same inode?
bool SameInode(const DSOInfo &dso, const struct stat *s) {
  return dso.maj == major(s->st_dev) && dso.min == minor(s->st_dev) &&
         dso.ino == s->st_ino;
}

}  // namespace quipper
