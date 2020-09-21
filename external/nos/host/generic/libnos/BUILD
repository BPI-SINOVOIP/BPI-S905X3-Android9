cc_library(
    name = "libnos",
    srcs = [
        "debug.cpp",
        "NuggetClient.cpp",
    ],
    hdrs = [
        "include/nos/debug.h",
        "include/nos/AppClient.h",
        "include/nos/NuggetClient.h",
        "include/nos/NuggetClientInterface.h",
    ],
    includes = [
        "./include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@nugget_host_generic//:nos_headers",
        "@nugget_host_generic_libnos_datagram//:libnos_datagram",
        "@nugget_host_generic_libnos_transport//:libnos_transport",
    ],
)
