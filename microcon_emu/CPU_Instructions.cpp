#include "CPU.hpp"
#include "Memory.hpp"

/*
 * ARMv7-M Instruction Implementation Bodies
 * 
 * All instruction handlers follow the same signature:
 * void INSTR_name(uint32 instr, CPU_struct_reg* reg)
 * 
 * Parameters:
 *   instr - The 16-bit or 32-bit instruction encoding
 *   reg   - Pointer to CPU register state
 * 
 * These functions are designed to be called from a two-step hash table
 * for instruction decoding and execution.
 */

// ===== ADC - Add with Carry =====
void INSTR_ADC_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ADC (immediate)
}

void INSTR_ADC_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ADC (register)
}

// ===== ADD - Addition =====
void INSTR_ADD_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ADD (immediate)
}

void INSTR_ADD_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ADD (register)
}

void INSTR_ADD_SP_PLUS_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ADD (SP plus immediate)
}

void INSTR_ADD_SP_PLUS_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ADD (SP plus register)
}

// ===== ADR - Form PC-relative Address =====
void INSTR_ADR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ADR
}

// ===== AND - Logical AND =====
void INSTR_AND_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute AND (immediate)
}

void INSTR_AND_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute AND (register)
}

// ===== ASR - Arithmetic Shift Right =====
void INSTR_ASR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ASR (immediate)
}

void INSTR_ASR_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ASR (register)
}

// ===== B - Branch =====
void INSTR_B(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute B (conditional and unconditional)
}

// ===== BFC - Bit Field Clear =====
void INSTR_BFC(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute BFC
}

// ===== BFI - Bit Field Insert =====
void INSTR_BFI(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute BFI
}

// ===== BIC - Bit Clear =====
void INSTR_BIC_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute BIC (immediate)
}

void INSTR_BIC_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute BIC (register)
}

// ===== BKPT - Breakpoint =====
void INSTR_BKPT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute BKPT
}

// ===== BL - Branch with Link =====
void INSTR_BL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute BL
}

// ===== BLX - Branch with Link and Exchange =====
void INSTR_BLX_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute BLX (register)
}

// ===== BX - Branch and Exchange =====
void INSTR_BX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute BX
}

// ===== CBNZ, CBZ - Compare and Branch on (Non-)Zero =====
void INSTR_CBNZ_CBZ(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CBNZ/CBZ
}

// ===== CDP, CDP2 - Coprocessor Data Processing =====
void INSTR_CDP_CDP2(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CDP/CDP2
}

// ===== CLREX - Clear Exclusive =====
void INSTR_CLREX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CLREX
}

// ===== CLZ - Count Leading Zeros =====
void INSTR_CLZ(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CLZ
}

// ===== CMN - Compare Negative =====
void INSTR_CMN_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CMN (immediate)
}

void INSTR_CMN_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CMN (register)
}

// ===== CMP - Compare =====
void INSTR_CMP_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CMP (immediate)
}

void INSTR_CMP_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CMP (register)
}

// ===== CPS - Change Processor State =====
void INSTR_CPS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CPS (CPSID/CPSIE)
}

// ===== CPY - Copy (deprecated, use MOV) =====
void INSTR_CPY(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CPY
}

// ===== CSDB - Consumption of Speculative Data Barrier =====
void INSTR_CSDB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute CSDB
}

// ===== DBG - Debug Hint =====
void INSTR_DBG(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute DBG
}

// ===== DMB - Data Memory Barrier =====
void INSTR_DMB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute DMB
}

// ===== DSB - Data Synchronization Barrier =====
void INSTR_DSB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute DSB
}

// ===== EOR - Exclusive OR =====
void INSTR_EOR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute EOR (immediate)
}

void INSTR_EOR_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute EOR (register)
}

// ===== ISB - Instruction Synchronization Barrier =====
void INSTR_ISB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ISB
}

// ===== IT - If-Then =====
void INSTR_IT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute IT
}

