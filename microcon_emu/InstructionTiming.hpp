#pragma once
#include "Proxy.hpp"

/*
 * ARMv7-M Instruction Timing Information
 * Based on ARM Cortex-M4 Technical Reference Manual
 * 
 * Cycle counts are based on a system with zero wait states.
 * 
 * Special cycle count notations:
 * - P: Pipeline refill cycles (1-3 cycles, depends on alignment and speculation)
 * - B: Barrier operation cycles (0+ for DSB/DMB, P equivalent for ISB)
 * - N: Number of registers in register list (including PC/LR)
 * - W: Cycles waiting for appropriate event
 */

enum class InstructionCategory {
	MOVE,
	ADD_SUB,
	MULTIPLY,
	DIVIDE,
	SATURATE,
	COMPARE,
	LOGICAL,
	SHIFT,
	ROTATE,
	COUNT,
	LOAD,
	STORE,
	PUSH_POP,
	SEMAPHORE,
	BRANCH,
	STATE_CHANGE,
	EXTEND,
	BIT_FIELD,
	REVERSE,
	HINT,
	BARRIER,
	DSP_MULTIPLY,
	DSP_SATURATE,
	DSP_PACKING,
	DSP_ADDITION,
	DSP_SUBTRACTION,
	DSP_PARALLEL,
	DSP_MISC
};

// Instruction timing information structure
struct InstructionTiming {
	const char* mnemonic;           // Instruction mnemonic
	const char* description;        // Brief description
	InstructionCategory category;   // Instruction category
	uint32 min_cycles;              // Minimum cycles
	uint32 max_cycles;              // Maximum cycles (0 if fixed)
	bool has_pipeline_refill;       // Uses P (pipeline refill)
	bool has_barrier_cycles;        // Uses B (barrier cycles)
	bool depends_on_reg_count;      // Uses N (register count)
	bool depends_on_wait;           // Uses W (wait cycles)
	const char* notes;              // Additional notes
};

