
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

// Defines how a machine instruction behaves in terms of IR expressions.
typedef struct insn_proto insn_proto_t;



// Defines how a machine instruction behaves in terms of IR expressions.
struct insn_proto {
    // Human-readable instruction name.
    char const *name;
    // Backend-specific extra information.
    void const *cookie;
};
