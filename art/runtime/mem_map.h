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

#ifndef ART_RUNTIME_MEM_MAP_H_
#define ART_RUNTIME_MEM_MAP_H_

#include <stddef.h>
#include <sys/types.h>

#include <map>
#include <mutex>
#include <string>

#include "android-base/thread_annotations.h"

namespace art {

#if defined(__LP64__) && (defined(__aarch64__) || defined(__mips__) || defined(__APPLE__))
#define USE_ART_LOW_4G_ALLOCATOR 1
#else
#if defined(__LP64__) && !defined(__x86_64__)
#error "Unrecognized 64-bit architecture."
#endif
#define USE_ART_LOW_4G_ALLOCATOR 0
#endif

#ifdef __linux__
static constexpr bool kMadviseZeroes = true;
#define HAVE_MREMAP_SYSCALL true
#else
static constexpr bool kMadviseZeroes = false;
// We cannot ever perform MemMap::ReplaceWith on non-linux hosts since the syscall is not
// present.
#define HAVE_MREMAP_SYSCALL false
#endif

// Used to keep track of mmap segments.
//
// On 64b systems not supporting MAP_32BIT, the implementation of MemMap will do a linear scan
// for free pages. For security, the start of this scan should be randomized. This requires a
// dynamic initializer.
// For this to work, it is paramount that there are no other static initializers that access MemMap.
// Otherwise, calls might see uninitialized values.
class MemMap {
 public:
  static constexpr bool kCanReplaceMapping = HAVE_MREMAP_SYSCALL;

  // Replace the data in this memmmap with the data in the memmap pointed to by source. The caller
  // relinquishes ownership of the source mmap.
  //
  // For the call to be successful:
  //   * The range [dest->Begin, dest->Begin() + source->Size()] must not overlap with
  //     [source->Begin(), source->End()].
  //   * Neither source nor dest may be 'reused' mappings (they must own all the pages associated
  //     with them.
  //   * kCanReplaceMapping must be true.
  //   * Neither source nor dest may use manual redzones.
  //   * Both source and dest must have the same offset from the nearest page boundary.
  //   * mremap must succeed when called on the mappings.
  //
  // If this call succeeds it will return true and:
  //   * Deallocate *source
  //   * Sets *source to nullptr
  //   * The protection of this will remain the same.
  //   * The size of this will be the size of the source
  //   * The data in this will be the data from source.
  //
  // If this call fails it will return false and make no changes to *source or this. The ownership
  // of the source mmap is returned to the caller.
  bool ReplaceWith(/*in-out*/MemMap** source, /*out*/std::string* error);

  // Request an anonymous region of length 'byte_count' and a requested base address.
  // Use null as the requested base address if you don't care.
  // "reuse" allows re-mapping an address range from an existing mapping.
  //
  // The word "anonymous" in this context means "not backed by a file". The supplied
  // 'name' will be used -- on systems that support it -- to give the mapping
  // a name.
  //
  // On success, returns returns a MemMap instance.  On failure, returns null.
  static MemMap* MapAnonymous(const char* name,
                              uint8_t* addr,
                              size_t byte_count,
                              int prot,
                              bool low_4gb,
                              bool reuse,
                              std::string* error_msg,
                              bool use_ashmem = true);

  // Create placeholder for a region allocated by direct call to mmap.
  // This is useful when we do not have control over the code calling mmap,
  // but when we still want to keep track of it in the list.
  // The region is not considered to be owned and will not be unmmaped.
  static MemMap* MapDummy(const char* name, uint8_t* addr, size_t byte_count);

  // Map part of a file, taking care of non-page aligned offsets.  The
  // "start" offset is absolute, not relative.
  //
  // On success, returns returns a MemMap instance.  On failure, returns null.
  static MemMap* MapFile(size_t byte_count,
                         int prot,
                         int flags,
                         int fd,
                         off_t start,
                         bool low_4gb,
                         const char* filename,
                         std::string* error_msg) {
    return MapFileAtAddress(nullptr,
                            byte_count,
                            prot,
                            flags,
                            fd,
                            start,
                            /*low_4gb*/low_4gb,
                            /*reuse*/false,
                            filename,
                            error_msg);
  }

  // Map part of a file, taking care of non-page aligned offsets.  The "start" offset is absolute,
  // not relative. This version allows requesting a specific address for the base of the mapping.
  // "reuse" allows us to create a view into an existing mapping where we do not take ownership of
  // the memory. If error_msg is null then we do not print /proc/maps to the log if
  // MapFileAtAddress fails. This helps improve performance of the fail case since reading and
  // printing /proc/maps takes several milliseconds in the worst case.
  //
  // On success, returns returns a MemMap instance.  On failure, returns null.
  static MemMap* MapFileAtAddress(uint8_t* addr,
                                  size_t byte_count,
                                  int prot,
                                  int flags,
                                  int fd,
                                  off_t start,
                                  bool low_4gb,
                                  bool reuse,
                                  const char* filename,
                                  std::string* error_msg);

