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

typedef enum CPU_op_enum {
    NOP,

    // --- Shift operations ---
    LSL_imm,    // logical shift left by immediate
    LSL_reg,    // logical shift left by register
    LSR_imm,    // logical shift right by immediate
    LSR_reg,    // logical shift right by register
    ASR_imm,    // arithmetic shift right by immediate
    ASR_reg,    // arithmetic shift right by register
    ROR_imm,    // rotate right by immediate
    ROR_reg,    // rotate right by register
    RRX,        // rotate right with extend

    // --- Move / Negate ---
    MOV_imm,    // MOV Rd, #imm
    MOV_reg,    // MOV Rd, Rm
    MOV_SP_imm, // MOV Rd, [SP, #imm] (alias)
    MOV_SP_reg, // MOV Rd, [SP, Rm]
    MVN_imm,    // MVN Rd, #imm
    MVN_reg,    // MVN Rd, Rm

    // --- Add / Sub / ADC / SBC ---
    ADC_imm,    // ADC Rd, Rn, #imm
    ADC_reg,    // ADC Rd, Rn, Rm
    ADD_imm,    // ADD Rd, Rn, #imm
    ADD_reg,    // ADD Rd, Rn, Rm
    ADD_SP_imm, // ADD SP, SP, #imm
    ADD_SP_reg, // ADD SP, SP, Rm
    SBC_imm,    // SBC Rd, Rn, #imm
    SBC_reg,    // SBC Rd, Rn, Rm
    SUB_imm,    // SUB Rd, Rn, #imm
    SUB_reg,    // SUB Rd, Rn, Rm
    SUB_SP_imm, // SUB SP, SP, #imm
    SUB_SP_reg, // SUB SP, SP, Rm

    // --- Logical operations ---
    AND_imm,    // AND Rd, Rn, #imm
    AND_reg,    // AND Rd, Rn, Rm
    ORR_imm,    // ORR Rd, Rn, #imm
    ORR_reg,    // ORR Rd, Rn, Rm
    EOR_imm,    // EOR Rd, Rn, #imm
    EOR_reg,    // EOR Rd, Rn, Rm
    BIC_imm,    // BIC Rd, Rn, #imm
    BIC_reg,    // BIC Rd, Rn, Rm

    // --- Compare & Test ---
    CMP_imm,    // CMP Rn, #imm
    CMP_reg,    // CMP Rn, Rm
    CMN_imm,    // CMN Rn, #imm
    CMN_reg,    // CMN Rn, Rm
    TST_imm,    // TST Rn, #imm
    TST_reg,    // TST Rn, Rm

    // --- Load / Store (Word) ---
    LDR_imm,    // LDR Rt, [Rn, #imm]
    LDR_reg,    // LDR Rt, [Rn, Rm]
    LDR_SP_imm, // LDR Rt, [SP, #imm]
    LDR_SP_reg, // LDR Rt, [SP, Rm]
    LDR_literal,// LDR Rt, =label

    STR_imm,    // STR Rt, [Rn, #imm]
    STR_reg,    // STR Rt, [Rn, Rm]
    STR_SP_imm, // STR Rt, [SP, #imm]
    STR_SP_reg, // STR Rt, [SP, Rm]

    // --- Load / Store (Byte / Half / Dual) ---
    LDRB_imm,   // LDRB Rt, [Rn, #imm]
    LDRB_reg,   // LDRB Rt, [Rn, Rm]
    STRB_imm,   // STRB Rt, [Rn, #imm]
    STRB_reg,   // STRB Rt, [Rn, Rm]

    LDRH_imm,   // LDRH Rt, [Rn, #imm]
    LDRH_reg,   // LDRH Rt, [Rn, Rm]
    STRH_imm,   // STRH Rt, [Rn, #imm]
    STRH_reg,   // STRH Rt, [Rn, Rm]

    LDRD_imm,   // LDRD Rt, [Rn, #imm]
    LDRD_reg,   // LDRD Rt, [Rn, Rm]
    STRD_imm,   // STRD Rt, [Rn, #imm]
    STRD_reg,   // STRD Rt, [Rn, Rm]

    // --- Multiply & Divide ---
    MUL_reg,    // MUL Rd, Rm, Rs
    MLA_reg,    // MLA Rd, Rm, Rs, Ra
    MLS_reg,    // MLS Rd, Rm, Rs, Ra (ARMv7-M optional)
    UMULL_reg,  // UMULL RdLo, RdHi, Rm, Rs
    UMLAL_reg,  // UMLAL RdLo, RdHi, Rm, Rs
    SMULL_reg,  // SMULL RdLo, RdHi, Rm, Rs
    SMLAL_reg,  // SMLAL RdLo, RdHi, Rm, Rs
    UDIV_reg,   // UDIV Rd, Rn, Rm (ARMv7-M division)
    SDIV_reg,   // SDIV Rd, Rn, Rm (ARMv7-M division)

    // --- Stack operations ---
    PUSH,
    POP,

    // --- Branches & Control ---
    B_cond,     // B<cond> label
    B_uncond,   // B label
    BL,         // BL label
    BX,         // BX Rm
    BLX,        // BLX Rm

    // --- System / Barrier / Exception ---
    SVC,        // SVC #imm
    BKPT,       // BKPT #imm
    DMB,        // Data Memory Barrier
    DSB,        // Data Synchronization Barrier
    ISB,        // Instruction Synchronization Barrier

} CPU_op_enum;

extern struct CPU_struct_reg CPU_var_reg;


// functions

extern void CPU_init(uint32 pc_pos);

// execute one cycle
extern void CPU_fetch();

