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

#ifndef ART_RUNTIME_JIT_PROFILE_COMPILATION_INFO_H_
#define ART_RUNTIME_JIT_PROFILE_COMPILATION_INFO_H_

#include <set>
#include <vector>

#include "base/arena_containers.h"
#include "base/arena_object.h"
#include "base/atomic.h"
#include "base/safe_map.h"
#include "bit_memory_region.h"
#include "dex/dex_cache_resolved_classes.h"
#include "dex/dex_file.h"
#include "dex/dex_file_types.h"
#include "dex/method_reference.h"
#include "dex/type_reference.h"
#include "mem_map.h"

namespace art {

/**
 *  Convenient class to pass around profile information (including inline caches)
 *  without the need to hold GC-able objects.
 */
struct ProfileMethodInfo {
  struct ProfileInlineCache {
    ProfileInlineCache(uint32_t pc,
                       bool missing_types,
                       const std::vector<TypeReference>& profile_classes)
        : dex_pc(pc), is_missing_types(missing_types), classes(profile_classes) {}

    const uint32_t dex_pc;
    const bool is_missing_types;
    const std::vector<TypeReference> classes;
  };

  explicit ProfileMethodInfo(MethodReference reference) : ref(reference) {}

  ProfileMethodInfo(MethodReference reference, const std::vector<ProfileInlineCache>& caches)
      : ref(reference),
        inline_caches(caches) {}

  MethodReference ref;
  std::vector<ProfileInlineCache> inline_caches;
};

/**
 * Profile information in a format suitable to be queried by the compiler and
 * performing profile guided compilation.
 * It is a serialize-friendly format based on information collected by the
 * interpreter (ProfileInfo).
 * Currently it stores only the hot compiled methods.
 */
class ProfileCompilationInfo {
 public:
  static const uint8_t kProfileMagic[];
  static const uint8_t kProfileVersion[];

  static const char* kDexMetadataProfileEntry;

  // Data structures for encoding the offline representation of inline caches.
  // This is exposed as public in order to make it available to dex2oat compilations
  // (see compiler/optimizing/inliner.cc).

  // A dex location together with its checksum.
  struct DexReference {
    DexReference() : dex_checksum(0), num_method_ids(0) {}

    DexReference(const std::string& location, uint32_t checksum, uint32_t num_methods)
        : dex_location(location), dex_checksum(checksum), num_method_ids(num_methods) {}

    bool operator==(const DexReference& other) const {
      return dex_checksum == other.dex_checksum &&
          dex_location == other.dex_location &&
          num_method_ids == other.num_method_ids;
    }

    bool MatchesDex(const DexFile* dex_file) const {
      return dex_checksum == dex_file->GetLocationChecksum() &&
           dex_location == GetProfileDexFileKey(dex_file->GetLocation());
    }

    std::string dex_location;
    uint32_t dex_checksum;
    uint32_t num_method_ids;
  };

  // Encodes a class reference in the profile.
  // The owning dex file is encoded as the index (dex_profile_index) it has in the
  // profile rather than as a full DexRefence(location,checksum).
  // This avoids excessive string copying when managing the profile data.
  // The dex_profile_index is an index in either of:
  //  - OfflineProfileMethodInfo#dex_references vector (public use)
  //  - DexFileData#profile_index (internal use).
  // Note that the dex_profile_index is not necessary the multidex index.
  // We cannot rely on the actual multidex index because a single profile may store
  // data from multiple splits. This means that a profile may contain a classes2.dex from split-A
  // and one from split-B.
  struct ClassReference : public ValueObject {
    ClassReference(uint8_t dex_profile_idx, const dex::TypeIndex type_idx) :
      dex_profile_index(dex_profile_idx), type_index(type_idx) {}

    bool operator==(const ClassReference& other) const {
      return dex_profile_index == other.dex_profile_index && type_index == other.type_index;
    }
    bool operator<(const ClassReference& other) const {
      return dex_profile_index == other.dex_profile_index
          ? type_index < other.type_index
          : dex_profile_index < other.dex_profile_index;
    }

