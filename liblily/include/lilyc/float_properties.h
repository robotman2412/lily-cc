
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// Placeholder definitions of floating-point properties for the Lily C Compiler.

#pragma once

#ifndef __LILYC__
#warning This header file is intended to be used with the Lily C compiler.
#define __LILYC_NEED_F32__
#define __LILYC_NEED_F64__
#endif



#ifdef __LILYC_NEED_F32__
#define _LILYC_IF_F32(x) x
#else
#define _LILYC_IF_F32(x)
#endif

#ifdef __LILYC_NEED_F64__
#define _LILYC_IF_F64(x) x
#else
#define _LILYC_IF_F64(x)
#endif



typedef float  __lily_f32;
typedef double __lily_f64;
