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

#include "seccomp_policy.h"

#include <assert.h>
#include <linux/audit.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>

#include <vector>

#include <android-base/logging.h>

#include "seccomp_bpfs.h"


#if defined __arm__ || defined __aarch64__

#define DUAL_ARCH
#define PRIMARY_ARCH AUDIT_ARCH_AARCH64
static const struct sock_filter* primary_app_filter = arm64_app_filter;
static const size_t primary_app_filter_size = arm64_app_filter_size;
static const struct sock_filter* primary_system_filter = arm64_system_filter;
static const size_t primary_system_filter_size = arm64_system_filter_size;
static const struct sock_filter* primary_global_filter = arm64_global_filter;
static const size_t primary_global_filter_size = arm64_global_filter_size;
#define SECONDARY_ARCH AUDIT_ARCH_ARM
static const struct sock_filter* secondary_app_filter = arm_app_filter;
static const size_t secondary_app_filter_size = arm_app_filter_size;
static const struct sock_filter* secondary_system_filter = arm_system_filter;
static const size_t secondary_system_filter_size = arm_system_filter_size;
static const struct sock_filter* secondary_global_filter = arm_global_filter;
static const size_t secondary_global_filter_size = arm_global_filter_size;

#elif defined __i386__ || defined __x86_64__

#define DUAL_ARCH
#define PRIMARY_ARCH AUDIT_ARCH_X86_64
static const struct sock_filter* primary_app_filter = x86_64_app_filter;
static const size_t primary_app_filter_size = x86_64_app_filter_size;
static const struct sock_filter* primary_system_filter = x86_64_system_filter;
static const size_t primary_system_filter_size = x86_64_system_filter_size;
static const struct sock_filter* primary_global_filter = x86_64_global_filter;
static const size_t primary_global_filter_size = x86_64_global_filter_size;
#define SECONDARY_ARCH AUDIT_ARCH_I386
static const struct sock_filter* secondary_app_filter = x86_app_filter;
static const size_t secondary_app_filter_size = x86_app_filter_size;
static const struct sock_filter* secondary_system_filter = x86_system_filter;
static const size_t secondary_system_filter_size = x86_system_filter_size;
static const struct sock_filter* secondary_global_filter = x86_global_filter;
static const size_t secondary_global_filter_size = x86_global_filter_size;

#elif defined __mips__ || defined __mips64__

#define DUAL_ARCH
#define PRIMARY_ARCH AUDIT_ARCH_MIPSEL64
static const struct sock_filter* primary_app_filter = mips64_app_filter;
static const size_t primary_app_filter_size = mips64_app_filter_size;
static const struct sock_filter* primary_system_filter = mips64_system_filter;
static const size_t primary_system_filter_size = mips64_system_filter_size;
static const struct sock_filter* primary_global_filter = mips64_global_filter;
static const size_t primary_global_filter_size = mips64_global_filter_size;
#define SECONDARY_ARCH AUDIT_ARCH_MIPSEL
static const struct sock_filter* secondary_app_filter = mips_app_filter;
static const size_t secondary_app_filter_size = mips_app_filter_size;
static const struct sock_filter* secondary_system_filter = mips_system_filter;
static const size_t secondary_system_filter_size = mips_system_filter_size;
static const struct sock_filter* secondary_global_filter = mips_global_filter;
static const size_t secondary_global_filter_size = mips_global_filter_size;

#else
#error No architecture was defined!
#endif


#define syscall_nr (offsetof(struct seccomp_data, nr))
#define arch_nr (offsetof(struct seccomp_data, arch))

typedef std::vector<sock_filter> filter;

inline void Disallow(filter& f) {
    f.push_back(BPF_STMT(BPF_RET|BPF_K, SECCOMP_RET_TRAP));
}

static void ExamineSyscall(filter& f) {
    f.push_back(BPF_STMT(BPF_LD|BPF_W|BPF_ABS, syscall_nr));
}

#ifdef DUAL_ARCH
static bool SetValidateArchitectureJumpTarget(size_t offset, filter& f) {
    size_t jump_length = f.size() - offset - 1;
    auto u8_jump_length = (__u8) jump_length;
    if (u8_jump_length != jump_length) {
        LOG(FATAL)
            << "Can't set jump greater than 255 - actual jump is " <<  jump_length;
        return false;
    }
    f[offset] = BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, SECONDARY_ARCH, u8_jump_length, 0);
    return true;
}

static size_t ValidateArchitectureAndJumpIfNeeded(filter& f) {
    f.push_back(BPF_STMT(BPF_LD|BPF_W|BPF_ABS, arch_nr));
    f.push_back(BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, PRIMARY_ARCH, 2, 0));
    f.push_back(BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, SECONDARY_ARCH, 1, 0));
    Disallow(f);
    return f.size() - 2;
}
#else
static void ValidateArchitecture(filter& f) {
    f.push_back(BPF_STMT(BPF_LD|BPF_W|BPF_ABS, arch_nr));
    f.push_back(BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, PRIMARY_ARCH, 1, 0));
    Disallow(f);
}
#endif

static bool install_filter(filter const& f) {
    struct sock_fprog prog = {
        static_cast<unsigned short>(f.size()),
        const_cast<struct sock_filter*>(&f[0]),
    };
    // This assumes either the current process has CAP_SYS_ADMIN, or PR_SET_NO_NEW_PRIVS bit is set.
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) < 0) {
        PLOG(FATAL) << "Could not set seccomp filter of size " << f.size();
        return false;
    }
    return true;
}

enum FilterType {
  APP,
  SYSTEM,
  GLOBAL
};

bool _set_seccomp_filter(FilterType type) {
    const sock_filter *p, *s;
    size_t p_size, s_size;
    filter f;

    switch (type) {
      case APP:
        p = primary_app_filter;
        p_size = primary_app_filter_size;
        s = secondary_app_filter;
        s_size = secondary_app_filter_size;
        break;
      case SYSTEM:
        p = primary_system_filter;
        p_size = primary_system_filter_size;
        s = secondary_system_filter;
        s_size = secondary_system_filter_size;
        break;
      case GLOBAL:
        p = primary_global_filter;
        p_size = primary_global_filter_size;
        s = secondary_global_filter;
        s_size = secondary_global_filter_size;
        break;

    }

#ifdef DUAL_ARCH
    // Note that for mixed 64/32 bit architectures, ValidateArchitecture inserts a
    // jump that must be changed to point to the start of the 32-bit policy
    // 32 bit syscalls will not hit the policy between here and the call to SetJump
    auto offset_to_secondary_filter = ValidateArchitectureAndJumpIfNeeded(f);
#else
    ValidateArchitecture(f);
#endif

    ExamineSyscall(f);

    for (size_t i = 0; i < p_size; ++i) {
        f.push_back(p[i]);
    }
    Disallow(f);

#ifdef DUAL_ARCH
    if (!SetValidateArchitectureJumpTarget(offset_to_secondary_filter, f)) {
        return false;
    }

    ExamineSyscall(f);

    for (size_t i = 0; i < s_size; ++i) {
        f.push_back(s[i]);
    }
    Disallow(f);
#endif

    return install_filter(f);
}

bool set_app_seccomp_filter() {
    return _set_seccomp_filter(FilterType::APP);
}

bool set_system_seccomp_filter() {
    return _set_seccomp_filter(FilterType::SYSTEM);
}

bool set_global_seccomp_filter() {
    return _set_seccomp_filter(FilterType::GLOBAL);
}
