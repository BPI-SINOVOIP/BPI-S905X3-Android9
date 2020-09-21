load("//tools/base/bazel:maven.bzl", "maven_java_import", "maven_pom")
load("//tools/base/bazel:utils.bzl", "java_jarjar")

java_jarjar(
    name = "intellij-core-171.2455.10-all",
    rules = "no_rules.txt",
    srcs = [
        ":171.2455.10/intellij-core.jar",
        ":171.2455.10/picocontainer.jar",
        ":171.2455.10/annotations.jar",
        ":171.2455.10/asm-all.jar",
    ],
)

maven_java_import(
    name = "intellij-core-171.2455.10",
    jars = [
        ":intellij-core-171.2455.10-all",
    ],
    pom = ":intellij-core-171.2455.10.pom",
    visibility = ["//visibility:public"],
)

maven_pom(
    name = "intellij-core-171.2455.10.pom",
    artifact = "intellij-core",
    group = "com.android.tools.external.intellij",
    version = "171.2455.10",
)
