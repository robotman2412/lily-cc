
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT
// clang-format off

#ifdef bool
#define IR_KEYWORDS_INC_BOOL_WAS_DEFINED
#undef bool
#endif

#ifdef true
#define IR_KEYWORDS_INC_TRUE_WAS_DEFINED
#undef true
#endif

#ifdef false
#define IR_KEYWORDS_INC_FALSE_WAS_DEFINED
#undef false
#endif

#ifndef IR_KEYW_DEF
#define IR_KEYW_DEF(keyw)
#endif

IR_KEYW_DEF(function)
IR_KEYW_DEF(ssa_function)
IR_KEYW_DEF(code)
IR_KEYW_DEF(var)
IR_KEYW_DEF(arg)
IR_KEYW_DEF(frame)
IR_KEYW_DEF(entry)

IR_KEYW_DEF(true)
IR_KEYW_DEF(false)

IR_KEYW_DEF(reloc)

#define IR_PRIM_DEF(x) IR_KEYW_DEF(x)
#include "ir_primitives.inc"

IR_KEYW_DEF(comb)

#define IR_OP1_DEF(x) IR_KEYW_DEF(x)
#include "ir_op1.inc"

#define IR_OP2_DEF(x) IR_KEYW_DEF(x)
#include "ir_op2.inc"

IR_KEYW_DEF(jump)
IR_KEYW_DEF(branch)
IR_KEYW_DEF(load)
IR_KEYW_DEF(store)
IR_KEYW_DEF(lea)
IR_KEYW_DEF(call)
IR_KEYW_DEF(return)

#undef IR_KEYW_DEF

#ifdef IR_KEYWORDS_INC_BOOL_WAS_DEFINED
#undef IR_KEYWORDS_INC_BOOL_WAS_DEFINED
#define bool _Bool
#endif

#ifdef IR_KEYWORDS_INC_TRUE_WAS_DEFINED
#undef IR_KEYWORDS_INC_TRUE_WAS_DEFINED
#define true 1
#endif

#ifdef IR_KEYWORDS_INC_FALSE_WAS_DEFINED
#undef IR_KEYWORDS_INC_FALSE_WAS_DEFINED
#define false 0
#endif
