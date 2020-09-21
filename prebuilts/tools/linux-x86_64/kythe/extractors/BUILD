package(default_visibility = ["//visibility:public"])

action_listener(
    name = "extract_kindex",
    extra_actions = [":extra_action"],
    mnemonics = ["Javac"],
    visibility = ["//visibility:public"],
)

extra_action(
    name = "extra_action",
    cmd = "$(location :java_extractor) $(EXTRA_ACTION_FILE) $(output $(ACTION_ID).java.kindex) $(location vnames.json)",
    data = ["vnames.json"],
    out_templates = ["$(ACTION_ID).java.kindex"],
    tools = [":java_extractor"],
)

java_binary(
    name = "java_extractor",
    main_class = "com.google.devtools.kythe.extractors.java.bazel.JavaExtractor",
    runtime_deps = [
        ":java_extractor_jar",
    ],
)

java_import(
    name = "java_extractor_jar",
    jars = ["bazel_java_extractor.jar"],
)
