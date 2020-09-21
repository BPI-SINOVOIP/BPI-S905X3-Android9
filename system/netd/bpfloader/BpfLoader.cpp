/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef LOG_TAG
#define LOG_TAG "bpfloader"
#endif

#include <arpa/inet.h>
#include <elf.h>
#include <error.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/bpf.h>
#include <linux/unistd.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <cutils/log.h>

#include <netdutils/MemBlock.h>
#include <netdutils/Misc.h>
#include <netdutils/Slice.h>
#include "bpf/BpfUtils.h"
#include "bpf/bpf_shared.h"

using android::base::unique_fd;
using android::netdutils::MemBlock;
using android::netdutils::Slice;

#define BPF_PROG_PATH "/system/etc/bpf"
#define BPF_PROG_SRC BPF_PROG_PATH "/bpf_kern.o"
#define MAP_LD_CMD_HEAD 0x18

#define FAIL(...)      \
    do {               \
        ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)); \
        exit(-1);     \
    } while (0)

// The BPF instruction bytes that we need to replace. x is a placeholder (e.g., COOKIE_TAG_MAP).
#define MAP_SEARCH_PATTERN(x)             \
    {                                     \
        0x18, 0x01, 0x00, 0x00,           \
        (x)[0], (x)[1], (x)[2], (x)[3],   \
        0x00, 0x00, 0x00, 0x00,           \
        (x)[4], (x)[5], (x)[6], (x)[7]    \
    }

// The bytes we'll replace them with. x is the actual fd number for the map at runtime.
// The second byte is changed from 0x01 to 0x11 since 0x11 is the special command used
// for bpf map fd loading. The original 0x01 is only a normal load command.
#define MAP_REPLACE_PATTERN(x)            \
    {                                     \
        0x18, 0x11, 0x00, 0x00,           \
        (x)[0], (x)[1], (x)[2], (x)[3],   \
        0x00, 0x00, 0x00, 0x00,           \
        (x)[4], (x)[5], (x)[6], (x)[7]    \
    }

#define DECLARE_MAP(_mapFd, _mapPath)                             \
    unique_fd _mapFd(android::bpf::mapRetrieve((_mapPath), 0));   \
    if (_mapFd < 0) {                                             \
        FAIL("Failed to get map from %s", (_mapPath));            \
    }

#define MAP_CMD_SIZE 16
#define LOG_BUF_SIZE 65536

namespace android {
namespace bpf {

struct ReplacePattern {
    std::array<uint8_t, MAP_CMD_SIZE> search;
    std::array<uint8_t, MAP_CMD_SIZE> replace;

    ReplacePattern(uint64_t dummyFd, int realFd) {
        // Ensure that the fd numbers are big-endian.
        uint8_t beDummyFd[sizeof(uint64_t)];
        uint8_t beRealFd[sizeof(uint64_t)];
        for (size_t i = 0; i < sizeof(uint64_t); i++) {
            beDummyFd[i] = (dummyFd >> (i * 8)) & 0xFF;
            beRealFd[i] = (realFd >> (i * 8)) & 0xFF;
        }
        search = MAP_SEARCH_PATTERN(beDummyFd);
        replace = MAP_REPLACE_PATTERN(beRealFd);
    }
};

MemBlock cgroupIngressProg;
MemBlock cgroupEgressProg;
MemBlock xtIngressProg;
MemBlock xtEgressProg;

MemBlock getProgFromMem(Slice buffer, Elf64_Shdr* section) {
    uint64_t progSize = (uint64_t)section->sh_size;
    Slice progSection = take(drop(buffer, section->sh_offset), progSize);
    if (progSection.size() < progSize) FAIL("programSection out of bound\n");

    MemBlock progCopy(progSection);
    if (progCopy.get().size() != progSize) {
        FAIL("program cannot be extracted");
    }
    return progCopy;
}

void parseProgramsFromFile(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        FAIL("Failed to open %s program: %s", path, strerror(errno));
    }

