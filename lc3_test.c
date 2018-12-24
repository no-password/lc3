#include "lc3.h"
#include <stdio.h>
#include <assert.h>
#include <signal.h>

void before() {
	zero_registers();
	zero_memory();
}

/**
 * Test r+r add and r+imm add
 */
void add_test() {
	/* r0 = dest
	 * r1 = 1
	 * r2 = 3
	 */
	const uint16_t rr_pos = 0x1042; //add r0 = r1 + r2
	const uint16_t rr_pos_expected = 4;
	const uint16_t rimm_pos = 0x1062; //add r0 = r1 + imm2
	const uint16_t rimm_pos_expected = 3;

	/* r0 = dest
	 * r1 = 1
	 * r2 = -3
	 */
	const uint16_t rr_neg = 0x1042; //add r0 = r1 + r2
	const int16_t rr_neg_expected = -2;
/*	const uint16_t rimm_neg = 0x1074; //add r0 = r1 + imm-4
	const int16_t rimm_neg_expected = -3;*/
	
	uint16_t pc = 0x3000;
	registers[R_PC] = pc;
	registers[R_1] = 1;
	registers[R_2] = 3;
	
	memory[pc] = rr_pos;
	lc3_add(mem_read(registers[R_PC]));
	assert(registers[R_0] == rr_pos_expected);
	
	memory[pc] = rimm_pos;
	lc3_add(mem_read(registers[R_PC]));
	assert(registers[R_0] == rimm_pos_expected);

	registers[R_2] = -3;
	
	memory[pc] = rr_neg;
	lc3_add(mem_read(registers[R_PC]));
	assert(((int16_t) registers[R_0]) == rr_neg_expected);

/*
	memory[pc] = rimm_neg;
	lc3_add(mem_read(registers[R_PC]));
	assert(((int16_t) registers[R_0]) == rimm_neg_expected);*/
}

void ldi_test() {
	const uint16_t ldi_pos_offset_addr = 0x111;
	const uint16_t ldi_pos_addr = 0x3010;
	const uint16_t ldi_pos_expected = 0x222;
	const uint16_t ldi_neg_offset_addr = 0x333;
	const uint16_t ldi_neg_addr = 0x2F10;
	const uint16_t ldi_neg_expected = 0x444;
	const uint16_t ldi_pos = 0xA010; /* positive 0x10 offset from RPC, value should be 0x111 */
	const uint16_t ldi_neg = 0xA110; /* negative 0x10 offset from RPC, value should be 0x222 */
	
	registers[R_PC] = 0x3000;
	mem_write(ldi_pos_addr, ldi_pos_offset_addr);
	mem_write(ldi_pos_offset_addr, ldi_pos_expected);
	mem_write(ldi_neg_addr, ldi_neg_offset_addr);
	mem_write(ldi_neg_offset_addr, ldi_neg_expected);

	
	lc3_ldi(ldi_pos);
	assert(registers[R_0] == ldi_pos_expected);
	printf("pass - pos\n");
	lc3_ldi(ldi_neg);
	assert(registers[R_0] == ldi_neg_expected);
	printf("pass - neg\n");
}

void and_test() {
	/* reg test:
	 * 0101 & 0011 = 0001
	 * imm test:
	 * 0110 & 0101 = 0100
	 */
	const uint16_t and_rr_instr = 0x5042; // r0 = r1 & r2
	const uint16_t and_rr_expected = 0x1;
	const uint16_t and_rimm_instr = 0x5065; // r0 = r1 & imm5
	const uint16_t and_rimm_expected = 0x4;

	registers[R_1] = 0x5;
	registers[R_2] = 0x3;
	lc3_and(and_rr_instr);
	assert(registers[R_0] == and_rr_expected);
	printf("pass - rr\n");

	registers[R_1] = 0x6;
	lc3_and(and_rimm_instr);
	assert(registers[R_0] == and_rimm_expected);
	printf("pass - rimm\n");
}

