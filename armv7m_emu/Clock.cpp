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


/* maxtickrate based on LCM of all objects (simulated tickrate) */
uint32 Clock_var_maxtickrate;
uint32 Clock_var_tickratemul;	// multiplier for maxtickrate

/* target tickrate(actual tickrate while running) */
uint32 Clock_var_tick;

uint32 Clock_var_wake;

uint32 Clock_arr_map;
uint32* Clock_schedule_arr;	// dynamically allocated
struct Clock_struct Clock_arr[CLOCK_MAX_SCHEDULE_SIZE];

void Clock_init(uint32 clockspeed, uint32 virtual_clockspeed) 
{
	Clock_arr_map = 0;
	Clock_var_wake = 1;
	Clock_var_tick = virtual_clockspeed;	// this is only to be used to control the speed of overall simulation
	Clock_var_maxtickrate = 0;				// virtual_clockspeed(Clock_var_tick) does not count in sim scheduling
	Clock_var_tickratemul = 1; // initial start as 1x
}

static inline void Clock_pause_sleep() {
	while (Clock_var_wake == 0) {
		Sleep(100);	// explicit wait state is better than just yielding
	}
}

// main body for counting ticks
void Clock_body_main()
{
	const uint32 one_second = 1000;
	uint32 syncsize = control_syncsize; //100ms as base sync (sleep_for +1 if less than, sleep_for -1 if more than)
	uint32 syncwhen = one_second / syncsize;	// 1sec / syncsize = 10 times every second
	uint32 sleep_for = one_second / control_fps;	// 16ms


	/*
	* how to calculate sleep:
	* base is 60hz (interface updates 60 times per sec) -> 16ms per sleep (we can just adjust this for base accuracy)
	* 
	* 1000hz (cpu) / 60hz = 16 cycles per frame
	* 
	* 16 + 16 + 16 + 16 + 16 + 16 + 17 + 17 + 17 + 17 + 17 + 17 = 200
	*/
	uint32 prevTime_msec = Clock_gettime_msec();
	uint32 now_msec = prevTime_msec;
	uint32 elapsedTime_msec = prevTime_msec;

	// for the simclock
	uint32 totalsimclock = Clock_var_maxtickrate * Clock_var_tickratemul;
	uint32 simclockcurrent = totalsimclock;
	uint32 simclockloopcount = control_fps;
	uint32 simclocktosend = 0;
	uint32 bn = 0;
	uint32 bi = 0;

	// body loop
	uint32 loopcount = 0;
	while (1) {
		// run + sleep = total frame
		
		// the nature of the app environment does not allow us to do Hi-Res sleep. we shall make it coarse. (but not one second coarse...)

		/*
		* Clock_var_tickratemul * Clock_var_maxtickrate = 1 second tape
		* (whatever value Clock_var_tickratemul or Clock_var_maxtickrate holds, it amounts to exact 1 second)
		* 
		* if Clock_var_tickratemul = 1 -> 1 sleep per 1 second
		* if Clock_var_tickratemul = 6 -> 6 sleeps per 1 second
		* -> it is based on 1 second. 
		* 
		* 1 second = 60hz pause time for control -> 60 sleeps per 1 second required.
		* control needs to tell the scheduler to pause 60 times per 1 second.
		* -> break down the tape 60 times and tell control every break (if tape is 1000hz)
		* -> break down the tape 60 times and tell control every break (if tape is 1khz)
		*    (if tape is 100khz, Clock_var_maxtickrate would stay the same while Clock_var_tickratemul would probably be 100x higher) 
		* 
		* for simplicity:
		* 0. calculate cycles to loop before getting interrupted to control
		* 1. new function for 1 second tape. this function tracks cycles.
		* 2. when cycle is reached, just save the current value n, i, j and return immediately.
		* 3. when control is done, we can resume where we left off with the saved value.
		* 
		* 4. user pause/resume is to be recognized in control.
		* 5. 
		*/

		/* 1000hz 6hz -> 166hz/167hz per frame:
		* 
		* 1st loop - 1000 / 6 -> 166
		* 2nd loop - 834 / 5 -> 166
		* 3rd loop - 668 / 4 -> 167
		* 4th loop - 501 / 3 -> 167
		* 5th loop - 334 / 2 -> 167
		* final loop - 167 / 1 -> 167
		* 
		*/

		// calculate simclock
		if (simclockloopcount == 0) {
			// reset if loopcount hits zero
			simclockloopcount = control_fps;
			simclockcurrent = totalsimclock;
			// i am 100 sure these will be at their ultimate max value when loopcount hits zero
			bn = 0;
			bi = 0;
		}
		simclocktosend = simclockcurrent / simclockloopcount;
		simclockcurrent -= simclocktosend;
		simclockloopcount -= 1;
		Clock_body_sub(simclocktosend, &bn, &bi);


		Clock_pause_sleep();	// user pauses simulation (infloop until resume)

		// sleep for control
		Sleep(sleep_for);
		loopcount++;

		/* clock time gets measured here
		*  this does not care about actual cycles spent in sim. this is only for correcting sync in winapi timer
		*/
		if (loopcount == syncwhen) {
			now_msec = Clock_gettime_msec();
			elapsedTime_msec = now_msec - prevTime_msec;
			prevTime_msec = now_msec;	// next time

			// sync
			if (elapsedTime_msec > syncsize) {
				sleep_for -= 1;
				// more adjustment
				syncsize -= elapsedTime_msec - syncsize;
			}
			else {
				sleep_for += 1;
				syncsize += syncsize - elapsedTime_msec;
			}

			loopcount = 0;
		}
	}
}

