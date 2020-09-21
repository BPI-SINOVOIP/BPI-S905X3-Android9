load("//tools/base/bazel:maven.bzl", "maven_java_import", "maven_pom")
load("//tools/base/bazel:utils.bzl", "java_jarjar")

java_jarjar(
    name = "uast-171.2455.10-all",
    rules = "no_rules.txt",
    srcs = [
        ":1.0/uast-common-1.0.jar",
        ":1.0/uast-java-1.0.jar",
    ],
)

maven_java_import(
    name = "uast-171.2455.10",
    jars = [
        ":uast-171.2455.10-all",
    ],
    pom = ":uast-171.2455.10.pom",
    visibility = ["//visibility:public"],
)

maven_pom(
    name = "uast-171.2455.10.pom",
    artifact = "uast",
    group = "com.android.tools.external.intellij",
    version = "171.2455.10",
)
