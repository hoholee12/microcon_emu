#include "Core.hpp"

int Core_var_status;
int Core_var_Memory_init;


int test_pericpu_count = 0;
int test_peri0_count = 0;
int test_peri1_count = 0;
int test_peri2_count = 0;
int test_peri3_count = 0;
int test_peri4_count = 0;

void test_pericpu() {
    eprintf("pericpu executing... the time is: %d\n", Clock_currenttime());
    test_pericpu_count += 1;
}
void test_peri0() {
    eprintf("mul0 -> peri0 executing... the time is: %d\n", Clock_currenttime());
    test_peri0_count += 1;
    eprintf("peri0: Clock_var_hintcycles: %d\n", CLOCK_GET_AVAILABLE_CYCLES());
}
void test_peri1() {
    eprintf("mul0 -> peri1 executing... the time is: %d\n", Clock_currenttime());
    test_peri1_count += 1;
    eprintf("peri1: Clock_var_hintcycles: %d\n", CLOCK_GET_AVAILABLE_CYCLES());
}
void test_peri2() {
    eprintf("mul1 -> peri2 executing... the time is: %d\n", Clock_currenttime());
    test_peri2_count += 1;
    eprintf("peri2: Clock_var_hintcycles: %d\n", CLOCK_GET_AVAILABLE_CYCLES());
}
void test_peri3() {
    eprintf("mul1 -> peri3 executing... the time is: %d\n", Clock_currenttime());
    test_peri3_count += 1;

    eprintf("counting result: cpu: %d, peri0(0.7): %d, peri1(0.7): %d, peri2(0.3): %d, peri3(0.5): %d, Clock_var_hintcycles: %d\n", test_pericpu_count, 
    	test_peri0_count, test_peri1_count, test_peri2_count, test_peri3_count, CLOCK_GET_AVAILABLE_CYCLES());
    // TODO: clock drift test (must not drift, otherwise accuracy is at stake!)

    CLOCK_SET_USE_CYCLES = CLOCK_GET_AVAILABLE_CYCLES();
    
    if (test_peri3_count % 10 == 0) {

        struct Clock_struct clockmul0;	// to mul0
        Clock_init();
        clockmul0.linked_by = 0;
        clockmul0.clock_type = Clock_type_enum::midobj;
        clockmul0.multiplier = 70;	// 0.7
        Clock_replace(2, &clockmul0);
        Clock_ready();
    }

    if (Core_var_Memory_init) {
        uint8* dat;
        // memory check
        eprintf("memory read on 0x20000000, it should work\n");
        dat = (uint8*)Memory_read(0x20000000, Memory_enum_size::u8, MEMORY_ATTRIB_S_R);
        if (dat != NULL) eprintf("val = %02x\n", *dat);
        eprintf("memory write on 0x20000000, it should work\n");
        Memory_write(0x20000000, Memory_enum_size::u8, 0x12, MEMORY_ATTRIB_S_W);

        eprintf("memory read on 0x20000000, it should work\n");
        dat = (uint8*)Memory_read(0x20000000, Memory_enum_size::u8, MEMORY_ATTRIB_S_R);
        if (dat != NULL) eprintf("val = %02x\n", *dat);

        eprintf("memory read on 0x1FFFFFFF, it should NOT work\n");
        dat = (uint8*)Memory_read(0x1FFFFFFF, Memory_enum_size::u8, MEMORY_ATTRIB_S_R);
        if (dat != NULL) eprintf("val = %02x\n", *dat);

        eprintf("memory read on 0x20000000 with wrong attribute, it should NOT work\n");
        dat = (uint8*)Memory_read(0x20000000, Memory_enum_size::u8, MEMORY_ATTRIB_U_R);
        if (dat != NULL) eprintf("val = %02x\n", *dat);

        // 32bit memory access check

        eprintf("memory write on 0x20000000, it should work\n");
        Memory_write(0x20000000, Memory_enum_size::u32, 0xDEADBEEF, MEMORY_ATTRIB_S_R);

        eprintf("memory read on 0x20000000, it should work\n");
        dat = (uint8*)Memory_read(0x20000000, Memory_enum_size::u8, MEMORY_ATTRIB_S_R);
        if (dat != NULL) eprintf("val = %02x\n", *dat);
        dat = (uint8*)Memory_read(0x20000001, Memory_enum_size::u8, MEMORY_ATTRIB_S_R);
        if (dat != NULL) eprintf("val = %02x\n", *dat);
        dat = (uint8*)Memory_read(0x20000002, Memory_enum_size::u8, MEMORY_ATTRIB_S_R);
        if (dat != NULL) eprintf("val = %02x\n", *dat);
        dat = (uint8*)Memory_read(0x20000003, Memory_enum_size::u8, MEMORY_ATTRIB_S_R);
        if (dat != NULL) eprintf("val = %02x\n", *dat);

        // memory write supported upto 4 byte writes in little-endian. only 1 byte supported for read.

    }
    
}