// ARMv7-M Cortex-M4 Instruction Timing Table
static const InstructionTiming ARMV7M_INSTRUCTION_TIMING[] = {
	// ===== MOVE INSTRUCTIONS =====
	{"MOV", "Move register", InstructionCategory::MOVE, 1, 0, false, false, false, false, "Simple register move"},
	{"MOVW", "Move 16-bit immediate", InstructionCategory::MOVE, 1, 0, false, false, false, false, "Move wide immediate"},
	{"MOVT", "Move immediate into top", InstructionCategory::MOVE, 1, 0, false, false, false, false, "Move top 16 bits"},
	{"MOV_PC", "Move to PC", InstructionCategory::MOVE, 1, 1, true, false, false, false, "1 + P cycles"},

	// ===== ADDITION/SUBTRACTION =====
	{"ADD", "Add", InstructionCategory::ADD_SUB, 1, 0, false, false, false, false, "Basic addition"},
	{"ADD_PC", "Add to PC", InstructionCategory::ADD_SUB, 1, 1, true, false, false, false, "1 + P cycles"},
	{"ADC", "Add with carry", InstructionCategory::ADD_SUB, 1, 0, false, false, false, false, "Add with carry flag"},
	{"ADR", "Form address", InstructionCategory::ADD_SUB, 1, 0, false, false, false, false, "PC-relative address"},
	{"SUB", "Subtract", InstructionCategory::ADD_SUB, 1, 0, false, false, false, false, "Basic subtraction"},
	{"SBC", "Subtract with borrow", InstructionCategory::ADD_SUB, 1, 0, false, false, false, false, "Subtract with carry"},
	{"RSB", "Reverse subtract", InstructionCategory::ADD_SUB, 1, 0, false, false, false, false, "Reverse operand order"},

	// ===== MULTIPLY INSTRUCTIONS =====
	{"MUL", "Multiply", InstructionCategory::MULTIPLY, 1, 0, false, false, false, false, "32-bit multiply"},
	{"MLA", "Multiply accumulate", InstructionCategory::MULTIPLY, 2, 0, false, false, false, false, "Multiply and add"},
	{"MLS", "Multiply subtract", InstructionCategory::MULTIPLY, 2, 0, false, false, false, false, "Multiply and subtract"},
	{"SMULL", "Signed multiply long", InstructionCategory::MULTIPLY, 1, 0, false, false, false, false, "64-bit signed result"},
	{"UMULL", "Unsigned multiply long", InstructionCategory::MULTIPLY, 1, 0, false, false, false, false, "64-bit unsigned result"},
	{"SMLAL", "Signed multiply accumulate long", InstructionCategory::MULTIPLY, 1, 0, false, false, false, false, "64-bit signed accumulate"},
	{"UMLAL", "Unsigned multiply accumulate long", InstructionCategory::MULTIPLY, 1, 0, false, false, false, false, "64-bit unsigned accumulate"},

	// ===== DIVIDE INSTRUCTIONS =====
	{"SDIV", "Signed divide", InstructionCategory::DIVIDE, 2, 12, false, false, false, false, "Early termination based on operands"},
	{"UDIV", "Unsigned divide", InstructionCategory::DIVIDE, 2, 12, false, false, false, false, "Early termination based on operands"},

	// ===== SATURATE INSTRUCTIONS =====
	{"SSAT", "Signed saturate", InstructionCategory::SATURATE, 1, 0, false, false, false, false, "Saturate to signed range"},
	{"USAT", "Unsigned saturate", InstructionCategory::SATURATE, 1, 0, false, false, false, false, "Saturate to unsigned range"},

	// ===== COMPARE INSTRUCTIONS =====
	{"CMP", "Compare", InstructionCategory::COMPARE, 1, 0, false, false, false, false, "Compare and set flags"},
	{"CMN", "Compare negative", InstructionCategory::COMPARE, 1, 0, false, false, false, false, "Compare with negated operand"},

	// ===== LOGICAL INSTRUCTIONS =====
	{"AND", "Logical AND", InstructionCategory::LOGICAL, 1, 0, false, false, false, false, "Bitwise AND"},
	{"EOR", "Exclusive OR", InstructionCategory::LOGICAL, 1, 0, false, false, false, false, "Bitwise XOR"},
	{"ORR", "Logical OR", InstructionCategory::LOGICAL, 1, 0, false, false, false, false, "Bitwise OR"},
	{"ORN", "Logical OR NOT", InstructionCategory::LOGICAL, 1, 0, false, false, false, false, "OR with inverted operand"},
	{"BIC", "Bit clear", InstructionCategory::LOGICAL, 1, 0, false, false, false, false, "AND with inverted operand"},
	{"MVN", "Move NOT", InstructionCategory::LOGICAL, 1, 0, false, false, false, false, "Move inverted value"},
	{"TST", "Test bits", InstructionCategory::LOGICAL, 1, 0, false, false, false, false, "AND and set flags only"},
	{"TEQ", "Test equivalence", InstructionCategory::LOGICAL, 1, 0, false, false, false, false, "XOR and set flags only"},

	// ===== SHIFT INSTRUCTIONS =====
	{"LSL_IMM", "Logical shift left (immediate)", InstructionCategory::SHIFT, 1, 0, false, false, false, false, "LSL with immediate"},
	{"LSL_REG", "Logical shift left (register)", InstructionCategory::SHIFT, 1, 0, false, false, false, false, "LSL with register"},
	{"LSR_IMM", "Logical shift right (immediate)", InstructionCategory::SHIFT, 1, 0, false, false, false, false, "LSR with immediate"},
	{"LSR_REG", "Logical shift right (register)", InstructionCategory::SHIFT, 1, 0, false, false, false, false, "LSR with register"},
	{"ASR_IMM", "Arithmetic shift right (immediate)", InstructionCategory::SHIFT, 1, 0, false, false, false, false, "ASR with immediate"},
	{"ASR_REG", "Arithmetic shift right (register)", InstructionCategory::SHIFT, 1, 0, false, false, false, false, "ASR with register"},

	// ===== ROTATE INSTRUCTIONS =====
	{"ROR_IMM", "Rotate right (immediate)", InstructionCategory::ROTATE, 1, 0, false, false, false, false, "ROR with immediate"},
	{"ROR_REG", "Rotate right (register)", InstructionCategory::ROTATE, 1, 0, false, false, false, false, "ROR with register"},
	{"RRX", "Rotate right with extend", InstructionCategory::ROTATE, 1, 0, false, false, false, false, "ROR through carry"},

	// ===== COUNT INSTRUCTIONS =====
	{"CLZ", "Count leading zeros", InstructionCategory::COUNT, 1, 0, false, false, false, false, "Count leading zero bits"},

	// ===== LOAD INSTRUCTIONS =====
	{"LDR", "Load word", InstructionCategory::LOAD, 2, 0, false, false, false, false, "Can pipeline to 1 cycle"},
	{"LDR_PC", "Load word to PC", InstructionCategory::LOAD, 2, 2, true, false, false, false, "2 + P cycles"},
	{"LDRH", "Load halfword", InstructionCategory::LOAD, 2, 0, false, false, false, false, "Can pipeline to 1 cycle"},
	{"LDRB", "Load byte", InstructionCategory::LOAD, 2, 0, false, false, false, false, "Can pipeline to 1 cycle"},
	{"LDRSH", "Load signed halfword", InstructionCategory::LOAD, 2, 0, false, false, false, false, "Can pipeline to 1 cycle"},
	{"LDRSB", "Load signed byte", InstructionCategory::LOAD, 2, 0, false, false, false, false, "Can pipeline to 1 cycle"},
	{"LDRT", "Load unprivileged word", InstructionCategory::LOAD, 2, 0, false, false, false, false, "User mode access"},
	{"LDRHT", "Load unprivileged halfword", InstructionCategory::LOAD, 2, 0, false, false, false, false, "User mode access"},
	{"LDRBT", "Load unprivileged byte", InstructionCategory::LOAD, 2, 0, false, false, false, false, "User mode access"},
	{"LDRSHT", "Load unprivileged signed halfword", InstructionCategory::LOAD, 2, 0, false, false, false, false, "User mode access"},
	{"LDRSBT", "Load unprivileged signed byte", InstructionCategory::LOAD, 2, 0, false, false, false, false, "User mode access"},
	{"LDR_LITERAL", "Load PC-relative", InstructionCategory::LOAD, 2, 0, false, false, false, false, "PC-relative load"},
	{"LDRD", "Load doubleword", InstructionCategory::LOAD, 1, 1, false, false, true, false, "1 + N cycles"},
	{"LDM", "Load multiple", InstructionCategory::LOAD, 1, 1, false, false, true, false, "1 + N cycles"},
	{"LDM_PC", "Load multiple with PC", InstructionCategory::LOAD, 1, 1, true, false, true, false, "1 + N + P cycles"},

	// ===== STORE INSTRUCTIONS =====
	{"STR", "Store word", InstructionCategory::STORE, 2, 0, false, false, false, false, "Can pipeline to 1 cycle"},
	{"STRH", "Store halfword", InstructionCategory::STORE, 2, 0, false, false, false, false, "Can pipeline to 1 cycle"},
	{"STRB", "Store byte", InstructionCategory::STORE, 2, 0, false, false, false, false, "Can pipeline to 1 cycle"},
	{"STRT", "Store unprivileged word", InstructionCategory::STORE, 2, 0, false, false, false, false, "User mode access"},
	{"STRHT", "Store unprivileged halfword", InstructionCategory::STORE, 2, 0, false, false, false, false, "User mode access"},
	{"STRBT", "Store unprivileged byte", InstructionCategory::STORE, 2, 0, false, false, false, false, "User mode access"},
	{"STRD", "Store doubleword", InstructionCategory::STORE, 1, 1, false, false, true, false, "1 + N cycles"},
	{"STM", "Store multiple", InstructionCategory::STORE, 1, 1, false, false, true, false, "1 + N cycles"},

	// ===== PUSH/POP INSTRUCTIONS =====
	{"PUSH", "Push registers", InstructionCategory::PUSH_POP, 1, 1, false, false, true, false, "1 + N cycles"},
	{"PUSH_LR", "Push with link register", InstructionCategory::PUSH_POP, 1, 1, false, false, true, false, "1 + N cycles"},
	{"POP", "Pop registers", InstructionCategory::PUSH_POP, 1, 1, false, false, true, false, "1 + N cycles"},
	{"POP_PC", "Pop and return", InstructionCategory::PUSH_POP, 1, 1, true, false, true, false, "1 + N + P cycles"},

	// ===== SEMAPHORE/EXCLUSIVE INSTRUCTIONS =====
	{"LDREX", "Load exclusive word", InstructionCategory::SEMAPHORE, 2, 0, false, false, false, false, "Exclusive monitor load"},
	{"LDREXH", "Load exclusive halfword", InstructionCategory::SEMAPHORE, 2, 0, false, false, false, false, "Exclusive monitor load"},
	{"LDREXB", "Load exclusive byte", InstructionCategory::SEMAPHORE, 2, 0, false, false, false, false, "Exclusive monitor load"},
	{"STREX", "Store exclusive word", InstructionCategory::SEMAPHORE, 2, 0, false, false, false, false, "Exclusive monitor store"},
	{"STREXH", "Store exclusive halfword", InstructionCategory::SEMAPHORE, 2, 0, false, false, false, false, "Exclusive monitor store"},
	{"STREXB", "Store exclusive byte", InstructionCategory::SEMAPHORE, 2, 0, false, false, false, false, "Exclusive monitor store"},
	{"CLREX", "Clear exclusive monitor", InstructionCategory::SEMAPHORE, 1, 0, false, false, false, false, "Clear exclusive state"},

	// ===== BRANCH INSTRUCTIONS =====
	{"B_COND", "Conditional branch", InstructionCategory::BRANCH, 1, 1, true, false, false, false, "1 if not taken, 1+P if taken"},
	{"B", "Unconditional branch", InstructionCategory::BRANCH, 1, 1, true, false, false, false, "1 + P cycles"},
	{"BL", "Branch with link", InstructionCategory::BRANCH, 1, 1, true, false, false, false, "1 + P cycles"},
	{"BX", "Branch with exchange", InstructionCategory::BRANCH, 1, 1, true, false, false, false, "1 + P cycles"},
	{"BLX", "Branch link with exchange", InstructionCategory::BRANCH, 1, 1, true, false, false, false, "1 + P cycles"},
	{"CBZ", "Compare and branch if zero", InstructionCategory::BRANCH, 1, 1, true, false, false, false, "1 if not taken, 1+P if taken"},
	{"CBNZ", "Compare and branch if non-zero", InstructionCategory::BRANCH, 1, 1, true, false, false, false, "1 if not taken, 1+P if taken"},
	{"TBB", "Table branch byte", InstructionCategory::BRANCH, 2, 2, true, false, false, false, "2 + P cycles"},
	{"TBH", "Table branch halfword", InstructionCategory::BRANCH, 2, 2, true, false, false, false, "2 + P cycles"},

	// ===== STATE CHANGE INSTRUCTIONS =====
	{"SVC", "Supervisor call", InstructionCategory::STATE_CHANGE, 0, 0, false, false, false, false, "Exception handling"},
	{"IT", "If-then-else", InstructionCategory::STATE_CHANGE, 1, 0, false, false, false, false, "Can fold to 0 cycles"},
	{"CPSID", "Disable interrupts", InstructionCategory::STATE_CHANGE, 1, 2, false, false, false, false, "1 or 2 cycles"},
	{"CPSIE", "Enable interrupts", InstructionCategory::STATE_CHANGE, 1, 2, false, false, false, false, "1 or 2 cycles"},
	{"MRS", "Read special register", InstructionCategory::STATE_CHANGE, 1, 2, false, false, false, false, "1 or 2 cycles"},
	{"MSR", "Write special register", InstructionCategory::STATE_CHANGE, 1, 2, false, false, false, false, "1 or 2 cycles"},
	{"BKPT", "Breakpoint", InstructionCategory::STATE_CHANGE, 0, 0, false, false, false, false, "Debug instruction"},

	// ===== EXTEND INSTRUCTIONS =====
	{"SXTH", "Sign extend halfword", InstructionCategory::EXTEND, 1, 0, false, false, false, false, "16-bit to 32-bit signed"},
	{"SXTB", "Sign extend byte", InstructionCategory::EXTEND, 1, 0, false, false, false, false, "8-bit to 32-bit signed"},
	{"UXTH", "Zero extend halfword", InstructionCategory::EXTEND, 1, 0, false, false, false, false, "16-bit to 32-bit unsigned"},
	{"UXTB", "Zero extend byte", InstructionCategory::EXTEND, 1, 0, false, false, false, false, "8-bit to 32-bit unsigned"},

	// ===== BIT FIELD INSTRUCTIONS =====
	{"UBFX", "Unsigned bit field extract", InstructionCategory::BIT_FIELD, 1, 0, false, false, false, false, "Extract bit field"},
	{"SBFX", "Signed bit field extract", InstructionCategory::BIT_FIELD, 1, 0, false, false, false, false, "Extract and sign extend"},
	{"BFC", "Bit field clear", InstructionCategory::BIT_FIELD, 1, 0, false, false, false, false, "Clear bit field"},
	{"BFI", "Bit field insert", InstructionCategory::BIT_FIELD, 1, 0, false, false, false, false, "Insert bit field"},

	// ===== REVERSE INSTRUCTIONS =====
	{"REV", "Reverse bytes in word", InstructionCategory::REVERSE, 1, 0, false, false, false, false, "Endian swap 32-bit"},
	{"REV16", "Reverse bytes in halfwords", InstructionCategory::REVERSE, 1, 0, false, false, false, false, "Endian swap 16-bit"},
	{"REVSH", "Reverse signed halfword", InstructionCategory::REVERSE, 1, 0, false, false, false, false, "Swap and sign extend"},
	{"RBIT", "Reverse bits in word", InstructionCategory::REVERSE, 1, 0, false, false, false, false, "Reverse all 32 bits"},

	// ===== HINT INSTRUCTIONS =====
	{"SEV", "Send event", InstructionCategory::HINT, 1, 0, false, false, false, false, "Wake from WFE"},
	{"WFE", "Wait for event", InstructionCategory::HINT, 1, 1, false, false, false, true, "1 + W cycles"},
	{"WFI", "Wait for interrupt", InstructionCategory::HINT, 1, 1, false, false, false, true, "1 + W cycles"},
	{"NOP", "No operation", InstructionCategory::HINT, 1, 0, false, false, false, false, "Do nothing"},

	// ===== BARRIER INSTRUCTIONS =====
	{"ISB", "Instruction synchronization barrier", InstructionCategory::BARRIER, 1, 1, false, true, false, false, "1 + B cycles (min B = P)"},
	{"DMB", "Data memory barrier", InstructionCategory::BARRIER, 1, 1, false, true, false, false, "1 + B cycles (min B = 0)"},
	{"DSB", "Data synchronization barrier", InstructionCategory::BARRIER, 1, 1, false, true, false, false, "1 + B cycles (min B = 0)"},

	// ===== DSP MULTIPLY INSTRUCTIONS =====
	{"SMMLA", "Signed MSB multiply accumulate", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "DSP extension"},
	{"SMMLS", "Signed MSB multiply subtract", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "DSP extension"},
	{"SMMUL", "Signed MSB multiply", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "DSP extension"},
	{"SMMLAR", "Signed MSB multiply accumulate rounded", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "DSP extension"},
	{"SMMLSR", "Signed MSB multiply subtract rounded", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "DSP extension"},
	{"SMMULR", "Signed MSB multiply rounded", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "DSP extension"},
	{"SMLABB", "Signed multiply accumulate BB", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 bottom*bottom"},
	{"SMLABT", "Signed multiply accumulate BT", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 bottom*top"},
	{"SMLALBB", "Signed multiply accumulate long BB", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 to 64-bit BB"},
	{"SMLALBT", "Signed multiply accumulate long BT", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 to 64-bit BT"},
	{"SMLALD", "Dual signed multiply accumulate long", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "Dual 16x16 to 64-bit"},
	{"SMLALTB", "Signed multiply accumulate long TB", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 to 64-bit TB"},
	{"SMLALTT", "Signed multiply accumulate long TT", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 to 64-bit TT"},
	{"SMULBB", "Signed multiply BB", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 bottom*bottom"},
	{"SMULBT", "Signed multiply BT", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 bottom*top"},
	{"SMULTB", "Signed multiply TB", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 top*bottom"},
	{"SMULTT", "Signed multiply TT", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 top*top"},
	{"SMULWB", "Signed multiply wide bottom", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x32 bottom"},
	{"SMULWT", "Signed multiply wide top", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x32 top"},
	{"SMUSD", "Dual signed multiply subtract", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "Dual 16x16 diff"},
	{"SMLATB", "Signed multiply accumulate TB", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 top*bottom"},
	{"SMLATT", "Signed multiply accumulate TT", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x16 top*top"},
	{"SMLAD", "Dual signed multiply accumulate", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "Dual 16x16 sum"},
	{"SMLAWB", "Signed multiply accumulate wide bottom", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x32 bottom accumulate"},
	{"SMLAWT", "Signed multiply accumulate wide top", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "16x32 top accumulate"},
	{"SMLSD", "Dual signed multiply subtract with accumulate", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "Dual 16x16 diff accumulate"},
	{"SMLSLD", "Dual signed multiply subtract long", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "Dual 16x16 to 64-bit diff"},
	{"SMUAD", "Dual signed multiply add", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "Dual 16x16 sum"},
	{"SMLADX", "Dual signed multiply accumulate exchanged", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "Exchange and accumulate"},
	{"UMAAL", "Unsigned multiply accumulate long", InstructionCategory::DSP_MULTIPLY, 1, 0, false, false, false, false, "64-bit dual accumulate"},

	// ===== DSP SATURATE INSTRUCTIONS =====
	{"SSAT16", "Dual 16-bit saturate", InstructionCategory::DSP_SATURATE, 1, 0, false, false, false, false, "Dual signed saturate"},
	{"USAT16", "Dual 16-bit unsigned saturate", InstructionCategory::DSP_SATURATE, 1, 0, false, false, false, false, "Dual unsigned saturate"},

	// ===== DSP PACKING/UNPACKING =====
	{"PKHTB", "Pack halfword top/bottom", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Pack with bottom shift"},
	{"PKHBT", "Pack halfword bottom/top", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Pack with top shift"},
	{"SXTB16", "Dual sign extend byte", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Dual 8->16 signed"},
	{"UXTB16", "Dual zero extend byte", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Dual 8->16 unsigned"},
	{"UXTAB", "Unsigned extend and add byte", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Extend and add"},
	{"UXTAB16", "Dual unsigned extend and add byte", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Dual extend and add"},
	{"UXTAH", "Unsigned extend and add halfword", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Extend and add"},
	{"SXTAB", "Signed extend and add byte", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Extend and add"},
	{"SXTAB16", "Dual signed extend and add byte", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Dual extend and add"},
	{"SXTAH", "Signed extend and add halfword", InstructionCategory::DSP_PACKING, 1, 0, false, false, false, false, "Extend and add"},

	// ===== DSP MISC DATA PROCESSING =====
	{"SEL", "Select bytes based on GE", InstructionCategory::DSP_MISC, 1, 0, false, false, false, false, "GE-based byte selection"},
	{"USAD8", "Unsigned sum of absolute differences", InstructionCategory::DSP_MISC, 1, 0, false, false, false, false, "Quad 8-bit SAD"},
	{"USADA8", "Unsigned SAD with accumulate", InstructionCategory::DSP_MISC, 1, 0, false, false, false, false, "Quad 8-bit SAD + accumulate"},

	// ===== DSP ADDITION INSTRUCTIONS =====
	{"UQADD16", "Dual unsigned saturating add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Dual 16-bit sat add"},
	{"UQADD8", "Quad unsigned saturating add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Quad 8-bit sat add"},
	{"QADD", "Saturating add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "32-bit sat add"},
	{"QADD16", "Dual saturating add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Dual 16-bit sat add"},
	{"QADD8", "Quad saturating add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Quad 8-bit sat add"},
	{"QDADD", "Saturating double and add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Double and sat add"},
	{"SADD8", "Quad signed add with GE", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Sets GE flags"},
	{"SADD16", "Dual signed add with GE", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Sets GE flags"},
	{"SHADD16", "Dual signed halving add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Average of two values"},
	{"SHADD8", "Quad signed halving add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Average of two values"},
	{"UADD16", "Dual unsigned add with GE", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Sets GE flags"},
	{"UADD8", "Quad unsigned add with GE", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Sets GE flags"},
	{"UHADD16", "Dual unsigned halving add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Average of two values"},
	{"UHADD8", "Quad unsigned halving add", InstructionCategory::DSP_ADDITION, 1, 0, false, false, false, false, "Average of two values"},

	// ===== DSP SUBTRACTION INSTRUCTIONS =====
	{"QDSUB", "Saturating double and subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Double and sat sub"},
	{"UQSUB16", "Dual unsigned saturating subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Dual 16-bit sat sub"},
	{"UQSUB8", "Quad unsigned saturating subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Quad 8-bit sat sub"},
	{"QSUB", "Saturating subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "32-bit sat sub"},
	{"QSUB16", "Dual saturating subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Dual 16-bit sat sub"},
	{"QSUB8", "Quad saturating subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Quad 8-bit sat sub"},
	{"SHSUB16", "Dual signed halving subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Halved subtraction"},
	{"SHSUB8", "Quad signed halving subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Halved subtraction"},
	{"SSUB16", "Dual signed subtract with GE", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Sets GE flags"},
	{"SSUB8", "Quad signed subtract with GE", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Sets GE flags"},
	{"UHSUB16", "Dual unsigned halving subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Halved subtraction"},
	{"UHSUB8", "Quad unsigned halving subtract", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Halved subtraction"},
	{"USUB16", "Dual unsigned subtract with GE", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Sets GE flags"},
	{"USUB8", "Quad unsigned subtract with GE", InstructionCategory::DSP_SUBTRACTION, 1, 0, false, false, false, false, "Sets GE flags"},

	// ===== DSP PARALLEL ADD/SUB =====
	{"UQASX", "Dual unsigned saturating add/sub exchange", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Add top, sub bottom"},
	{"UQSAX", "Dual unsigned saturating sub/add exchange", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Sub top, add bottom"},
	{"SASX", "Dual signed add/sub exchange with GE", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Sets GE flags"},
	{"QASX", "Dual saturating add/sub exchange", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Saturating parallel ops"},
	{"QSAX", "Dual saturating sub/add exchange", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Saturating parallel ops"},
	{"SHASX", "Dual signed halving add/sub exchange", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Halved parallel ops"},
	{"SHSAX", "Dual signed halving sub/add exchange", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Halved parallel ops"},
	{"SSAX", "Dual signed sub/add exchange with GE", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Sets GE flags"},
	{"UASX", "Dual unsigned add/sub exchange with GE", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Sets GE flags"},
	{"UHASX", "Dual unsigned halving add/sub exchange", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Halved parallel ops"},
	{"UHSAX", "Dual unsigned halving sub/add exchange", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Halved parallel ops"},
	{"USAX", "Dual unsigned sub/add exchange with GE", InstructionCategory::DSP_PARALLEL, 1, 0, false, false, false, false, "Sets GE flags"},
};

// Helper function to get instruction count
constexpr size_t GetInstructionTimingCount() {
	return sizeof(ARMV7M_INSTRUCTION_TIMING) / sizeof(InstructionTiming);
}

// Helper function to find instruction timing by mnemonic
inline const InstructionTiming* FindInstructionTiming(const char* mnemonic) {
	for (size_t i = 0; i < GetInstructionTimingCount(); i++) {
		if (strcmp(ARMV7M_INSTRUCTION_TIMING[i].mnemonic, mnemonic) == 0) {
			return &ARMV7M_INSTRUCTION_TIMING[i];
		}
	}
	return nullptr;
}

// Category name helper
inline const char* GetCategoryName(InstructionCategory cat) {
	switch (cat) {
		case InstructionCategory::MOVE: return "Move";
		case InstructionCategory::ADD_SUB: return "Add/Subtract";
		case InstructionCategory::MULTIPLY: return "Multiply";
		case InstructionCategory::DIVIDE: return "Divide";
		case InstructionCategory::SATURATE: return "Saturate";
		case InstructionCategory::COMPARE: return "Compare";
		case InstructionCategory::LOGICAL: return "Logical";
		case InstructionCategory::SHIFT: return "Shift";
		case InstructionCategory::ROTATE: return "Rotate";
		case InstructionCategory::COUNT: return "Count";
		case InstructionCategory::LOAD: return "Load";
		case InstructionCategory::STORE: return "Store";
		case InstructionCategory::PUSH_POP: return "Push/Pop";
		case InstructionCategory::SEMAPHORE: return "Semaphore";
		case InstructionCategory::BRANCH: return "Branch";
		case InstructionCategory::STATE_CHANGE: return "State Change";
		case InstructionCategory::EXTEND: return "Extend";
		case InstructionCategory::BIT_FIELD: return "Bit Field";
		case InstructionCategory::REVERSE: return "Reverse";
		case InstructionCategory::HINT: return "Hint";
		case InstructionCategory::BARRIER: return "Barrier";
		case InstructionCategory::DSP_MULTIPLY: return "DSP Multiply";
		case InstructionCategory::DSP_SATURATE: return "DSP Saturate";
		case InstructionCategory::DSP_PACKING: return "DSP Packing";
		case InstructionCategory::DSP_ADDITION: return "DSP Addition";
		case InstructionCategory::DSP_SUBTRACTION: return "DSP Subtraction";
		case InstructionCategory::DSP_PARALLEL: return "DSP Parallel";
		case InstructionCategory::DSP_MISC: return "DSP Misc";
		default: return "Unknown";
	}
}