    uint8_t dex_profile_index;  // the index of the owning dex in the profile info
    dex::TypeIndex type_index;  // the type index of the class
  };

  // The set of classes that can be found at a given dex pc.
  using ClassSet = ArenaSet<ClassReference>;

  // Encodes the actual inline cache for a given dex pc (whether or not the receiver is
  // megamorphic and its possible types).
  // If the receiver is megamorphic or is missing types the set of classes will be empty.
  struct DexPcData : public ArenaObject<kArenaAllocProfile> {
    explicit DexPcData(ArenaAllocator* allocator)
        : is_missing_types(false),
          is_megamorphic(false),
          classes(std::less<ClassReference>(), allocator->Adapter(kArenaAllocProfile)) {}
    void AddClass(uint16_t dex_profile_idx, const dex::TypeIndex& type_idx);
    void SetIsMegamorphic() {
      if (is_missing_types) return;
      is_megamorphic = true;
      classes.clear();
    }
    void SetIsMissingTypes() {
      is_megamorphic = false;
      is_missing_types = true;
      classes.clear();
    }
    bool operator==(const DexPcData& other) const {
      return is_megamorphic == other.is_megamorphic &&
          is_missing_types == other.is_missing_types &&
          classes == other.classes;
    }

    // Not all runtime types can be encoded in the profile. For example if the receiver
    // type is in a dex file which is not tracked for profiling its type cannot be
    // encoded. When types are missing this field will be set to true.
    bool is_missing_types;
    bool is_megamorphic;
    ClassSet classes;
  };

  // The inline cache map: DexPc -> DexPcData.
  using InlineCacheMap = ArenaSafeMap<uint16_t, DexPcData>;

  // Maps a method dex index to its inline cache.
  using MethodMap = ArenaSafeMap<uint16_t, InlineCacheMap>;

  // Profile method hotness information for a single method. Also includes a pointer to the inline
  // cache map.
  class MethodHotness {
   public:
    enum Flag {
      kFlagHot = 0x1,
      kFlagStartup = 0x2,
      kFlagPostStartup = 0x4,
    };

    bool IsHot() const {
      return (flags_ & kFlagHot) != 0;
    }

    bool IsStartup() const {
      return (flags_ & kFlagStartup) != 0;
    }

    bool IsPostStartup() const {
      return (flags_ & kFlagPostStartup) != 0;
    }

    void AddFlag(Flag flag) {
      flags_ |= flag;
    }

    uint8_t GetFlags() const {
      return flags_;
    }

    bool IsInProfile() const {
      return flags_ != 0;
    }

   private:
    const InlineCacheMap* inline_cache_map_ = nullptr;
    uint8_t flags_ = 0;

    const InlineCacheMap* GetInlineCacheMap() const {
      return inline_cache_map_;
    }

    void SetInlineCacheMap(const InlineCacheMap* info) {
      inline_cache_map_ = info;
    }

    friend class ProfileCompilationInfo;
  };

  // Encodes the full set of inline caches for a given method.
  // The dex_references vector is indexed according to the ClassReference::dex_profile_index.
  // i.e. the dex file of any ClassReference present in the inline caches can be found at
  // dex_references[ClassReference::dex_profile_index].
  struct OfflineProfileMethodInfo {
    explicit OfflineProfileMethodInfo(const InlineCacheMap* inline_cache_map)
        : inline_caches(inline_cache_map) {}

    bool operator==(const OfflineProfileMethodInfo& other) const;

    const InlineCacheMap* const inline_caches;
    std::vector<DexReference> dex_references;
  };

  // Public methods to create, extend or query the profile.
  ProfileCompilationInfo();
  explicit ProfileCompilationInfo(ArenaPool* arena_pool);

  ~ProfileCompilationInfo();

