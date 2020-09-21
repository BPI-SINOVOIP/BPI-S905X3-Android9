/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "avb_atx_slot_verify.h"

#include <libavb/avb_sha.h>
#include <libavb/libavb.h>
#include <libavb_atx/libavb_atx.h>

/* Chosen to be generous but still require a huge number of increase operations
 * before exhausting the 64-bit space.
 */
static const uint64_t kRollbackIndexIncreaseThreshold = 1000000000;

/* By convention, when a rollback index is not used the value remains zero. */
static const uint64_t kRollbackIndexNotUsed = 0;

typedef struct _AvbAtxOpsContext {
  size_t key_version_location[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS];
  uint64_t key_version_value[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS];
  size_t next_key_version_slot;
} AvbAtxOpsContext;

typedef struct _AvbAtxOpsWithContext {
  AvbAtxOps atx_ops;
  AvbAtxOpsContext context;
} AvbAtxOpsWithContext;

/* Returns context associated with |atx_ops| returned by
 * setup_ops_with_context().
 */
static AvbAtxOpsContext* get_ops_context(AvbAtxOps* atx_ops) {
  return &((AvbAtxOpsWithContext*)atx_ops)->context;
}

/* An implementation of AvbAtxOps::set_key_version that saves the key version
 * information to ops context data.
 */
static void save_key_version_to_context(AvbAtxOps* atx_ops,
                                        size_t rollback_index_location,
                                        uint64_t key_version) {
  AvbAtxOpsContext* context = get_ops_context(atx_ops);
  size_t offset = context->next_key_version_slot++;
  if (offset < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS) {
    context->key_version_location[offset] = rollback_index_location;
    context->key_version_value[offset] = key_version;
  }
}

/* Attaches context data to |existing_ops| and returns new ops. The
 * |ops_with_context| will be used to store the new combined ops and context.
 * The set_key_version function will be replaced in order to collect the key
 * version information in the context.
 */
static AvbAtxOps* setup_ops_with_context(
    const AvbAtxOps* existing_ops, AvbAtxOpsWithContext* ops_with_context) {
  avb_memset(ops_with_context, 0, sizeof(AvbAtxOpsWithContext));
  ops_with_context->atx_ops = *existing_ops;
  // Close the loop on the circular reference.
  ops_with_context->atx_ops.ops->atx_ops = &ops_with_context->atx_ops;
  ops_with_context->atx_ops.set_key_version = save_key_version_to_context;
  return &ops_with_context->atx_ops;
}

/* Updates the stored rollback index value for |location| to match |value|. */
static AvbSlotVerifyResult update_rollback_index(AvbOps* ops,
                                                 size_t location,
                                                 uint64_t value) {
  AvbIOResult io_result = AVB_IO_RESULT_OK;
  uint64_t current_value;
  io_result = ops->read_rollback_index(ops, location, &current_value);
  if (io_result == AVB_IO_RESULT_ERROR_OOM) {
    return AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
  } else if (io_result != AVB_IO_RESULT_OK) {
    avb_error("Error getting rollback index for slot.\n");
    return AVB_SLOT_VERIFY_RESULT_ERROR_IO;
  }
  if (current_value == value) {
    // No update necessary.
    return AVB_SLOT_VERIFY_RESULT_OK;
  }
  // The difference between the new and current value must not exceed the
  // increase threshold, and the value must not decrease.
  if (value - current_value > kRollbackIndexIncreaseThreshold) {
    avb_error("Rollback index value cannot increase beyond the threshold.\n");
    return AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX;
  }
  // This should have been checked during verification, but check again here as
  // a safeguard.
  if (value < current_value) {
    avb_error("Rollback index value cannot decrease.\n");
    return AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX;
  }
  io_result = ops->write_rollback_index(ops, location, value);
  if (io_result == AVB_IO_RESULT_ERROR_OOM) {
    return AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
  } else if (io_result != AVB_IO_RESULT_OK) {
    avb_error("Error setting stored rollback index.\n");
    return AVB_SLOT_VERIFY_RESULT_ERROR_IO;
  }
  return AVB_SLOT_VERIFY_RESULT_OK;
}

AvbSlotVerifyResult avb_atx_slot_verify(
    AvbAtxOps* atx_ops,
    const char* ab_suffix,
    AvbAtxLockState lock_state,
    AvbAtxSlotState slot_state,
    AvbAtxOemDataState oem_data_state,
    AvbSlotVerifyData** verify_data,
    uint8_t vbh_extension[AVB_SHA256_DIGEST_SIZE]) {
  const char* partitions_without_oem[] = {"boot", NULL};
  const char* partitions_with_oem[] = {"boot", "oem_bootloader", NULL};
  AvbSlotVerifyResult result = AVB_SLOT_VERIFY_RESULT_OK;
  AvbSHA256Ctx ctx;
  size_t i = 0;
  AvbAtxOpsWithContext ops_with_context;

  atx_ops = setup_ops_with_context(atx_ops, &ops_with_context);

  result = avb_slot_verify(atx_ops->ops,
                           (oem_data_state == AVB_ATX_OEM_DATA_NOT_USED)
                               ? partitions_without_oem
                               : partitions_with_oem,
                           ab_suffix,
                           (lock_state == AVB_ATX_UNLOCKED)
                               ? AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR
                               : AVB_SLOT_VERIFY_FLAGS_NONE,
                           AVB_HASHTREE_ERROR_MODE_EIO,
                           verify_data);

  if (result != AVB_SLOT_VERIFY_RESULT_OK || lock_state == AVB_ATX_UNLOCKED) {
    return result;
  }

  /* Compute the Android Things Verified Boot Hash (VBH) extension. */
  avb_sha256_init(&ctx);
  for (i = 0; i < (*verify_data)->num_vbmeta_images; i++) {
    avb_sha256_update(&ctx,
                      (*verify_data)->vbmeta_images[i].vbmeta_data,
                      (*verify_data)->vbmeta_images[i].vbmeta_size);
  }
  avb_memcpy(vbh_extension, avb_sha256_final(&ctx), AVB_SHA256_DIGEST_SIZE);

  /* Increase rollback index values to match the verified slot. */
  if (slot_state == AVB_ATX_SLOT_MARKED_SUCCESSFUL) {
    for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++) {
      uint64_t rollback_index_value = (*verify_data)->rollback_indexes[i];
      if (rollback_index_value != kRollbackIndexNotUsed) {
        result = update_rollback_index(atx_ops->ops, i, rollback_index_value);
        if (result != AVB_SLOT_VERIFY_RESULT_OK) {
          goto out;
        }
      }
    }

    /* Also increase rollback index values for Android Things key version
     * locations.
     */
    for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++) {
      size_t rollback_index_location =
          ops_with_context.context.key_version_location[i];
      uint64_t rollback_index_value =
          ops_with_context.context.key_version_value[i];
      if (rollback_index_value != kRollbackIndexNotUsed) {
        result = update_rollback_index(
            atx_ops->ops, rollback_index_location, rollback_index_value);
        if (result != AVB_SLOT_VERIFY_RESULT_OK) {
          goto out;
        }
      }
    }
  }

out:
  if (result != AVB_SLOT_VERIFY_RESULT_OK) {
    avb_slot_verify_data_free(*verify_data);
    *verify_data = NULL;
  }
  return result;
}
