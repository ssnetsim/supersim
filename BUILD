licenses(["notice"])

exports_files([
    "LICENSE",
    "NOTICE",
])

COPTS = [
    "-UNDEBUG",
    "-Wno-unused-parameter",
]

LIBS = [
    "@libcolhash//:colhash",
    "@libprim//:prim",
    "@libfactory//:factory",
    "@librnd//:rnd",
    "@libmut//:mut",
    "@libbits//:bits",
    "@libstrop//:strop",
    "@libfio//:fio",
    "@libsettings//:settings",
    "@zlib//:zlib",
    "@jsoncpp//:jsoncpp",
]

cc_library(
    name = "lib",
    srcs = glob(
        ["src/**/*.cc"],
        exclude = [
            "src/main.cc",
            "src/**/*_TEST*",
            "src/**/*_TESTLIB*",
        ],
    ),
    hdrs = glob(
        [
            "src/**/*.h",
            "src/**/*.tcc",
        ],
        exclude = [
            "src/**/*_TEST*",
            "src/**/*_TESTLIB*",
        ],
    ),
    copts = COPTS,
    includes = [
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = LIBS,
    alwayslink = 1,
)

cc_library(
    name = "main",
    srcs = ["src/main.cc"],
    copts = COPTS,
    includes = [
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":lib",
    ] + LIBS,
)

cc_binary(
    name = "supersim",
    copts = COPTS,
    includes = [
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":main",
    ],
)

cc_library(
    name = "test_lib",
    testonly = 1,
    srcs = glob([
        "src/**/*_TESTLIB.cc",
    ]),
    hdrs = glob([
        "src/**/*_TESTLIB.h",
        "src/**/*_TESTLIB.tcc",
    ]),
    copts = COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":lib",
        "@googletest//:gtest_main",
    ] + LIBS,
    alwayslink = 1,
)

[
    cc_test(
        name = test_file.replace(".cc", ""),
        srcs = [test_file],
        args = [
            "--gtest_color=yes",
        ],
        copts = COPTS,
        visibility = ["//visibility:public"],
        deps = [
            ":test_lib",
        ] + LIBS,
    )
    for test_file in glob(["src/**/*_TEST.cc"])
]

genrule(
    name = "lint",
    srcs = glob([
        "src/**/*.cc",
    ]) + glob([
        "src/**/*.h",
        "src/**/*.tcc",
    ]),
    outs = ["linted"],
    cmd = """
    python $(location @cpplint//:cpplint) \
      --root=$$(pwd)/src \
      --headers=h,tcc \
      --extensions=cc,h,tcc \
      --quiet $(SRCS) > $@
    echo // $$(date) > $@
  """,
    tools = [
        "@cpplint",
    ],
    visibility = ["//visibility:public"],
)
