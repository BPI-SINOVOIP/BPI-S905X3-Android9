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

#ifndef SIMPLE_PERF_UTILS_H_
#define SIMPLE_PERF_UTILS_H_

#include <stddef.h>
#include <time.h>

#include <string>
#include <vector>

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <ziparchive/zip_archive.h>

static inline uint64_t Align(uint64_t value, uint64_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

#ifdef _WIN32
#define CLOSE_ON_EXEC_MODE ""
#else
#define CLOSE_ON_EXEC_MODE "e"
#endif

// OneTimeAllocator is used to allocate memory many times and free only once at the end.
// It reduces the cost to free each allocated memory.
class OneTimeFreeAllocator {
 public:
  explicit OneTimeFreeAllocator(size_t unit_size = 8192u)
      : unit_size_(unit_size), cur_(nullptr), end_(nullptr) {
  }

  ~OneTimeFreeAllocator() {
    Clear();
  }

  void Clear();
  const char* AllocateString(const std::string& s);

 private:
  const size_t unit_size_;
  std::vector<char*> v_;
  char* cur_;
  char* end_;
};

class FileHelper {
 public:
  static FileHelper OpenReadOnly(const std::string& filename);
  static FileHelper OpenWriteOnly(const std::string& filename);

  FileHelper(FileHelper&& other) {
    fd_ = other.fd_;
    other.fd_ = -1;
  }

  ~FileHelper();

  explicit operator bool() const {
    return fd_ != -1;
  }

  int fd() const {
    return fd_;
  }

 private:
  explicit FileHelper(int fd) : fd_(fd) {}
  int fd_;

  DISALLOW_COPY_AND_ASSIGN(FileHelper);
};

class ArchiveHelper {
 public:
  ArchiveHelper(int fd, const std::string& debug_filename);
  ~ArchiveHelper();

  explicit operator bool() const {
    return valid_;
  }
  ZipArchiveHandle &archive_handle() {
    return handle_;
  }

 private:
  ZipArchiveHandle handle_;
  bool valid_;

  DISALLOW_COPY_AND_ASSIGN(ArchiveHelper);
};

template <class T>
void MoveFromBinaryFormat(T& data, const char*& p) {
  static_assert(std::is_standard_layout<T>::value, "not standard layout");
  memcpy(&data, p, sizeof(T));
  p += sizeof(T);
}

template <class T>
void MoveFromBinaryFormat(T& data, char*& p) {
  static_assert(std::is_standard_layout<T>::value, "not standard layout");
  memcpy(&data, p, sizeof(T));
  p += sizeof(T);
}

template <class T>
void MoveFromBinaryFormat(T* data_p, size_t n, const char*& p) {
  static_assert(std::is_standard_layout<T>::value, "not standard layout");
  size_t size = n * sizeof(T);
  memcpy(data_p, p, size);
  p += size;
}

template <class T>
void MoveToBinaryFormat(const T& data, char*& p) {
  static_assert(std::is_standard_layout<T>::value, "not standard layout");
  memcpy(p, &data, sizeof(T));
  p += sizeof(T);
}

template <class T>
void MoveToBinaryFormat(const T* data_p, size_t n, char*& p) {
  static_assert(std::is_standard_layout<T>::value, "not standard layout");
  size_t size = n * sizeof(T);
  memcpy(p, data_p, size);
  p += size;
}

void PrintIndented(size_t indent, const char* fmt, ...);
void FprintIndented(FILE* fp, size_t indent, const char* fmt, ...);

bool IsPowerOfTwo(uint64_t value);

std::vector<std::string> GetEntriesInDir(const std::string& dirpath);
std::vector<std::string> GetSubDirs(const std::string& dirpath);
bool IsDir(const std::string& dirpath);
bool IsRegularFile(const std::string& filename);
uint64_t GetFileSize(const std::string& filename);
bool MkdirWithParents(const std::string& path);

bool XzDecompress(const std::string& compressed_data, std::string* decompressed_data);

bool GetLogSeverity(const std::string& name, android::base::LogSeverity* severity);
std::string GetLogSeverityName();

bool IsRoot();

struct KernelSymbol {
  uint64_t addr;
  char type;
  const char* name;
  const char* module;  // If nullptr, the symbol is not in a kernel module.
};

bool ProcessKernelSymbols(std::string& symbol_data,
                          const std::function<bool(const KernelSymbol&)>& callback);

size_t GetPageSize();

uint64_t ConvertBytesToValue(const char* bytes, uint32_t size);

timeval SecondToTimeval(double time_in_sec);

std::string GetSimpleperfVersion();

#endif  // SIMPLE_PERF_UTILS_H_