void test_peri_print() {
    test_peri4_count += 1;
    // eprintf("%d seconds passed. slept: %d\n", test_peri4_count, Clock_var_sleepfor);

}

enum hell { op321, op123, op456, op654, op234, op432, op111 };

void hello_opt(int* data, int* value) {
	int i = 0;
	static int op_table[1000];

	op_table[321] = op321;
	op_table[123] = op123;
	op_table[456] = op456;
	op_table[654] = op654;
	op_table[234] = op234;
	op_table[432] = op432;
	op_table[111] = op111;
	while (1) {
		switch (op_table[data[i++]]) {	// the case values must be adjacent to each other for the compiler to optimize it to jump table (jump table confirmed at O0)
		case op321:
			*value *= *value;
			break;
		case op123:
			*value += *value;
			break;
		case op456:
			*value /= *value;
			break;
		case op654:
			*value -= *value;
			break;
		case op234:
			*value = *value + 123;
			break;
		case op432:
			*value = *value + 321;
			break;
		case op111:
			return;
		default:
			__assume(0);	// this shit dont really matter, the compiler already optimizes it out on O0
			break;
		}
	}
}

void Core_mainThread() {
	// FOR NOW: we shall do test running here

	int* data = (int *)ecalloc(0x10000, sizeof(int));

	for (int i = 0; i < 0x10000; i++) {
		switch (i % 6) {
		case 0:
			data[i] = 123;
			break;
		case 1:
			data[i] = 321;
			break;

		case 2:
			data[i] = 234;
			break;
		case 3:
			data[i] = 432;
			break;
		case 4:
			data[i] = 456;
			break;
		case 5:
			data[i] = 654;
			break;

		}
	}
	data[0x10000 - 1] = 111;
	int value = 121;

	hello_opt(data, &value);			// 1st place

	// eprintf("value = %d\n", value);





	//return;

	Clock_body_main();

}

