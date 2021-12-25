
#ifndef GR8CPU_R3_IASM_H
#define GR8CPU_R3_IASM_H

typedef enum r3_iasm_token_id {
    // Keyword tokens.
    R3_KEYW_BKI,  R3_KEYW_BRK,  R3_KEYW_CALL, R3_KEYW_RET, 
    R3_KEYW_PSH,  R3_KEYW_PUL,  R3_KEYW_POP,  R3_KEYW_JMP, 
    R3_KEYW_BEQ,  R3_KEYW_BNE,  R3_KEYW_BGT,  R3_KEYW_BLE, 
    R3_KEYW_BLT,  R3_KEYW_BGE,  R3_KEYW_BCS,  R3_KEYW_BCC, 
    R3_KEYW_MOV,  R3_KEYW_ADD,  R3_KEYW_SUB,  R3_KEYW_CMP, 
    R3_KEYW_INC,  R3_KEYW_DEC,  R3_KEYW_SHL,  R3_KEYW_SHR, 
    R3_KEYW_ADDC, R3_KEYW_SUBC, R3_KEYW_CMPC, R3_KEYW_INCC, 
    R3_KEYW_DECC, R3_KEYW_SHLC, R3_KEYW_SHRC, R3_KEYW_ROL, 
    R3_KEYW_ROR,  R3_KEYW_AND,  R3_KEYW_OR,   R3_KEYW_XOR, 
    R3_KEYW_RTI,  R3_KEYW_JMPT, R3_KEYW_CALT, R3_KEYW_SIRQ, 
    R3_KEYW_CIRQ, R3_KEYW_GPTR, R3_KEYW_VIRQ, R3_KEYW_VNMI, 
    R3_KEYW_VST,  R3_KEYW_HLT,  R3_KEYW_A,    R3_KEYW_X, 
    R3_KEYW_Y,    R3_KEYW_STL,  R3_KEYW_STH,  R3_KEYW_F,
    // Addressing modes.
    R3_TKN_ADDR_REG,
    R3_TKN_ADDR_IMM,
    R3_TKN_ADDR_MEM,
    // Other tokens.
    R3_TKN_COMMA,
    R3_TKN_LPAR,
    R3_TKN_RPAR,
    R3_TKN_LBRAC,
    R3_TKN_RBRAC,
    R3_TKN_IDENT,
    R3_TKN_IVAL,
    R3_TKN_OTHER,
    R3_TKN_END
} r3_iasm_token_id_t;

struct r3_iasm_token;
struct r3_iasm_modes;
struct r3_iasm_mode;

typedef struct r3_iasm_token r3_token_t;
typedef struct r3_iasm_modes r3_iasm_modes_t;
typedef struct r3_iasm_mode r3_iasm_mode_t;

#include <stdint.h>
#include <stddef.h>

struct r3_iasm_token {
    r3_iasm_token_id_t type;
    uint8_t            addr_mode;
    union {
        char           other;
        char          *ident;
        long long      ival;
    };
};

struct r3_iasm_modes {
    size_t          num;
    r3_iasm_mode_t *modes;
};

struct r3_iasm_mode {
    size_t  n_args;
    uint8_t opcode;
    uint8_t n_words;
    uint8_t arg_modes[2];
};

#endif //GR8CPU_R3_IASM_H
