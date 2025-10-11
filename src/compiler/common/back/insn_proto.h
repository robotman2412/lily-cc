
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

// Defines the prototype for a machine-specific assembly menemonic.
typedef struct insn_proto insn_proto_t;



// Defines the prototype for a machine-specific assembly menemonic.
struct insn_proto {
    // Human-readable instruction name.
    char const *name;
    // Backend-specific extra information.
    void const *cookie;
};
