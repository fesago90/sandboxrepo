cc_library(
    name = "streams",
    srcs = [
        "streams.h",
        "transport.h",
        "udp_sink.h",
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

cc_test(
    name = "streams_test",
    srcs = [
        "streams_test.cc",
    ],
    deps = [
        ":streams",
        "@googletest//:gtest_main",
    ],
)
