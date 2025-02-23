#include "Clock.hpp"

/*
*
* clockspeed simulator
* range from 1hz ~ 100,000,000hz(100mhz)
* features:
* - live changing of clockspeed
* - ability to schedule multiple peripherals(including the cpu)
* - support for intermediatry object for immitating middle clock generator
* ex)
* - master clock
* -- middle object
* --- cpu
* --- middle object
* ---- peripheral1
* ---- peripheral2
* --- peripheral3
* 
* calculate clockspeed by multiplier of each middle objects:
* ex)
* 100mhz (master) -> 200mhz (middle x2.0) -> 200mhz (cpu)
*                                         -> 50mhz (middle x0.25) -> 50mhz (peripheral1)
*                                                                 -> 50mhz (peripheral2)
*                                         -> 200mhz (peripheral3)
* 
*/

void Clock_init(uint32 clockspeed, uint32 virtual_clockspeed) 
{
	Clock_arr_map = 0;
	Clock_var_tick = virtual_clockspeed;
	Clock_var_maxtickrate = 0;
}

// main body for counting ticks and launching objfuncs.
void Clock_body()
{



}

// function to insert clock objects in. master must always be index 0!!
void Clock_add(uint32 index, Clock_struct* clock_obj) 
{
	if (index < 0 && index >= CLOCK_MAX_SCHEDULE_SIZE) {
		printf("clock obj index out of range!\n");
		return;
	}

	if ((index == 0 && clock_obj->clock_type != Clock_type_enum::master)
	|| (index != 0 && clock_obj->clock_type == Clock_type_enum::master)) {
		printf("only master obj can be inserted to index 0\n");
		return;
	}


	if ((Clock_arr_map >> index) & 0b1) {
		printf("index already taken\n");
		return;
	}

	Clock_arr_map |= (0b1 << index);
	Clock_arr[index] = *clock_obj;	// copy obj

}

/* Clock schedule arr
*
* malloc memory
* one long cylinder that plays instruments based on its bulge pins
*
* uint32 (32bits) -> 32 functions possible at one tick
*/

uint32 Clock_ready_getmaxvalperobj(uint32 masterclock, uint32 startindex) {
	uint32 idx = startindex;
	uint32 maxval = 1;
	// if start from peri, start from midobj one up
	if (Clock_arr[idx].clock_type == Clock_type_enum::peri) {
		idx = Clock_arr[idx].linked_by;
	}
	// keep looping until master
	while (Clock_arr[idx].clock_type == Clock_type_enum::midobj) {
		maxval *= Clock_arr[idx].multiplier;
		idx = Clock_arr[idx].linked_by;
	}

	// error if end is not master
	if (Clock_arr[idx].clock_type != Clock_type_enum::master) {
		printf("could not find master clock. check if your clock obj links are proper\n");
		return 0;
	}

	return masterclock * maxval;
}

uint32 Clock_gcd(uint32 a, uint32 b) {
	while (b != 0) { uint32 temp = b; b = a % b; a = temp; }
	return a;
}
uint32 Clock_lcm(uint32 a, uint32 b) {
	return (a / Clock_gcd(a, b)) * b;
}

void Clock_ready() 
{
	// calculate LCM
	uint32 masterclock = 0;
	uint32 tmpclock = 0;
	uint32 Clock_tickratearr[CLOCK_MAX_SCHEDULE_SIZE];
	for (int i = 0; i < CLOCK_MAX_SCHEDULE_SIZE; i++) {
		if ((Clock_arr_map >> i) & 0b1) {
			switch (Clock_arr[i].clock_type) {
			case Clock_type_enum::master:
				masterclock = Clock_arr[i].baseclock;
				Clock_var_maxtickrate = masterclock;
				Clock_tickratearr[i] = masterclock;
				break;
			case Clock_type_enum::midobj:
			case Clock_type_enum::peri:	//including cpu
				tmpclock = Clock_ready_getmaxvalperobj(masterclock, i);
				Clock_var_maxtickrate = Clock_lcm(Clock_var_maxtickrate, tmpclock);
				Clock_tickratearr[i] = tmpclock;
				break;
			default:
				break;
			}
		
		}
	}

	// generate tape
	Clock_schedule_arr = (uint32*)calloc(Clock_var_maxtickrate, sizeof(uint32));

	// hammer the pins in


}