// ===== LDC, LDC2 - Load Coprocessor =====
void INSTR_LDC_LDC2_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDC/LDC2 (immediate)
}

void INSTR_LDC_LDC2_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDC/LDC2 (literal)
}

// ===== LDM - Load Multiple =====
void INSTR_LDM_LDMIA_LDMFD(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDM/LDMIA/LDMFD
}

void INSTR_LDMDB_LDMEA(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDMDB/LDMEA
}

// ===== LDR - Load Register =====
void INSTR_LDR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDR (immediate)
}

void INSTR_LDR_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDR (literal)
}

void INSTR_LDR_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDR (register)
}

// ===== LDRB - Load Register Byte =====
void INSTR_LDRB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRB (immediate)
}

void INSTR_LDRB_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRB (literal)
}

void INSTR_LDRB_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRB (register)
}

// ===== LDRBT - Load Register Byte Unprivileged =====
void INSTR_LDRBT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRBT
}

// ===== LDRD - Load Register Dual =====
void INSTR_LDRD_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRD (immediate)
}

void INSTR_LDRD_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRD (literal)
}

// ===== LDREX - Load Register Exclusive =====
void INSTR_LDREX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDREX
}

void INSTR_LDREXB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDREXB
}

void INSTR_LDREXH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDREXH
}

// ===== LDRH - Load Register Halfword =====
void INSTR_LDRH_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRH (immediate)
}

void INSTR_LDRH_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRH (literal)
}

void INSTR_LDRH_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRH (register)
}

// ===== LDRHT - Load Register Halfword Unprivileged =====
void INSTR_LDRHT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRHT
}

// ===== LDRSB - Load Register Signed Byte =====
void INSTR_LDRSB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRSB (immediate)
}

void INSTR_LDRSB_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRSB (literal)
}

void INSTR_LDRSB_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRSB (register)
}

// ===== LDRSBT - Load Register Signed Byte Unprivileged =====
void INSTR_LDRSBT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRSBT
}

// ===== LDRSH - Load Register Signed Halfword =====
void INSTR_LDRSH_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRSH (immediate)
}

void INSTR_LDRSH_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRSH (literal)
}

void INSTR_LDRSH_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRSH (register)
}

// ===== LDRSHT - Load Register Signed Halfword Unprivileged =====
void INSTR_LDRSHT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRSHT
}

// ===== LDRT - Load Register Unprivileged =====
void INSTR_LDRT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LDRT
}

// ===== LSL - Logical Shift Left =====
void INSTR_LSL_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LSL (immediate)
}

void INSTR_LSL_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LSL (register)
}

// ===== LSR - Logical Shift Right =====
void INSTR_LSR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LSR (immediate)
}

void INSTR_LSR_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute LSR (register)
}

// ===== MCR, MCR2 - Move to Coprocessor from ARM Register =====
void INSTR_MCR_MCR2(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MCR/MCR2
}

// ===== MCRR, MCRR2 - Move to Coprocessor from two ARM Registers =====
void INSTR_MCRR_MCRR2(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MCRR/MCRR2
}

// ===== MLA - Multiply Accumulate =====
void INSTR_MLA(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MLA
}

// ===== MLS - Multiply and Subtract =====
void INSTR_MLS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MLS
}

// ===== MOV - Move =====
void INSTR_MOV_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MOV (immediate)
}

void INSTR_MOV_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MOV (register)
}

void INSTR_MOV_SHIFTED_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MOV (shifted register)
}

// ===== MOVT - Move Top =====
void INSTR_MOVT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MOVT
}

// ===== MRC, MRC2 - Move to ARM Register from Coprocessor =====
void INSTR_MRC_MRC2(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MRC/MRC2
}

// ===== MRRC, MRRC2 - Move to two ARM Registers from Coprocessor =====
void INSTR_MRRC_MRRC2(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MRRC/MRRC2
}