    struct stat stat;
    if (fstat(fd, &stat)) FAIL("Fail to get file size");

    off_t fileLen = stat.st_size;
    char* baseAddr = (char*)mmap(NULL, fileLen, PROT_READ, MAP_PRIVATE, fd, 0);
    if (baseAddr == MAP_FAILED) FAIL("Failed to map the program into memory");

    if ((uint32_t)fileLen < sizeof(Elf64_Ehdr)) FAIL("file size too small for Elf64_Ehdr");

    Slice buffer(baseAddr, fileLen);

    Slice elfHeader = take(buffer, sizeof(Elf64_Ehdr));

    if (elfHeader.size() < sizeof(Elf64_Ehdr)) FAIL("bpf buffer does not have complete elf header");

    Elf64_Ehdr* elf = (Elf64_Ehdr*)elfHeader.base();
    // Find section names string table. This is the section whose index is e_shstrndx.
    if (elf->e_shstrndx == SHN_UNDEF) {
        FAIL("cannot locate namesSection\n");
    }
    size_t totalSectionSize = (elf->e_shnum) * sizeof(Elf64_Shdr);
    Slice sections = take(drop(buffer, elf->e_shoff), totalSectionSize);
    if (sections.size() < totalSectionSize) {
        FAIL("sections corrupted");
    }

    Slice namesSection = take(drop(sections, elf->e_shstrndx * sizeof(Elf64_Shdr)),
                              sizeof(Elf64_Shdr));
    if (namesSection.size() != sizeof(Elf64_Shdr)) {
        FAIL("namesSection corrupted");
    }
    size_t strTabOffset = ((Elf64_Shdr*) namesSection.base())->sh_offset;
    size_t strTabSize = ((Elf64_Shdr*) namesSection.base())->sh_size;

    Slice strTab = take(drop(buffer, strTabOffset), strTabSize);
    if (strTab.size() < strTabSize) {
        FAIL("string table out of bound\n");
    }