  // Add the given methods to the current profile object.
  bool AddMethods(const std::vector<ProfileMethodInfo>& methods, MethodHotness::Flag flags);

  // Add the given classes to the current profile object.
  bool AddClasses(const std::set<DexCacheResolvedClasses>& resolved_classes);

  // Add multiple type ids for classes in a single dex file. Iterator is for type_ids not
  // class_defs.
  template <class Iterator>
  bool AddClassesForDex(const DexFile* dex_file, Iterator index_begin, Iterator index_end) {
    DexFileData* data = GetOrAddDexFileData(dex_file);
    if (data == nullptr) {
      return false;
    }
    data->class_set.insert(index_begin, index_end);
    return true;
  }
  // Add a single type id for a dex file.
  bool AddClassForDex(const TypeReference& ref) {
    DexFileData* data = GetOrAddDexFileData(ref.dex_file);
    if (data == nullptr) {
      return false;
    }
    data->class_set.insert(ref.TypeIndex());
    return true;
  }


  // Add a method index to the profile (without inline caches). The method flags determine if it is
  // hot, startup, or post startup, or a combination of the previous.
  bool AddMethodIndex(MethodHotness::Flag flags,
                      const std::string& dex_location,
                      uint32_t checksum,
                      uint16_t method_idx,
                      uint32_t num_method_ids);
  bool AddMethodIndex(MethodHotness::Flag flags, const MethodReference& ref);

  // Add a method to the profile using its online representation (containing runtime structures).
  bool AddMethod(const ProfileMethodInfo& pmi, MethodHotness::Flag flags);

  // Bulk add sampled methods and/or hot methods for a single dex, fast since it only has one
  // GetOrAddDexFileData call.
  template <class Iterator>
  bool AddMethodsForDex(MethodHotness::Flag flags,
                        const DexFile* dex_file,
                        Iterator index_begin,
                        Iterator index_end) {
    DexFileData* data = GetOrAddDexFileData(dex_file);
    if (data == nullptr) {
      return false;
    }
    for (Iterator it = index_begin; it != index_end; ++it) {
      DCHECK_LT(*it, data->num_method_ids);
      if (!data->AddMethod(flags, *it)) {
        return false;
      }
    }
    return true;
  }

  // Add hotness flags for a simple method.
  bool AddMethodHotness(const MethodReference& method_ref, const MethodHotness& hotness);

  // Load or Merge profile information from the given file descriptor.
  // If the current profile is non-empty the load will fail.
  // If merge_classes is set to false, classes will not be merged/loaded.
  // If filter_fn is present, it will be used to filter out profile data belonging
  // to dex file which do not comply with the filter
  // (i.e. for which filter_fn(dex_location, dex_checksum) is false).
  using ProfileLoadFilterFn = std::function<bool(const std::string&, uint32_t)>;
  // Profile filter method which accepts all dex locations.
  // This is convenient to use when we need to accept all locations without repeating the same
  // lambda.
  static bool ProfileFilterFnAcceptAll(const std::string& dex_location, uint32_t checksum);

  bool Load(
      int fd,
      bool merge_classes = true,
      const ProfileLoadFilterFn& filter_fn = ProfileFilterFnAcceptAll);

  // Verify integrity of the profile file with the provided dex files.
  // If there exists a DexData object which maps to a dex_file, then it verifies that:
  // - The checksums of the DexData and dex_file are equals.
  // - No method id exceeds NumMethodIds corresponding to the dex_file.
  // - No class id exceeds NumTypeIds corresponding to the dex_file.
  // - For every inline_caches, class_ids does not exceed NumTypeIds corresponding to
  //   the dex_file they are in.
  bool VerifyProfileData(const std::vector<const DexFile *> &dex_files);

  // Load profile information from the given file
  // If the current profile is non-empty the load will fail.
  // If clear_if_invalid is true and the file is invalid the method clears the
  // the file and returns true.
  bool Load(const std::string& filename, bool clear_if_invalid);

