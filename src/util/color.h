
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>



#define ANSI_CLRLN   "\033[2K"
#define ANSI_DEFAULT "\033[0m"
#define ANSI_BOLD    "\033[1m"

/* Foreground */
#define ANSI_BLACK_FG        "\033[30m"
#define ANSI_DARK_RED_FG     "\033[31m"
#define ANSI_DARK_GREEN_FG   "\033[32m"
#define ANSI_DARK_YELLOW_FG  "\033[33m"
#define ANSI_DARK_BLUE_FG    "\033[34m"
#define ANSI_DARK_MAGENTA_FG "\033[35m"
#define ANSI_DARK_CYAN_FG    "\033[36m"
#define ANSI_DARK_GRAY_FG    "\033[37m"
#define ANSI_GRAY_FG         "\033[90m"
#define ANSI_RED_FG          "\033[91m"
#define ANSI_GREEN_FG        "\033[92m"
#define ANSI_YELLOW_FG       "\033[93m"
#define ANSI_BLUE_FG         "\033[94m"
#define ANSI_MAGENTA_FG      "\033[95m"
#define ANSI_CYAN_FG         "\033[96m"
#define ANSI_WHITE_FG        "\033[97m"

/* Background */
#define ANSI_BLACK_BG        "\033[40m"
#define ANSI_DARK_RED_BG     "\033[41m"
#define ANSI_DARK_GREEN_BG   "\033[42m"
#define ANSI_DARK_YELLOW_BG  "\033[43m"
#define ANSI_DARK_BLUE_BG    "\033[44m"
#define ANSI_DARK_MAGENTA_BG "\033[45m"
#define ANSI_DARK_CYAN_BG    "\033[46m"
#define ANSI_DARK_GRAY_BG    "\033[47m"
#define ANSI_GRAY_BG         "\033[10m"
#define ANSI_RED_BG          "\033[101m"
#define ANSI_GREEN_BG        "\033[102m"
#define ANSI_YELLOW_BG       "\033[103m"
#define ANSI_BLUE_BG         "\033[104m"
#define ANSI_MAGENTA_BG      "\033[105m"
#define ANSI_CYAN_BG         "\033[106m"
#define ANSI_WHITE_BG        "\033[107m"



#ifdef LILY_DISABLE_COLOR
// Whether to enable color.
#define do_color 0
#else
// Whether to enable color.
extern bool do_color;
#endif



// Returns `msg` if `do_color` is true, "" otherwise.
#define color_str(msg) (do_color ? (msg) : "")
// Printf `msg` to `stdout` if `do_color` is true.
#define color_fputs(msg, to)                                                                                           \
    ({                                                                                                                 \
        if (do_color) {                                                                                                \
            fputs((msg), (to));                                                                                        \
        }                                                                                                              \
    })
// Formats `msg` to `stdout` if `do_color` is true.
#define color_fprintf(to, msg, ...)                                                                                    \
    ({                                                                                                                 \
        if (do_color) {                                                                                                \
            printf((to), (msg)__VA_OPT__(, ) __VA_ARGS__);                                                             \
        }                                                                                                              \
    })
// Printf `msg` to `stdout` if `do_color` is true.
#define color_puts(msg)                                                                                                \
    ({                                                                                                                 \
        if (do_color) {                                                                                                \
            fputs((msg), stdout);                                                                                      \
        }                                                                                                              \
    })
// Formats `msg` to `stdout` if `do_color` is true.
#define color_printf(msg, ...)                                                                                         \
    ({                                                                                                                 \
        if (do_color) {                                                                                                \
            printf((msg)__VA_OPT__(, ) __VA_ARGS__);                                                                   \
        }                                                                                                              \
    })
