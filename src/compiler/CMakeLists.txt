
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

add_subdirectory(common)
add_subdirectory(front/c)
add_subdirectory(back/riscv)

# The pure essence of Lily-CC, for use in a compiler binary, language server or other program that wants to use Lily-CC's facilities.
add_library(lilycc-lib INTERFACE)
target_link_libraries(lilycc-lib INTERFACE riscv-backend)
target_link_libraries(lilycc-lib INTERFACE compiler-common)
target_link_libraries(lilycc-lib INTERFACE c-frontend)
