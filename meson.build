
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

project(
    'lilycc',
    'c',
    version: '0.0.1',
    default_options: [
        'c_std=gnu11',
        'warning_level=2',
        'buildtype=debug',
        'b_ndebug=if-release',
    ]
)

add_global_arguments(
    '-Wno-type-limits', '-Wno-unused-function',
    '-Werror=all', '-Werror=enum-conversion', '-Werror=incompatible-pointer-types',
    '-Wno-error=unused-parameter', '-Wno-error=unused-variable', '-Wno-error=unused-but-set-variable', '-Wno-error=unused',
    language: 'c'
)

subdir('src/util')
subdir('src/compiler')
subdir('src/main')
subdir('src/test')
