/*
 * Copyright 2011, The Android Open Source Project
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


#define LOG_NDEBUG 0
#define LOG_TAG "MemoryLeackTrackUtil"
#include <utils/Log.h>
#include <utils/Vector.h>
#include <utils/String8.h>

#include "MemoryLeakTrackUtil.h"

//#include <bionic_malloc.h>

//#include <bionic_macros.h>
//#include <bionic_malloc_dispatch.h>

extern std::string backtrace_string(const uintptr_t* frames, size_t frame_count);

namespace android {

struct MmapInfo {
    String8 fileName;
    unsigned long startAddr;
    unsigned long endAddr;
};

static inline char* skip_whitespace(const char *s) {
    /* In POSIX/C locale (the only locale we care about: do we REALLY want
    * to allow Unicode whitespace in, say, .conf files? nuts!)
    * isspace is only these chars: "\t\n\v\f\r" and space.
    * "\t\n\v\f\r" happen to have ASCII codes 9,10,11,12,13.
    * Use that.
    */
    while (*s == ' ' || (unsigned char)(*s - 9) <= (13 - 9))
        s++;

    return (char *) s;
}


static inline char *trim(char *s) {
    size_t len = strlen(s);

    /* trim trailing whitespace */
    while (len && isspace(s[len-1])) {
        --len;
    }

    /* trim leading whitespace */
    if (len) {
        char *nws = skip_whitespace(s);
        if ((nws - s) != 0) {
            len -= (nws - s);
            memmove(s, nws, len);
        }
    }
    s[len] = '\0';

    return s;
}


class MmapTable {
public:
    MmapTable() {
        FILE *fp = fopen("/proc/self/maps", "r");
        if (fp != nullptr) {
            char line[1024];
            while (fgets( line, sizeof(line), fp)) {
                unsigned long startAddrValue =0;
                unsigned long endAddrValue =0;
                char permissions[5] = {'\0'};  // Ensure NUL-terminated string.
                unsigned long long offset =0;
                unsigned char devMajor =0;
                unsigned char devMinor =0;
                unsigned long inode =0;
                int pathIndex = 0;

                if (sscanf(line, "%lx-%lx %4c %llx %hhx:%hhx %lu %n",
                         &startAddrValue, &endAddrValue, permissions, &offset,
                         &devMajor, &devMinor, &inode, &pathIndex) == 7) {
                    //ALOGD("parsed maps:%lx-%lx %s %llx %hhx:%hhx %lu [%s]",
                    //     startAddrValue, endAddrValue, permissions, offset,
                    //     devMajor, devMinor, inode, line+pathIndex);
                    if (strlen(line+pathIndex) < 3) {
                        continue;
                    }

                    if (strncmp(permissions, "r-xp", 4) != 0) {
                        continue;
                    }

                    MmapInfo *info = new MmapInfo();
                    info->fileName = trim(line+pathIndex);
                    info->startAddr = startAddrValue;
                    info->endAddr = endAddrValue;
                    maptable.push_back(info);
                }
            }
            fclose( fp );
        }
    }

    ~MmapTable() {
        for (size_t i = 0; i < maptable.size(); ++i) {
            delete maptable[i];
        }
    }

    String8 translateAddr(unsigned long addr) {
        char buf[32] = {'\0'};

        for (size_t i = 0; i < maptable.size(); ++i) {
            MmapInfo *info = maptable[i];
            if ((info->startAddr <= addr) && (info->endAddr >= addr)) {
                sprintf(buf, "PC 0x%08lx ", addr-info->startAddr);
                return String8(buf) + info->fileName;
            }
        }

        sprintf(buf, "PC 0x%08lx [No map Info]", addr);
        return String8(buf);
    }

    Vector<MmapInfo*> maptable;
};

//extern "C" void get_malloc_leak_info(uint8_t** info, size_t* overallSize,
//        size_t* infoSize, size_t* totalMemory, size_t* backtraceSize);

//extern "C" void free_malloc_leak_info(uint8_t* info);


bool dumpMemoryAddresses(int fd) {
    const size_t SIZE = 256;
    char buffer[SIZE];

    typedef struct {
        size_t size;
        size_t dups;
        intptr_t * backtrace;
    } AllocEntry;

    uint8_t *info = NULL;
    size_t overallSize = 0;
    size_t infoSize = 0;
    size_t totalMemory = 0;
    size_t backtraceSize = 0;

    //get_malloc_leak_info(&info, &overallSize, &infoSize, &totalMemory, &backtraceSize);
    if (info) {
        MmapTable mapTable;

        uint8_t *ptr = info;
        size_t count = overallSize / infoSize;

        dprintf(fd, " Allocation count %i\n", count);
        dprintf(fd, " Total memory %i\n", totalMemory);

        AllocEntry * entries = new AllocEntry[count];

        for (size_t i = 0; i < count; i++) {
            // Each entry should be size_t, size_t, intptr_t[backtraceSize]
            AllocEntry *e = &entries[i];

            e->size = *reinterpret_cast<size_t *>(ptr);
            ptr += sizeof(size_t);

            e->dups = *reinterpret_cast<size_t *>(ptr);
            ptr += sizeof(size_t);

            e->backtrace = reinterpret_cast<intptr_t *>(ptr);
            ptr += sizeof(intptr_t) * backtraceSize;
        }

        // Now we need to sort the entries.  They come sorted by size but
        // not by stack trace which causes problems using diff.
        bool moved;
        do {
            moved = false;
            for (size_t i = 0; i < (count - 1); i++) {
                AllocEntry *e1 = &entries[i];
                AllocEntry *e2 = &entries[i+1];

                bool swap = e1->size < e2->size;
                if (e1->size == e2->size) {
                    for (size_t j = 0; j < backtraceSize; j++) {
                        if (e1->backtrace[j] == e2->backtrace[j]) {
                            continue;
                        }
                        swap = e1->backtrace[j] < e2->backtrace[j];
                        break;
                    }
                }
                if (swap) {
                    AllocEntry t = entries[i];
                    entries[i] = entries[i+1];
                    entries[i+1] = t;
                    moved = true;
                }
            }
        } while (moved);

        int ignored = 0;
        for (size_t i = 0; i < count; i++) {
            AllocEntry *e = &entries[i];

            if (e->size <= 16 && e->dups == 1) {
                ignored++;
                continue;
            }

            dprintf(fd, "size %8i, dup %4i,  ", e->size, e->dups);
            for (size_t ct = 0; (ct < backtraceSize) && e->backtrace[ct]; ct++) {
                if (ct) {
                    dprintf(fd, "                          ");
                }
                //dprintf(fd, "0x%08x", e->backtrace[ct]);
                dprintf(fd, "#%02d %s\n", ct, mapTable.translateAddr(e->backtrace[ct]).c_str());
            }
            dprintf(fd, "\n");
        }
        dprintf(fd, "Ignore dump %d of (only 1 alloc, size<=16bytes)\n", ignored);

        delete[] entries;
        //free_malloc_leak_info(info);
    } else {
        return false;
    }

    return true;
}


}  // namespace android
