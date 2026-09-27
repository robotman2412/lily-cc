
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

test_back_riscv_srcs = [
    'codegen_test.c',
]

test_back_riscv_include = [
    '.',
]

test_back_riscv_target = static_library(
    'test-back-riscv',
    test_back_riscv_srcs,
    include_directories: test_back_riscv_include,
    dependencies: [ back_riscv_dep, harness_dep ],
)

test_back_riscv_dep = declare_dependency(
    link_whole: test_back_riscv_target,
    dependencies: [ back_riscv_dep, harness_dep ],
    include_directories: test_back_riscv_include,
)
