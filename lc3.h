#ifndef LC3_H
#define LC3_H

/* defines */
typedef short int16_t;
#define INT16_T_MAX 32767
typedef unsigned short  uint16_t;
#define UINT16_T_MAX 65535 
//int8_t already defnined
#define INT8_T_MAX 127
typedef unsigned char uint8_t;
#define UINT8_T_MAX 255

#include <assert.h>
#include <stdbool.h>

/* Memory */
uint16_t memory[UINT16_T_MAX];

/* Registers 
 * R0 - R7 General purpose
 * RPC - program coutner */
enum {
	R_0 = 0,
	R_1,
	R_2,
	R_3,
	R_4,
	R_5,
	R_6,
	R_7,
	R_PC,
	R_COND,
	R_COUNT //not actually a register
};

uint16_t registers[R_COUNT];

/* Instructions */
enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

/* Condition Flags */
enum {
	FL_POS = 1 << 0, 
	FL_ZRO = 1 << 1,
	FL_NEG = 1 << 2
};

/* Memory read/write */
uint16_t mem_read(uint16_t loc);

uint16_t mem_write(uint16_t loc, uint16_t val);

/* Instructions */
bool lc3_add(uint16_t instr);
bool lc3_ldi(uint16_t instr);
bool lc3_and(uint16_t instr);
bool lc3_br(uint16_t instr);
bool lc3_jmp_ret(uint16_t instr);
bool lc3_jsrr(uint16_t instr);
bool lc3_ld(uint16_t instr);
bool lc3_ldr(uint16_t isntr);
bool lc3_lea(uint16_t instr);
bool lc3_not(uint16_t instr);
bool lc3_st(uint16_t instr);

/* utilitly */
void dump_registers();
void zero_registers();
void zero_memory();

#endif
