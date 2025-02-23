#include "Core.hpp"

int Core_var_status;
int Core_var_Memory_init;

void Core_mainThread() {
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