// ===== MRS - Move from Special Register =====
void INSTR_MRS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MRS
}

// ===== MSR - Move to Special Register =====
void INSTR_MSR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MSR
}

// ===== MUL - Multiply =====
void INSTR_MUL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MUL
}

// ===== MVN - Move NOT =====
void INSTR_MVN_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MVN (immediate)
}

void INSTR_MVN_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute MVN (register)
}

// ===== NEG - Negate =====
void INSTR_NEG(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute NEG
}

// ===== NOP - No Operation =====
void INSTR_NOP(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute NOP
}

// ===== ORN - Logical OR NOT =====
void INSTR_ORN_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ORN (immediate)
}

void INSTR_ORN_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ORN (register)
}

// ===== ORR - Logical OR =====
void INSTR_ORR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ORR (immediate)
}

void INSTR_ORR_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ORR (register)
}

// ===== PKHBT, PKHTB - Pack Halfword =====
void INSTR_PKHBT_PKHTB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute PKHBT/PKHTB
}

// ===== PLD - Preload Data =====
void INSTR_PLD_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute PLD (immediate)
}

void INSTR_PLD_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute PLD (literal)
}

void INSTR_PLD_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute PLD (register)
}

// ===== PLI - Preload Instruction =====
void INSTR_PLI_IMMEDIATE_LITERAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute PLI (immediate/literal)
}

void INSTR_PLI_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute PLI (register)
}

// ===== POP - Pop Multiple Registers =====
void INSTR_POP(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute POP
}

// ===== PSSBB - Physical Speculative Store Bypass Barrier =====
void INSTR_PSSBB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute PSSBB
}

// ===== PUSH - Push Multiple Registers =====
void INSTR_PUSH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute PUSH
}

// ===== QADD - Saturating Add =====
void INSTR_QADD(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QADD
}

void INSTR_QADD16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QADD16
}

void INSTR_QADD8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QADD8
}

// ===== QASX - Saturating Add and Subtract with Exchange =====
void INSTR_QASX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QASX
}

// ===== QDADD - Saturating Double and Add =====
void INSTR_QDADD(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QDADD
}

// ===== QDSUB - Saturating Double and Subtract =====
void INSTR_QDSUB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QDSUB
}

// ===== QSAX - Saturating Subtract and Add with Exchange =====
void INSTR_QSAX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QSAX
}

// ===== QSUB - Saturating Subtract =====
void INSTR_QSUB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QSUB
}

void INSTR_QSUB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QSUB16
}

void INSTR_QSUB8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute QSUB8
}

// ===== RBIT - Reverse Bits =====
void INSTR_RBIT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute RBIT
}

// ===== REV - Byte-Reverse Word =====
void INSTR_REV(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute REV
}

void INSTR_REV16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute REV16
}

void INSTR_REVSH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute REVSH
}

// ===== ROR - Rotate Right =====
void INSTR_ROR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ROR (immediate)
}

void INSTR_ROR_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute ROR (register)
}

// ===== RRX - Rotate Right with Extend =====
void INSTR_RRX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute RRX
}

// ===== RSB - Reverse Subtract =====
void INSTR_RSB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute RSB (immediate)
}

void INSTR_RSB_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute RSB (register)
}

// ===== SADD16 - Signed Add 16-bit =====
void INSTR_SADD16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SADD16
}

void INSTR_SADD8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SADD8
}

// ===== SASX - Signed Add and Subtract with Exchange =====
void INSTR_SASX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SASX
}

// ===== SBC - Subtract with Carry =====
void INSTR_SBC_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SBC (immediate)
}

void INSTR_SBC_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SBC (register)
}

// ===== SBFX - Signed Bit Field Extract =====
void INSTR_SBFX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SBFX
}

// ===== SDIV - Signed Divide =====
void INSTR_SDIV(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SDIV
}

// ===== SEL - Select Bytes =====
void INSTR_SEL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SEL
}

