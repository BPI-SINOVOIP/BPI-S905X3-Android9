/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FAKE_AVB_OPS_H_
#define FAKE_AVB_OPS_H_

#include <base/files/file_util.h>
#include <map>
#include <set>
#include <string>

#include <libavb_ab/libavb_ab.h>
#include <libavb_atx/libavb_atx.h>

namespace avb {

// A delegate interface for ops callbacks. This allows tests to override default
// fake implementations. For convenience, test fixtures can inherit
// FakeAvbOpsDelegateWithDefaults and only override as needed.
class FakeAvbOpsDelegate {
 public:
  virtual ~FakeAvbOpsDelegate() {}
  virtual AvbIOResult read_from_partition(const char* partition,
                                          int64_t offset,
                                          size_t num_bytes,
                                          void* buffer,
                                          size_t* out_num_read) = 0;

  virtual AvbIOResult get_preloaded_partition(
      const char* partition,
      size_t num_bytes,
      uint8_t** out_pointer,
      size_t* out_num_bytes_preloaded) = 0;

  virtual AvbIOResult write_to_partition(const char* partition,
                                         int64_t offset,
                                         size_t num_bytes,
                                         const void* buffer) = 0;

  virtual AvbIOResult validate_vbmeta_public_key(
      AvbOps* ops,
      const uint8_t* public_key_data,
      size_t public_key_length,
      const uint8_t* public_key_metadata,
      size_t public_key_metadata_length,
      bool* out_key_is_trusted) = 0;

  virtual AvbIOResult read_rollback_index(AvbOps* ops,
                                          size_t rollback_index_slot,
                                          uint64_t* out_rollback_index) = 0;

  virtual AvbIOResult write_rollback_index(AvbOps* ops,
                                           size_t rollback_index_slot,
                                           uint64_t rollback_index) = 0;

  virtual AvbIOResult read_is_device_unlocked(AvbOps* ops,
                                              bool* out_is_device_unlocked) = 0;

  virtual AvbIOResult get_unique_guid_for_partition(AvbOps* ops,
                                                    const char* partition,
                                                    char* guid_buf,
                                                    size_t guid_buf_size) = 0;

  virtual AvbIOResult get_size_of_partition(AvbOps* ops,
                                            const char* partition,
                                            uint64_t* out_size) = 0;

  virtual AvbIOResult read_persistent_value(const char* name,
                                            size_t buffer_size,
                                            uint8_t* out_buffer,
                                            size_t* out_num_bytes_read) = 0;

  virtual AvbIOResult write_persistent_value(const char* name,
                                             size_t value_size,
                                             const uint8_t* value) = 0;

  virtual AvbIOResult read_permanent_attributes(
      AvbAtxPermanentAttributes* attributes) = 0;

  virtual AvbIOResult read_permanent_attributes_hash(
      uint8_t hash[AVB_SHA256_DIGEST_SIZE]) = 0;

  virtual void set_key_version(size_t rollback_index_location,
                               uint64_t key_version) = 0;
};

// Provides fake implementations of AVB ops. All instances of this class must be
// created on the same thread.
class FakeAvbOps : public FakeAvbOpsDelegate {
 public:
  FakeAvbOps();
  virtual ~FakeAvbOps();

  static FakeAvbOps* GetInstanceFromAvbOps(AvbOps* ops) {
    return reinterpret_cast<FakeAvbOps*>(ops->user_data);
  }
  static FakeAvbOps* GetInstanceFromAvbABOps(AvbABOps* ab_ops) {
    return reinterpret_cast<FakeAvbOps*>(ab_ops->ops->user_data);
  }

  AvbOps* avb_ops() {
    return &avb_ops_;
  }

  AvbABOps* avb_ab_ops() {
    return &avb_ab_ops_;
  }

  AvbAtxOps* avb_atx_ops() {
    return &avb_atx_ops_;
  }

  FakeAvbOpsDelegate* delegate() {
    return delegate_;
  }

  // Does not take ownership of |delegate|.
  void set_delegate(FakeAvbOpsDelegate* delegate) {
    delegate_ = delegate;
  }

