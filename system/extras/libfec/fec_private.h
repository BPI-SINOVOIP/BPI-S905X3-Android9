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

#ifndef __FEC_PRIVATE_H__
#define __FEC_PRIVATE_H__

#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <new>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

#include <crypto_utils/android_pubkey.h>
#include <fec/ecc.h>
#include <fec/io.h>
#include <openssl/sha.h>
#include <utils/Compat.h>

/* processing parameters */
#define WORK_MIN_THREADS 1
#define WORK_MAX_THREADS 64

/* verity parameters */
#define VERITY_CACHE_BLOCKS 4096
#define VERITY_NO_CACHE UINT64_MAX

/* verity definitions */
#define VERITY_METADATA_SIZE (8 * FEC_BLOCKSIZE)
#define VERITY_TABLE_ARGS 10 /* mandatory arguments */
#define VERITY_MIN_TABLE_SIZE (VERITY_TABLE_ARGS * 2) /* for a sanity check */
#define VERITY_MAX_TABLE_SIZE (VERITY_METADATA_SIZE - sizeof(verity_header))

/* verity header and metadata */
#define VERITY_MAGIC 0xB001B001
#define VERITY_MAGIC_DISABLE 0x46464F56
#define VERITY_VERSION 0
#define VERITY_TABLE_FIELDS 10
#define VERITY_TABLE_VERSION 1

struct verity_header {
    uint32_t magic;
    uint32_t version;
    uint8_t signature[ANDROID_PUBKEY_MODULUS_SIZE];
    uint32_t length;
};

/* file handle */
struct ecc_info {
    bool valid;
    int roots;
    int rsn;
    uint32_t size;
    uint64_t blocks;
    uint64_t rounds;
    uint64_t start; /* offset in file */
};

struct verity_info {
    bool disabled;
    char *table;
    uint32_t hash_data_blocks;
    uint32_t hash_size;
    uint64_t hash_data_offset;
    uint64_t hash_start;
    uint8_t *hash;
    uint32_t salt_size;
    uint8_t *salt;
    uint64_t data_blocks;
    uint64_t metadata_start; /* offset in file */
    uint8_t zero_hash[SHA256_DIGEST_LENGTH];
    verity_header header;
    verity_header ecc_header;
};

struct verity_block_info {
    uint64_t index;
    bool valid;
};

struct fec_handle {
    ecc_info ecc;
    int fd;
    int flags; /* additional flags passed to fec_open */
    int mode; /* mode for open(2) */
    pthread_mutex_t mutex;
    uint64_t errors;
    uint64_t data_size;
    uint64_t pos;
    uint64_t size;
    verity_info verity;
};

/* I/O helpers */
extern bool raw_pread(fec_handle *f, void *buf, size_t count,
        uint64_t offset);
extern bool raw_pwrite(fec_handle *f, const void *buf, size_t count,
        uint64_t offset);

/* processing functions */
typedef ssize_t (*read_func)(fec_handle *f, uint8_t *dest, size_t count,
        uint64_t offset, size_t *errors);

extern ssize_t process(fec_handle *f, uint8_t *buf, size_t count,
        uint64_t offset, read_func func);

/* verity functions */
extern uint64_t verity_get_size(uint64_t file_size, uint32_t *verity_levels,
        uint32_t *level_hashes);

extern int verity_parse_header(fec_handle *f, uint64_t offset);

extern bool verity_check_block(fec_handle *f, const uint8_t *expected,
        const uint8_t *block);

/* helper macros */
#ifndef unlikely
    #define unlikely(x) __builtin_expect(!!(x), 0)
    #define likely(x)   __builtin_expect(!!(x), 1)
#endif

#ifndef stringify
    #define __stringify(x) #x
    #define stringify(x) __stringify(x)
#endif

/*  warnings, errors, debug output */
#ifdef FEC_NO_KLOG
    #define __log(func, type, format, args...) \
        fprintf(stderr, "fec: <%d> " type ": %s: " format "\n", \
            (int)syscall(SYS_gettid), __FUNCTION__,  ##args)
#else
    #include <cutils/klog.h>

    #define __log(func, type, format, args...) \
        KLOG_##func("fec", "<%d> " type ": %s: " format "\n", \
            (int)syscall(SYS_gettid), __FUNCTION__, ##args)
#endif

#ifdef NDEBUG
    #define debug(format, args...)
#else
    #define debug(format, args...) __log(DEBUG, "debug", format, ##args)
#endif

#define warn(format, args...) __log(WARNING, "warning", format, ##args)
#define error(format, args...) __log(ERROR, "error", format, ##args)

#define check(p) \
    if (unlikely(!(p))) { \
        error("`%s' failed", #p); \
        errno = EFAULT; \
        return -1; \
    }

#endif /* __FEC_PRIVATE_H__ */
