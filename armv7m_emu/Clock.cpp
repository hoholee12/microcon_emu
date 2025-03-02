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
	Clock_var_wake = 1;
	Clock_var_tick = virtual_clockspeed;	// this is only to be used to control the speed of overall simulation
	Clock_var_maxtickrate = 0;				// virtual_clockspeed(Clock_var_tick) does not count in sim scheduling
}

// main body for counting ticks and launching objfuncs.
void Clock_body()
{
	const uint32 one_second = 1000;
	uint32 sleep_for = one_second;	// TODO

	while (1) {
		// the nature of the os does not allow us to do hires sleep. we shall make it coarse.
		for (int i = 0; i < Clock_var_maxtickrate; i++) {
			uint32 Clock_curmap = Clock_schedule_arr[i];
			if (Clock_curmap != 0) {	// just check the bitmap in its entirety and skip if there isnt anything to trigger.
				for (int j = 0; j < CLOCK_MAX_SCHEDULE_SIZE; j++) {
					if ((Clock_curmap >> j) & 0x1) {
						Clock_arr[j].objfunc();	// run!
					}
				}
				Clock_pause_sleep();	// user pauses simulation (infloop until resume)
			}
		}
		Sleep(sleep_for);
	}
}

static inline void Clock_pause_sleep() {
	while (Clock_var_wake == 0) {
		Sleep(100);
	}
}

void Clock_pause() {
	Clock_var_wake = 0;
}

void Clock_resume() {
	Clock_var_wake = 1;
}

// function to insert clock objects in. master must always be index 0!!
void Clock_add(uint32 index, Clock_struct* clock_obj) 
{
	// sanity check
	if (index < 0 && index >= CLOCK_MAX_SCHEDULE_SIZE) {
		printf("clock obj index out of range!\n");
		return;
	}

	if ((index == 0 && clock_obj->clock_type != Clock_type_enum::master)
	|| (index != 0 && clock_obj->clock_type == Clock_type_enum::master)) {
		printf("only master obj can be inserted to index 0\n");
		return;
	}


	if ((Clock_arr_map >> index) & 0x1) {
		printf("index already taken\n");
		return;
	}
	// end sanity check

	Clock_arr_map |= (0x1 << index);
	Clock_arr[index] = *clock_obj;	// copy obj

}

/* Clock schedule arr
*
* malloc memory
* one long cylinder with bulging pins that plays instruments when strike
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
		printf("could not find master clock. check if your clock obj links are properly setup\n");
		return 0;
	}

	return masterclock * maxval;
}

void Clock_ready()
{
	uint32 masterclock = 0;
	uint32 tmpclock = 0;
	uint32 Clock_tickratearr[CLOCK_MAX_SCHEDULE_SIZE];

	uint32 usec_per_tick = 0;

	memset(Clock_tickratearr, 0, CLOCK_MAX_SCHEDULE_SIZE);

	// calculate LCM
	for (int i = 0; i < CLOCK_MAX_SCHEDULE_SIZE; i++) {
		if ((Clock_arr_map >> i) & 0x1) {
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

	// get lowest possible clock number by dividing 10s
	int tens = 10;
	while (1) {
		if (Clock_var_maxtickrate - (Clock_var_maxtickrate / tens) * tens == 0) {	/* Clock_var_maxtickrate % tens */
			Clock_var_maxtickrate /= tens;
			Clock_var_tickratemul = tens;

			tens *= 10;
		}
		else {
			break;
		}
	}

	// generate tape
	/*
	* TODO: we might need extra tapes if there are more than 32 peripherals to manage.
	*/

	Clock_schedule_arr = (uint32*)calloc(Clock_var_maxtickrate, sizeof(uint32));

	// hammer the pins in
	/*
	* per peripheral
	*     loop schedule arr
	*
	*/

	for (int i = 0; i < CLOCK_MAX_SCHEDULE_SIZE; i++) {
		if ((Clock_arr_map >> i) & 0x1) {
			switch (Clock_arr[i].clock_type) {
			case Clock_type_enum::peri:	//including cpu
				// get peri 1000 / 500 = 2
				usec_per_tick = Clock_var_maxtickrate / Clock_tickratearr[i];
				for (int j = 0; j < Clock_var_maxtickrate; j++) {
					if (j - (j / usec_per_tick) * usec_per_tick == 0) {	/* j % usec_per_tick */
						Clock_schedule_arr[j] |= (0x1 >> i);	// mark
					}
				}
				break;
			default:
				break;
			}
		}
	}
}