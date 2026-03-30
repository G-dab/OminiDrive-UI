load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "app",
    srcs = [
        "main.cpp",
        "src/base/src/PageManager.cpp",
    ],
    deps = [
        "//third_party:imgui",
    ],
    copts = ["/utf-8"],
    linkopts = [
        "opengl32.lib",
    ],
)