void br_test() {
	/* branch pos offfset:
	 * rpc = 0x3000, offset = 0x010, expecetd_rpc = 0x3010
	 * branch neg offset:
	 * rpc = 0x3050, offset = -0x10, expected_rpc = 0x3040
	 */

	const uint16_t br_pos_instr		= 0x10;
	const uint16_t brn_pos_instr    = 0x810;
	const uint16_t brz_pos_instr    = 0x410;
	const uint16_t brp_pos_instr    = 0x210;
	//const uint16_t brzp_pos_instr   = 0x610;
	//const uint16_t brnp_pos_instr   = 0xA10;
	//const uint16_t brnz_pos_instr   = 0xC10;
	//const uint16_t brnzp_pos_instr  = 0xE10;
	const uint16_t pos_expected = 0x3010;
	//const uint16_t br_neg_instr		= 0x1F0;
	//const uint16_t brn_neg_instr    = 0x9F0;
	const uint16_t brz_neg_instr    = 0x5F0;
	const uint16_t brp_neg_instr    = 0x3F0;
	const uint16_t brzp_neg_instr   = 0x7F0;
	const uint16_t brnp_neg_instr   = 0xBF0;
	const uint16_t brnz_neg_instr   = 0xDF0;
	const uint16_t brnzp_neg_instr  = 0xFF0;
	const uint16_t neg_expected = 0x3040;

	registers[R_PC] = 0x3000;
	lc3_br(br_pos_instr);
	assert(registers[R_PC] == pos_expected);
	printf("pass - br pos\n");

	registers[R_PC] = 0x3000;
	registers[R_COND] = 0x1;
	lc3_br(brn_pos_instr);
	assert(registers[R_PC] == pos_expected);
	printf("pass - brn pos\n");

	registers[R_PC] = 0x3000;
	registers[R_COND] = 0x1;
	lc3_br(brz_pos_instr);
	assert(registers[R_PC] == pos_expected);
	printf("pass - brz pos\n");
	
	registers[R_PC] = 0x3000;
	registers[R_COND] = 0x1;
	lc3_br(brp_pos_instr);
	assert(registers[R_PC] == pos_expected);
	printf("pass - brp pos\n");
	
	registers[R_PC] = 0x3050;
	lc3_br(brzp_neg_instr);
	assert(registers[R_PC] == neg_expected);
	printf("pass - br neg\n");
	
	registers[R_PC] = 0x3050;
	lc3_br(brnp_neg_instr);
	assert(registers[R_PC] == neg_expected);
	printf("pass - brn neg\n");
	
	registers[R_PC] = 0x3050;
	lc3_br(brz_neg_instr);
	assert(registers[R_PC] == neg_expected);
	printf("pass - brz neg\n");
	
	registers[R_PC] = 0x3050;
	lc3_br(brp_neg_instr);
	assert(registers[R_PC] == neg_expected);
	printf("pass - brp neg\n");
	
	registers[R_PC] = 0x3050;
	lc3_br(brzp_neg_instr);
	assert(registers[R_PC] == neg_expected);
	printf("pass - brzp neg\n");
	
	registers[R_PC] = 0x3050;
	lc3_br(brnp_neg_instr);
	assert(registers[R_PC] == neg_expected);
	printf("pass - brnp neg\n");
	
	registers[R_PC] = 0x3050;
	lc3_br(brnz_neg_instr);
	assert(registers[R_PC] == neg_expected);
	printf("pass - brnz neg\n");
	
	registers[R_PC] = 0x3050;
	lc3_br(brnzp_neg_instr);
	assert(registers[R_PC] == neg_expected);
	printf("pass - brnzp neg\n");
}

void jmp_ret_test() {
	/*
	 * jump to address in r7
	 */
	const uint16_t jmp_ret_instr = 0xC1C0;
	const uint16_t addr_expected = 0x2000;

	registers[R_PC] = 0x3000;
	registers[R_7] = 0x2000;
	lc3_jmp_ret(jmp_ret_instr);
	assert(registers[R_PC] == addr_expected);
}