    for (int i = 0; i < elf->e_shnum; i++) {
        Slice section = take(drop(sections, i * sizeof(Elf64_Shdr)), sizeof(Elf64_Shdr));
        if (section.size() < sizeof(Elf64_Shdr)) {
            FAIL("section %d is out of bound, section size: %zu, header size: %zu, total size: %zu",
                 i, section.size(), sizeof(Elf64_Shdr), sections.size());
        }
        Elf64_Shdr* sectionPtr = (Elf64_Shdr*)section.base();
        Slice nameSlice = drop(strTab, sectionPtr->sh_name);
        if (nameSlice.size() == 0) {
            FAIL("nameSlice out of bound, i: %d, strTabSize: %zu, sh_name: %u", i, strTabSize,
                 sectionPtr->sh_name);
        }
        if (!strcmp((char *)nameSlice.base(), BPF_CGROUP_INGRESS_PROG_NAME)) {
            cgroupIngressProg = getProgFromMem(buffer, sectionPtr);
        } else if (!strcmp((char *)nameSlice.base(), BPF_CGROUP_EGRESS_PROG_NAME)) {
            cgroupEgressProg = getProgFromMem(buffer, sectionPtr);
        } else if (!strcmp((char *)nameSlice.base(), XT_BPF_INGRESS_PROG_NAME)) {
            xtIngressProg = getProgFromMem(buffer, sectionPtr);
        } else if (!strcmp((char *)nameSlice.base(), XT_BPF_EGRESS_PROG_NAME)) {
            xtEgressProg = getProgFromMem(buffer, sectionPtr);
        }
    }
}

int loadProg(Slice prog, bpf_prog_type type, const std::vector<ReplacePattern>& mapPatterns) {
    if (prog.size() == 0) {
        FAIL("Couldn't find or parse program type %d", type);
    }
    Slice remaining = prog;
    while (remaining.size() >= MAP_CMD_SIZE) {
        // Scan the program, examining all possible places that might be the start of a map load
        // operation (i.e., all bytes of value MAP_LD_CMD_HEAD).
        // In each of these places, check whether it is the start of one of the patterns we want to
        // replace, and if so, replace it.
        Slice mapHead = findFirstMatching(remaining, MAP_LD_CMD_HEAD);
        if (mapHead.size() < MAP_CMD_SIZE) break;
        bool replaced = false;
        for (const auto& pattern : mapPatterns) {
            if (!memcmp(mapHead.base(), pattern.search.data(), MAP_CMD_SIZE)) {
                memcpy(mapHead.base(), pattern.replace.data(), MAP_CMD_SIZE);
                replaced = true;
                break;
            }
        }
        remaining = drop(mapHead, replaced ? MAP_CMD_SIZE : sizeof(uint8_t));
    }
    char bpf_log_buf[LOG_BUF_SIZE];
    Slice bpfLog = Slice(bpf_log_buf, sizeof(bpf_log_buf));
    return bpfProgLoad(type, prog, "Apache 2.0", 0, bpfLog);
}

int loadAndAttachProgram(bpf_attach_type type, const char* path, const char* name,
                         std::vector<ReplacePattern> mapPatterns) {

    unique_fd fd;
    if (type == BPF_CGROUP_INET_INGRESS) {
        fd.reset(loadProg(cgroupIngressProg, BPF_PROG_TYPE_CGROUP_SKB, mapPatterns));
    } else if (type == BPF_CGROUP_INET_EGRESS) {
        fd.reset(loadProg(cgroupEgressProg, BPF_PROG_TYPE_CGROUP_SKB, mapPatterns));
    } else if (!strcmp(name, XT_BPF_INGRESS_PROG_NAME)) {
        fd.reset(loadProg(xtIngressProg, BPF_PROG_TYPE_SOCKET_FILTER, mapPatterns));
    } else if (!strcmp(name, XT_BPF_EGRESS_PROG_NAME)) {
        fd.reset(loadProg(xtEgressProg, BPF_PROG_TYPE_SOCKET_FILTER, mapPatterns));
    } else {
        FAIL("Unrecognized program type: %s", name);
    }

    if (fd < 0) {
        FAIL("load %s failed: %s", name, strerror(errno));
    }
    int ret = 0;
    if (type == BPF_CGROUP_INET_EGRESS || type == BPF_CGROUP_INET_INGRESS) {
        unique_fd cg_fd(open(CGROUP_ROOT_PATH, O_DIRECTORY | O_RDONLY | O_CLOEXEC));
        if (cg_fd < 0) {
            FAIL("Failed to open the cgroup directory");
        }
        ret = attachProgram(type, fd, cg_fd);
        if (ret) {
            FAIL("%s attach failed: %s", name, strerror(errno));
        }
    }

    ret = mapPin(fd, path);
    if (ret) {
        FAIL("Pin %s as file %s failed: %s", name, path, strerror(errno));
    }
    return 0;
}

}  // namespace bpf
}  // namespace android

using android::bpf::APP_UID_STATS_MAP_PATH;
using android::bpf::BPF_EGRESS_PROG_PATH;
using android::bpf::BPF_INGRESS_PROG_PATH;
using android::bpf::COOKIE_TAG_MAP_PATH;
using android::bpf::DOZABLE_UID_MAP_PATH;
using android::bpf::IFACE_STATS_MAP_PATH;
using android::bpf::POWERSAVE_UID_MAP_PATH;
using android::bpf::STANDBY_UID_MAP_PATH;
using android::bpf::TAG_STATS_MAP_PATH;
using android::bpf::UID_COUNTERSET_MAP_PATH;
using android::bpf::UID_STATS_MAP_PATH;
using android::bpf::XT_BPF_EGRESS_PROG_PATH;
using android::bpf::XT_BPF_INGRESS_PROG_PATH;

using android::bpf::ReplacePattern;
using android::bpf::loadAndAttachProgram;