  // Merge the data from another ProfileCompilationInfo into the current object. Only merges
  // classes if merge_classes is true. This is used for creating the boot profile since
  // we don't want all of the classes to be image classes.
  bool MergeWith(const ProfileCompilationInfo& info, bool merge_classes = true);

  // Merge profile information from the given file descriptor.
  bool MergeWith(const std::string& filename);

  // Save the profile data to the given file descriptor.
  bool Save(int fd);

  // Save the current profile into the given file. The file will be cleared before saving.
  bool Save(const std::string& filename, uint64_t* bytes_written);

  // Return the number of methods that were profiled.
  uint32_t GetNumberOfMethods() const;

  // Return the number of resolved classes that were profiled.
  uint32_t GetNumberOfResolvedClasses() const;

  // Returns the profile method info for a given method reference.
  MethodHotness GetMethodHotness(const MethodReference& method_ref) const;
  MethodHotness GetMethodHotness(const std::string& dex_location,
                                 uint32_t dex_checksum,
                                 uint16_t dex_method_index) const;

  // Return true if the class's type is present in the profiling info.
  bool ContainsClass(const DexFile& dex_file, dex::TypeIndex type_idx) const;

  // Return the method data for the given location and index from the profiling info.
  // If the method index is not found or the checksum doesn't match, null is returned.
  // Note: the inline cache map is a pointer to the map stored in the profile and
  // its allocation will go away if the profile goes out of scope.
  std::unique_ptr<OfflineProfileMethodInfo> GetMethod(const std::string& dex_location,
                                                      uint32_t dex_checksum,
                                                      uint16_t dex_method_index) const;

  // Dump all the loaded profile info into a string and returns it.
  // If dex_files is not null then the method indices will be resolved to their
  // names.
  // This is intended for testing and debugging.
  std::string DumpInfo(const std::vector<std::unique_ptr<const DexFile>>* dex_files,
                       bool print_full_dex_location = true) const;
  std::string DumpInfo(const std::vector<const DexFile*>* dex_files,
                       bool print_full_dex_location = true) const;

  // Return the classes and methods for a given dex file through out args. The out args are the set
  // of class as well as the methods and their associated inline caches. Returns true if the dex
  // file is register and has a matching checksum, false otherwise.
  bool GetClassesAndMethods(const DexFile& dex_file,
                            /*out*/std::set<dex::TypeIndex>* class_set,
                            /*out*/std::set<uint16_t>* hot_method_set,
                            /*out*/std::set<uint16_t>* startup_method_set,
                            /*out*/std::set<uint16_t>* post_startup_method_method_set) const;

  // Perform an equality test with the `other` profile information.
  bool Equals(const ProfileCompilationInfo& other);

  // Return the class descriptors for all of the classes in the profiles' class sets.
  std::set<DexCacheResolvedClasses> GetResolvedClasses(
      const std::vector<const DexFile*>& dex_files_) const;

  // Return the profile key associated with the given dex location.
  static std::string GetProfileDexFileKey(const std::string& dex_location);

  // Generate a test profile which will contain a percentage of the total maximum
  // number of methods and classes (method_ratio and class_ratio).
  static bool GenerateTestProfile(int fd,
                                  uint16_t number_of_dex_files,
                                  uint16_t method_ratio,
                                  uint16_t class_ratio,
                                  uint32_t random_seed);

  // Generate a test profile which will randomly contain classes and methods from
  // the provided list of dex files.
  static bool GenerateTestProfile(int fd,
                                  std::vector<std::unique_ptr<const DexFile>>& dex_files,
                                  uint16_t method_percentage,
                                  uint16_t class_percentage,
                                  uint32_t random_seed);

  // Check that the given profile method info contain the same data.
  static bool Equals(const ProfileCompilationInfo::OfflineProfileMethodInfo& pmi1,
                     const ProfileCompilationInfo::OfflineProfileMethodInfo& pmi2);

