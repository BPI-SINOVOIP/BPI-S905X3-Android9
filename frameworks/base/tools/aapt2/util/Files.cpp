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

#include "util/Files.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <string>

#include "android-base/errors.h"
#include "android-base/file.h"
#include "android-base/logging.h"
#include "android-base/unique_fd.h"
#include "android-base/utf8.h"

#include "util/Util.h"

#ifdef _WIN32
// Windows includes.
#include <windows.h>
#endif

using ::android::FileMap;
using ::android::StringPiece;
using ::android::base::ReadFileToString;
using ::android::base::SystemErrorCodeToString;
using ::android::base::unique_fd;

namespace aapt {
namespace file {

#ifdef _WIN32
FileType GetFileType(const std::string& path) {
  std::wstring path_utf16;
  if (!::android::base::UTF8PathToWindowsLongPath(path.c_str(), &path_utf16)) {
    return FileType::kNonexistant;
  }

  DWORD result = GetFileAttributesW(path_utf16.c_str());
  if (result == INVALID_FILE_ATTRIBUTES) {
    return FileType::kNonexistant;
  }

  if (result & FILE_ATTRIBUTE_DIRECTORY) {
    return FileType::kDirectory;
  }

  // Too many types to consider, just let open fail later.
  return FileType::kRegular;
}
#else
FileType GetFileType(const std::string& path) {
  struct stat sb;
  int result = stat(path.c_str(), &sb);

  if (result == -1) {
    if (errno == ENOENT || errno == ENOTDIR) {
      return FileType::kNonexistant;
    }
    return FileType::kUnknown;
  }

  if (S_ISREG(sb.st_mode)) {
    return FileType::kRegular;
  } else if (S_ISDIR(sb.st_mode)) {
    return FileType::kDirectory;
  } else if (S_ISCHR(sb.st_mode)) {
    return FileType::kCharDev;
  } else if (S_ISBLK(sb.st_mode)) {
    return FileType::kBlockDev;
  } else if (S_ISFIFO(sb.st_mode)) {
    return FileType::kFifo;
#if defined(S_ISLNK)
  } else if (S_ISLNK(sb.st_mode)) {
    return FileType::kSymlink;
#endif
#if defined(S_ISSOCK)
  } else if (S_ISSOCK(sb.st_mode)) {
    return FileType::kSocket;
#endif
  } else {
    return FileType::kUnknown;
  }
}
#endif

bool mkdirs(const std::string& path) {
  constexpr const mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP;
  // Start after the first character so that we don't consume the root '/'.
  // This is safe to do with unicode because '/' will never match with a continuation character.
  size_t current_pos = 1u;
  while ((current_pos = path.find(sDirSep, current_pos)) != std::string::npos) {
    std::string parent_path = path.substr(0, current_pos);
    int result = ::android::base::utf8::mkdir(parent_path.c_str(), mode);
    if (result < 0 && errno != EEXIST) {
      return false;
    }
    current_pos += 1;
  }
  return ::android::base::utf8::mkdir(path.c_str(), mode) == 0 || errno == EEXIST;
}

StringPiece GetStem(const StringPiece& path) {
  const char* start = path.begin();
  const char* end = path.end();
  for (const char* current = end - 1; current != start - 1; --current) {
    if (*current == sDirSep) {
      return StringPiece(start, current - start);
    }
  }
  return {};
}

StringPiece GetFilename(const StringPiece& path) {
  const char* end = path.end();
  const char* last_dir_sep = path.begin();
  for (const char* c = path.begin(); c != end; ++c) {
    if (*c == sDirSep) {
      last_dir_sep = c + 1;
    }
  }
  return StringPiece(last_dir_sep, end - last_dir_sep);
}

StringPiece GetExtension(const StringPiece& path) {
  StringPiece filename = GetFilename(path);
  const char* const end = filename.end();
  const char* c = std::find(filename.begin(), end, '.');
  if (c != end) {
    return StringPiece(c, end - c);
  }
  return {};
}

void AppendPath(std::string* base, StringPiece part) {
  CHECK(base != nullptr);
  const bool base_has_trailing_sep = (!base->empty() && *(base->end() - 1) == sDirSep);
  const bool part_has_leading_sep = (!part.empty() && *(part.begin()) == sDirSep);
  if (base_has_trailing_sep && part_has_leading_sep) {
    // Remove the part's leading sep
    part = part.substr(1, part.size() - 1);
  } else if (!base_has_trailing_sep && !part_has_leading_sep) {
    // None of the pieces has a separator.
    *base += sDirSep;
  }
  base->append(part.data(), part.size());
}

std::string PackageToPath(const StringPiece& package) {
  std::string out_path;
  for (StringPiece part : util::Tokenize(package, '.')) {
    AppendPath(&out_path, part);
  }
  return out_path;
}

Maybe<FileMap> MmapPath(const std::string& path, std::string* out_error) {
  int flags = O_RDONLY | O_CLOEXEC | O_BINARY;
  unique_fd fd(TEMP_FAILURE_RETRY(::android::base::utf8::open(path.c_str(), flags)));
  if (fd == -1) {
    if (out_error) {
      *out_error = SystemErrorCodeToString(errno);
    }
    return {};
  }

  struct stat filestats = {};
  if (fstat(fd, &filestats) != 0) {
    if (out_error) {
      *out_error = SystemErrorCodeToString(errno);
    }
    return {};
  }

  FileMap filemap;
  if (filestats.st_size == 0) {
    // mmap doesn't like a length of 0. Instead we return an empty FileMap.
    return std::move(filemap);
  }

  if (!filemap.create(path.c_str(), fd, 0, filestats.st_size, true)) {
    if (out_error) {
      *out_error = SystemErrorCodeToString(errno);
    }
    return {};
  }
  return std::move(filemap);
}

bool AppendArgsFromFile(const StringPiece& path, std::vector<std::string>* out_arglist,
                        std::string* out_error) {
  std::string contents;
  if (!ReadFileToString(path.to_string(), &contents, true /*follow_symlinks*/)) {
    if (out_error) {
      *out_error = "failed to read argument-list file";
    }
    return false;
  }

  for (StringPiece line : util::Tokenize(contents, ' ')) {
    line = util::TrimWhitespace(line);
    if (!line.empty()) {
      out_arglist->push_back(line.to_string());
    }
  }
  return true;
}

bool FileFilter::SetPattern(const StringPiece& pattern) {
  pattern_tokens_ = util::SplitAndLowercase(pattern, ':');
  return true;
}

bool FileFilter::operator()(const std::string& filename, FileType type) const {
  if (filename == "." || filename == "..") {
    return false;
  }

  const char kDir[] = "dir";
  const char kFile[] = "file";
  const size_t filename_len = filename.length();
  bool chatty = true;
  for (const std::string& token : pattern_tokens_) {
    const char* token_str = token.c_str();
    if (*token_str == '!') {
      chatty = false;
      token_str++;
    }

    if (strncasecmp(token_str, kDir, sizeof(kDir)) == 0) {
      if (type != FileType::kDirectory) {
        continue;
      }
      token_str += sizeof(kDir);
    }

    if (strncasecmp(token_str, kFile, sizeof(kFile)) == 0) {
      if (type != FileType::kRegular) {
        continue;
      }
      token_str += sizeof(kFile);
    }

    bool ignore = false;
    size_t n = strlen(token_str);
    if (*token_str == '*') {
      // Math suffix.
      token_str++;
      n--;
      if (n <= filename_len) {
        ignore =
            strncasecmp(token_str, filename.c_str() + filename_len - n, n) == 0;
      }
    } else if (n > 1 && token_str[n - 1] == '*') {
      // Match prefix.
      ignore = strncasecmp(token_str, filename.c_str(), n - 1) == 0;
    } else {
      ignore = strcasecmp(token_str, filename.c_str()) == 0;
    }

    if (ignore) {
      if (chatty) {
        diag_->Warn(DiagMessage()
                    << "skipping "
                    << (type == FileType::kDirectory ? "dir '" : "file '")
                    << filename << "' due to ignore pattern '" << token << "'");
      }
      return false;
    }
  }
  return true;
}

Maybe<std::vector<std::string>> FindFiles(const android::StringPiece& path, IDiagnostics* diag,
                                          const FileFilter* filter) {
  const std::string root_dir = path.to_string();
  std::unique_ptr<DIR, decltype(closedir)*> d(opendir(root_dir.data()), closedir);
  if (!d) {
    diag->Error(DiagMessage() << SystemErrorCodeToString(errno));
    return {};
  }

  std::vector<std::string> files;
  std::vector<std::string> subdirs;
  while (struct dirent* entry = readdir(d.get())) {
    if (util::StartsWith(entry->d_name, ".")) {
      continue;
    }

    std::string file_name = entry->d_name;
    std::string full_path = root_dir;
    AppendPath(&full_path, file_name);
    const FileType file_type = GetFileType(full_path);

    if (filter != nullptr) {
      if (!(*filter)(file_name, file_type)) {
        continue;
      }
    }

    if (file_type == file::FileType::kDirectory) {
      subdirs.push_back(std::move(file_name));
    } else {
      files.push_back(std::move(file_name));
    }
  }

  // Now process subdirs.
  for (const std::string& subdir : subdirs) {
    std::string full_subdir = root_dir;
    AppendPath(&full_subdir, subdir);
    Maybe<std::vector<std::string>> subfiles = FindFiles(full_subdir, diag, filter);
    if (!subfiles) {
      return {};
    }

    for (const std::string& subfile : subfiles.value()) {
      std::string new_file = subdir;
      AppendPath(&new_file, subfile);
      files.push_back(new_file);
    }
  }
  return files;
}

}  // namespace file
}  // namespace aapt
