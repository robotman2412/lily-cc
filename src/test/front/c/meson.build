
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

test_front_c_srcs = [
    'c_compiler2_test.c',
    'c_parser2_test.c',
    'c_preproc_test.c',
    'c_tokenizer_test.c',
]

test_front_c_include = [
    '.',
]

test_front_c_target = static_library(
    'test-front-c',
    test_front_c_srcs,
    include_directories: test_front_c_include,
    dependencies: [ front_c_dep, harness_dep ],
)

test_front_c_dep = declare_dependency(
    link_whole: test_front_c_target,
    dependencies: [ front_c_dep, harness_dep ],
    include_directories: test_front_c_include,
)