  ArenaAllocator* GetAllocator() { return &allocator_; }

  // Return all of the class descriptors in the profile for a set of dex files.
  std::unordered_set<std::string> GetClassDescriptors(const std::vector<const DexFile*>& dex_files);

  // Return true if the fd points to a profile file.
  bool IsProfileFile(int fd);

  // Update the profile keys corresponding to the given dex files based on their current paths.
  // This method allows fix-ups in the profile for dex files that might have been renamed.
  // The new profile key will be constructed based on the current dex location.
  //
  // The matching [profile key <-> dex_file] is done based on the dex checksum and the number of
  // methods ids. If neither is a match then the profile key is not updated.
  //
  // If the new profile key would collide with an existing key (for a different dex)
  // the method returns false. Otherwise it returns true.
  bool UpdateProfileKeys(const std::vector<std::unique_ptr<const DexFile>>& dex_files);

  // Checks if the profile is empty.
  bool IsEmpty() const;

  // Clears all the data from the profile.
  void ClearData();

 private:
  enum ProfileLoadStatus {
    kProfileLoadWouldOverwiteData,
    kProfileLoadIOError,
    kProfileLoadVersionMismatch,
    kProfileLoadBadData,
    kProfileLoadSuccess
  };

  const uint32_t kProfileSizeWarningThresholdInBytes = 500000U;
  const uint32_t kProfileSizeErrorThresholdInBytes = 1000000U;

  // Internal representation of the profile information belonging to a dex file.
  // Note that we could do without profile_key (the key used to encode the dex
  // file in the profile) and profile_index (the index of the dex file in the
  // profile) fields in this struct because we can infer them from
  // profile_key_map_ and info_. However, it makes the profiles logic much
  // simpler if we have references here as well.
  struct DexFileData : public DeletableArenaObject<kArenaAllocProfile> {
    DexFileData(ArenaAllocator* allocator,
                const std::string& key,
                uint32_t location_checksum,
                uint16_t index,
                uint32_t num_methods)
        : allocator_(allocator),
          profile_key(key),
          profile_index(index),
          checksum(location_checksum),
          method_map(std::less<uint16_t>(), allocator->Adapter(kArenaAllocProfile)),
          class_set(std::less<dex::TypeIndex>(), allocator->Adapter(kArenaAllocProfile)),
          num_method_ids(num_methods),
          bitmap_storage(allocator->Adapter(kArenaAllocProfile)) {
      bitmap_storage.resize(ComputeBitmapStorage(num_method_ids));
      if (!bitmap_storage.empty()) {
        method_bitmap =
            BitMemoryRegion(MemoryRegion(
                &bitmap_storage[0], bitmap_storage.size()), 0, ComputeBitmapBits(num_method_ids));
      }
    }

    static size_t ComputeBitmapBits(uint32_t num_method_ids) {
      return num_method_ids * kBitmapIndexCount;
    }
    static size_t ComputeBitmapStorage(uint32_t num_method_ids) {
      return RoundUp(ComputeBitmapBits(num_method_ids), kBitsPerByte) / kBitsPerByte;
    }

    bool operator==(const DexFileData& other) const {
      return checksum == other.checksum && method_map == other.method_map;
    }

    // Mark a method as executed at least once.
    bool AddMethod(MethodHotness::Flag flags, size_t index);

    void MergeBitmap(const DexFileData& other) {
      DCHECK_EQ(bitmap_storage.size(), other.bitmap_storage.size());
      for (size_t i = 0; i < bitmap_storage.size(); ++i) {
        bitmap_storage[i] |= other.bitmap_storage[i];
      }
    }

    void SetMethodHotness(size_t index, MethodHotness::Flag flags);
    MethodHotness GetHotnessInfo(uint32_t dex_method_index) const;

