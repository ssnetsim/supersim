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
    "@nlohmann_json//:nlohmann_json",
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
    includes = [
        "src",
    ],
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

test_suite(
    name = "unit_tests",
    tests = [test_file.replace(".cc", "")
             for test_file in glob(["src/**/*_TEST.cc"])],
    visibility = ["//visibility:public"],
)

genrule(
    name = "lint",
    srcs = glob([
        "src/**/*.cc",
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

py_binary(
    name = "run_example",
    srcs = ["scripts/run_example.py"],
    main = "scripts/run_example.py",
    python_version = "PY3",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "config_files",
    srcs = glob(["config/*"]),
)

[
    sh_test(
        name = config_file + "_check",
        srcs = ["scripts/run_example.sh"],
        args = [
            config_file,
        ],
        data = [
            ":config_files",
            ":run_example",
            ":supersim",
        ],
        visibility = ["//visibility:public"],
    )
    for config_file in glob(["config/*.json"])
]

test_suite(
    name = "config_tests",
    tests = [config_file + "_check" for config_file in glob(["config/*.json"])],
    visibility = ["//visibility:public"],
)
