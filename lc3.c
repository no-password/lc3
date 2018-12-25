#include "lc3.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

bool running = true;

/* Unix stuff */
struct termios original_tio;

void disable_input_buffering() {
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal) {
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

void read_img_file(FILE* file);
int read_image(const char* image_path);

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Need executable\n");
		exit(EXIT_FAILURE);
	}

	int i;
	for (i = 0; i < argc; i++) {
		if (!read_image(argv[i]))  {
			printf("failed to load image file: %s\n", argv[i]);
			exit(EXIT_FAILURE);
		}
	}

	signal(SIGINT, handle_interrupt);
	disable_input_buffering();

	while (running) {
		uint16_t instr = mem_read(registers[R_PC]++);
		uint8_t op = instr >> 12;
		//printf("op = %X\n", op);
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
			case OP_AND:
				lc3_and(instr);
				break;
			case OP_NOT:
				lc3_not(instr);
				break;
			case OP_BR:
				lc3_br(instr);
				break;
			case OP_JMP:
				lc3_jmp_ret(instr);
				break;
			case OP_JSR:
				lc3_jsrr(instr);
				break;
			case OP_LEA:
				lc3_lea(instr);
				break;
			case OP_LD:
				lc3_ld(instr);
				break;
			case OP_LDI:
				lc3_ldi(instr);
				break;
			case OP_LDR:
				lc3_ldr(instr);
				break;
			case OP_ST:
				lc3_st(instr);
				break;
			case OP_STR:
				lc3_str(instr);
				break;
			case OP_TRAP:
				lc3_trap(instr);
				break;
			default:
				restore_input_buffering();
				fprintf(stderr, "Invalid instruction: %x\n", instr);
				exit(EXIT_FAILURE);
		}
	}
	
	restore_input_buffering();

	return 0;
}

uint16_t swap16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

void read_image_file(FILE* file) {
    /* the origin tells us where in memory to place the image */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    /* we know the maximum file size so we only need one fread */
    uint16_t max_read = UINT16_T_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    /* swap to little endian */
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}

int read_image(const char* image_path) {
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0; };
    read_image_file(file);
    fclose(file);
    return 1;
}

/* Memory read/write */
uint16_t check_key() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

uint16_t mem_read(uint16_t loc) {
	//assert(loc > 0 && loc <= UINT16_T_MAX);
	if (loc == MR_KBSR) {
		if (check_key()) {
			memory[MR_KBSR] = (1 << 15);
			memory[MR_KBDR] = getchar();
		} else {
			memory[MR_KBSR] = 0;
		}
	}

	return memory[loc];
}

uint16_t mem_write(uint16_t loc, uint16_t val) {
	//assert(loc > 0 && loc <= UINT16_T_MAX);
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

	update_flags(dst_reg);

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
/*
	uint16_t n = (instr >> 11) & 0x1;
	uint16_t z = (instr >> 10) & 0x1;
	uint16_t p = (instr >> 9) & 0x1;
	int16_t pc_offset = sign_extend(instr & 0x1FF, 9);

	if (! (n | z | p)) {
		registers[R_PC] = registers[R_PC] + pc_offset;
	} else if ((n & registers[R_COND]) | (z & registers[R_COND]) | (p & registers[R_COND])) {
		registers[R_PC] = registers[R_PC] + pc_offset;
	}
*/
	uint16_t pc_offset = sign_extend((instr) & 0x1FF, 9);
	uint16_t flag = (instr >> 9) & 0x7;
	if (flag & registers[R_COND]) {
		registers[R_PC] += pc_offset;
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

bool lc3_sti(uint16_t instr) {
	uint16_t src_reg = (instr >> 9) & 0x7;
	int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	mem_write(mem_read(registers[R_PC] + pc_offset), registers[src_reg]);

	return true;
}

bool lc3_str(uint16_t instr) {
	uint16_t base_reg = (instr >> 6) & 0x7;
	uint16_t src_reg = (instr >> 9) & 0x7;
	int16_t offset = sign_extend(instr & 0x3F, 6);

	mem_write(registers[base_reg] + offset, registers[src_reg]);

	return true;
}

static void lc3_getc() {
	registers[R_0] = (uint16_t) getchar();
}

static void lc3_out() {
	putc((char) registers[R_0], stdout);
	fflush(stdout);
}

static void lc3_in() {
	lc3_getc();
}

static void lc3_putsp() {
	uint16_t* c = memory + registers[R_0];
	while (*c) {
		char c1 = (*c) & 0xFF;
		putc(c1, stdout);

		char c2 = (*c) >> 8;
		if (c2) {
			putc(c2, stdout);
		}

		fflush(stdout);
	}
}

static void lc3_halt() {
	running = false;
}

static void lc3_puts() {
	//printf("lc3_puts\n");
	uint16_t* c = memory + registers[R_0];
	while (*c) {
		putc((char)*c, stdout);
		++c;
	}

	fflush(stdout);
}

bool lc3_trap(uint16_t instr) {
	switch (instr & 0xFF) {
		case TRAP_GETC:
			lc3_getc();
			break;
		case TRAP_OUT:
			lc3_out();
			break;
		case TRAP_PUTS:
			lc3_puts();
			break;
		case TRAP_IN:
			lc3_in();
			break;
		case TRAP_PUTSP:
			lc3_putsp();
			break;
		case TRAP_HALT:
			lc3_halt();
			break;
		default:
			printf("Invalid trap code: 0x%X\n", instr & 0xFF);
			exit(EXIT_FAILURE);
	}

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
