
#ifndef TEMPLATE_CONFIG_H
#define TEMPLATE_CONFIG_H

#define ARCH_ID "template"

// Word sizes.
#define WORD_BITS 16
#define MEM_BITS  16
#define ADDR_BITS 16

// Register names.
#define NUM_REGS 4
#define REG_NAMES { "R0", "R1", "R2", "R3" }

// Generator fallbacks used.
#define FALLBACK_gen_expression
#define FALLBACK_gen_expr_inline

#endif //TEMPLATE_CONFIG_H