// ===== SEV - Send Event =====
void INSTR_SEV(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SEV
}

// ===== SHADD16 - Signed Halving Add 16-bit =====
void INSTR_SHADD16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SHADD16
}

void INSTR_SHADD8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SHADD8
}

// ===== SHASX - Signed Halving Add and Subtract with Exchange =====
void INSTR_SHASX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SHASX
}

// ===== SHSAX - Signed Halving Subtract and Add with Exchange =====
void INSTR_SHSAX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SHSAX
}

// ===== SHSUB16 - Signed Halving Subtract 16-bit =====
void INSTR_SHSUB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SHSUB16
}

void INSTR_SHSUB8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SHSUB8
}

// ===== SMLABB, SMLABT, SMLATB, SMLATT - Signed Multiply Accumulate =====
void INSTR_SMLABB_SMLABT_SMLATB_SMLATT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMLABB/SMLABT/SMLATB/SMLATT
}

// ===== SMLAD - Signed Multiply Accumulate Dual =====
void INSTR_SMLAD_SMLADX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMLAD/SMLADX
}

// ===== SMLAL - Signed Multiply Accumulate Long =====
void INSTR_SMLAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMLAL
}

// ===== SMLALBB - Signed Multiply Accumulate Long (halfwords) =====
void INSTR_SMLALBB_SMLALBT_SMLALTB_SMLALTT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMLALBB/SMLALBT/SMLALTB/SMLALTT
}

// ===== SMLALD - Signed Multiply Accumulate Long Dual =====
void INSTR_SMLALD_SMLALDX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMLALD/SMLALDX
}

// ===== SMLAWB, SMLAWT - Signed Multiply Accumulate Word =====
void INSTR_SMLAWB_SMLAWT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMLAWB/SMLAWT
}

// ===== SMLSD - Signed Multiply Subtract Dual =====
void INSTR_SMLSD_SMLSDX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMLSD/SMLSDX
}

// ===== SMLSLD - Signed Multiply Subtract Long Dual =====
void INSTR_SMLSLD_SMLSLDX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMLSLD/SMLSLDX
}

// ===== SMMLA - Signed Most Significant Word Multiply Accumulate =====
void INSTR_SMMLA_SMMLAR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMMLA/SMMLAR
}

// ===== SMMLS - Signed Most Significant Word Multiply Subtract =====
void INSTR_SMMLS_SMMLSR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMMLS/SMMLSR
}

// ===== SMMUL - Signed Most Significant Word Multiply =====
void INSTR_SMMUL_SMMULR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMMUL/SMMULR
}

// ===== SMUAD - Signed Dual Multiply Add =====
void INSTR_SMUAD_SMUADX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMUAD/SMUADX
}

// ===== SMULBB - Signed Multiply (halfwords) =====
void INSTR_SMULBB_SMULBT_SMULTB_SMULTT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMULBB/SMULBT/SMULTB/SMULTT
}

// ===== SMULL - Signed Multiply Long =====
void INSTR_SMULL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMULL
}

// ===== SMULWB, SMULWT - Signed Multiply Word =====
void INSTR_SMULWB_SMULWT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMULWB/SMULWT
}

// ===== SMUSD - Signed Dual Multiply Subtract =====
void INSTR_SMUSD_SMUSDX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SMUSD/SMUSDX
}

// ===== SSAT - Signed Saturate =====
void INSTR_SSAT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SSAT
}

void INSTR_SSAT16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SSAT16
}

// ===== SSAX - Signed Subtract and Add with Exchange =====
void INSTR_SSAX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SSAX
}

// ===== SSBB - Speculative Store Bypass Barrier =====
void INSTR_SSBB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SSBB
}

// ===== SSUB16 - Signed Subtract 16-bit =====
void INSTR_SSUB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SSUB16
}

void INSTR_SSUB8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SSUB8
}

// ===== STC, STC2 - Store Coprocessor =====
void INSTR_STC_STC2(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STC/STC2
}

