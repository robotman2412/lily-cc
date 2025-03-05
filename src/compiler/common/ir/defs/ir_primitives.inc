
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT
// clang-format off

#ifndef IR_PRIM_DEF
// Primary spelling of a keyword
#define IR_PRIM_DEF(keyw)
#endif

IR_PRIM_DEF(s8)
IR_PRIM_DEF(u8)
IR_PRIM_DEF(s16)
IR_PRIM_DEF(u16)
IR_PRIM_DEF(s32)
IR_PRIM_DEF(u32)
IR_PRIM_DEF(s64)
IR_PRIM_DEF(u64)
IR_PRIM_DEF(s128)
IR_PRIM_DEF(u128)
IR_PRIM_DEF(bool)
IR_PRIM_DEF(f32)
IR_PRIM_DEF(f64)

#undef IR_PRIM_DEF