  // Releases the memory mapping.
  ~MemMap() REQUIRES(!MemMap::mem_maps_lock_);

  const std::string& GetName() const {
    return name_;
  }

  bool Sync();

  bool Protect(int prot);

  void MadviseDontNeedAndZero();

  int GetProtect() const {
    return prot_;
  }

  uint8_t* Begin() const {
    return begin_;
  }

  size_t Size() const {
    return size_;
  }

  // Resize the mem-map by unmapping pages at the end. Currently only supports shrinking.
  void SetSize(size_t new_size);

  uint8_t* End() const {
    return Begin() + Size();
  }

  void* BaseBegin() const {
    return base_begin_;
  }

  size_t BaseSize() const {
    return base_size_;
  }

  void* BaseEnd() const {
    return reinterpret_cast<uint8_t*>(BaseBegin()) + BaseSize();
  }

  bool HasAddress(const void* addr) const {
    return Begin() <= addr && addr < End();
  }

  // Unmap the pages at end and remap them to create another memory map.
  MemMap* RemapAtEnd(uint8_t* new_end,
                     const char* tail_name,
                     int tail_prot,
                     std::string* error_msg,
                     bool use_ashmem = true);

  static bool CheckNoGaps(MemMap* begin_map, MemMap* end_map)
      REQUIRES(!MemMap::mem_maps_lock_);
  static void DumpMaps(std::ostream& os, bool terse = false)
      REQUIRES(!MemMap::mem_maps_lock_);

  // Init and Shutdown are NOT thread safe.
  // Both may be called multiple times and MemMap objects may be created any
  // time after the first call to Init and before the first call to Shutodwn.
  static void Init() REQUIRES(!MemMap::mem_maps_lock_);
  static void Shutdown() REQUIRES(!MemMap::mem_maps_lock_);

  // If the map is PROT_READ, try to read each page of the map to check it is in fact readable (not
  // faulting). This is used to diagnose a bug b/19894268 where mprotect doesn't seem to be working
  // intermittently.
  void TryReadable();

  // Align the map by unmapping the unaligned parts at the lower and the higher ends.
  void AlignBy(size_t size);

  // For annotation reasons.
  static std::mutex* GetMemMapsLock() RETURN_CAPABILITY(mem_maps_lock_) {
    return nullptr;
  }

 private:
  MemMap(const std::string& name,
         uint8_t* begin,
         size_t size,
         void* base_begin,
         size_t base_size,
         int prot,
         bool reuse,
         size_t redzone_size = 0) REQUIRES(!MemMap::mem_maps_lock_);

  static void DumpMapsLocked(std::ostream& os, bool terse)
      REQUIRES(MemMap::mem_maps_lock_);
  static bool HasMemMap(MemMap* map)
      REQUIRES(MemMap::mem_maps_lock_);
  static MemMap* GetLargestMemMapAt(void* address)
      REQUIRES(MemMap::mem_maps_lock_);
  static bool ContainedWithinExistingMap(uint8_t* ptr, size_t size, std::string* error_msg)
      REQUIRES(!MemMap::mem_maps_lock_);

  // Internal version of mmap that supports low 4gb emulation.
  static void* MapInternal(void* addr,
                           size_t length,
                           int prot,
                           int flags,
                           int fd,
                           off_t offset,
                           bool low_4gb)
      REQUIRES(!MemMap::mem_maps_lock_);
  static void* MapInternalArtLow4GBAllocator(size_t length,
                                             int prot,
                                             int flags,
                                             int fd,
                                             off_t offset)
      REQUIRES(!MemMap::mem_maps_lock_);

  const std::string name_;
  uint8_t* begin_;  // Start of data. May be changed by AlignBy.
  size_t size_;  // Length of data.

  void* base_begin_;  // Page-aligned base address. May be changed by AlignBy.
  size_t base_size_;  // Length of mapping. May be changed by RemapAtEnd (ie Zygote).
  int prot_;  // Protection of the map.

  // When reuse_ is true, this is just a view of an existing mapping
  // and we do not take ownership and are not responsible for
  // unmapping.
  const bool reuse_;

  // When already_unmapped_ is true the destructor will not call munmap.
  bool already_unmapped_;

  const size_t redzone_size_;

#if USE_ART_LOW_4G_ALLOCATOR
  static uintptr_t next_mem_pos_;   // Next memory location to check for low_4g extent.
#endif

  static std::mutex* mem_maps_lock_;

  friend class MemMapTest;  // To allow access to base_begin_ and base_size_.
};

std::ostream& operator<<(std::ostream& os, const MemMap& mem_map);

// Zero and release pages if possible, no requirements on alignments.
void ZeroAndReleasePages(void* address, size_t length);

}  // namespace art

#endif  // ART_RUNTIME_MEM_MAP_H_