// ===== STM - Store Multiple =====
void INSTR_STM_STMIA_STMEA(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STM/STMIA/STMEA
}

void INSTR_STMDB_STMFD(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STMDB/STMFD
}

// ===== STR - Store Register =====
void INSTR_STR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STR (immediate)
}

void INSTR_STR_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STR (register)
}

// ===== STRB - Store Register Byte =====
void INSTR_STRB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STRB (immediate)
}

void INSTR_STRB_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STRB (register)
}

// ===== STRBT - Store Register Byte Unprivileged =====
void INSTR_STRBT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STRBT
}

// ===== STRD - Store Register Dual =====
void INSTR_STRD_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STRD (immediate)
}

// ===== STREX - Store Register Exclusive =====
void INSTR_STREX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STREX
}

void INSTR_STREXB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STREXB
}

void INSTR_STREXH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STREXH
}

// ===== STRH - Store Register Halfword =====
void INSTR_STRH_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STRH (immediate)
}

void INSTR_STRH_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STRH (register)
}

// ===== STRHT - Store Register Halfword Unprivileged =====
void INSTR_STRHT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STRHT
}

// ===== STRT - Store Register Unprivileged =====
void INSTR_STRT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute STRT
}

// ===== SUB - Subtract =====
void INSTR_SUB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SUB (immediate)
}

void INSTR_SUB_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SUB (register)
}

void INSTR_SUB_SP_MINUS_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SUB (SP minus immediate)
}

void INSTR_SUB_SP_MINUS_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SUB (SP minus register)
}

// ===== SVC - Supervisor Call =====
void INSTR_SVC(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SVC
}

// ===== SXTAB - Signed Extend and Add Byte =====
void INSTR_SXTAB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SXTAB
}

void INSTR_SXTAB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SXTAB16
}

void INSTR_SXTAH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SXTAH
}

// ===== SXTB - Signed Extend Byte =====
void INSTR_SXTB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SXTB
}

void INSTR_SXTB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SXTB16
}

// ===== SXTH - Signed Extend Halfword =====
void INSTR_SXTH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute SXTH
}

// ===== TBB, TBH - Table Branch Byte/Halfword =====
void INSTR_TBB_TBH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute TBB/TBH
}

// ===== TEQ - Test Equivalence =====
void INSTR_TEQ_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute TEQ (immediate)
}

void INSTR_TEQ_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute TEQ (register)
}

// ===== TST - Test =====
void INSTR_TST_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute TST (immediate)
}

void INSTR_TST_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute TST (register)
}

// ===== UADD16 - Unsigned Add 16-bit =====
void INSTR_UADD16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UADD16
}

void INSTR_UADD8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UADD8
}

// ===== UASX - Unsigned Add and Subtract with Exchange =====
void INSTR_UASX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UASX
}

// ===== UBFX - Unsigned Bit Field Extract =====
void INSTR_UBFX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UBFX
}

// ===== UDF - Permanently Undefined =====
void INSTR_UDF(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UDF (generate exception)
}

// ===== UDIV - Unsigned Divide =====
void INSTR_UDIV(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UDIV
}

// ===== UHADD16 - Unsigned Halving Add 16-bit =====
void INSTR_UHADD16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UHADD16
}

void INSTR_UHADD8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UHADD8
}

// ===== UHASX - Unsigned Halving Add and Subtract with Exchange =====
void INSTR_UHASX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UHASX
}

// ===== UHSAX - Unsigned Halving Subtract and Add with Exchange =====
void INSTR_UHSAX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UHSAX
}

// ===== UHSUB16 - Unsigned Halving Subtract 16-bit =====
void INSTR_UHSUB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UHSUB16
}

void INSTR_UHSUB8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UHSUB8
}

// ===== UMAAL - Unsigned Multiply Accumulate Accumulate Long =====
void INSTR_UMAAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UMAAL
}

// ===== UMLAL - Unsigned Multiply Accumulate Long =====
void INSTR_UMLAL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UMLAL
}

