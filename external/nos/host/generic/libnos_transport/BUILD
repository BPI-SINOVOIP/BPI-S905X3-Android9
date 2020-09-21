cc_library(
    name = "libnos_transport",
    srcs = [
        "transport.c",
    ],
    hdrs = [
        "include/nos/transport.h",
    ],
    includes = [
        "./include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@nugget_host_generic//:nos_headers",
        "@nugget_host_generic_libnos_datagram//:libnos_datagram",
    ],
)
