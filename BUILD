load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "OminiDrive-UI",
    srcs = [
        "main.cpp",
        "src/base/src/PageManager.cpp",
        "src/tools/src/SerialPortService.cpp",
        "src/ui/src/SerialAssistantPage.cpp",
    ],
    data = ["//assets:ui_fonts"],
    deps = [
        "//third_party:imgui",
        "@bazel_tools//tools/cpp/runfiles",
    ],
    copts = ["/utf-8"],
    linkopts = [
        "opengl32.lib",
    ],
)

