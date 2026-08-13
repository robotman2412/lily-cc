
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

subdir('test-common')
subdir('common')
subdir('front/c')
subdir('back/riscv')

runner = executable(
    'lily-test',
    'main.c',
    install: true,
    dependencies: [ test_front_c_dep, test_back_riscv_dep, test_common_dep, harness_dep, util_dep ],
)

test('tests', runner)
