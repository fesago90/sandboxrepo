cc_library(
    name = "streams",
    srcs = [
        "streams.h",
    ],
    deps = ["@fmt"],
)

cc_binary(
    name = "benchmark",
    srcs = [
        "benchmark.cc",
    ],
    deps = [
        ":streams",
        "@fmt",
        "@google_benchmark//:benchmark_main",
    ],
)