void Core_start(Thread_data* mydata) {

	// FOR NOW: we shall do test related init in here

	logalloc_init();
// #ifdef RELATIVE_INDEXING
// 	efree((uint32*)(logalloc_pool + 0x2));
// #else
// 	efree((uint32*)(logalloc_pool + 0x3));
// #endif
	uint32* hello = emalloc(10 * sizeof(uint32));
	for(int i = -3; i < 10; i++){
		eprintf("%d\n", hello[i]);
	}
	efree(hello);
	hello = emalloc(10 * sizeof(uint32));
	for (int i = -3; i < 10; i++) {
		eprintf("%d\n", hello[i]);
	}
	efree(hello);
	hello = emalloc(12 * sizeof(uint32)); // should work fine upto this point
	// for (int i = -3; i < 12; i++) {
	// 	hello[i] = i;	// it will corrupt header
	// 	eprintf("%d\n", hello[i]);
	// }
	// efree(hello); // this should cause assertion error

	/* memory allocator aging test */
	// /* TC1: allocate and free a 999KB block 1000 times */
	// for (int i = 0; i < 1000; i++) {
	// 	uint32* test = emalloc(999 * 1024);
	// 	efree(test);
	// }

	// /* TC2: allocate a 9KB block 1000 times */
	// for (int i = 0; i < 1000; i++) {
	// 	uint32* test = emalloc(9 * 1024);
	// 	efree(test);
	// }

	// /* TC3: small allocations (1KB), high frequency */
	// for (int i = 0; i < 5000; i++) {
	// 	uint32* test = emalloc(1 * 1024);
	// 	efree(test);
	// }

	// /* TC4: medium allocations (100KB), moderate frequency */
	// for (int i = 0; i < 500; i++) {
	// 	uint32* test = emalloc(100 * 1024);
	// 	efree(test);
	// }

	// /* TC5: mixed size allocations in single iteration */
	// for (int i = 0; i < 500; i++) {
	// 	uint32* test1 = emalloc(10 * 1024);
	// 	uint32* test2 = emalloc(50 * 1024);
	// 	uint32* test3 = emalloc(5 * 1024);
	// 	efree(test1);
	// 	efree(test2);
	// 	efree(test3);
	// }

	// /* TC6: multiple allocations before freeing (fragmentation test) */
	// for (int i = 0; i < 100; i++) {
	// 	uint32* blocks[10];
	// 	for (int j = 0; j < 10; j++) {
	// 		blocks[j] = emalloc(32 * 1024);
	// 	}
	// 	for (int j = 0; j < 10; j++) {
	// 		efree(blocks[j]);
	// 	}
	// }

	// /* TC7: alternating large and small blocks */
	// for (int i = 0; i < 500; i++) {
	// 	uint32* large = emalloc(512 * 1024);
	// 	uint32* small_block = emalloc(2 * 1024);
	// 	efree(small_block);
	// 	efree(large);
	// }

	// /* TC8: stress test with varying allocation patterns */
	// for (int i = 0; i < 200; i++) {
	// 	uint32* test1 = emalloc(64 * 1024);
	// 	uint32* test2 = emalloc(16 * 1024);
	// 	uint32* test3 = emalloc(128 * 1024);
	// 	efree(test2);
	// 	efree(test1);
	// 	efree(test3);
	// }

	/* TC9: test block coalescing of consecutive freed blocks */
	eprintf("TC9: test block coalescing - fill pool near capacity\n");
	uint32* coal_blocks[200];
	int coal_count = 0;
	uint32 total_alloc_size = 0;

	// Fill pool to near capacity (200 x 5KB ≈ 1MB)
	for (int i = 0; i < 200; i++) {
		coal_blocks[i] = emalloc(5 * 1024);
		if (coal_blocks[i] == NULL) {
			eprintf("TC9: pool full at allocation %d\n", i);
			break;
		}
		
		// Write pattern to the 5KB block
		int write_words = (5 * 1024) / sizeof(uint32);
		for (int w = 0; w < write_words; w++) {
			coal_blocks[i][w] = 0xCAFEBABE + i;
		}
		
		// Verify data integrity immediately
		int verify_error = 0;
		for (int w = 0; w < write_words; w++) {
			if (coal_blocks[i][w] != (0xCAFEBABE + i)) {
				eprintf("TC9: DATA CORRUPTION in initial 5KB block %d, word %d: expected 0x%x, got 0x%x\n", i, w, 0xCAFEBABE + i, coal_blocks[i][w]);
				verify_error = 1;
				break;
			}
		}
		
		if (i > 0) {
			uint32 chunk_size = (uint32)coal_blocks[i] - (uint32)coal_blocks[i-1];
			total_alloc_size += chunk_size;
			if (!verify_error) {
				eprintf("TC9: alloc %d at 0x%p, chunk: %u bytes, cumulative: %.2f KB [VERIFIED]\n", i, coal_blocks[i], chunk_size, total_alloc_size / 1024.0);
			}
		} else {
			if (!verify_error) {
				eprintf("TC9: alloc %d at 0x%p (first block) [VERIFIED]\n", i, coal_blocks[i]);
			}
		}
		coal_count++;
	}
	eprintf("TC9: allocated %d blocks, total actual size: %u bytes (%.2f KB)\n", coal_count, total_alloc_size, total_alloc_size / 1024.0);

	// Free pattern: free 2 consecutive, keep 2 consecutive, repeat
	eprintf("TC9: freeing pattern - free 2 consecutive, keep 2 consecutive, repeat\n");
	int freed_pairs = 0;
	uint32 total_freed_size = 0;
	for (int i = 0; i < coal_count; i += 4) {
		if (i + 1 < coal_count) {
			// Verify data in blocks being freed (should be uncorrupted)
			int write_words = (5 * 1024) / sizeof(uint32);
			int verify_error_0 = 0, verify_error_1 = 0;
			for (int w = 0; w < write_words; w++) {
				if (coal_blocks[i][w] != (0xCAFEBABE + i)) {
					eprintf("TC9: CORRUPTION BEFORE FREE block [%d], word %d\n", i, w);
					verify_error_0 = 1;
					break;
				}
				if (coal_blocks[i+1][w] != (0xCAFEBABE + i + 1)) {
					eprintf("TC9: CORRUPTION BEFORE FREE block [%d], word %d\n", i+1, w);
					verify_error_1 = 1;
					break;
				}
			}
			
			uint32 freed_size = ((uint32)coal_blocks[i+1] - (uint32)coal_blocks[i]) + 5128;
			total_freed_size += freed_size;
			eprintf("TC9: freeing [%d-%d] at 0x%p, 0x%p, gap: %u bytes, cumulative freed: %.2f KB\n", i, i+1, coal_blocks[i], coal_blocks[i+1], freed_size, total_freed_size / 1024.0);
			efree(coal_blocks[i]);
			efree(coal_blocks[i+1]);
			coal_blocks[i] = NULL;
			coal_blocks[i+1] = NULL;
			freed_pairs++;
		}
		
		// Verify remaining blocks haven't been corrupted
		if (i + 2 < coal_count) {
			int write_words = (5 * 1024) / sizeof(uint32);
			for (int w = 0; w < write_words; w++) {
				if (coal_blocks[i+2][w] != (0xCAFEBABE + i + 2)) {
					eprintf("TC9: CORRUPTION in kept block [%d], word %d\n", i+2, w);
					break;
				}
			}
		}
		if (i + 3 < coal_count) {
			int write_words = (5 * 1024) / sizeof(uint32);
			for (int w = 0; w < write_words; w++) {
				if (coal_blocks[i+3][w] != (0xCAFEBABE + i + 3)) {
					eprintf("TC9: CORRUPTION in kept block [%d], word %d\n", i+3, w);
					break;
				}
			}
		}
	}
	eprintf("TC9: freed %d pairs, total freed size: %u bytes (%.2f KB)\n", freed_pairs, total_freed_size, total_freed_size / 1024.0);

	// Try to allocate 10KB blocks into the coalesced gaps
	eprintf("TC9: attempting to allocate 10KB blocks into coalesced gaps (keep alive to verify different addresses)\n");
	uint32* coal_realloc_blocks[52];
	uint32 total_realloc_size = 0;
	int realloc_count = 0;
	
	// Allocate all 52 blocks and keep them alive
	for (int j = 0; j < 52; j++) {
		coal_realloc_blocks[j] = emalloc(10 * 1024);
		if (coal_realloc_blocks[j] == NULL) {
			eprintf("TC9: failed to allocate 10KB block at attempt %d\n", j);
			break;
		}
		
		// Write pattern to the block
		int write_words = (10 * 1024) / sizeof(uint32);
		for (int w = 0; w < write_words; w++) {
			coal_realloc_blocks[j][w] = 0xDEADBEEF + j;
		}
		
		eprintf("TC9: realloc %d at 0x%p - allocated\n", j, coal_realloc_blocks[j]);
		realloc_count++;
	}
	
	// Verify all blocks have correct data and different addresses
	eprintf("TC9: verifying all %d blocks have unique addresses and correct data\n", realloc_count);
	for (int j = 0; j < realloc_count; j++) {
		int write_words = (10 * 1024) / sizeof(uint32);
		int verify_error = 0;
		
		// Verify data integrity
		for (int w = 0; w < write_words; w++) {
			if (coal_realloc_blocks[j][w] != (0xDEADBEEF + j)) {
				eprintf("TC9: DATA CORRUPTION in block %d, word %d: expected 0x%x, got 0x%x\n", j, w, 0xDEADBEEF + j, coal_realloc_blocks[j][w]);
				verify_error = 1;
				break;
			}
		}
		
		// Check for duplicate addresses
		for (int k = 0; k < j; k++) {
			if (coal_realloc_blocks[j] == coal_realloc_blocks[k]) {
				eprintf("TC9: ADDRESS COLLISION - blocks %d and %d both at 0x%p\n", j, k, coal_realloc_blocks[j]);
				verify_error = 1;
			}
		}
		
		if (!verify_error) {
			eprintf("TC9: block %d at 0x%p - data verified OK\n", j, coal_realloc_blocks[j]);
		}
	}
	
	// Free all blocks
	eprintf("TC9: freeing all %d reallocated blocks\n", realloc_count);
	for (int j = 0; j < realloc_count; j++) {
		efree(coal_realloc_blocks[j]);
	}
	eprintf("TC9: write/verify/free test complete - allocated/verified/freed %d blocks\n", realloc_count);

	// Free remaining original blocks
	for (int i = 0; i < coal_count; i++) {
		if (coal_blocks[i] != NULL) {
			efree(coal_blocks[i]);
		}
	}
	eprintf("TC9: cleanup complete\n");

	exit(0);


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
	struct Clock_struct testmul;
	struct Clock_struct testperi;

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

	testmul.linked_by = 0;
	testmul.clock_type = Clock_type_enum::midobj;
	testmul.multiplier = 1;	// 0.01 -> every 1 second
	Clock_add(9, &testmul);

	testperi.linked_by = 9;
	testperi.clock_type = Clock_type_enum::peri;
	testperi.objfunc = test_peri_print;
	Clock_add(10, &testperi);

	Clock_ready();
	
	mydata->func = Core_mainThread;
	mydata->param1 = 0;
	mydata->param2 = 0;
	eprintf("start!\n");
	Core_var_status = 1;	// start


	// core module status init
	Core_var_Memory_init = 0;
	Memory_init();
	Core_var_Memory_init = 1;
}


void Core_pause() {
	eprintf("pause!\n");
	Core_var_status = 2;	// pause

}


void Core_resume() {
	eprintf("resume!\n");
	Core_var_status = 1;	// start

}