void jsrr_test() {
	/*
	 * jsr_pos: start at 0x3000, jump  to 0x3000 + 0x10
	 * jsr_neg: start at 0x3010, jump  to 0x3010 - 0x10
	 * jsrr: start at 0x3000,jump to 0x4000 (r0)
	 */

	const uint16_t jsr_pos_instr = 0x4810;
	const uint16_t jsr_pos_rpc_expected = 0x3010;
	const uint16_t jsr_pos_r7_expected = 0x3000;
	const uint16_t jsr_neg_instr = 0x4FF0;
	const uint16_t jsr_neg_rpc_expected = 0x3000;
	const uint16_t jsr_neg_r7_expected = 0x3010;
	const uint16_t jsrr_instr = 0x4000;
	const uint16_t jsrr_rpc_expected = 0x4000;
	const uint16_t jsrr_r7_expected = 0x3000;

	registers[R_PC] = 0x3000;
	registers[R_0] = 0x4000;

	lc3_jsrr(jsr_pos_instr);
	dump_registers();
	assert(registers[R_PC] == jsr_pos_rpc_expected);
	assert(registers[R_7] == jsr_pos_r7_expected);
	printf("pass - jsr pos\n");
	
	lc3_jsrr(jsr_neg_instr);
	assert(registers[R_PC] == jsr_neg_rpc_expected);
	assert(registers[R_7] == jsr_neg_r7_expected);
	printf("pass - jsr neg\n");

	lc3_jsrr(jsrr_instr);
	assert(registers[R_PC] == jsrr_rpc_expected);
	assert(registers[R_7] == jsrr_r7_expected);
	printf("pass - jsrr\n");
}

void ld_test() {
	/*
	 * memory[0x3000] = -1
	 * memory[0x3020] = 10
	 * read value into r1
	 */
	const uint16_t ld_pos_instr = 0x2210;
	const uint16_t ld_pos_expected = 10;
	const uint16_t ld_neg_instr = 0x23F0;
	const int16_t ld_neg_expected = -1;

	registers[R_PC] = 0x3010;
	mem_write(0x3020, ld_pos_expected);
	mem_write(0x3000, (uint16_t) ld_neg_expected);

	lc3_ld(ld_pos_instr);
	dump_registers();
	assert(registers[R_1] == ld_pos_expected);
	assert(registers[R_COND] == FL_POS);
	printf("pass - pos\n");

	lc3_ld(ld_neg_instr);
	int16_t val = (int16_t) registers[R_1];
	assert(val == ld_neg_expected);
	assert(registers[R_COND] == FL_NEG);
	printf("pass - neg\n");
}
/*
void ldi_test() {
	* rpc = 0x3000
	 * ldi_pos: offset = +0x10
	 * memory[0x3020] = 0x2000
	 * memory[0x2000] = -12;
	 * ldi_neg: offset = -0x10
	 * memory[0x3000] = 0x2500
	 * memory[0x2500] = 12;
	 *

	const uint16_t ldi_pos_instr = 0xA210; //ldi r1,+LABEL
	const int16_t ldi_pos_expected = -12;
	const uint16_t ldi_neg_instr = 0xA319; //ldi r1,-LABEL
	const int16_t ldi_neg_expecetd = 12;

	mem_write(0x3020, 0x2000);
	mem_write(0x2000, (uint16_t) ldi_pos_expected);
	mem_write(0x3000, 0x2500);
	mem_write(0x2500, (uint16_t) ldi_neg_expected);

	registers[R_PC] = 0x3000;
	lc3_ldi(ldi_pos_instr);
	assert(((int16_t)  registers[R_1]) == ldi_pos_expected);
	assert(registers[R_COND] == FL_NEG);
	printf("pass - ldi_pos\n");

	ld3_ldi(ldi_neg_instr);
	assert(((int16_t) registers[R_1]) == ldi_neg_expeceted);
	assert(registers[R_COND] == FL_POS);
	printf("pass - ldi_neg\n");
}*/

