
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

test_common_srcs = [
    'arith128_test.c',
    'compiler_test.c',
    'ir_test.c',
    'set_test.c',
    'vec_test.c',
]

test_common_include = [
    '.',
]

test_common_target = static_library(
    'test-common',
    test_common_srcs,
    include_directories: test_common_include,
    dependencies: [ common_dep, harness_dep ],
)

test_common_dep = declare_dependency(
    link_whole: test_common_target,
    dependencies: [ common_dep, harness_dep ],
    include_directories: test_common_include,
)
