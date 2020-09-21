################################################################################
# proto cc libraries
################################################################################

cc_proto_library(
    name = "nugget_app_keymaster_keymaster_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_keymaster_keymaster_proto",
    ],
)

cc_proto_library(
    name = "nugget_app_protoapi_control_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_protoapi_control_proto",
    ],
)

cc_proto_library(
    name = "nugget_app_protoapi_diagnostics_api_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_protoapi_diagnostics_api_proto",
    ],
)

cc_proto_library(
    name = "nugget_app_protoapi_testing_api_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_protoapi_testing_api_proto",
    ],
)

cc_proto_library(
    name = "nugget_app_weaver_weaver_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_weaver_weaver_proto",
    ],
)

cc_proto_library(
    name = "nugget_app_avb_avb_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_avb_avb_proto",
    ],
)

cc_proto_library(
    name = "nugget_protobuf_options_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_protobuf_options_proto",
    ],
)

################################################################################
# rpc proto cc_libraries
################################################################################

cc_library(
    name = "keymaster_client_proto",
    srcs = [
        "nugget/app/keymaster/Keymaster.client.cpp",
    ],
    hdrs = [
        "nugget/app/keymaster/Keymaster.client.h",
    ],
    includes = [
        "./nugget/app/keymaster",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_keymaster_keymaster_cc_proto",
        "nugget_protobuf_options_cc_proto",
        "@nugget_host_generic_libnos//:libnos",
    ],
)

cc_library(
    name = "weaver_client_proto",
    srcs = [
        "nugget/app/weaver/Weaver.client.cpp",
    ],
    hdrs = [
        "nugget/app/weaver/Weaver.client.h",
    ],
    includes = [
        "./nugget/app/weaver/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_weaver_weaver_cc_proto",
        "nugget_protobuf_options_cc_proto",
        "@nugget_host_generic_libnos//:libnos",
    ],
)

cc_library(
    name = "avb_client_proto",
    srcs = [
        "nugget/app/avb/Avb.client.cpp",
    ],
    hdrs = [
        "nugget/app/avb/Avb.client.h",
    ],
    includes = [
        "./nugget/app/avb/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "nugget_app_avb_avb_cc_proto",
        "nugget_protobuf_options_cc_proto",
        "@nugget_host_generic_libnos//:libnos",
    ],
)

################################################################################
# rpc proto genrules.
################################################################################

GEN_SERVICE = "$(location @com_google_protobuf//:protoc) --plugin=protoc-gen-nos-client-cpp=$(location @nugget_host_generic_libnos_generator//:protoc_gen_nos_client_cpp)"

PROTO_ROOT = "$$(dirname $$(dirname $$(dirname $(location nugget/protobuf/options.proto))))"

genrule(
    name = "avb_proto_client_cc_genrule",
    srcs = [
        "nugget/app/avb/avb.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "nugget/app/avb/Avb.client.cpp",
    ],
    cmd = GEN_SERVICE + " --proto_path=" + PROTO_ROOT +
          " --nos-client-cpp_out=source:$$(dirname $(location nugget/app/avb/Avb.client.cpp)) " +
          "$(location nugget/app/avb/avb.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@nugget_host_generic_libnos_generator//:protoc_gen_nos_client_cpp",
    ],
)

genrule(
    name = "avb_proto_client_h_genrule",
    srcs = [
        "nugget/app/avb/avb.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "nugget/app/avb/Avb.client.h",
    ],
    cmd = GEN_SERVICE + " --proto_path=" + PROTO_ROOT +
          " --nos-client-cpp_out=header:$$(dirname $(location nugget/app/avb/Avb.client.h)) " +
          "$(location nugget/app/avb/avb.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@nugget_host_generic_libnos_generator//:protoc_gen_nos_client_cpp",
    ],
)

genrule(
    name = "weaver_proto_client_cc_genrule",
    srcs = [
        "nugget/app/weaver/weaver.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "nugget/app/weaver/Weaver.client.cpp",
    ],
    cmd = GEN_SERVICE + " --proto_path=" + PROTO_ROOT +
          " --nos-client-cpp_out=source:$$(dirname $(location nugget/app/weaver/Weaver.client.cpp)) " +
          "$(location nugget/app/weaver/weaver.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@nugget_host_generic_libnos_generator//:protoc_gen_nos_client_cpp",
    ],
)