static void usage(void) {
    ALOGE( "Usage: ./bpfloader [-i] [-e]\n"
           "   -i load ingress bpf program\n"
           "   -e load egress bpf program\n"
           "   -p load prerouting xt_bpf program\n"
           "   -m load mangle xt_bpf program\n");
}

int main(int argc, char** argv) {
    int ret = 0;
    DECLARE_MAP(cookieTagMap, COOKIE_TAG_MAP_PATH);
    DECLARE_MAP(uidCounterSetMap, UID_COUNTERSET_MAP_PATH);
    DECLARE_MAP(appUidStatsMap, APP_UID_STATS_MAP_PATH);
    DECLARE_MAP(uidStatsMap, UID_STATS_MAP_PATH);
    DECLARE_MAP(tagStatsMap, TAG_STATS_MAP_PATH);
    DECLARE_MAP(ifaceStatsMap, IFACE_STATS_MAP_PATH);
    DECLARE_MAP(dozableUidMap, DOZABLE_UID_MAP_PATH);
    DECLARE_MAP(standbyUidMap, STANDBY_UID_MAP_PATH);
    DECLARE_MAP(powerSaveUidMap, POWERSAVE_UID_MAP_PATH);

    const std::vector<ReplacePattern> mapPatterns = {
        ReplacePattern(COOKIE_TAG_MAP, cookieTagMap.get()),
        ReplacePattern(UID_COUNTERSET_MAP, uidCounterSetMap.get()),
        ReplacePattern(APP_UID_STATS_MAP, appUidStatsMap.get()),
        ReplacePattern(UID_STATS_MAP, uidStatsMap.get()),
        ReplacePattern(TAG_STATS_MAP, tagStatsMap.get()),
        ReplacePattern(IFACE_STATS_MAP, ifaceStatsMap.get()),
        ReplacePattern(DOZABLE_UID_MAP, dozableUidMap.get()),
        ReplacePattern(STANDBY_UID_MAP, standbyUidMap.get()),
        ReplacePattern(POWERSAVE_UID_MAP, powerSaveUidMap.get()),
    };

    int opt;
    bool doIngress = false, doEgress = false, doPrerouting = false, doMangle = false;
    while ((opt = getopt(argc, argv, "iepm")) != -1) {
        switch (opt) {
            case 'i':
                doIngress = true;
                break;
            case 'e':
                doEgress = true;
                break;
            case 'p':
                doPrerouting = true;
                break;
            case 'm':
                doMangle = true;
                break;
            default:
                usage();
                FAIL("unknown argument %c", opt);
        }
    }
    android::bpf::parseProgramsFromFile(BPF_PROG_SRC);

    if (doIngress) {
        ret = loadAndAttachProgram(BPF_CGROUP_INET_INGRESS, BPF_INGRESS_PROG_PATH,
                                   BPF_CGROUP_INGRESS_PROG_NAME, mapPatterns);
        if (ret) {
            FAIL("Failed to set up ingress program");
        }
    }
    if (doEgress) {
        ret = loadAndAttachProgram(BPF_CGROUP_INET_EGRESS, BPF_EGRESS_PROG_PATH,
                                   BPF_CGROUP_EGRESS_PROG_NAME, mapPatterns);
        if (ret) {
            FAIL("Failed to set up ingress program");
        }
    }
    if (doPrerouting) {
        ret = loadAndAttachProgram(MAX_BPF_ATTACH_TYPE, XT_BPF_INGRESS_PROG_PATH,
                                   XT_BPF_INGRESS_PROG_NAME, mapPatterns);
        if (ret) {
            FAIL("Failed to set up xt_bpf program");
        }
    }
    if (doMangle) {
        ret = loadAndAttachProgram(MAX_BPF_ATTACH_TYPE, XT_BPF_EGRESS_PROG_PATH,
                                   XT_BPF_EGRESS_PROG_NAME, mapPatterns);
        if (ret) {
            FAIL("Failed to set up xt_bpf program");
        }
    }
    return ret;
}
