#include "Core.hpp"

int Core_var_status;
int Core_var_Memory_init;


int test_pericpu_count = 0;
int test_peri0_count = 0;
int test_peri1_count = 0;
int test_peri2_count = 0;
int test_peri3_count = 0;

void test_pericpu() {
	// printf("pericpu executing... the time is: %d\n", Clock_currenttime());
	test_pericpu_count += 1;
}
void test_peri0() {
	// printf("mul0 -> peri0 executing... the time is: %d\n", Clock_currenttime());
	test_peri0_count += 1;
}
void test_peri1() {
	// printf("mul0 -> peri1 executing... the time is: %d\n", Clock_currenttime());
	test_peri1_count += 1;
}
void test_peri2() {
	// printf("mul1 -> peri2 executing... the time is: %d\n", Clock_currenttime());
	test_peri2_count += 1;
}
void test_peri3() {
	// printf("mul1 -> peri3 executing... the time is: %d\n", Clock_currenttime());
	test_peri3_count += 1;

	printf("counting result: cpu: %d, peri0(0.7): %d, peri1(0.7): %d, peri2(0.3): %d, peri3(0.5): %d\n", test_pericpu_count, 
		test_peri0_count, test_peri1_count, test_peri2_count, test_peri3_count);
}


void Core_mainThread() {
	// FOR NOW: we shall do test running here

	Clock_body_main();


	while (1) {
		Sleep(100);
		if (Core_var_status != 1) {
			printf("paused\n");
			continue;
		}
	
		printf("running\n");
	
		if (Core_var_Memory_init) {
			uint8* dat;
			// memory check
			printf("memory read on 0x20000000, it should work\n");
			dat = (uint8*)Memory_read(0x20000000, Memory_enum_size::u8);
			if (dat != NULL) printf("val = %02x\n", *dat);
			printf("memory write on 0x20000000, it should work\n");
			Memory_write(0x20000000, Memory_enum_size::u8, 0x12);

			printf("memory read on 0x20000000, it should work\n");
			dat = (uint8*)Memory_read(0x20000000, Memory_enum_size::u8);
			if (dat != NULL) printf("val = %02x\n", *dat);

			printf("memory read on 0x1FFFFFFF, it should NOT work\n");
			dat = (uint8*)Memory_read(0x1FFFFFFF, Memory_enum_size::u8);
			if(dat != NULL) printf("val = %02x\n", *dat);



			// 32bit memory access check

			printf("memory write on 0x20000000, it should work\n");
			Memory_write(0x20000000, Memory_enum_size::u32, 0xDEADBEEF);

			printf("memory read on 0x20000000, it should work\n");
			dat = (uint8*)Memory_read(0x20000000, Memory_enum_size::u8);
			if (dat != NULL) printf("val = %02x\n", *dat);
			dat = (uint8*)Memory_read(0x20000001, Memory_enum_size::u8);
			if (dat != NULL) printf("val = %02x\n", *dat);
			dat = (uint8*)Memory_read(0x20000002, Memory_enum_size::u8);
			if (dat != NULL) printf("val = %02x\n", *dat);
			dat = (uint8*)Memory_read(0x20000003, Memory_enum_size::u8);
			if (dat != NULL) printf("val = %02x\n", *dat);

			// memory write supported upto 4 byte writes in little-endian. only 1 byte supported for read.

		}
	}

}

void Core_start(Thread_data* mydata) {

	// FOR NOW: we shall do test related init in here

	Clock_init();
	
	// do scheduling first
	struct Clock_struct clockgen;	// for master clock
	struct Clock_struct clockpericpu;	// cpu
	struct Clock_struct clockmul0;	// for peri0 peri1
	struct Clock_struct clockmul1;	// for per2
	struct Clock_struct clockperi0;		// peri0
	struct Clock_struct clockperi1;		// peri1
	struct Clock_struct clockperi2;		// peri2
	struct Clock_struct clockmul2;	// to mul0
	struct Clock_struct clockperi3;	// peri3

	clockgen.baseclock = 100;	// 10hz
	clockgen.clock_type = Clock_type_enum::master;
	Clock_add(0, &clockgen);

	clockpericpu.linked_by = 0;
	clockpericpu.clock_type = Clock_type_enum::peri;
	clockpericpu.objfunc = test_pericpu;
	Clock_add(1, &clockpericpu);

	clockmul0.linked_by = 0;
	clockmul0.clock_type = Clock_type_enum::midobj;
	clockmul0.multiplier = 70;	// 0.7
	Clock_add(2, &clockmul0);
	
	clockperi0.linked_by = 2;
	clockperi0.clock_type = Clock_type_enum::peri;
	clockperi0.objfunc = test_peri0;
	Clock_add(3, &clockperi0);

	clockperi1.linked_by = 2;
	clockperi1.clock_type = Clock_type_enum::peri;
	clockperi1.objfunc = test_peri1;
	Clock_add(4, &clockperi1);

	clockmul1.linked_by = 0;
	clockmul1.clock_type = Clock_type_enum::midobj;
	clockmul1.multiplier = 30;	// 0.3
	Clock_add(5, &clockmul1);

	clockperi2.linked_by = 5;
	clockperi2.clock_type = Clock_type_enum::peri;
	clockperi2.objfunc = test_peri2;
	Clock_add(6, &clockperi2);

	clockmul2.linked_by = 2;
	clockmul2.clock_type = Clock_type_enum::midobj;
	clockmul2.multiplier = 70;  // 0.7 * 0.7 = 0.49999...0.5
	Clock_add(7, &clockmul2);

	clockperi3.linked_by = 7;
	clockperi3.clock_type = Clock_type_enum::peri;
	clockperi3.objfunc = test_peri3;
	Clock_add(8, &clockperi3);

	Clock_ready();
	
	mydata->func = Core_mainThread;
	mydata->param1 = 0;
	mydata->param2 = 0;
	printf("start!\n");
	Core_var_status = 1;	// start


	// core module status init
	Core_var_Memory_init = 0;
	Memory_init();
	Core_var_Memory_init = 1;
}


void Core_pause() {
	printf("pause!\n");
	Core_var_status = 2;	// pause

}


void Core_resume() {
	printf("resume!\n");
	Core_var_status = 1;	// start

}


