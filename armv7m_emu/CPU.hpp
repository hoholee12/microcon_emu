#pragma once
#include "Proxy.hpp"

// full target interpreter for armv7m base
// what this does:
// - it provides cpu-fetch, cpu-execute (actual interpretation happens in Interpreter.cpp), cpu-writeback instructions
// - it contains cpu-state, cpu-registers


// variables

// cpu register
/*
*	r0~r15 (r13: SP, r14: LR, r15: PC)
* 
* 
*/
struct CPU_struct_reg {
	uint32 r[16];	// r13: SP, r14: LR, r15: PC
};
extern struct CPU_struct_reg CPU_var_reg;

#define CPU_write_SP(x) do{CPU_var_reg.r[13] = x; }while(0);
#define CPU_write_LR(x) do{CPU_var_reg.r[14] = x; }while(0);
#define CPU_write_PC(x) do{CPU_var_reg.r[15] = x; }while(0);

// functions

extern void CPU_init(uint32 pc_pos);

// execute one cycle
extern void CPU_fetch();

