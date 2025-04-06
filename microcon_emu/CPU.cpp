#include "CPU.hpp"
#include "Memory.hpp"


struct CPU_struct_reg CPU_var_reg = { 0, };

uint32 CPU_op_vect[0xFF];
uint32 CPU_op_vect_ext[0xFF];

void CPU_init(uint32 pc_pos, uint32 sp_pos) {
	// init to zero
	memset(&CPU_var_reg, 0, sizeof(CPU_var_reg));
	// set start pc
	CPU_var_reg.R[15] = pc_pos;
	CPU_var_reg.R[13] = CPU_var_reg.MSP = sp_pos;	// starts with kernel, so MSP


}

// execute one cycle: this goes in the clock scheduler as cpu peri
void CPU_fetch() {
	// capture context
	CPU_struct_reg CPU_reg_capture = CPU_var_reg;

	// get pc and get data from memory
	uint32 data = *(uint32*)Memory_read(CPU_reg_capture.R[15], Memory_enum_size::u32);

	// 


}

