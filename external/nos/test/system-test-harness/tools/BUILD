cc_library(
    name = "nugget_tools",
    srcs = [
        "avb_tools.cc",
        "keymaster_tools.cc",
        "nugget_tools.cc",
    ],
    hdrs = [
        "avb_tools.h",
        "keymaster_tools.h",
        "nugget_tools.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@gtest//:gtest",
        "@nugget_host_generic_libnos//:libnos",
        "@nugget_host_generic_nugget_proto//:avb_client_proto",
        "@nugget_host_generic_nugget_proto//:keymaster_client_proto",
        "@nugget_host_linux_citadel_libnos_datagram//:libnos_datagram",
    ],
)
