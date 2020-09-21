# Manually extrated proto files from the jar
# So bazel can consume them without wrapping the
# proto compiler in a extract jar step.
filegroup(
    name = "include",
    srcs = glob(["**/*.proto"]),
    visibility = ["//visibility:public"],
)
