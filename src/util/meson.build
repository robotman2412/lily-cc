
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

util_srcs = [
    'arith128.c',
    'arrays.c',
    'char_repr.c',
    'color.c',
    'hash.c',
    'list.c',
    'map.c',
    'refcount.c',
    'set.c',
    'lilycc_malloc.c',
    'unreachable.c',
    'utf8.c',
    'vec.c',
]

util_include = [
    '.',
]

util_target = static_library(
    'util',
    util_srcs,
    include_directories: util_include,
)

util_dep = declare_dependency(
    link_whole: util_target,
    include_directories: util_include,
)

util_headers_dep = declare_dependency(
    include_directories: util_include,
)
