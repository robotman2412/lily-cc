
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

executable(
    'lilycc',
    'main.c',
    install: true,
    dependencies: [ front_c_dep, back_riscv_dep, common_dep ],
)

executable(
    'cpp',
    'cpp.c',
    install: true,
    dependencies: [ front_c_dep ],
)

executable(
    'lily-explainer',
    'explainer.c',
    install: true,
    dependencies: [ front_c_dep ],
)
