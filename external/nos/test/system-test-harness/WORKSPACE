workspace(name = "nugget_test_systemtestharness")

new_http_archive(
    name = "gtest",
    url = "https://github.com/google/googletest/archive/release-1.8.0.zip",
    build_file = "../../core/nugget/BUILD.gtest",
    sha256 = "f3ed3b58511efd272eb074a3a6d6fb79d7c2e6a0e374323d1e6bcbcc1ef141bf",
    strip_prefix = "googletest-release-1.8.0",
)

http_archive(
    name = "com_google_protobuf",
    url = "https://github.com/google/protobuf/archive/v3.5.0.zip",
    strip_prefix = "protobuf-3.5.0",
    sha256 = "cef7f1b5a7c5fba672bec2a319246e8feba471f04dcebfe362d55930ee7c1c30",
)

http_archive(
    name = "com_google_protobuf_cc",
    url = "https://github.com/google/protobuf/archive/v3.5.0.zip",
    strip_prefix = "protobuf-3.5.0",
    sha256 = "cef7f1b5a7c5fba672bec2a319246e8feba471f04dcebfe362d55930ee7c1c30",
)

http_archive(
    name = "com_github_gflags_gflags",
    url = "https://github.com/gflags/gflags/archive/v2.2.1.zip",
    strip_prefix = "gflags-2.2.1",
    sha256 = "4e44b69e709c826734dbbbd5208f61888a2faf63f239d73d8ba0011b2dccc97a",
)

http_archive(
    name = "boringssl",
    url = "https://boringssl.googlesource.com/boringssl/+archive/74ffd81aa7ec3d0aa3d3d820dbeda934958ca81a.tar.gz",
    # branch master-with-bazel
)

new_local_repository(
    name = "nugget_thirdparty_libmpsse",
    path = "../../third_party/libmpsse/src",
    build_file_content = """
cc_library(
    name = "libmpsse",
    srcs = [
        "fast.c",
        "mpsse.c",
        "support.c",
    ],
    hdrs = [
        "config.h",
        "mpsse.h",
        "support.h",
    ],
    copts = [
        "-Wall",
        "-fPIC",
        "-fno-strict-aliasing",
        "-g",
        "-O2",
    ],
    defines = [
        "LIBFTDI1=0",
    ],
    linkopts = [
        "-lftdi",
    ],
    visibility = ["//visibility:public"],
)
""",
)

## This is the rule for when this repository is outside of repo.
# new_git_repository(
#     name = "ahdlc",
#     remote = "https://nugget-os.googlesource.com/third_party/ahdlc",
# )

## Use this when a subproject of repo.
new_local_repository(
    name = "nugget_thirdparty_ahdlc",
    path = "../../third_party/ahdlc",
    build_file_content = """
cc_library(
    name = "ahdlc",
    srcs = [
        "src/lib/crc_16.c",
        "src/lib/frame_layer.c",
    ],
    hdrs = [
        "src/lib/inc/crc_16.h",
        "src/lib/inc/frame_layer.h",
        "src/lib/inc/frame_layer_types.h",
    ],
    visibility = ["//visibility:public"],
)
""",
)

local_repository(
    name = "nugget_core_nugget",
    path = "../../core/nugget",
)

local_repository(
    name = "nugget_host_generic",
    path = "../../host/generic",
)

local_repository(
    name = "nugget_host_generic_libnos",
    path = "../../host/generic/libnos",
)

local_repository(
    name = "nugget_host_generic_libnos_datagram",
    path = "../../host/generic/libnos_datagram",
)

local_repository(
    name = "nugget_host_generic_libnos_transport",
    path = "../../host/generic/libnos_transport",
)

local_repository(
    name = "nugget_host_generic_libnos_generator",
    path = "../../host/generic/libnos/generator",
)

local_repository(
    name = "nugget_host_generic_nugget_proto",
    path = "../../host/generic/nugget/proto",
)

local_repository(
    name = "nugget_host_linux_citadel_libnos_datagram",
    path = "../../host/linux/citadel/libnos_datagram",
)

local_repository(
    name = "nugget_test_systemtestharness_tools",
    path = "tools",
)