genrule(
    name = "weaver_proto_client_h_genrule",
    srcs = [
        "nugget/app/weaver/weaver.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "nugget/app/weaver/Weaver.client.h",
    ],
    cmd = GEN_SERVICE + " --proto_path=" + PROTO_ROOT +
          " --nos-client-cpp_out=header:$$(dirname $(location nugget/app/weaver/Weaver.client.h)) " +
          "$(location nugget/app/weaver/weaver.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@nugget_host_generic_libnos_generator//:protoc_gen_nos_client_cpp",
    ],
)

genrule(
    name = "keymaster_proto_client_cc_genrule",
    srcs = [
        "nugget/app/keymaster/keymaster.proto",
        "nugget/app/keymaster/keymaster_defs.proto",
        "nugget/app/keymaster/keymaster_types.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "nugget/app/keymaster/Keymaster.client.cpp",
    ],
    cmd = GEN_SERVICE + " --proto_path=" + PROTO_ROOT +
          " --nos-client-cpp_out=source:$$(dirname $(location nugget/app/keymaster/Keymaster.client.cpp)) " +
          "$(location nugget/app/keymaster/keymaster.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@nugget_host_generic_libnos_generator//:protoc_gen_nos_client_cpp",
    ],
)

genrule(
    name = "keymaster_proto_client_h_genrule",
    srcs = [
        "nugget/app/keymaster/keymaster.proto",
        "nugget/app/keymaster/keymaster_defs.proto",
        "nugget/app/keymaster/keymaster_types.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "nugget/app/keymaster/Keymaster.client.h",
    ],
    cmd = GEN_SERVICE + " --proto_path=" + PROTO_ROOT +
          " --nos-client-cpp_out=header:$$(dirname $(location nugget/app/keymaster/Keymaster.client.h)) " +
          "$(location nugget/app/keymaster/keymaster.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@nugget_host_generic_libnos_generator//:protoc_gen_nos_client_cpp",
    ],
)

################################################################################
# proto libraries
################################################################################

proto_library(
    name = "google_protobuf_descriptor_proto",
    srcs = [
        "google/protobuf/descriptor.proto",
    ],
)

proto_library(
    name = "nugget_app_avb_avb_proto",
    srcs = [
        "nugget/app/avb/avb.proto",
    ],
    deps = [
        "nugget_protobuf_options_proto",
    ],
)

proto_library(
    name = "nugget_app_keymaster_keymaster_proto",
    srcs = [
        "nugget/app/keymaster/keymaster.proto",
        "nugget/app/keymaster/keymaster_defs.proto",
        "nugget/app/keymaster/keymaster_types.proto",
    ],
    deps = [
        "nugget_protobuf_options_proto",
    ],
)

proto_library(
    name = "nugget_app_protoapi_control_proto",
    srcs = [
        "nugget/app/protoapi/control.proto",
    ],
    deps = [
        "nugget_app_protoapi_header_proto",
    ],
)

proto_library(
    name = "nugget_app_protoapi_diagnostics_api_proto",
    srcs = [
        "nugget/app/protoapi/diagnostics_api.proto",
    ],
    deps = [
        "nugget_app_protoapi_header_proto",
    ],
)

proto_library(
    name = "nugget_app_protoapi_gchips_types_proto",
    srcs = [
        "nugget/app/protoapi/gchips_types.proto",
    ],
)

proto_library(
    name = "nugget_app_protoapi_header_proto",
    srcs = [
        "nugget/app/protoapi/header.proto",
    ],
)

proto_library(
    name = "nugget_app_protoapi_testing_api_proto",
    srcs = [
        "nugget/app/protoapi/testing_api.proto",
    ],
    deps = [
        "nugget_app_protoapi_header_proto",
    ],
)

proto_library(
    name = "nugget_app_weaver_weaver_proto",
    srcs = [
        "nugget/app/weaver/weaver.proto",
    ],
    deps = [
        "nugget_protobuf_options_proto",
    ],
)

proto_library(
    name = "nugget_protobuf_options_proto",
    srcs = [
        "nugget/protobuf/options.proto",
    ],
    deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
)
