LOCAL_PATH := $(call my-dir)
CLANG_ROOT_PATH := $(LOCAL_PATH)

.PHONY: clang-toolchain-minimal clang-toolchain-full llvm-tools
clang-toolchain-minimal: \
    clang

clang-toolchain-full: \
    clang-toolchain-minimal \
    asan_test \
    clang-check \
    clang-format \
    clang-tidy \
    FileCheck \
    llvm-as \
    llvm-dis \
    llvm-link \
    llvm-symbolizer \
    LLVMgold \
    libprofile_rt

llvm-tools: \
    bugpoint \
    BugpointPasses \
    count \
    llc \
    lli \
    lli-child-target \
    LLVMHello \
    llvm-ar \
    llvm-as \
    llvm-bcanalyzer \
    llvm-config \
    llvm-cov \
    llvm-c-test \
    llvm-cxxdump \
    llvm-diff \
    llvm-dis \
    llvm-dsymutil \
    llvm-dwarfdump \
    llvm-dwp \
    llvm-extract \
    llvm-link \
    llvm-lto \
    llvm-mc \
    llvm-mcmarkup \
    llvm-nm \
    llvm-objdump \
    llvm-pdbdump \
    llvm-profdata \
    llvm-readobj \
    llvm-rtdyld \
    llvm-size \
    llvm-split \
    llvm-symbolizer \
    not \
    obj2yaml \
    opt \
    sancov \
    sanstats \
    verify-uselistorder \
    yaml2obj \
    yaml-bench

ifneq ($(HOST_OS),darwin)
clang-toolchain-minimal: \
    libasan \
    libasan_32 \
    libasan_cxx \
    libasan_cxx_32 \
    libprofile_rt_32 \
    libtsan \
    libtsan_cxx \
    libubsan_standalone \
    libubsan_standalone_32 \
    libubsan_standalone_cxx \
    libubsan_standalone_cxx_32

clang-toolchain-full: \
    host_cross_clang \
    host_cross_clang_64 \

# Build libomp on Linux host.  Build modules for the host and some specific
# targets.
clang-toolchain-full: libomp
ifneq (,$(filter arm arm64 x86 x86_64,$(TARGET_ARCH)))
clang-toolchain-full: libomp-$(TARGET_ARCH)
endif # ifneq  (,$(filter arm arm64 x86 x86_64,$(TARGET_ARCH)))

endif # ifneq ($(HOST_OS),darwin)

ifneq (,$(filter arm arm64 x86 mips mips64,$(TARGET_ARCH)))
clang-toolchain-minimal: \
    $(ADDRESS_SANITIZER_RUNTIME_LIBRARY) \
    $(UBSAN_RUNTIME_LIBRARY)

endif
