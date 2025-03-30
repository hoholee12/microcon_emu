#pragma once
#include "Proxy.hpp"

// full target interpreter for armv7m base
// what this does:
// - it provides cpu-fetch, cpu-execute (actual interpretation happens in Interpreter.cpp), cpu-writeback instructions
// - it contains cpu-state, cpu-registers


/*
* 
* TODO: cycle-accurate simulator with a pinpoint accuracy on pipeline stalls
* (branch prediction included)
*/


// variables

// cpu register
/*
*	r0~r15 (r13: SP, r14: LR, r15: PC)
*	apsr (n: negative(1 if result is negative number), z: zero(1 if zero, 0 if not), c: carry(unsigned overflow),
*   v: overflow (signed overflow - probably bad), q: for DSP instructions- not supported)
* 
*/
struct CPU_struct_reg {
	uint32 r[16];	// r13: SP, r14: LR, r15: PC
	uint32 apsr;	// n(31), z(30), c(29), v(28), q(27), 0, 0, 0, 0, 0, 0, 0, ge2, ge1, ge0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
					// <ge is for dsp extension only>
	uint32 fpscr;	// for floating point control status reg. n, z, c, v is in same bit index as apsr.

	uint32 exception;	// exception number. should be 0 on running.
	uint32 primask, faultmask, basepri[16];

	uint32 ccr;	// control register
				// BP(18): enable branch prediction
				// IC(17): enable icache
				// DC(16): enable dcache
				// STKALIGN(9): 0 - 4byte sp align, 1 - 8byte sp align <on exception entry>
				// BFHFNMIGN(8): 0 - fault causes lockup, 1 - fault ignored
				// DIV_0_TRP(4): 0 - divby0 dont trap, 1 - traps
				// UNALIGN_TRP(3): 0 - unaligned access dont trap, 1 - traps
				// USERSETMPEND(1): 0 - unprivileged cannot access STIR(software trigger interrupt reg), 1 - can
				// NONBASETHRDENA(0): 0 - attempting to return to thread mode WHILE there are remaining active exceptions will trigger another exception, 1 - can return fine because assume its controlled.
};

extern struct CPU_struct_reg CPU_var_reg;

#define CPU_write_SP(x) do{CPU_var_reg.r[13] = x; }while(0);
#define CPU_write_LR(x) do{CPU_var_reg.r[14] = x; }while(0);
#define CPU_write_PC(x) do{CPU_var_reg.r[15] = x; }while(0);
#define CPU_write_apsr_n(x) do{uint32 temp = (x & 0x1) << 31; CPU_var_reg.apsr |= temp; }while(0);
#define CPU_write_apsr_z(x) do{uint32 temp = (x & 0x1) << 30; CPU_var_reg.apsr |= temp; }while(0);
#define CPU_write_apsr_c(x) do{uint32 temp = (x & 0x1) << 29; CPU_var_reg.apsr |= temp; }while(0);
#define CPU_write_apsr_v(x) do{uint32 temp = (x & 0x1) << 28; CPU_var_reg.apsr |= temp; }while(0);


#define CPU_read_SP() (CPU_var_reg.r[13])
#define CPU_read_LR() (CPU_var_reg.r[14])
#define CPU_read_PC() (CPU_var_reg.r[15])

// functions

extern void CPU_init(uint32 pc_pos);

// execute one cycle
extern void CPU_fetch();