// tn = Clock_var_tickratemul state, ti = Clock_var_maxtickrate
void Clock_body_sub(uint32 _cyclecountdown, uint32* tn, uint32* ti) {
	uint32 cyclecountdown = _cyclecountdown;
	uint32 Clock_curmap = 0;

	for (uint32 n = *tn; n < Clock_var_tickratemul; n++) {	// tickratemultiplier (run the tape n times before next sleep)
		for (uint32 i = *ti; i < Clock_var_maxtickrate; i++) {	// main tape roll
			// time to give it back to control
			if (cyclecountdown == 0) {
				// backup state
				*tn = n;
				*ti = i;
				return;	// get out !!!
			}
			cyclecountdown -= 1;
			Clock_curmap = Clock_schedule_arr[i];	// each bitmap frame
			if (Clock_curmap != 0) {	// just check the bitmap in its entirety first
				// iterate and launch procedures
				for (uint32 j = 0; j < CLOCK_MAX_SCHEDULE_SIZE; j++) {
					if ((Clock_curmap >> j) & 0x1) {
						Clock_arr[j].objfunc();	// run!
					}
				}
			}
		}
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
	uint32 maxval = 100;	// percentage
	uint32 countermax = 100;
	// if start from peri, start from midobj one up
	if (Clock_arr[idx].clock_type == Clock_type_enum::peri) {
		idx = Clock_arr[idx].linked_by;
	}
	// keep looping until master
	while (Clock_arr[idx].clock_type == Clock_type_enum::midobj) {
		maxval *= Clock_arr[idx].multiplier;
		countermax *= 100;
		idx = Clock_arr[idx].linked_by;
	}

	// error if end is not master
	if (Clock_arr[idx].clock_type != Clock_type_enum::master) {
		printf("could not find master clock. check if your clock obj links are properly setup\n");
		return 0;
	}

	return masterclock * maxval / countermax;	// percentage
}

void Clock_ready()
{
	uint32 masterclock = 0;
	uint32 tmpclock = 0;
	uint32 Clock_tickratearr[CLOCK_MAX_SCHEDULE_SIZE];

	uint32 usec_per_tick = 0;

	memset(Clock_tickratearr, 0, CLOCK_MAX_SCHEDULE_SIZE);

	// calculate LCM
	for (uint32 i = 0; i < CLOCK_MAX_SCHEDULE_SIZE; i++) {
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

	// get lowest possible clock number by dividing gcd
	/*
	* a = 1000, b = 700, c = 500 -> gcd = 100
	* maxclock = 1000
	* Clock_var_maxtickrate (maxclock / gcd) = 10
	* Clock_var_tickratemul (gcd) = 100
	* Clock_arr /= gcd
	* we do this to try to reduce memusage & sleep calls as much as possible.
	*/
	uint32 gcd_result = Clock_var_maxtickrate;
	for (uint32 i = 0; i < CLOCK_MAX_SCHEDULE_SIZE; i++) {
		if ((Clock_arr_map >> i) & 0x1) {
			switch (Clock_arr[i].clock_type) {
			case Clock_type_enum::master:
				gcd_result = Clock_gcd(gcd_result, Clock_arr[i].baseclock);
				break;
			case Clock_type_enum::peri:
				gcd_result = Clock_gcd(gcd_result, Clock_tickratearr[i]);
				break;
			}
		}
	}
	// loop again to apply to other clocks
	for (uint32 i = 0; i < CLOCK_MAX_SCHEDULE_SIZE; i++) {
		if ((Clock_arr_map >> i) & 0x1) {
			switch (Clock_arr[i].clock_type) {
			case Clock_type_enum::master:
			case Clock_type_enum::peri:
				Clock_tickratearr[i] /= gcd_result;
				break;
			}
		}
	}
	Clock_var_maxtickrate /= gcd_result;
	Clock_var_tickratemul = gcd_result;


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

	for (uint32 i = 0; i < CLOCK_MAX_SCHEDULE_SIZE; i++) {
		if ((Clock_arr_map >> i) & 0x1) {
			switch (Clock_arr[i].clock_type) {
			case Clock_type_enum::peri:	//including cpu
				// get peri 1000 / 500 = 2
				usec_per_tick = Clock_var_maxtickrate / Clock_tickratearr[i];
				for (uint32 j = 0; j < Clock_var_maxtickrate; j++) {
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