void ldr_test() {
	/*
	 * baseR = r1
	 * destR = r0
	 * registers[r1] = 0x3010
	 * memory[0x3020] = -12;
	 * memory[0x3000] = 12;
	 */
	const uint16_t ldr_pos_instr = 0x6050; //ldr r0,r1,imm10
	const int16_t ldr_pos_expected = -12;
	const uint16_t ldr_neg_instr = 0x6070; //ldr r0,r1,imm-10
	const int16_t ldr_neg_expected = 12;
	int16_t val = 0;

	mem_write(0x3020, (uint16_t) -12);
	mem_write(0x3000, (uint16_t) 12);
	registers[R_1] = 0x3010;

	lc3_ldr(ldr_pos_instr);
	val = (int16_t) registers[R_0];
	assert(val == ldr_pos_expected);
	assert(registers[R_COND] == FL_NEG);
	printf("pass - pos\n");

	lc3_ldr(ldr_neg_instr);
	val = (int16_t) registers[R_0];
	assert(val == ldr_neg_expected);
	assert(registers[R_COND] == FL_POS);
}

void lea_test() {
	/*
	 * rpc = 0x3010;
	 * dest_reg = r1
	 */
	const uint16_t lea_pos_instr = 0xE210; //lea r1,+LABEL
	const uint16_t lea_pos_expected = 0x3020;
	const uint16_t lea_neg_instr = 0xE3F0; //lea r1,-LABEL
	const uint16_t lea_neg_expected = 0x3000;

	mem_write(0x3020, (uint16_t) lea_pos_expected);
	mem_write(0x3000, (uint16_t) lea_neg_expected);
	registers[R_PC] = 0x3010;

	lc3_lea(lea_pos_instr);
	assert(registers[R_1] == lea_pos_expected);
	assert(registers[R_COND] == FL_POS);
	printf("pass - pos\n");
	
	lc3_lea(lea_neg_instr);
	assert(registers[R_1] == lea_neg_expected);
	assert(registers[R_COND] == FL_POS);
	printf("pass - neg\n");
}

void not_test() {
	const uint16_t not_instr = 0x907F; //not r0,r1
	registers[R_1] = 12;

	lc3_not(not_instr);
	assert(registers[R_0] == (uint16_t) ~12);
	assert(registers[R_COND] == FL_NEG);
	printf("pass - neg\n");

	registers[R_1] = -8;
	lc3_not(not_instr);
	assert(registers[R_0] == (uint16_t) ~(-8));
	assert(registers[R_COND] == FL_POS);
	printf("pass - pos\n");
}

void st_test() {
	/**
	 * rpc = 0x3010
	 * memory[0x3020] = 0x1111;
	 * memory[0x3000] = 0x2222;
	 */
	const uint16_t st_instr_pos = 0x3210; //st r1,imm10
	const uint16_t st_instr_neg = 0x33F0; //st r1,imm-10
	const uint16_t pos_expected = 0x1111;
	const uint16_t neg_expected = 0x2222;
	registers[R_PC] = 0x3010;

	registers[R_1] = pos_expected;
	lc3_st(st_instr_pos);
	assert(mem_read(0x3020) == pos_expected);
	printf("pass - pos\n");

	registers[R_1] = neg_expected;
	lc3_st(st_instr_neg);
	assert(((int16_t) mem_read(0x3000) == neg_expected));
	printf("pass - neg\n");
}

int main(int argc, char** argv) {
	before();
	printf("Begin: add_test\n");
	add_test();
	printf("PASSED: add_test\n");
	
	before();
	printf("Begin: ldi_test\n");
	ldi_test();
	printf("PASSED: ldi_test\n");
	
	before();
	printf("Begin: and_test\n");
	and_test();
	printf("PASSED: and_test\n");

	before();
	printf("Begin: br_test\n");
	br_test();
	printf("PASSED: br_test\n");
	
	before();
	printf("Begin: jmp_ret_test\n");
	br_test();
	printf("PASSED: jmp_ret_test\n");

	before();
	printf("Begin: jsrr_test\n");
	jsrr_test();
	printf("PASSED: jsrr_test\n");

	before();
	printf("Begin: ld_test\n");
	ld_test();
	printf("PASSED: ld_test\n");

	before();
	printf("Begin: ldr_test\n");
	ldr_test();
	printf("PASSED: ldr_test\n");

	before();
	printf("Begin: lea_test\n");
	lea_test();
	printf("PASSED: lea_test\n");

	before();
	printf("Begin: not_test\n");
	not_test();
	printf("PASSED: not_test\n");

	before();
	printf("Begin: st_test\n");
	st_test();
	printf("PASSED: st_test\n");
	return 0;
}
