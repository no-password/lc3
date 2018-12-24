#include "lc3.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

int run(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Need executable\n");
		exit(EXIT_FAILURE);
	}

#ifndef NDEBUG
	zero_memory();
	zero_registers();
#endif

	
	bool running = true;
	while (running) {
#ifndef NDEBUG
		dump_registers();
#endif
		uint16_t instr = mem_read(registers[R_PC]++);
		uint8_t op = instr >> 12;
		
		switch(op) {
			case OP_ADD:
				if (!lc3_add(instr)) {
					fprintf(stderr, "Invalid add instruction: %x\n", instr);
					exit(EXIT_FAILURE);
				}
				break;
			case OP_RTI:
				running = false;
				break;
			default:
				fprintf(stderr, "Invalid instruction: %x\n", instr);
				exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}

/* Memory read/write */
uint16_t mem_read(uint16_t loc) {
	assert(loc > 0 && loc <= UINT16_T_MAX);
	return memory[loc];
}

uint16_t mem_write(uint16_t loc, uint16_t val) {
	assert(loc > 0 && loc <= UINT16_T_MAX);
	memory[loc] = val;
	return memory[loc];
}

/* sign extend */
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

/* Update flags */
void update_flags(uint16_t r)
{
    if (registers[r] == 0)
    {
        registers[R_COND] = FL_ZRO;
    }
    else if (registers[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
        registers[R_COND] = FL_NEG;
    }
    else
    {
        registers[R_COND] = FL_POS;
    }
}

/* Instructions */
bool lc3_add(uint16_t instr) {
	uint16_t dst_reg = (instr >> 9) & 0x7;
	uint16_t src_reg = (instr >> 6) & 0x7;
	uint16_t is_imm  = (instr >> 5) & 0x1;

	if (is_imm) {
		int16_t imm = sign_extend(instr & 0x1F, 5);
		registers[dst_reg] = registers[src_reg] + imm;
	} else {
		uint16_t src_reg2 = instr & 0x7;
		registers[dst_reg] = ((int16_t) registers[src_reg]) + ((int16_t) registers[src_reg2]);
	}

	return true;
}

bool lc3_ldi(uint16_t instr) {
	uint16_t dst_reg = (instr >> 9) & 0x7;
	int16_t pc_offset = (int16_t) sign_extend(instr & 0x1FF, 9);
	registers[dst_reg] = mem_read(mem_read(registers[R_PC] + pc_offset));
	update_flags(dst_reg);

	return true;
}

bool lc3_and(uint16_t instr) {
	uint16_t dst_reg = (instr >> 9) & 0x7;
	uint16_t src_reg = (instr >> 6) & 0x7;
	uint16_t is_imm  = (instr >> 5) & 0x1;

	if (is_imm) {
		int16_t imm = sign_extend(instr & 0x1F, 5);
		registers[dst_reg] = registers[src_reg] & imm;
	} else {
		uint16_t src_reg2 = (instr & 0x7);
		registers[dst_reg] = registers[src_reg] & registers[src_reg2];
	}
	update_flags(dst_reg);

	return true;
}

bool lc3_br(uint16_t instr) {
	uint16_t n = (instr >> 11) & 0x1;
	uint16_t z = (instr >> 10) & 0x1;
	uint16_t p = (instr >> 9) & 0x1;
	int16_t pc_offset = sign_extend(instr & 0x1FF, 9);

	if (! (n | z | p)) {
		registers[R_PC] = registers[R_PC] + pc_offset;
	} else if ((n & registers[R_COND]) | (z & registers[R_COND]) | (p & registers[R_COND])) {
		registers[R_PC] = registers[R_PC] + pc_offset;
	}

	return true;
}

bool lc3_jmp_ret(uint16_t instr) {
	uint16_t base_reg = (instr >> 6) & 0x7;
	registers[R_PC] = registers[base_reg];
	return true;
}

bool lc3_jsrr(uint16_t instr) {
	registers[R_7] = registers[R_PC];
	uint16_t is_absolute = (instr >> 11) & 0x1;
	if (is_absolute) {
		int16_t pc_offset = sign_extend(instr & 0x7FF, 11);
		registers[R_PC] += pc_offset;
	} else {
		uint16_t base_reg = (instr >> 6) & 0x7;
		registers[R_PC] = registers[base_reg];
	}
	return true;
}

bool lc3_ld(uint16_t instr) {
	uint16_t dest_reg = (instr >> 9) & 0x7;
	int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	registers[dest_reg] = mem_read(registers[R_PC] + pc_offset);
	update_flags(dest_reg);

	return true;
}

bool lc3_ldr(uint16_t instr) {
	uint16_t dest_reg = (instr >> 9) & 0x7;
	uint16_t base_reg = (instr >> 6) & 0x7;
	int16_t offset = sign_extend(instr & 0x3F, 6);
	registers[dest_reg] = mem_read(registers[base_reg] + offset);
	update_flags(dest_reg);

	return true;
}

bool lc3_lea(uint16_t instr) {
	uint16_t dest_reg = (instr >> 9) & 0x7;
	int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	registers[dest_reg] = registers[R_PC] + pc_offset;
	update_flags(dest_reg);

	return true;
}

bool lc3_not(uint16_t instr) {
	uint16_t dest_reg = (instr >> 9) & 0x7;
	uint16_t src_reg = (instr >> 6) & 0x7;

	registers[dest_reg] = ~(registers[src_reg]);
	update_flags(dest_reg);

	return true;
}

bool lc3_st(uint16_t instr) {
	uint16_t src_reg = (instr >> 9) & 0x7;
	int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	mem_write(pc_offset + registers[R_PC], registers[src_reg]);

	return true;
}

/* Utility */
void dump_registers() {
	int i;
	for (i = 0; i <= 7; i++) {
		printf("r%d\t0x%X\n", i, registers[i]);
	}

	printf("rpc\t0x%X\n", registers[R_PC]);
	printf("rcond\t0x%X\n", registers[R_COND]);
}

void zero_registers() {
	printf("Zero-ing registers\n");
	uint16_t i;
	for (i = 0; i < R_COUNT; i++) {
		registers[i] = 0;
	}
}

void zero_memory() {
	printf("Zero-ing memory\n");
	uint16_t i = 0;
	for (i = 0; i < UINT16_T_MAX; i++) {
		memory[i] = 0;
	}
}