    // The allocator used to allocate new inline cache maps.
    ArenaAllocator* const allocator_;
    // The profile key this data belongs to.
    std::string profile_key;
    // The profile index of this dex file (matches ClassReference#dex_profile_index).
    uint8_t profile_index;
    // The dex checksum.
    uint32_t checksum;
    // The methonds' profile information.
    MethodMap method_map;
    // The classes which have been profiled. Note that these don't necessarily include
    // all the classes that can be found in the inline caches reference.
    ArenaSet<dex::TypeIndex> class_set;
    // Find the inline caches of the the given method index. Add an empty entry if
    // no previous data is found.
    InlineCacheMap* FindOrAddMethod(uint16_t method_index);
    // Num method ids.
    uint32_t num_method_ids;
    ArenaVector<uint8_t> bitmap_storage;
    BitMemoryRegion method_bitmap;

   private:
    enum BitmapIndex {
      kBitmapIndexStartup,
      kBitmapIndexPostStartup,
      kBitmapIndexCount,
    };

    size_t MethodBitIndex(bool startup, size_t index) const {
      DCHECK_LT(index, num_method_ids);
      // The format is [startup bitmap][post startup bitmap]
      // This compresses better than ([startup bit][post statup bit])*

      return index + (startup
          ? kBitmapIndexStartup * num_method_ids
          : kBitmapIndexPostStartup * num_method_ids);
    }
  };

  // Return the profile data for the given profile key or null if the dex location
  // already exists but has a different checksum
  DexFileData* GetOrAddDexFileData(const std::string& profile_key,
                                   uint32_t checksum,
                                   uint32_t num_method_ids);

  DexFileData* GetOrAddDexFileData(const DexFile* dex_file) {
    return GetOrAddDexFileData(GetProfileDexFileKey(dex_file->GetLocation()),
                               dex_file->GetLocationChecksum(),
                               dex_file->NumMethodIds());
  }

  // Add a method to the profile using its offline representation.
  // This is mostly used to facilitate testing.
  bool AddMethod(const std::string& dex_location,
                 uint32_t dex_checksum,
                 uint16_t method_index,
                 uint32_t num_method_ids,
                 const OfflineProfileMethodInfo& pmi,
                 MethodHotness::Flag flags);

  // Add a class index to the profile.
  bool AddClassIndex(const std::string& dex_location,
                     uint32_t checksum,
                     dex::TypeIndex type_idx,
                     uint32_t num_method_ids);

  // Add all classes from the given dex cache to the the profile.
  bool AddResolvedClasses(const DexCacheResolvedClasses& classes);

  // Encode the known dex_files into a vector. The index of a dex_reference will
  // be the same as the profile index of the dex file (used to encode the ClassReferences).
  void DexFileToProfileIndex(/*out*/std::vector<DexReference>* dex_references) const;

  // Return the dex data associated with the given profile key or null if the profile
  // doesn't contain the key.
  const DexFileData* FindDexData(const std::string& profile_key,
                                 uint32_t checksum,
                                 bool verify_checksum = true) const;

  // Return the dex data associated with the given dex file or null if the profile doesn't contain
  // the key or the checksum mismatches.
  const DexFileData* FindDexData(const DexFile* dex_file) const;

  // Inflate the input buffer (in_buffer) of size in_size. It returns a buffer of
  // compressed data for the input buffer of "compressed_data_size" size.
  std::unique_ptr<uint8_t[]> DeflateBuffer(const uint8_t* in_buffer,
                                           uint32_t in_size,
                                           /*out*/uint32_t* compressed_data_size);

  // Inflate the input buffer(in_buffer) of size in_size. out_size is the expected output
  // size of the buffer. It puts the output in out_buffer. It returns Z_STREAM_END on
  // success. On error, it returns Z_STREAM_ERROR if the compressed data is inconsistent
  // and Z_DATA_ERROR if the stream ended prematurely or the stream has extra data.
  int InflateBuffer(const uint8_t* in_buffer,
                    uint32_t in_size,
                    uint32_t out_size,
                    /*out*/uint8_t* out_buffer);

