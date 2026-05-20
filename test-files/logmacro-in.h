extern int printf(const char* fmt, ...);

#define ANSI_RESET       "\x1b[0m"

#define ANSI_BLACK       "\x1b[30m"
#define ANSI_RED         "\x1b[31m"
#define ANSI_GREEN       "\x1b[32m"
#define ANSI_YELLOW      "\x1b[33m"
#define ANSI_BLUE        "\x1b[34m"
#define ANSI_MAGENTA     "\x1b[35m"
#define ANSI_CYAN        "\x1b[36m"
#define ANSI_WHITE       "\x1b[37m"

#define ANSI_BBLACK      "\x1b[90m"
#define ANSI_BRED        "\x1b[91m"
#define ANSI_BGREEN      "\x1b[92m"
#define ANSI_BYELLOW     "\x1b[93m"
#define ANSI_BBLUE       "\x1b[94m"
#define ANSI_BMAGENTA    "\x1b[95m"
#define ANSI_BCYAN       "\x1b[96m"
#define ANSI_BWHITE      "\x1b[97m"

#define ANSI_BG_BLACK    "\x1b[40m"
#define ANSI_BG_RED      "\x1b[41m"
#define ANSI_BG_GREEN    "\x1b[42m"
#define ANSI_BG_YELLOW   "\x1b[43m"
#define ANSI_BG_BLUE     "\x1b[44m"
#define ANSI_BG_MAGENTA  "\x1b[45m"
#define ANSI_BG_CYAN     "\x1b[46m"
#define ANSI_BG_WHITE    "\x1b[47m"

#define ANSI_BG_BBLACK   "\x1b[100m"
#define ANSI_BG_BRED     "\x1b[101m"
#define ANSI_BG_BGREEN   "\x1b[102m"
#define ANSI_BG_BYELLOW  "\x1b[103m"
#define ANSI_BG_BBLUE    "\x1b[104m"
#define ANSI_BG_BMAGENTA "\x1b[105m"
#define ANSI_BG_BCYAN    "\x1b[106m"
#define ANSI_BG_BWHITE   "\x1b[107m"

#define LOG(fmt, ...)                \
do {                                 \
    printf(fmt "\n" __VA_OPT__(,) __VA_ARGS__); \
} while (0);

#define LOG_TAGGED(tag, tag_color, fmt, ...)    \
do {                                            \
    LOG("%s[%s]%s " fmt,                        \
    tag_color, tag, ANSI_RESET __VA_OPT__(,) __VA_ARGS__); \
} while (0);

#define LOG_TAGGED_SUFFIX(tag, tag_color, suffix, suffix_color, fmt, ...) \
do {                                                                      \
    LOG("%s[%s]%s " fmt " %s[%s]%s",                                      \
    tag_color, tag, ANSI_RESET                                           \
    __VA_OPT__(,) __VA_ARGS__,                                                        \
    suffix_color, suffix, ANSI_RESET);                                    \
} while (0);

#define LOG_TAGGED_OK(tag, tag_color, fmt, ...)                               \
do {                                                                          \
    LOG_TAGGED_SUFFIX(tag, tag_color, "OK", ANSI_BGREEN, fmt __VA_OPT__(,) __VA_ARGS__); \
} while (0);

#define LOG_TAGGED_PASS(tag, tag_color, fmt, ...)                               \
do {                                                                            \
    LOG_TAGGED_SUFFIX(tag, tag_color, "PASS", ANSI_BGREEN, fmt __VA_OPT__(,) __VA_ARGS__); \
} while (0);

#define LOG_TAGGED_WARN(tag, tag_color, fmt, ...)                                \
do {                                                                             \
    LOG_TAGGED_SUFFIX(tag, tag_color, "WARN", ANSI_BYELLOW, fmt __VA_OPT__(,) __VA_ARGS__); \
} while (0);

#define LOG_TAGGED_FAIL(tag, tag_color, fmt, ...)                             \
do {                                                                          \
    LOG_TAGGED_SUFFIX(tag, tag_color, "FAIL", ANSI_BRED, fmt __VA_OPT__(,) __VA_ARGS__); \
} while (0);

int main() {
    LOG("Plain log message");
    LOG_TAGGED("INFO", ANSI_BBLUE, "Connected to %s:%d", "localhost", 8080);
    LOG_TAGGED_SUFFIX("BUILD", ANSI_BMAGENTA, "DONE", ANSI_BGREEN, "Compiled %d files", 12);
    LOG_TAGGED_OK("SYSTEM", ANSI_BCYAN, "Initialization completed in %d ms", 42);
    LOG_TAGGED_PASS("TEST", ANSI_GREEN, "All %d test cases succeeded", 128);
    LOG_TAGGED_WARN("MEMORY", ANSI_BYELLOW, "Usage exceeded %d%%", 85);
    LOG_TAGGED_FAIL("NETWORK", ANSI_BRED, "Failed to connect to %s", "192.168.1.1");
}