  void set_partition_dir(const base::FilePath& partition_dir) {
    partition_dir_ = partition_dir;
  }

  void set_expected_public_key(const std::string& expected_public_key) {
    expected_public_key_ = expected_public_key;
  }

  void set_expected_public_key_metadata(
      const std::string& expected_public_key_metadata) {
    expected_public_key_metadata_ = expected_public_key_metadata;
  }

  void set_stored_rollback_indexes(
      const std::map<size_t, uint64_t>& stored_rollback_indexes) {
    stored_rollback_indexes_ = stored_rollback_indexes;
  }

  std::map<size_t, uint64_t> get_stored_rollback_indexes() {
    return stored_rollback_indexes_;
  }

  std::map<size_t, uint64_t> get_verified_rollback_indexes() {
    return verified_rollback_indexes_;
  }

  void set_stored_is_device_unlocked(bool stored_is_device_unlocked) {
    stored_is_device_unlocked_ = stored_is_device_unlocked;
  }

  void set_permanent_attributes(const AvbAtxPermanentAttributes& attributes) {
    permanent_attributes_ = attributes;
  }

  void set_permanent_attributes_hash(const std::string& hash) {
    permanent_attributes_hash_ = hash;
  }

  void enable_get_preloaded_partition();

  bool preload_partition(const std::string& partition,
                         const base::FilePath& path);

  // Gets the partition names that were passed to the
  // read_from_partition() operation.
  std::set<std::string> get_partition_names_read_from();

  // FakeAvbOpsDelegate methods.
  AvbIOResult read_from_partition(const char* partition,
                                  int64_t offset,
                                  size_t num_bytes,
                                  void* buffer,
                                  size_t* out_num_read) override;

  AvbIOResult get_preloaded_partition(const char* partition,
                                      size_t num_bytes,
                                      uint8_t** out_pointer,
                                      size_t* out_num_bytes_preloaded) override;

  AvbIOResult write_to_partition(const char* partition,
                                 int64_t offset,
                                 size_t num_bytes,
                                 const void* buffer) override;

  AvbIOResult validate_vbmeta_public_key(AvbOps* ops,
                                         const uint8_t* public_key_data,
                                         size_t public_key_length,
                                         const uint8_t* public_key_metadata,
                                         size_t public_key_metadata_length,
                                         bool* out_key_is_trusted) override;

  AvbIOResult read_rollback_index(AvbOps* ops,
                                  size_t rollback_index_location,
                                  uint64_t* out_rollback_index) override;

  AvbIOResult write_rollback_index(AvbOps* ops,
                                   size_t rollback_index_location,
                                   uint64_t rollback_index) override;

  AvbIOResult read_is_device_unlocked(AvbOps* ops,
                                      bool* out_is_device_unlocked) override;

  AvbIOResult get_unique_guid_for_partition(AvbOps* ops,
                                            const char* partition,
                                            char* guid_buf,
                                            size_t guid_buf_size) override;

  AvbIOResult get_size_of_partition(AvbOps* ops,
                                    const char* partition,
                                    uint64_t* out_size) override;

  AvbIOResult read_persistent_value(const char* name,
                                    size_t buffer_size,
                                    uint8_t* out_buffer,
                                    size_t* out_num_bytes_read) override;

  AvbIOResult write_persistent_value(const char* name,
                                     size_t value_size,
                                     const uint8_t* value) override;

  AvbIOResult read_permanent_attributes(
      AvbAtxPermanentAttributes* attributes) override;

  AvbIOResult read_permanent_attributes_hash(
      uint8_t hash[AVB_SHA256_DIGEST_SIZE]) override;

  void set_key_version(size_t rollback_index_location,
                       uint64_t key_version) override;

 private:
  AvbOps avb_ops_;
  AvbABOps avb_ab_ops_;
  AvbAtxOps avb_atx_ops_;

  FakeAvbOpsDelegate* delegate_;

  base::FilePath partition_dir_;

  std::string expected_public_key_;
  std::string expected_public_key_metadata_;

  std::map<size_t, uint64_t> stored_rollback_indexes_;
  std::map<size_t, uint64_t> verified_rollback_indexes_;

  bool stored_is_device_unlocked_;