  // Parsing functionality.

  // The information present in the header of each profile line.
  struct ProfileLineHeader {
    std::string dex_location;
    uint16_t class_set_size;
    uint32_t method_region_size_bytes;
    uint32_t checksum;
    uint32_t num_method_ids;
  };

  /**
   * Encapsulate the source of profile data for loading.
   * The source can be either a plain file or a zip file.
   * For zip files, the profile entry will be extracted to
   * the memory map.
   */
  class ProfileSource {
   public:
    /**
     * Create a profile source for the given fd. The ownership of the fd
     * remains to the caller; as this class will not attempt to close it at any
     * point.
     */
    static ProfileSource* Create(int32_t fd) {
      DCHECK_GT(fd, -1);
      return new ProfileSource(fd, /*map*/ nullptr);
    }

    /**
     * Create a profile source backed by a memory map. The map can be null in
     * which case it will the treated as an empty source.
     */
    static ProfileSource* Create(std::unique_ptr<MemMap>&& mem_map) {
      return new ProfileSource(/*fd*/ -1, std::move(mem_map));
    }

    /**
     * Read bytes from this source.
     * Reading will advance the current source position so subsequent
     * invocations will read from the las position.
     */
    ProfileLoadStatus Read(uint8_t* buffer,
                           size_t byte_count,
                           const std::string& debug_stage,
                           std::string* error);

    /** Return true if the source has 0 data. */
    bool HasEmptyContent() const;
    /** Return true if all the information from this source has been read. */
    bool HasConsumedAllData() const;

   private:
    ProfileSource(int32_t fd, std::unique_ptr<MemMap>&& mem_map)
        : fd_(fd), mem_map_(std::move(mem_map)), mem_map_cur_(0) {}

    bool IsMemMap() const { return fd_ == -1; }

    int32_t fd_;  // The fd is not owned by this class.
    std::unique_ptr<MemMap> mem_map_;
    size_t mem_map_cur_;  // Current position in the map to read from.
  };

  // A helper structure to make sure we don't read past our buffers in the loops.
  struct SafeBuffer {
   public:
    explicit SafeBuffer(size_t size) : storage_(new uint8_t[size]) {
      ptr_current_ = storage_.get();
      ptr_end_ = ptr_current_ + size;
    }

    // Reads the content of the descriptor at the current position.
    ProfileLoadStatus Fill(ProfileSource& source,
                           const std::string& debug_stage,
                           /*out*/std::string* error);

    // Reads an uint value (high bits to low bits) and advances the current pointer
    // with the number of bits read.
    template <typename T> bool ReadUintAndAdvance(/*out*/ T* value);

    // Compares the given data with the content current pointer. If the contents are
    // equal it advances the current pointer by data_size.
    bool CompareAndAdvance(const uint8_t* data, size_t data_size);

    // Advances current pointer by data_size.
    void Advance(size_t data_size);

    // Returns the count of unread bytes.
    size_t CountUnreadBytes();

    // Returns the current pointer.
    const uint8_t* GetCurrentPtr();

    // Get the underlying raw buffer.
    uint8_t* Get() { return storage_.get(); }

   private:
    std::unique_ptr<uint8_t[]> storage_;
    uint8_t* ptr_end_;
    uint8_t* ptr_current_;
  };

  ProfileLoadStatus OpenSource(int32_t fd,
                               /*out*/ std::unique_ptr<ProfileSource>* source,
                               /*out*/ std::string* error);

  // Entry point for profile loading functionality.
  ProfileLoadStatus LoadInternal(
      int32_t fd,
      std::string* error,
      bool merge_classes = true,
      const ProfileLoadFilterFn& filter_fn = ProfileFilterFnAcceptAll);

