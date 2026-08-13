
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

harness_srcs = [
    'testcase.c',
]

harness_include = [
    '.',
]

harness_target = static_library(
    'harness',
    harness_srcs,
    include_directories: harness_include,
    dependencies: [ util_headers_dep ],
)

harness_dep = declare_dependency(
    link_with: harness_target,
    dependencies: [ util_headers_dep ],
    include_directories: harness_include,
)