  AvbAtxPermanentAttributes permanent_attributes_;
  std::string permanent_attributes_hash_;

  std::set<std::string> partition_names_read_from_;
  std::map<std::string, uint8_t*> preloaded_partitions_;

  std::map<std::string, std::string> stored_values_;
};

// A delegate implementation that calls FakeAvbOps by default.
class FakeAvbOpsDelegateWithDefaults : public FakeAvbOpsDelegate {
 public:
  AvbIOResult read_from_partition(const char* partition,
                                  int64_t offset,
                                  size_t num_bytes,
                                  void* buffer,
                                  size_t* out_num_read) override {
    return ops_.read_from_partition(
        partition, offset, num_bytes, buffer, out_num_read);
  }

  AvbIOResult get_preloaded_partition(
      const char* partition,
      size_t num_bytes,
      uint8_t** out_pointer,
      size_t* out_num_bytes_preloaded) override {
    return ops_.get_preloaded_partition(
        partition, num_bytes, out_pointer, out_num_bytes_preloaded);
  }

  AvbIOResult write_to_partition(const char* partition,
                                 int64_t offset,
                                 size_t num_bytes,
                                 const void* buffer) override {
    return ops_.write_to_partition(partition, offset, num_bytes, buffer);
  }

  AvbIOResult validate_vbmeta_public_key(AvbOps* ops,
                                         const uint8_t* public_key_data,
                                         size_t public_key_length,
                                         const uint8_t* public_key_metadata,
                                         size_t public_key_metadata_length,
                                         bool* out_key_is_trusted) override {
    return ops_.validate_vbmeta_public_key(ops,
                                           public_key_data,
                                           public_key_length,
                                           public_key_metadata,
                                           public_key_metadata_length,
                                           out_key_is_trusted);
  }

  AvbIOResult read_rollback_index(AvbOps* ops,
                                  size_t rollback_index_slot,
                                  uint64_t* out_rollback_index) override {
    return ops_.read_rollback_index(
        ops, rollback_index_slot, out_rollback_index);
  }

  AvbIOResult write_rollback_index(AvbOps* ops,
                                   size_t rollback_index_slot,
                                   uint64_t rollback_index) override {
    return ops_.write_rollback_index(ops, rollback_index_slot, rollback_index);
  }

  AvbIOResult read_is_device_unlocked(AvbOps* ops,
                                      bool* out_is_device_unlocked) override {
    return ops_.read_is_device_unlocked(ops, out_is_device_unlocked);
  }

  AvbIOResult get_unique_guid_for_partition(AvbOps* ops,
                                            const char* partition,
                                            char* guid_buf,
                                            size_t guid_buf_size) override {
    return ops_.get_unique_guid_for_partition(
        ops, partition, guid_buf, guid_buf_size);
  }

  AvbIOResult get_size_of_partition(AvbOps* ops,
                                    const char* partition,
                                    uint64_t* out_size) override {
    return ops_.get_size_of_partition(ops, partition, out_size);
  }

  AvbIOResult read_persistent_value(const char* name,
                                    size_t buffer_size,
                                    uint8_t* out_buffer,
                                    size_t* out_num_bytes_read) override {
    return ops_.read_persistent_value(
        name, buffer_size, out_buffer, out_num_bytes_read);
  }

  AvbIOResult write_persistent_value(const char* name,
                                     size_t value_size,
                                     const uint8_t* value) override {
    return ops_.write_persistent_value(name, value_size, value);
  }

  AvbIOResult read_permanent_attributes(
      AvbAtxPermanentAttributes* attributes) override {
    return ops_.read_permanent_attributes(attributes);
  }

  AvbIOResult read_permanent_attributes_hash(
      uint8_t hash[AVB_SHA256_DIGEST_SIZE]) override {
    return ops_.read_permanent_attributes_hash(hash);
  }

  void set_key_version(size_t rollback_index_location,
                       uint64_t key_version) override {
    ops_.set_key_version(rollback_index_location, key_version);
  }

 protected:
  FakeAvbOps ops_;
};

}  // namespace avb

#endif /* FAKE_AVB_OPS_H_ */