  // Read the profile header from the given fd and store the number of profile
  // lines into number_of_dex_files.
  ProfileLoadStatus ReadProfileHeader(ProfileSource& source,
                                      /*out*/uint8_t* number_of_dex_files,
                                      /*out*/uint32_t* size_uncompressed_data,
                                      /*out*/uint32_t* size_compressed_data,
                                      /*out*/std::string* error);

  // Read the header of a profile line from the given fd.
  ProfileLoadStatus ReadProfileLineHeader(SafeBuffer& buffer,
                                          /*out*/ProfileLineHeader* line_header,
                                          /*out*/std::string* error);

  // Read individual elements from the profile line header.
  bool ReadProfileLineHeaderElements(SafeBuffer& buffer,
                                     /*out*/uint16_t* dex_location_size,
                                     /*out*/ProfileLineHeader* line_header,
                                     /*out*/std::string* error);

  // Read a single profile line from the given fd.
  ProfileLoadStatus ReadProfileLine(SafeBuffer& buffer,
                                    uint8_t number_of_dex_files,
                                    const ProfileLineHeader& line_header,
                                    const SafeMap<uint8_t, uint8_t>& dex_profile_index_remap,
                                    bool merge_classes,
                                    /*out*/std::string* error);

  // Read all the classes from the buffer into the profile `info_` structure.
  bool ReadClasses(SafeBuffer& buffer,
                   const ProfileLineHeader& line_header,
                   /*out*/std::string* error);

  // Read all the methods from the buffer into the profile `info_` structure.
  bool ReadMethods(SafeBuffer& buffer,
                   uint8_t number_of_dex_files,
                   const ProfileLineHeader& line_header,
                   const SafeMap<uint8_t, uint8_t>& dex_profile_index_remap,
                   /*out*/std::string* error);

  // The method generates mapping of profile indices while merging a new profile
  // data into current data. It returns true, if the mapping was successful.
  bool RemapProfileIndex(const std::vector<ProfileLineHeader>& profile_line_headers,
                         const ProfileLoadFilterFn& filter_fn,
                         /*out*/SafeMap<uint8_t, uint8_t>* dex_profile_index_remap);

  // Read the inline cache encoding from line_bufer into inline_cache.
  bool ReadInlineCache(SafeBuffer& buffer,
                       uint8_t number_of_dex_files,
                       const SafeMap<uint8_t, uint8_t>& dex_profile_index_remap,
                       /*out*/InlineCacheMap* inline_cache,
                       /*out*/std::string* error);

  // Encode the inline cache into the given buffer.
  void AddInlineCacheToBuffer(std::vector<uint8_t>* buffer,
                              const InlineCacheMap& inline_cache);

  // Return the number of bytes needed to encode the profile information
  // for the methods in dex_data.
  uint32_t GetMethodsRegionSize(const DexFileData& dex_data);

  // Group `classes` by their owning dex profile index and put the result in
  // `dex_to_classes_map`.
  void GroupClassesByDex(
      const ClassSet& classes,
      /*out*/SafeMap<uint8_t, std::vector<dex::TypeIndex>>* dex_to_classes_map);

  // Find the data for the dex_pc in the inline cache. Adds an empty entry
  // if no previous data exists.
  DexPcData* FindOrAddDexPc(InlineCacheMap* inline_cache, uint32_t dex_pc);

  friend class ProfileCompilationInfoTest;
  friend class CompilerDriverProfileTest;
  friend class ProfileAssistantTest;
  friend class Dex2oatLayoutTest;

  ArenaPool default_arena_pool_;
  ArenaAllocator allocator_;

  // Vector containing the actual profile info.
  // The vector index is the profile index of the dex data and
  // matched DexFileData::profile_index.
  ArenaVector<DexFileData*> info_;

  // Cache mapping profile keys to profile index.
  // This is used to speed up searches since it avoids iterating
  // over the info_ vector when searching by profile key.
  ArenaSafeMap<const std::string, uint8_t> profile_key_map_;
};

}  // namespace art

#endif  // ART_RUNTIME_JIT_PROFILE_COMPILATION_INFO_H_
