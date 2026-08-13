
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

back_riscv_srcs = [
    'rv_abi.c',
    'rv_backend.c',
    'rv_instructions.c',
    'rv_isel.c',
    'rv_misc.c',
]

back_riscv_include = [
    '.',
]

back_riscv_target = shared_library(
    'lily-back-riscv',
    back_riscv_srcs,
    install: true,
    include_directories: back_riscv_include,
    dependencies: [ common_dep ],
)

back_riscv_dep = declare_dependency(
    link_with: back_riscv_target,
    include_directories: back_riscv_include,
    dependencies: [ common_dep ],
)