// ===== UMULL - Unsigned Multiply Long =====
void INSTR_UMULL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UMULL
}

// ===== UQADD16 - Unsigned Saturating Add 16-bit =====
void INSTR_UQADD16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UQADD16
}

void INSTR_UQADD8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UQADD8
}

// ===== UQASX - Unsigned Saturating Add and Subtract with Exchange =====
void INSTR_UQASX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UQASX
}

// ===== UQSAX - Unsigned Saturating Subtract and Add with Exchange =====
void INSTR_UQSAX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UQSAX
}

// ===== UQSUB16 - Unsigned Saturating Subtract 16-bit =====
void INSTR_UQSUB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UQSUB16
}

void INSTR_UQSUB8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UQSUB8
}

// ===== USAD8 - Unsigned Sum of Absolute Differences =====
void INSTR_USAD8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute USAD8
}

// ===== USADA8 - Unsigned Sum of Absolute Differences and Accumulate =====
void INSTR_USADA8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute USADA8
}

// ===== USAT - Unsigned Saturate =====
void INSTR_USAT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute USAT
}

void INSTR_USAT16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute USAT16
}

// ===== USAX - Unsigned Subtract and Add with Exchange =====
void INSTR_USAX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute USAX
}

// ===== USUB16 - Unsigned Subtract 16-bit =====
void INSTR_USUB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute USUB16
}

void INSTR_USUB8(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute USUB8
}

// ===== UXTAB - Unsigned Extend and Add Byte =====
void INSTR_UXTAB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UXTAB
}

void INSTR_UXTAB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UXTAB16
}

void INSTR_UXTAH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UXTAH
}

// ===== UXTB - Unsigned Extend Byte =====
void INSTR_UXTB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UXTB
}

void INSTR_UXTB16(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UXTB16
}

// ===== UXTH - Unsigned Extend Halfword =====
void INSTR_UXTH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute UXTH
}

// ===== FLOATING POINT INSTRUCTIONS =====

// ===== VABS - Vector Absolute =====
void INSTR_VABS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VABS
}

// ===== VADD - Vector Add =====
void INSTR_VADD(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VADD
}

// ===== VCMP, VCMPE - Vector Compare =====
void INSTR_VCMP_VCMPE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VCMP/VCMPE
}

// ===== VCVTA, VCVTN, VCVTP, VCVTM - Vector Convert (rounding modes) =====
void INSTR_VCVTA_VCVTN_VCVTP_AND_VCVTM(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VCVTA/VCVTN/VCVTP/VCVTM
}

// ===== VCVT, VCVTR - Vector Convert (float/int) =====
void INSTR_VCVT_VCVTR_BETWEEN_FLOATING_POINT_AND_INTEGER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VCVT/VCVTR (float<->int)
}

// ===== VCVT - Vector Convert (float/fixed) =====
void INSTR_VCVT_BETWEEN_FLOATING_POINT_AND_FIXED_POINT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VCVT (float<->fixed)
}

// ===== VCVT - Vector Convert (double/single) =====
void INSTR_VCVT_BETWEEN_DOUBLE_PRECISION_AND_SINGLE_PRECISION(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VCVT (double<->single)
}

// ===== VCVTB, VCVTT - Vector Convert (half precision) =====
void INSTR_VCVTB_VCVTT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VCVTB/VCVTT
}

// ===== VDIV - Vector Divide =====
void INSTR_VDIV(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VDIV
}

// ===== VFMA, VFMS - Vector Fused Multiply Accumulate/Subtract =====
void INSTR_VFMA_VFMS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VFMA/VFMS
}

// ===== VFNMA, VFNMS - Vector Fused Negate Multiply Accumulate/Subtract =====
void INSTR_VFNMA_VFNMS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VFNMA/VFNMS
}

// ===== VLDM - Vector Load Multiple =====
void INSTR_VLDM(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VLDM
}

