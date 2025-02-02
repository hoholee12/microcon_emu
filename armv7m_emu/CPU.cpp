#include "CPU.hpp"


struct CPU_struct_reg CPU_var_reg = { 0, };

void CPU_init(uint32 pc_pos, uint32 sp_pos) {
	// init to zero
	memset(&CPU_var_reg, NULL, 16);
	// set start pc
	CPU_write_PC(pc_pos);
	CPU_write_SP(sp_pos);

	// clear all apsr flags
	CPU_write_apsr_n(0);
	CPU_write_apsr_z(0);
	CPU_write_apsr_c(0);
	CPU_write_apsr_v(0);


}

// execute one cycle
void CPU_fetch() {

	// hello



}