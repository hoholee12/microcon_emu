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

typedef struct {
    void (*func)(uint32 instr, CPU_struct_reg* reg);
} CPU_op_vect_ext_t;

typedef struct {
    CPU_op_vect_ext_t* op_ext;
} CPU_op_vect_t;

extern CPU_op_vect_t** CPU_op_vect;

/* extracted from DDI0403Ee_arm_v7m_ref chapter 7.7 : instructions in alphabetical order */
typedef enum CPU_op_enum {
    ADC_IMMEDIATE, /* ADC (immediate) */
    ADC_REGISTER, /* ADC (register) */
    ADD_IMMEDIATE, /* ADD (immediate) */
    ADD_REGISTER, /* ADD (register) */
    ADD_SP_PLUS_IMMEDIATE, /* ADD (SP plus immediate) */
    ADD_SP_PLUS_REGISTER, /* ADD (SP plus register) */
    ADR, /* ADR */
    AND_IMMEDIATE, /* AND (immediate) */
    AND_REGISTER, /* AND (register) */
    ASR_IMMEDIATE, /* ASR (immediate) */
    ASR_REGISTER, /* ASR (register) */
    B, /* B */
    BFC, /* BFC */
    BFI, /* BFI */
    BIC_IMMEDIATE, /* BIC (immediate) */
    BIC_REGISTER, /* BIC (register) */
    BKPT, /* BKPT */
    BL, /* BL */
    BLX_REGISTER, /* BLX (register) */
    BX, /* BX */
    CBNZ_CBZ, /* CBNZ, CBZ */
    CDP_CDP2, /* CDP, CDP2 */
    CLREX, /* CLREX */
    CLZ, /* CLZ */
    CMN_IMMEDIATE, /* CMN (immediate) */
    CMN_REGISTER, /* CMN (register) */
    CMP_IMMEDIATE, /* CMP (immediate) */
    CMP_REGISTER, /* CMP (register) */
    CPS, /* CPS */
    CPY, /* CPY */
    CSDB, /* CSDB */
    DBG, /* DBG */
    DMB, /* DMB */
    DSB, /* DSB */
    EOR_IMMEDIATE, /* EOR (immediate) */
    EOR_REGISTER, /* EOR (register) */
    ISB, /* ISB */
    IT, /* IT */
    LDC_LDC2_IMMEDIATE, /* LDC, LDC2 (immediate) */
    LDC_LDC2_LITERAL, /* LDC, LDC2 (literal) */
    LDM_LDMIA_LDMFD, /* LDM, LDMIA, LDMFD */
    LDMDB_LDMEA, /* LDMDB, LDMEA */
    LDR_IMMEDIATE, /* LDR (immediate) */
    LDR_LITERAL, /* LDR (literal) */
    LDR_REGISTER, /* LDR (register) */
    LDRB_IMMEDIATE, /* LDRB (immediate) */
    LDRB_LITERAL, /* LDRB (literal) */
    LDRB_REGISTER, /* LDRB (register) */
    LDRBT, /* LDRBT */
    LDRD_IMMEDIATE, /* LDRD (immediate) */
    LDRD_LITERAL, /* LDRD (literal) */
    LDREX, /* LDREX */
    LDREXB, /* LDREXB */
    LDREXH, /* LDREXH */
    LDRH_IMMEDIATE, /* LDRH (immediate) */
    LDRH_LITERAL, /* LDRH (literal) */
    LDRH_REGISTER, /* LDRH (register) */
    LDRHT, /* LDRHT */
    LDRSB_IMMEDIATE, /* LDRSB (immediate) */
    LDRSB_LITERAL, /* LDRSB (literal) */
    LDRSB_REGISTER, /* LDRSB (register) */
    LDRSBT, /* LDRSBT */
    LDRSH_IMMEDIATE, /* LDRSH (immediate) */
    LDRSH_LITERAL, /* LDRSH (literal) */
    LDRSH_REGISTER, /* LDRSH (register) */
    LDRSHT, /* LDRSHT */
    LDRT, /* LDRT */
    LSL_IMMEDIATE, /* LSL (immediate) */
    LSL_REGISTER, /* LSL (register) */
    LSR_IMMEDIATE, /* LSR (immediate) */
    LSR_REGISTER, /* LSR (register) */
    MCR_MCR2, /* MCR, MCR2 */
    MCRR_MCRR2, /* MCRR, MCRR2 */
    MLA, /* MLA */
    MLS, /* MLS */
    MOV_IMMEDIATE, /* MOV (immediate) */
    MOV_REGISTER, /* MOV (register) */
    MOV_SHIFTED_REGISTER, /* MOV (shifted register) */
    MOVT, /* MOVT */
    MRC_MRC2, /* MRC, MRC2 */
    MRRC_MRRC2, /* MRRC, MRRC2 */
    MRS, /* MRS */
    MSR, /* MSR */
    MUL, /* MUL */
    MVN_IMMEDIATE, /* MVN (immediate) */
    MVN_REGISTER, /* MVN (register) */
    NEG, /* NEG */
    NOP, /* NOP */
    ORN_IMMEDIATE, /* ORN (immediate) */
    ORN_REGISTER, /* ORN (register) */
    ORR_IMMEDIATE, /* ORR (immediate) */
    ORR_REGISTER, /* ORR (register) */
    PKHBT_PKHTB, /* PKHBT, PKHTB */
    PLD_IMMEDIATE, /* PLD (immediate) */
    PLD_LITERAL, /* PLD (literal) */
    PLD_REGISTER, /* PLD (register) */
    PLI_IMMEDIATE_LITERAL, /* PLI (immediate, literal) */
    PLI_REGISTER, /* PLI (register) */
    POP, /* POP */
    PSSBB, /* PSSBB */
    PUSH, /* PUSH */
    QADD, /* QADD */
    QADD16, /* QADD16 */
    QADD8, /* QADD8 */
    QASX, /* QASX */
    QDADD, /* QDADD */
    QDSUB, /* QDSUB */
    QSAX, /* QSAX */
    QSUB, /* QSUB */
    QSUB16, /* QSUB16 */
    QSUB8, /* QSUB8 */
    RBIT, /* RBIT */
    REV, /* REV */
    REV16, /* REV16 */
    REVSH, /* REVSH */
    ROR_IMMEDIATE, /* ROR (immediate) */
    ROR_REGISTER, /* ROR (register) */
    RRX, /* RRX */
    RSB_IMMEDIATE, /* RSB (immediate) */
    RSB_REGISTER, /* RSB (register) */
    SADD16, /* SADD16 */
    SADD8, /* SADD8 */
    SASX, /* SASX */
    SBC_IMMEDIATE, /* SBC (immediate) */
    SBC_REGISTER, /* SBC (register) */
    SBFX, /* SBFX */
    SDIV, /* SDIV */
    SEL, /* SEL */
    SEV, /* SEV */
    SHADD16, /* SHADD16 */
    SHADD8, /* SHADD8 */
    SHASX, /* SHASX */
    SHSAX, /* SHSAX */
    SHSUB16, /* SHSUB16 */
    SHSUB8, /* SHSUB8 */
    SMLABB_SMLABT_SMLATB_SMLATT, /* SMLABB, SMLABT, SMLATB, SMLATT */
    SMLAD_SMLADX, /* SMLAD, SMLADX */
    SMLAL, /* SMLAL */
    SMLALBB_SMLALBT_SMLALTB_SMLALTT, /* SMLALBB, SMLALBT, SMLALTB, SMLALTT */
    SMLALD_SMLALDX, /* SMLALD, SMLALDX */
    SMLAWB_SMLAWT, /* SMLAWB, SMLAWT */
    SMLSD_SMLSDX, /* SMLSD, SMLSDX */
    SMLSLD_SMLSLDX, /* SMLSLD, SMLSLDX */
    SMMLA_SMMLAR, /* SMMLA, SMMLAR */
    SMMLS_SMMLSR, /* SMMLS, SMMLSR */
    SMMUL_SMMULR, /* SMMUL, SMMULR */
    SMUAD_SMUADX, /* SMUAD, SMUADX */
    SMULBB_SMULBT_SMULTB_SMULTT, /* SMULBB, SMULBT, SMULTB, SMULTT */
    SMULL, /* SMULL */
    SMULWB_SMULWT, /* SMULWB, SMULWT */
    SMUSD_SMUSDX, /* SMUSD, SMUSDX */
    SSAT, /* SSAT */
    SSAT16, /* SSAT16 */
    SSAX, /* SSAX */
    SSBB, /* SSBB */
    SSUB16, /* SSUB16 */
    SSUB8, /* SSUB8 */
    STC_STC2, /* STC, STC2 */
    STM_STMIA_STMEA, /* STM, STMIA, STMEA */
    STMDB_STMFD, /* STMDB, STMFD */
    STR_IMMEDIATE, /* STR (immediate) */
    STR_REGISTER, /* STR (register) */
    STRB_IMMEDIATE, /* STRB (immediate) */
    STRB_REGISTER, /* STRB (register) */
    STRBT, /* STRBT */
    STRD_IMMEDIATE, /* STRD (immediate) */
    STREX, /* STREX */
    STREXB, /* STREXB */
    STREXH, /* STREXH */
    STRH_IMMEDIATE, /* STRH (immediate) */
    STRH_REGISTER, /* STRH (register) */
    STRHT, /* STRHT */
    STRT, /* STRT */
    SUB_IMMEDIATE, /* SUB (immediate) */
    SUB_REGISTER, /* SUB (register) */
    SUB_SP_MINUS_IMMEDIATE, /* SUB (SP minus immediate) */
    SUB_SP_MINUS_REGISTER, /* SUB (SP minus register) */
    SVC, /* SVC */
    SXTAB, /* SXTAB */
    SXTAB16, /* SXTAB16 */
    SXTAH, /* SXTAH */
    SXTB, /* SXTB */
    SXTB16, /* SXTB16 */
    SXTH, /* SXTH */
    TBB_TBH, /* TBB, TBH */
    TEQ_IMMEDIATE, /* TEQ (immediate) */
    TEQ_REGISTER, /* TEQ (register) */
    TST_IMMEDIATE, /* TST (immediate) */
    TST_REGISTER, /* TST (register) */
    UADD16, /* UADD16 */
    UADD8, /* UADD8 */
    UASX, /* UASX */
    UBFX, /* UBFX */
    UDF, /* UDF */
    UDIV, /* UDIV */
    UHADD16, /* UHADD16 */
    UHADD8, /* UHADD8 */
    UHASX, /* UHASX */
    UHSAX, /* UHSAX */
    UHSUB16, /* UHSUB16 */
    UHSUB8, /* UHSUB8 */
    UMAAL, /* UMAAL */
    UMLAL, /* UMLAL */
    UMULL, /* UMULL */
    UQADD16, /* UQADD16 */
    UQADD8, /* UQADD8 */
    UQASX, /* UQASX */
    UQSAX, /* UQSAX */
    UQSUB16, /* UQSUB16 */
    UQSUB8, /* UQSUB8 */
    USAD8, /* USAD8 */
    USADA8, /* USADA8 */
    USAT, /* USAT */
    USAT16, /* USAT16 */
    USAX, /* USAX */
    USUB16, /* USUB16 */
    USUB8, /* USUB8 */
    UXTAB, /* UXTAB */
    UXTAB16, /* UXTAB16 */
    UXTAH, /* UXTAH */
    UXTB, /* UXTB */
    UXTB16, /* UXTB16 */
    UXTH, /* UXTH */
    VABS, /* VABS */
    VADD, /* VADD */
    VCMP_VCMPE, /* VCMP, VCMPE */
    VCVTA_VCVTN_VCVTP_AND_VCVTM, /* VCVTA, VCVTN, VCVTP, and VCVTM */
    VCVT_VCVTR_BETWEEN_FLOATING_POINT_AND_INTEGER, /* VCVT, VCVTR (between floating-point and integer) */
    VCVT_BETWEEN_FLOATING_POINT_AND_FIXED_POINT, /* VCVT (between floating-point and fixed-point) */
    VCVT_BETWEEN_DOUBLE_PRECISION_AND_SINGLE_PRECISION, /* VCVT (between double-precision and single-precision) */
    VCVTB_VCVTT, /* VCVTB, VCVTT */
    VDIV, /* VDIV */
    VFMA_VFMS, /* VFMA, VFMS */
    VFNMA_VFNMS, /* VFNMA, VFNMS */
    VLDM, /* VLDM */
    VLDR, /* VLDR */
    VMAXNM_VMINNM, /* VMAXNM, VMINNM */
    VMLA_VMLS, /* VMLA, VMLS */
    VMOV_IMMEDIATE, /* VMOV (immediate) */
    VMOV_REGISTER, /* VMOV (register) */
    VMOV_ARM_CORE_REGISTER_TO_SCALAR, /* VMOV (Arm core register to scalar) */
    VMOV_SCALAR_TO_ARM_CORE_REGISTER, /* VMOV (scalar to Arm core register) */
    VMOV_BETWEEN_ARM_CORE_REGISTER_AND_SINGLE_PRECISION_REGISTER, /* VMOV (between Arm core register and single-precision register) */
    VMOV_BETWEEN_TWO_ARM_CORE_REGISTERS_AND_TWO_SINGLE_PRECISION_REGISTERS, /* VMOV (between two Arm core registers and two single-precision registers) */
    VMOV_BETWEEN_TWO_ARM_CORE_REGISTERS_AND_A_DOUBLEWORD_REGISTER, /* VMOV (between two Arm core registers and a doubleword register) */
    VMRS, /* VMRS */
    VMSR, /* VMSR */
    VMUL, /* VMUL */
    VNEG, /* VNEG */
    VNMLA_VNMLS_VNMUL, /* VNMLA, VNMLS, VNMUL */
    VPOP, /* VPOP */
    VPUSH, /* VPUSH */
    VRINTA_VRINTN_VRINTP_AND_VRINTM, /* VRINTA, VRINTN, VRINTP, and VRINTM */
    VRINTX, /* VRINTX */
    VRINTZ_VRINTR, /* VRINTZ, VRINTR */
    VSEL, /* VSEL */
    VSQRT, /* VSQRT */
    VSTM, /* VSTM */
    VSTR, /* VSTR */
    VSUB, /* VSUB */
    WFE, /* WFE */
    WFI, /* WFI */
    YIELD, /* YIELD */
    NUMBER_OF_CPU_OPCODES
} CPU_op_enum;

extern struct CPU_struct_reg* CPU_var_reg;

// functions

extern void CPU_init(uint32 pc_pos);

// execute one cycle
extern void CPU_fetch();

