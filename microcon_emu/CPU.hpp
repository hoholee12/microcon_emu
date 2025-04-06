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

struct APSR_t{
	uint32 N			: 1;
	uint32 Z			: 1;
	uint32 C			: 1;
	uint32 V			: 1;
	uint32 Q			: 1;
	uint32 notmine1		: 7;
	uint32 GE3			: 1;
	uint32 GE2			: 1;
	uint32 GE1			: 1;
	uint32 GE0			: 1;
	uint32 notmine0		: 16;
};

struct IPSR_t{
	uint32 notmine0		: 23;
	uint32 exception	: 9;
};

struct EPSR_t{
	uint32 notmine2		: 5;
	uint32 ICIT1		: 2;
	uint32 T			: 1;
	uint32 notmine1		: 8;
	uint32 ICIT0		: 6;
	uint32 notmine0		: 10;
};

union xPSR_t {
	struct APSR_t APSR;
	struct IPSR_t IPSR;
	struct EPSR_t EPSR;
};

struct CONTROL_t {
	uint32 nPRIV		: 1;
	uint32 SPSEL		: 1;
	uint32 FPCA			: 2;
	uint32 notmine0		: 28;
};

struct CPU_struct_reg {
	uint32 R[16];	// r13: SP, r14: LR, r15: PC
	xPSR_t xPSR;	// <APSR.GE is for dsp extension only>
					// contains APSR, IPSR, EPSR

	uint32 MSP, PSP;	//main stack (for kernel use probably), process stack (user)

	uint32 fpscr;	// for floating point control status reg. n, z, c, v is in same bit index as apsr.

	uint32 PRIMASK, FAULTMASK, BASEPRI;	// special purpose mask regs (if unprivileged, they are RAZ/WI (read as zero / write ignored))
				// PRIMASK: exception. setting it to 1 raises execution priority to 0 (theres only 1 bit)
				// BASEPRI: for priority level (beginning 8bits are used) (used for masking any interrupts that has less priority than this limit) (0 disables the feature)
				// FAULTMASK: setting it to 1 raises execution priority to -1(hardfault) (only privilege less than -1 can touch this value(as in actually modify the register content; we're not talking about cpsid/cpsie); aka no one can; clears to 0 when returning from any exception(except Non-Maskable Interrupt)) (theres only 1 bit)
				//
				// cpsid f -> FAULTMASK:1
				// cpsie f -> FAULTMASK:0
				// cpsid i -> PRIMASK:1
				// cpsie i -> PRIMASK:0

	CONTROL_t CONTROL;
				// control register
				// nPRIV(0): 0 -> thread mode has privilege, 1 -> thread mode doesnt have privilege
				// SPSEL(1): 0 -> use MSP as current stack, 1 -> use PSP as current stack (in thread mode); value cannot be changed in handler mode
				// FPCA(2): 0 -> FP extension not active, 1 -> FP extension active

	uint32 CCR;	// control register (?)
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

extern uint32 CPU_op_vect[0xFF]; // 8 bits
extern uint32 CPU_op_vect_ext[0xFF]; // extend to 16 bits

enum CPU_op_enum {
	NOP,
	/* shift op */
	LSL,
	LSR,
	ASR,
	ROR,
	RRX,

	/* add op */
	ADC_imm,
	ADC_reg,
	ADD_imm,
	ADD_reg,
	ADD_sp_imm,
	ADD_sp_reg,

	/* TODO... */








};

extern struct CPU_struct_reg CPU_var_reg;


// functions

extern void CPU_init(uint32 pc_pos);

// execute one cycle
extern void CPU_fetch();