// ===== VLDR - Vector Load Register =====
void INSTR_VLDR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VLDR
}

// ===== VMAXNM, VMINNM - Vector Maximum/Minimum Number =====
void INSTR_VMAXNM_VMINNM(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMAXNM/VMINNM
}

// ===== VMLA, VMLS - Vector Multiply Accumulate/Subtract =====
void INSTR_VMLA_VMLS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMLA/VMLS
}

// ===== VMOV - Vector Move (immediate) =====
void INSTR_VMOV_IMMEDIATE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMOV (immediate)
}

// ===== VMOV - Vector Move (register) =====
void INSTR_VMOV_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMOV (register)
}

// ===== VMOV - ARM core register to scalar =====
void INSTR_VMOV_ARM_CORE_REGISTER_TO_SCALAR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMOV (ARM->scalar)
}

// ===== VMOV - Scalar to ARM core register =====
void INSTR_VMOV_SCALAR_TO_ARM_CORE_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMOV (scalar->ARM)
}

// ===== VMOV - Between ARM core register and single-precision =====
void INSTR_VMOV_BETWEEN_ARM_CORE_REGISTER_AND_SINGLE_PRECISION_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMOV (ARM<->single)
}

// ===== VMOV - Between two ARM core registers and two single-precision =====
void INSTR_VMOV_BETWEEN_TWO_ARM_CORE_REGISTERS_AND_TWO_SINGLE_PRECISION_REGISTERS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMOV (2xARM<->2xsingle)
}

// ===== VMOV - Between two ARM core registers and doubleword =====
void INSTR_VMOV_BETWEEN_TWO_ARM_CORE_REGISTERS_AND_A_DOUBLEWORD_REGISTER(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMOV (2xARM<->double)
}

// ===== VMRS - Move to ARM core register from floating-point system register =====
void INSTR_VMRS(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMRS
}

// ===== VMSR - Move to floating-point system register from ARM core register =====
void INSTR_VMSR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMSR
}

// ===== VMUL - Vector Multiply =====
void INSTR_VMUL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VMUL
}

// ===== VNEG - Vector Negate =====
void INSTR_VNEG(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VNEG
}

// ===== VNMLA, VNMLS, VNMUL - Vector Negate Multiply... =====
void INSTR_VNMLA_VNMLS_VNMUL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VNMLA/VNMLS/VNMUL
}

// ===== VPOP - Vector Pop =====
void INSTR_VPOP(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VPOP
}

// ===== VPUSH - Vector Push =====
void INSTR_VPUSH(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VPUSH
}

// ===== VRINTA, VRINTN, VRINTP, VRINTM - Vector Round (modes) =====
void INSTR_VRINTA_VRINTN_VRINTP_AND_VRINTM(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VRINTA/VRINTN/VRINTP/VRINTM
}

// ===== VRINTX - Vector Round (inexact) =====
void INSTR_VRINTX(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VRINTX
}

// ===== VRINTZ, VRINTR - Vector Round (zero/nearest) =====
void INSTR_VRINTZ_VRINTR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VRINTZ/VRINTR
}

// ===== VSEL - Vector Select =====
void INSTR_VSEL(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VSEL
}

// ===== VSQRT - Vector Square Root =====
void INSTR_VSQRT(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VSQRT
}

// ===== VSTM - Vector Store Multiple =====
void INSTR_VSTM(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VSTM
}

// ===== VSTR - Vector Store Register =====
void INSTR_VSTR(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VSTR
}

// ===== VSUB - Vector Subtract =====
void INSTR_VSUB(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute VSUB
}

// ===== WFE - Wait For Event =====
void INSTR_WFE(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute WFE
}

// ===== WFI - Wait For Interrupt =====
void INSTR_WFI(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute WFI
}

// ===== YIELD - Yield =====
void INSTR_YIELD(uint32 instr, CPU_struct_reg* reg) {
	// TODO: Decode and execute YIELD
}
