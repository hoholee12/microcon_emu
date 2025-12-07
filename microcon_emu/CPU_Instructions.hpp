#pragma once
#include "CPU.hpp"

/*
 * ARMv7-M Instruction Implementation Function Declarations
 * 
 * All instruction handlers follow the signature:
 * void INSTR_name(uint32 instr, CPU_struct_reg* reg)
 */

// ===== ADC - Add with Carry =====
extern void INSTR_ADC_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_ADC_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== ADD - Addition =====
extern void INSTR_ADD_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_ADD_REGISTER(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_ADD_SP_PLUS_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_ADD_SP_PLUS_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== ADR - Form PC-relative Address =====
extern void INSTR_ADR(uint32 instr, CPU_struct_reg* reg);

// ===== AND - Logical AND =====
extern void INSTR_AND_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_AND_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== ASR - Arithmetic Shift Right =====
extern void INSTR_ASR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_ASR_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== B - Branch =====
extern void INSTR_B(uint32 instr, CPU_struct_reg* reg);

// ===== BFC - Bit Field Clear =====
extern void INSTR_BFC(uint32 instr, CPU_struct_reg* reg);

// ===== BFI - Bit Field Insert =====
extern void INSTR_BFI(uint32 instr, CPU_struct_reg* reg);

// ===== BIC - Bit Clear =====
extern void INSTR_BIC_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_BIC_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== BKPT - Breakpoint =====
extern void INSTR_BKPT(uint32 instr, CPU_struct_reg* reg);

// ===== BL - Branch with Link =====
extern void INSTR_BL(uint32 instr, CPU_struct_reg* reg);

// ===== BLX - Branch with Link and Exchange =====
extern void INSTR_BLX_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== BX - Branch and Exchange =====
extern void INSTR_BX(uint32 instr, CPU_struct_reg* reg);

// ===== CBNZ, CBZ - Compare and Branch on (Non-)Zero =====
extern void INSTR_CBNZ_CBZ(uint32 instr, CPU_struct_reg* reg);

// ===== CDP, CDP2 - Coprocessor Data Processing =====
extern void INSTR_CDP_CDP2(uint32 instr, CPU_struct_reg* reg);

// ===== CLREX - Clear Exclusive =====
extern void INSTR_CLREX(uint32 instr, CPU_struct_reg* reg);

// ===== CLZ - Count Leading Zeros =====
extern void INSTR_CLZ(uint32 instr, CPU_struct_reg* reg);

// ===== CMN - Compare Negative =====
extern void INSTR_CMN_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_CMN_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== CMP - Compare =====
extern void INSTR_CMP_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_CMP_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== CPS - Change Processor State =====
extern void INSTR_CPS(uint32 instr, CPU_struct_reg* reg);

// ===== CPY - Copy =====
extern void INSTR_CPY(uint32 instr, CPU_struct_reg* reg);

// ===== CSDB - Consumption of Speculative Data Barrier =====
extern void INSTR_CSDB(uint32 instr, CPU_struct_reg* reg);

// ===== DBG - Debug Hint =====
extern void INSTR_DBG(uint32 instr, CPU_struct_reg* reg);

// ===== DMB - Data Memory Barrier =====
extern void INSTR_DMB(uint32 instr, CPU_struct_reg* reg);

// ===== DSB - Data Synchronization Barrier =====
extern void INSTR_DSB(uint32 instr, CPU_struct_reg* reg);

// ===== EOR - Exclusive OR =====
extern void INSTR_EOR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_EOR_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== ISB - Instruction Synchronization Barrier =====
extern void INSTR_ISB(uint32 instr, CPU_struct_reg* reg);

// ===== IT - If-Then =====
extern void INSTR_IT(uint32 instr, CPU_struct_reg* reg);

// ===== LDC, LDC2 - Load Coprocessor =====
extern void INSTR_LDC_LDC2_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDC_LDC2_LITERAL(uint32 instr, CPU_struct_reg* reg);

// ===== LDM - Load Multiple =====
extern void INSTR_LDM_LDMIA_LDMFD(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDMDB_LDMEA(uint32 instr, CPU_struct_reg* reg);

// ===== LDR - Load Register =====
extern void INSTR_LDR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDR_LITERAL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDR_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== LDRB - Load Register Byte =====
extern void INSTR_LDRB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRB_LITERAL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRB_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== LDRBT - Load Register Byte Unprivileged =====
extern void INSTR_LDRBT(uint32 instr, CPU_struct_reg* reg);

// ===== LDRD - Load Register Dual =====
extern void INSTR_LDRD_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRD_LITERAL(uint32 instr, CPU_struct_reg* reg);

// ===== LDREX - Load Register Exclusive =====
extern void INSTR_LDREX(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDREXB(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDREXH(uint32 instr, CPU_struct_reg* reg);

// ===== LDRH - Load Register Halfword =====
extern void INSTR_LDRH_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRH_LITERAL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRH_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== LDRHT - Load Register Halfword Unprivileged =====
extern void INSTR_LDRHT(uint32 instr, CPU_struct_reg* reg);

// ===== LDRSB - Load Register Signed Byte =====
extern void INSTR_LDRSB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRSB_LITERAL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRSB_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== LDRSBT - Load Register Signed Byte Unprivileged =====
extern void INSTR_LDRSBT(uint32 instr, CPU_struct_reg* reg);

// ===== LDRSH - Load Register Signed Halfword =====
extern void INSTR_LDRSH_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRSH_LITERAL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LDRSH_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== LDRSHT - Load Register Signed Halfword Unprivileged =====
extern void INSTR_LDRSHT(uint32 instr, CPU_struct_reg* reg);

// ===== LDRT - Load Register Unprivileged =====
extern void INSTR_LDRT(uint32 instr, CPU_struct_reg* reg);

// ===== LSL - Logical Shift Left =====
extern void INSTR_LSL_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LSL_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== LSR - Logical Shift Right =====
extern void INSTR_LSR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_LSR_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== MCR, MCR2 - Move to Coprocessor from ARM Register =====
extern void INSTR_MCR_MCR2(uint32 instr, CPU_struct_reg* reg);

// ===== MCRR, MCRR2 - Move to Coprocessor from two ARM Registers =====
extern void INSTR_MCRR_MCRR2(uint32 instr, CPU_struct_reg* reg);

// ===== MLA - Multiply Accumulate =====
extern void INSTR_MLA(uint32 instr, CPU_struct_reg* reg);

// ===== MLS - Multiply and Subtract =====
extern void INSTR_MLS(uint32 instr, CPU_struct_reg* reg);

// ===== MOV - Move =====
extern void INSTR_MOV_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_MOV_REGISTER(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_MOV_SHIFTED_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== MOVT - Move Top =====
extern void INSTR_MOVT(uint32 instr, CPU_struct_reg* reg);

// ===== MRC, MRC2 - Move to ARM Register from Coprocessor =====
extern void INSTR_MRC_MRC2(uint32 instr, CPU_struct_reg* reg);

// ===== MRRC, MRRC2 - Move to two ARM Registers from Coprocessor =====
extern void INSTR_MRRC_MRRC2(uint32 instr, CPU_struct_reg* reg);

// ===== MRS - Move from Special Register =====
extern void INSTR_MRS(uint32 instr, CPU_struct_reg* reg);

// ===== MSR - Move to Special Register =====
extern void INSTR_MSR(uint32 instr, CPU_struct_reg* reg);

// ===== MUL - Multiply =====
extern void INSTR_MUL(uint32 instr, CPU_struct_reg* reg);

// ===== MVN - Move NOT =====
extern void INSTR_MVN_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_MVN_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== NEG - Negate =====
extern void INSTR_NEG(uint32 instr, CPU_struct_reg* reg);

// ===== NOP - No Operation =====
extern void INSTR_NOP(uint32 instr, CPU_struct_reg* reg);

// ===== ORN - Logical OR NOT =====
extern void INSTR_ORN_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_ORN_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== ORR - Logical OR =====
extern void INSTR_ORR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_ORR_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== PKHBT, PKHTB - Pack Halfword =====
extern void INSTR_PKHBT_PKHTB(uint32 instr, CPU_struct_reg* reg);

// ===== PLD - Preload Data =====
extern void INSTR_PLD_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_PLD_LITERAL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_PLD_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== PLI - Preload Instruction =====
extern void INSTR_PLI_IMMEDIATE_LITERAL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_PLI_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== POP - Pop Multiple Registers =====
extern void INSTR_POP(uint32 instr, CPU_struct_reg* reg);

// ===== PSSBB - Physical Speculative Store Bypass Barrier =====
extern void INSTR_PSSBB(uint32 instr, CPU_struct_reg* reg);

// ===== PUSH - Push Multiple Registers =====
extern void INSTR_PUSH(uint32 instr, CPU_struct_reg* reg);

// ===== QADD - Saturating Add =====
extern void INSTR_QADD(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_QADD16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_QADD8(uint32 instr, CPU_struct_reg* reg);

// ===== QASX - Saturating Add and Subtract with Exchange =====
extern void INSTR_QASX(uint32 instr, CPU_struct_reg* reg);

// ===== QDADD - Saturating Double and Add =====
extern void INSTR_QDADD(uint32 instr, CPU_struct_reg* reg);

// ===== QDSUB - Saturating Double and Subtract =====
extern void INSTR_QDSUB(uint32 instr, CPU_struct_reg* reg);

// ===== QSAX - Saturating Subtract and Add with Exchange =====
extern void INSTR_QSAX(uint32 instr, CPU_struct_reg* reg);

// ===== QSUB - Saturating Subtract =====
extern void INSTR_QSUB(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_QSUB16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_QSUB8(uint32 instr, CPU_struct_reg* reg);

// ===== RBIT - Reverse Bits =====
extern void INSTR_RBIT(uint32 instr, CPU_struct_reg* reg);

// ===== REV - Byte-Reverse Word =====
extern void INSTR_REV(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_REV16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_REVSH(uint32 instr, CPU_struct_reg* reg);

// ===== ROR - Rotate Right =====
extern void INSTR_ROR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_ROR_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== RRX - Rotate Right with Extend =====
extern void INSTR_RRX(uint32 instr, CPU_struct_reg* reg);

// ===== RSB - Reverse Subtract =====
extern void INSTR_RSB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_RSB_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== SADD16 - Signed Add 16-bit =====
extern void INSTR_SADD16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SADD8(uint32 instr, CPU_struct_reg* reg);

// ===== SASX - Signed Add and Subtract with Exchange =====
extern void INSTR_SASX(uint32 instr, CPU_struct_reg* reg);

// ===== SBC - Subtract with Carry =====
extern void INSTR_SBC_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SBC_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== SBFX - Signed Bit Field Extract =====
extern void INSTR_SBFX(uint32 instr, CPU_struct_reg* reg);

// ===== SDIV - Signed Divide =====
extern void INSTR_SDIV(uint32 instr, CPU_struct_reg* reg);

// ===== SEL - Select Bytes =====
extern void INSTR_SEL(uint32 instr, CPU_struct_reg* reg);

// ===== SEV - Send Event =====
extern void INSTR_SEV(uint32 instr, CPU_struct_reg* reg);

// ===== SHADD16 - Signed Halving Add 16-bit =====
extern void INSTR_SHADD16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SHADD8(uint32 instr, CPU_struct_reg* reg);

// ===== SHASX - Signed Halving Add and Subtract with Exchange =====
extern void INSTR_SHASX(uint32 instr, CPU_struct_reg* reg);

// ===== SHSAX - Signed Halving Subtract and Add with Exchange =====
extern void INSTR_SHSAX(uint32 instr, CPU_struct_reg* reg);

// ===== SHSUB16 - Signed Halving Subtract 16-bit =====
extern void INSTR_SHSUB16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SHSUB8(uint32 instr, CPU_struct_reg* reg);

// ===== SMLABB, SMLABT, SMLATB, SMLATT - Signed Multiply Accumulate =====
extern void INSTR_SMLABB_SMLABT_SMLATB_SMLATT(uint32 instr, CPU_struct_reg* reg);

// ===== SMLAD - Signed Multiply Accumulate Dual =====
extern void INSTR_SMLAD_SMLADX(uint32 instr, CPU_struct_reg* reg);

// ===== SMLAL - Signed Multiply Accumulate Long =====
extern void INSTR_SMLAL(uint32 instr, CPU_struct_reg* reg);

// ===== SMLALBB - Signed Multiply Accumulate Long (halfwords) =====
extern void INSTR_SMLALBB_SMLALBT_SMLALTB_SMLALTT(uint32 instr, CPU_struct_reg* reg);

// ===== SMLALD - Signed Multiply Accumulate Long Dual =====
extern void INSTR_SMLALD_SMLALDX(uint32 instr, CPU_struct_reg* reg);

// ===== SMLAWB, SMLAWT - Signed Multiply Accumulate Word =====
extern void INSTR_SMLAWB_SMLAWT(uint32 instr, CPU_struct_reg* reg);

// ===== SMLSD - Signed Multiply Subtract Dual =====
extern void INSTR_SMLSD_SMLSDX(uint32 instr, CPU_struct_reg* reg);

// ===== SMLSLD - Signed Multiply Subtract Long Dual =====
extern void INSTR_SMLSLD_SMLSLDX(uint32 instr, CPU_struct_reg* reg);

// ===== SMMLA - Signed Most Significant Word Multiply Accumulate =====
extern void INSTR_SMMLA_SMMLAR(uint32 instr, CPU_struct_reg* reg);

// ===== SMMLS - Signed Most Significant Word Multiply Subtract =====
extern void INSTR_SMMLS_SMMLSR(uint32 instr, CPU_struct_reg* reg);

// ===== SMMUL - Signed Most Significant Word Multiply =====
extern void INSTR_SMMUL_SMMULR(uint32 instr, CPU_struct_reg* reg);

// ===== SMUAD - Signed Dual Multiply Add =====
extern void INSTR_SMUAD_SMUADX(uint32 instr, CPU_struct_reg* reg);

// ===== SMULBB - Signed Multiply (halfwords) =====
extern void INSTR_SMULBB_SMULBT_SMULTB_SMULTT(uint32 instr, CPU_struct_reg* reg);

// ===== SMULL - Signed Multiply Long =====
extern void INSTR_SMULL(uint32 instr, CPU_struct_reg* reg);

// ===== SMULWB, SMULWT - Signed Multiply Word =====
extern void INSTR_SMULWB_SMULWT(uint32 instr, CPU_struct_reg* reg);

// ===== SMUSD - Signed Dual Multiply Subtract =====
extern void INSTR_SMUSD_SMUSDX(uint32 instr, CPU_struct_reg* reg);

// ===== SSAT - Signed Saturate =====
extern void INSTR_SSAT(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SSAT16(uint32 instr, CPU_struct_reg* reg);

// ===== SSAX - Signed Subtract and Add with Exchange =====
extern void INSTR_SSAX(uint32 instr, CPU_struct_reg* reg);

// ===== SSBB - Speculative Store Bypass Barrier =====
extern void INSTR_SSBB(uint32 instr, CPU_struct_reg* reg);

// ===== SSUB16 - Signed Subtract 16-bit =====
extern void INSTR_SSUB16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SSUB8(uint32 instr, CPU_struct_reg* reg);

// ===== STC, STC2 - Store Coprocessor =====
extern void INSTR_STC_STC2(uint32 instr, CPU_struct_reg* reg);

// ===== STM - Store Multiple =====
extern void INSTR_STM_STMIA_STMEA(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_STMDB_STMFD(uint32 instr, CPU_struct_reg* reg);

// ===== STR - Store Register =====
extern void INSTR_STR_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_STR_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== STRB - Store Register Byte =====
extern void INSTR_STRB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_STRB_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== STRBT - Store Register Byte Unprivileged =====
extern void INSTR_STRBT(uint32 instr, CPU_struct_reg* reg);

// ===== STRD - Store Register Dual =====
extern void INSTR_STRD_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);

// ===== STREX - Store Register Exclusive =====
extern void INSTR_STREX(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_STREXB(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_STREXH(uint32 instr, CPU_struct_reg* reg);

// ===== STRH - Store Register Halfword =====
extern void INSTR_STRH_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_STRH_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== STRHT - Store Register Halfword Unprivileged =====
extern void INSTR_STRHT(uint32 instr, CPU_struct_reg* reg);

// ===== STRT - Store Register Unprivileged =====
extern void INSTR_STRT(uint32 instr, CPU_struct_reg* reg);

// ===== SUB - Subtract =====
extern void INSTR_SUB_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SUB_REGISTER(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SUB_SP_MINUS_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SUB_SP_MINUS_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== SVC - Supervisor Call =====
extern void INSTR_SVC(uint32 instr, CPU_struct_reg* reg);

// ===== SXTAB - Signed Extend and Add Byte =====
extern void INSTR_SXTAB(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SXTAB16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SXTAH(uint32 instr, CPU_struct_reg* reg);

// ===== SXTB - Signed Extend Byte =====
extern void INSTR_SXTB(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_SXTB16(uint32 instr, CPU_struct_reg* reg);

// ===== SXTH - Signed Extend Halfword =====
extern void INSTR_SXTH(uint32 instr, CPU_struct_reg* reg);

// ===== TBB, TBH - Table Branch Byte/Halfword =====
extern void INSTR_TBB_TBH(uint32 instr, CPU_struct_reg* reg);

// ===== TEQ - Test Equivalence =====
extern void INSTR_TEQ_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_TEQ_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== TST - Test =====
extern void INSTR_TST_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_TST_REGISTER(uint32 instr, CPU_struct_reg* reg);

// ===== UADD16 - Unsigned Add 16-bit =====
extern void INSTR_UADD16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_UADD8(uint32 instr, CPU_struct_reg* reg);

// ===== UASX - Unsigned Add and Subtract with Exchange =====
extern void INSTR_UASX(uint32 instr, CPU_struct_reg* reg);

// ===== UBFX - Unsigned Bit Field Extract =====
extern void INSTR_UBFX(uint32 instr, CPU_struct_reg* reg);

// ===== UDF - Permanently Undefined =====
extern void INSTR_UDF(uint32 instr, CPU_struct_reg* reg);

// ===== UDIV - Unsigned Divide =====
extern void INSTR_UDIV(uint32 instr, CPU_struct_reg* reg);

// ===== UHADD16 - Unsigned Halving Add 16-bit =====
extern void INSTR_UHADD16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_UHADD8(uint32 instr, CPU_struct_reg* reg);

// ===== UHASX - Unsigned Halving Add and Subtract with Exchange =====
extern void INSTR_UHASX(uint32 instr, CPU_struct_reg* reg);

// ===== UHSAX - Unsigned Halving Subtract and Add with Exchange =====
extern void INSTR_UHSAX(uint32 instr, CPU_struct_reg* reg);

// ===== UHSUB16 - Unsigned Halving Subtract 16-bit =====
extern void INSTR_UHSUB16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_UHSUB8(uint32 instr, CPU_struct_reg* reg);

// ===== UMAAL - Unsigned Multiply Accumulate Accumulate Long =====
extern void INSTR_UMAAL(uint32 instr, CPU_struct_reg* reg);

// ===== UMLAL - Unsigned Multiply Accumulate Long =====
extern void INSTR_UMLAL(uint32 instr, CPU_struct_reg* reg);

// ===== UMULL - Unsigned Multiply Long =====
extern void INSTR_UMULL(uint32 instr, CPU_struct_reg* reg);

// ===== UQADD16 - Unsigned Saturating Add 16-bit =====
extern void INSTR_UQADD16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_UQADD8(uint32 instr, CPU_struct_reg* reg);

// ===== UQASX - Unsigned Saturating Add and Subtract with Exchange =====
extern void INSTR_UQASX(uint32 instr, CPU_struct_reg* reg);

// ===== UQSAX - Unsigned Saturating Subtract and Add with Exchange =====
extern void INSTR_UQSAX(uint32 instr, CPU_struct_reg* reg);

// ===== UQSUB16 - Unsigned Saturating Subtract 16-bit =====
extern void INSTR_UQSUB16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_UQSUB8(uint32 instr, CPU_struct_reg* reg);

// ===== USAD8 - Unsigned Sum of Absolute Differences =====
extern void INSTR_USAD8(uint32 instr, CPU_struct_reg* reg);

// ===== USADA8 - Unsigned Sum of Absolute Differences and Accumulate =====
extern void INSTR_USADA8(uint32 instr, CPU_struct_reg* reg);

// ===== USAT - Unsigned Saturate =====
extern void INSTR_USAT(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_USAT16(uint32 instr, CPU_struct_reg* reg);

// ===== USAX - Unsigned Subtract and Add with Exchange =====
extern void INSTR_USAX(uint32 instr, CPU_struct_reg* reg);

// ===== USUB16 - Unsigned Subtract 16-bit =====
extern void INSTR_USUB16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_USUB8(uint32 instr, CPU_struct_reg* reg);

// ===== UXTAB - Unsigned Extend and Add Byte =====
extern void INSTR_UXTAB(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_UXTAB16(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_UXTAH(uint32 instr, CPU_struct_reg* reg);

// ===== UXTB - Unsigned Extend Byte =====
extern void INSTR_UXTB(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_UXTB16(uint32 instr, CPU_struct_reg* reg);

// ===== UXTH - Unsigned Extend Halfword =====
extern void INSTR_UXTH(uint32 instr, CPU_struct_reg* reg);

// ===== FLOATING POINT INSTRUCTIONS =====

extern void INSTR_VABS(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VADD(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VCMP_VCMPE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VCVTA_VCVTN_VCVTP_AND_VCVTM(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VCVT_VCVTR_BETWEEN_FLOATING_POINT_AND_INTEGER(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VCVT_BETWEEN_FLOATING_POINT_AND_FIXED_POINT(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VCVT_BETWEEN_DOUBLE_PRECISION_AND_SINGLE_PRECISION(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VCVTB_VCVTT(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VDIV(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VFMA_VFMS(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VFNMA_VFNMS(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VLDM(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VLDR(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMAXNM_VMINNM(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMLA_VMLS(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMOV_IMMEDIATE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMOV_REGISTER(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMOV_ARM_CORE_REGISTER_TO_SCALAR(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMOV_SCALAR_TO_ARM_CORE_REGISTER(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMOV_BETWEEN_ARM_CORE_REGISTER_AND_SINGLE_PRECISION_REGISTER(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMOV_BETWEEN_TWO_ARM_CORE_REGISTERS_AND_TWO_SINGLE_PRECISION_REGISTERS(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMOV_BETWEEN_TWO_ARM_CORE_REGISTERS_AND_A_DOUBLEWORD_REGISTER(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMRS(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMSR(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VMUL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VNEG(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VNMLA_VNMLS_VNMUL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VPOP(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VPUSH(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VRINTA_VRINTN_VRINTP_AND_VRINTM(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VRINTX(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VRINTZ_VRINTR(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VSEL(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VSQRT(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VSTM(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VSTR(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_VSUB(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_WFE(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_WFI(uint32 instr, CPU_struct_reg* reg);
extern void INSTR_YIELD(uint32 instr, CPU_struct_reg* reg);

// ===== INSTRUCTION LOOKUP TABLE =====

// Function pointer type for instruction handlers
typedef void (*InstrHandlerFunc)(uint32 instr, CPU_struct_reg* reg);

// Lookup table mapping CPU_op_enum to handler functions
static const InstrHandlerFunc INSTRUCTION_HANDLERS[NUMBER_OF_CPU_OPCODES] = {
	INSTR_ADC_IMMEDIATE,
	INSTR_ADC_REGISTER,
	INSTR_ADD_IMMEDIATE,
	INSTR_ADD_REGISTER,
	INSTR_ADD_SP_PLUS_IMMEDIATE,
	INSTR_ADD_SP_PLUS_REGISTER,
	INSTR_ADR,
	INSTR_AND_IMMEDIATE,
	INSTR_AND_REGISTER,
	INSTR_ASR_IMMEDIATE,
	INSTR_ASR_REGISTER,
	INSTR_B,
	INSTR_BFC,
	INSTR_BFI,
	INSTR_BIC_IMMEDIATE,
	INSTR_BIC_REGISTER,
	INSTR_BKPT,
	INSTR_BL,
	INSTR_BLX_REGISTER,
	INSTR_BX,
	INSTR_CBNZ_CBZ,
	INSTR_CDP_CDP2,
	INSTR_CLREX,
	INSTR_CLZ,
	INSTR_CMN_IMMEDIATE,
	INSTR_CMN_REGISTER,
	INSTR_CMP_IMMEDIATE,
	INSTR_CMP_REGISTER,
	INSTR_CPS,
	INSTR_CPY,
	INSTR_CSDB,
	INSTR_DBG,
	INSTR_DMB,
	INSTR_DSB,
	INSTR_EOR_IMMEDIATE,
	INSTR_EOR_REGISTER,
	INSTR_ISB,
	INSTR_IT,
	INSTR_LDC_LDC2_IMMEDIATE,
	INSTR_LDC_LDC2_LITERAL,
	INSTR_LDM_LDMIA_LDMFD,
	INSTR_LDMDB_LDMEA,
	INSTR_LDR_IMMEDIATE,
	INSTR_LDR_LITERAL,
	INSTR_LDR_REGISTER,
	INSTR_LDRB_IMMEDIATE,
	INSTR_LDRB_LITERAL,
	INSTR_LDRB_REGISTER,
	INSTR_LDRBT,
	INSTR_LDRD_IMMEDIATE,
	INSTR_LDRD_LITERAL,
	INSTR_LDREX,
	INSTR_LDREXB,
	INSTR_LDREXH,
	INSTR_LDRH_IMMEDIATE,
	INSTR_LDRH_LITERAL,
	INSTR_LDRH_REGISTER,
	INSTR_LDRHT,
	INSTR_LDRSB_IMMEDIATE,
	INSTR_LDRSB_LITERAL,
	INSTR_LDRSB_REGISTER,
	INSTR_LDRSBT,
	INSTR_LDRSH_IMMEDIATE,
	INSTR_LDRSH_LITERAL,
	INSTR_LDRSH_REGISTER,
	INSTR_LDRSHT,
	INSTR_LDRT,
	INSTR_LSL_IMMEDIATE,
	INSTR_LSL_REGISTER,
	INSTR_LSR_IMMEDIATE,
	INSTR_LSR_REGISTER,
	INSTR_MCR_MCR2,
	INSTR_MCRR_MCRR2,
	INSTR_MLA,
	INSTR_MLS,
	INSTR_MOV_IMMEDIATE,
	INSTR_MOV_REGISTER,
	INSTR_MOV_SHIFTED_REGISTER,
	INSTR_MOVT,
	INSTR_MRC_MRC2,
	INSTR_MRRC_MRRC2,
	INSTR_MRS,
	INSTR_MSR,
	INSTR_MUL,
	INSTR_MVN_IMMEDIATE,
	INSTR_MVN_REGISTER,
	INSTR_NEG,
	INSTR_NOP,
	INSTR_ORN_IMMEDIATE,
	INSTR_ORN_REGISTER,
	INSTR_ORR_IMMEDIATE,
	INSTR_ORR_REGISTER,
	INSTR_PKHBT_PKHTB,
	INSTR_PLD_IMMEDIATE,
	INSTR_PLD_LITERAL,
	INSTR_PLD_REGISTER,
	INSTR_PLI_IMMEDIATE_LITERAL,
	INSTR_PLI_REGISTER,
	INSTR_POP,
	INSTR_PSSBB,
	INSTR_PUSH,
	INSTR_QADD,
	INSTR_QADD16,
	INSTR_QADD8,
	INSTR_QASX,
	INSTR_QDADD,
	INSTR_QDSUB,
	INSTR_QSAX,
	INSTR_QSUB,
	INSTR_QSUB16,
	INSTR_QSUB8,
	INSTR_RBIT,
	INSTR_REV,
	INSTR_REV16,
	INSTR_REVSH,
	INSTR_ROR_IMMEDIATE,
	INSTR_ROR_REGISTER,
	INSTR_RRX,
	INSTR_RSB_IMMEDIATE,
	INSTR_RSB_REGISTER,
	INSTR_SADD16,
	INSTR_SADD8,
	INSTR_SASX,
	INSTR_SBC_IMMEDIATE,
	INSTR_SBC_REGISTER,
	INSTR_SBFX,
	INSTR_SDIV,
	INSTR_SEL,
	INSTR_SEV,
	INSTR_SHADD16,
	INSTR_SHADD8,
	INSTR_SHASX,
	INSTR_SHSAX,
	INSTR_SHSUB16,
	INSTR_SHSUB8,
	INSTR_SMLABB_SMLABT_SMLATB_SMLATT,
	INSTR_SMLAD_SMLADX,
	INSTR_SMLAL,
	INSTR_SMLALBB_SMLALBT_SMLALTB_SMLALTT,
	INSTR_SMLALD_SMLALDX,
	INSTR_SMLAWB_SMLAWT,
	INSTR_SMLSD_SMLSDX,
	INSTR_SMLSLD_SMLSLDX,
	INSTR_SMMLA_SMMLAR,
	INSTR_SMMLS_SMMLSR,
	INSTR_SMMUL_SMMULR,
	INSTR_SMUAD_SMUADX,
	INSTR_SMULBB_SMULBT_SMULTB_SMULTT,
	INSTR_SMULL,
	INSTR_SMULWB_SMULWT,
	INSTR_SMUSD_SMUSDX,
	INSTR_SSAT,
	INSTR_SSAT16,
	INSTR_SSAX,
	INSTR_SSBB,
	INSTR_SSUB16,
	INSTR_SSUB8,
	INSTR_STC_STC2,
	INSTR_STM_STMIA_STMEA,
	INSTR_STMDB_STMFD,
	INSTR_STR_IMMEDIATE,
	INSTR_STR_REGISTER,
	INSTR_STRB_IMMEDIATE,
	INSTR_STRB_REGISTER,
	INSTR_STRBT,
	INSTR_STRD_IMMEDIATE,
	INSTR_STREX,
	INSTR_STREXB,
	INSTR_STREXH,
	INSTR_STRH_IMMEDIATE,
	INSTR_STRH_REGISTER,
	INSTR_STRHT,
	INSTR_STRT,
	INSTR_SUB_IMMEDIATE,
	INSTR_SUB_REGISTER,
	INSTR_SUB_SP_MINUS_IMMEDIATE,
	INSTR_SUB_SP_MINUS_REGISTER,
	INSTR_SVC,
	INSTR_SXTAB,
	INSTR_SXTAB16,
	INSTR_SXTAH,
	INSTR_SXTB,
	INSTR_SXTB16,
	INSTR_SXTH,
	INSTR_TBB_TBH,
	INSTR_TEQ_IMMEDIATE,
	INSTR_TEQ_REGISTER,
	INSTR_TST_IMMEDIATE,
	INSTR_TST_REGISTER,
	INSTR_UADD16,
	INSTR_UADD8,
	INSTR_UASX,
	INSTR_UBFX,
	INSTR_UDF,
	INSTR_UDIV,
	INSTR_UHADD16,
	INSTR_UHADD8,
	INSTR_UHASX,
	INSTR_UHSAX,
	INSTR_UHSUB16,
	INSTR_UHSUB8,
	INSTR_UMAAL,
	INSTR_UMLAL,
	INSTR_UMULL,
	INSTR_UQADD16,
	INSTR_UQADD8,
	INSTR_UQASX,
	INSTR_UQSAX,
	INSTR_UQSUB16,
	INSTR_UQSUB8,
	INSTR_USAD8,
	INSTR_USADA8,
	INSTR_USAT,
	INSTR_USAT16,
	INSTR_USAX,
	INSTR_USUB16,
	INSTR_USUB8,
	INSTR_UXTAB,
	INSTR_UXTAB16,
	INSTR_UXTAH,
	INSTR_UXTB,
	INSTR_UXTB16,
	INSTR_UXTH,
	INSTR_VABS,
	INSTR_VADD,
	INSTR_VCMP_VCMPE,
	INSTR_VCVTA_VCVTN_VCVTP_AND_VCVTM,
	INSTR_VCVT_VCVTR_BETWEEN_FLOATING_POINT_AND_INTEGER,
	INSTR_VCVT_BETWEEN_FLOATING_POINT_AND_FIXED_POINT,
	INSTR_VCVT_BETWEEN_DOUBLE_PRECISION_AND_SINGLE_PRECISION,
	INSTR_VCVTB_VCVTT,
	INSTR_VDIV,
	INSTR_VFMA_VFMS,
	INSTR_VFNMA_VFNMS,
	INSTR_VLDM,
	INSTR_VLDR,
	INSTR_VMAXNM_VMINNM,
	INSTR_VMLA_VMLS,
	INSTR_VMOV_IMMEDIATE,
	INSTR_VMOV_REGISTER,
	INSTR_VMOV_ARM_CORE_REGISTER_TO_SCALAR,
	INSTR_VMOV_SCALAR_TO_ARM_CORE_REGISTER,
	INSTR_VMOV_BETWEEN_ARM_CORE_REGISTER_AND_SINGLE_PRECISION_REGISTER,
	INSTR_VMOV_BETWEEN_TWO_ARM_CORE_REGISTERS_AND_TWO_SINGLE_PRECISION_REGISTERS,
	INSTR_VMOV_BETWEEN_TWO_ARM_CORE_REGISTERS_AND_A_DOUBLEWORD_REGISTER,
	INSTR_VMRS,
	INSTR_VMSR,
	INSTR_VMUL,
	INSTR_VNEG,
	INSTR_VNMLA_VNMLS_VNMUL,
	INSTR_VPOP,
	INSTR_VPUSH,
	INSTR_VRINTA_VRINTN_VRINTP_AND_VRINTM,
	INSTR_VRINTX,
	INSTR_VRINTZ_VRINTR,
	INSTR_VSEL,
	INSTR_VSQRT,
	INSTR_VSTM,
	INSTR_VSTR,
	INSTR_VSUB,
	INSTR_WFE,
	INSTR_WFI,
	INSTR_YIELD,
};

// Helper function to execute an instruction by enum
inline void CPU_ExecuteInstruction(CPU_op_enum opcode, uint32 instr, CPU_struct_reg* reg) {
	if (opcode < NUMBER_OF_CPU_OPCODES && INSTRUCTION_HANDLERS[opcode] != nullptr) {
		INSTRUCTION_HANDLERS[opcode](instr, reg);
	}
}
