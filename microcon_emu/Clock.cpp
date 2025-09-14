#include "Clock.hpp"

/*
*
* clock based peripheral scheduler
* 
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
uint32 Clock_var_maxtickrate_prev;
uint32 Clock_var_tickratemul;	// multiplier for maxtickrate
uint32 Clock_var_totalsimclock;	// multiply of these two
uint32 Clock_var_totalsimclock_prev;

int Clock_var_sleepfor;
uint32 Clock_var_vectormode;	// shows what mode the scheduler is running with. 0 is tape, 1 is vectorarr

uint32 Clock_var_poweron; // 0 when regen, 1 when ready to run
uint32 Clock_var_poweron_count = 0;
uint32 Clock_var_poweron_prevcount = 0;
uint32 Clock_var_poweron_interruptable = 1;

/* scheduler - hint for how many cycles it can use before handing off to another peri */
uint32 Clock_var_availcycles[CLOCK_MAX_SCHEDULE_SIZE];
uint32 Clock_var_availcycles_idx = 0;
/* user - acknowledge and send cycles actually used, so that the scheduler can skip next iterations */
/* always set this 1 by default */
uint32 Clock_var_usecycles = 1;

/* simple freerunning tick */
uint32 Clock_var_tick = 0;

uint32 Clock_var_wake;

uint32 Clock_arr_map = 0;

/* Clock schedule vector
*
* malloc memory
* one long cylinder that plays instruments with its pins
*
* uint32 (32bits) -> 32 functions possible at one tick
*/
uint32* Clock_schedule_vect;	// dynamically allocated
uint32* Clock_schedule_linkvect;	// used when vectormode enabled
uint32 Clock_schedule_vect_alloc;	// 1 when vect, 3 when vect + linkvect is allocated

struct Clock_struct Clock_arr[CLOCK_MAX_SCHEDULE_SIZE];

uint32 Clock_var_maxindex = 0;
uint32 Clock_tickratearr[CLOCK_MAX_SCHEDULE_SIZE];
uint32 Clock_div_arr[CLOCK_MAX_SCHEDULE_SIZE];

void Clock_init() 
{
	while (Clock_var_poweron_interruptable == 0) {}	// wait until scheduler finishes

	// deallocate previous entry
	if ((Clock_schedule_vect_alloc & 0x1) != 0x0) {
		efree(Clock_schedule_vect);
	}
	if ((Clock_schedule_vect_alloc & 0x2) != 0x0) {
		efree(Clock_schedule_linkvect);
	}

	Clock_var_wake = 1;
	Clock_var_maxtickrate_prev = Clock_var_maxtickrate;
	Clock_var_maxtickrate = 0;
	Clock_var_tickratemul = 1; // initial start as 1x

	Clock_var_vectormode = 0;
	Clock_var_poweron = 0;

	Clock_var_totalsimclock_prev = Clock_var_totalsimclock;
	Clock_var_totalsimclock = 0;
}

static inline void Clock_pause_sleep() {
	while (Clock_var_wake == 0) {
		Sleep(100);	// explicit wait state is better than just yielding
	}
}

// main body (synchronized by 60fps)
void Clock_body_main()
{
	// -------- everything here before the while loop is only to be executed once. //
	const int one_second = 1000;
	uint32 frame_count = 0;
	uint32 short_term_index = 0;
	uint32 short_term_timestamps[control_syncinterval_long] = { 0 };
	uint32 sleep_for = one_second / control_fps;	// lets start with 16ms

	// for the simclock
	Clock_var_totalsimclock = Clock_var_maxtickrate * Clock_var_tickratemul;
	Clock_var_totalsimclock_prev = Clock_var_totalsimclock;
	Clock_var_maxtickrate_prev = Clock_var_maxtickrate;
	uint32 simclockcurrent = Clock_var_totalsimclock;
	uint32 simclockloopcount = control_fps;
	uint32 simclocktosend = 0;
	uint32 bn = 0;
	uint32 bi = 0;

	// -------- everything here before the while loop is only to be executed once. //

	while (1) {

		// recalculate simclock when clock scheduler has regenerated.
		/*
		* we need to scale the bn and bi according to the new Clock_var_totalsimclock value.
		* 
		* integer only (multiply by 100)
		* 
		* bn * Clock_var_maxtickrate_prev + bi = prev_bpos = previous position on the tape (1234)
		* Clock_var_totalsimclock_prev = previous tape size (5000)
		* 
		* Clock_var_totalsimclock = regenerated tape size (7500)
		* cur_percentage = Clock_var_totalsimclock * 100 / Clock_var_totalsimclock_prev (7500 * 100 / 5000 = 150) (100th percentile)
		* cur_bpos = prev_bpos * cur_percentage / 100 (1234 * 150 / 100 = 1851)
		*/
		if (Clock_var_poweron_count != Clock_var_poweron_prevcount) {
			Clock_var_poweron_prevcount = Clock_var_poweron_count;
			Clock_var_totalsimclock = Clock_var_maxtickrate * Clock_var_tickratemul;

			// TODO - figure out why this causes huge drift forward
			// simclockloopcount = control_fps;
			simclockcurrent = Clock_var_totalsimclock;
			
			uint32 prev_bpos = bn * Clock_var_maxtickrate_prev + bi;
			uint32 cur_percentage = Clock_var_totalsimclock * 100 / Clock_var_totalsimclock_prev;
			uint32 cur_bpos = prev_bpos * cur_percentage / 100;

			// now we calculate the individual bn, bi
			bn = cur_bpos / Clock_var_maxtickrate;
			bi = cur_bpos - (cur_bpos / Clock_var_maxtickrate) * Clock_var_maxtickrate; /* cur_bpos % Clock_var_maxtickrate */

		}

		// wait until clock regen finished
		// this is to happen when clockspeed changed or peripheral added mid-run.
		while (Clock_var_poweron == 0) {}

		// run + sleep = total frame
		
		// the nature of the app environment does not allow us to do Hi-Res sleep. we shall make it coarse. (but not one second coarse...)


		/* TODO:
		* 1. if masterclock * 2 < tapesize: (tape too big for perf)
		*	switch to vectorarr<nextinterval:map>
		* 2. allow adding of new clock while running:
		*	clock_init > clock_add > clock_ready procedure is to be used.
		*	clock_ready will remember previous gen state and scale to the newly generated tape size accordingly
		*/


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
			simclockcurrent = Clock_var_totalsimclock;
			// these will be at their ultimate max value when loopcount hits zero
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
		Clock_var_sleepfor = sleep_for;	// update telemetry

		/* clock time gets measured here
		* this does not care about actual cycles spent in sim. this is only for correcting sync in winapi timer
		* 
		* long-term is accurate. short-term drifts like crazy.
		* we use 0.2 second intervals to capture time.
		* we use 1 second intervals to calculate drift. (no influential short term drift)
		* 
		* TODO: this is not good enough. we need a more accurate timer
		*/
		frame_count += 1;

		if (frame_count == control_syncinterval) {
			short_term_timestamps[short_term_index] = Clock_gettime_msec();
			short_term_index += 1;
			frame_count = 0;
		}

		if (short_term_index == control_syncinterval_long) {
			int actual_elapsed = short_term_timestamps[control_syncinterval_long - 1] -
				short_term_timestamps[0];

			int long_term_drift = actual_elapsed - (one_second * control_longsync);	// idk why we need control_longsync here

			// sync
			if (long_term_drift > 0) {
				if (sleep_for > 1) {	// if this becomes zero, its basically yield()
					sleep_for -= 1;
				}
			}
			else {
				sleep_for += 1;
			}

			// printf("drift: %d, sleep_for: %d\n", long_term_drift, sleep_for);

			// shift timestamps left to make room for the next (not big enough to cause perf issue)
			for (int i = 1; i < control_syncinterval_long; i++) {
				short_term_timestamps[i - 1] = short_term_timestamps[i];
			}
			short_term_index -= 1;
		}
	}
}

// tn = Clock_var_tickratemul state, ti = Clock_var_maxtickrate
/*
* cyclecountdown = cycles to simulate before handing off back to control
* tn, ti = position in the tape playback
* 
* Clock_var_tick = for telemetry (no need to worry about this for now)
* 
**/
void Clock_body_sub(int _cyclecountdown, uint32* tn, uint32* ti) {
	int cyclecountdown = _cyclecountdown;
	uint32 Clock_curmap = 0;

	Clock_var_poweron_interruptable = 0;

	for (uint32 n = *tn; n < Clock_var_tickratemul; n++) {	// tickratemultiplier (run the tape n times before next sleep)
		for (uint32 i = *ti; i < Clock_var_maxtickrate; i++) {	// main tape roll
			// time to give it back to control - TODO: this is inefficient. we only need one branch in loop
			if (cyclecountdown <= 0) {
				// backup state
				*tn = n;
				*ti = i + cyclecountdown;	// just undo cycles (is ok because nothing happens inbetween skip)
				Clock_var_tick += cyclecountdown;	// undo Clock_var_tick

				Clock_var_poweron_interruptable = 1;
				return;	// get out !!!
			}

			Clock_curmap = Clock_schedule_vect[i];	// each bitmap frame
			if (Clock_curmap != 0) {	// just check the bitmap in its entirety first

				// backup before launching procedures
				Clock_var_poweron_prevcount = Clock_var_poweron_count;

				// check hintavail first
				for (uint32 j = 0; j < Clock_var_maxindex; j++) {
					if ((Clock_curmap >> j) & 0x1) {
						// set hint cycles
						Clock_var_availcycles[j] = Clock_hintavail_cycle(j, n, i);
					}
				}


				// iterate and launch procedures
				for (uint32 j = 0; j < Clock_var_maxindex; j++) {
					if ((Clock_curmap >> j) & 0x1) {
						// set hint cycles
						if (Clock_var_availcycles[j] == 0) {
							continue; // do nothing when usable cycle is empty
						}
						Clock_var_availcycles_idx = j;
						Clock_var_usecycles = 1;

						Clock_var_poweron_interruptable = 1;
						Clock_arr[j].objfunc();	// run!
						Clock_var_poweron_interruptable = 0;
						
						if (Clock_var_availcycles[j] < Clock_var_usecycles) {
							printf("too much cycles used!! expect desync\n");
							Clock_var_availcycles[j] = 0;
						}
						else {
							Clock_var_availcycles[j] -= Clock_var_usecycles;
						}
					}
				}

				// the invoked function can change peri setup whilst running(init -> add -> ready)
				// when that shit happens, get out instantly
				if (Clock_var_poweron_count != Clock_var_poweron_prevcount) {
					// backup state
					*tn = n;
					*ti = i + cyclecountdown;	// just undo cycles (is ok because nothing happens inbetween skip)
					Clock_var_tick += cyclecountdown;	// undo Clock_var_tick

					Clock_var_poweron_interruptable = 1;
					return;
				}


				// fast skip to the next bump in the tape
				if (Clock_var_vectormode == 1) {

					// if end of tape, get to the next interation
					if (Clock_schedule_linkvect[i] == 0) {
						cyclecountdown -= (Clock_var_maxtickrate + 1 - i);
						Clock_var_tick += (Clock_var_maxtickrate + 1 - i);	// will be optimized out by any decent compiler
						break;	// i is reset
					}

					// keep skipping
					Clock_var_tick += (Clock_schedule_linkvect[i] - i - 1);
					cyclecountdown -= (Clock_schedule_linkvect[i] - i - 1); // will be optimized out by any decent compiler
					i = Clock_schedule_linkvect[i] - 1;	// -1 because i++ is applied after this.
				}
			}


			// count
			Clock_var_tick += 1;	// freerunning tick
			cyclecountdown -= 1;	// countdown tick

		}
	}

	Clock_var_poweron_interruptable = 1;
}

void Clock_pause() {
	Clock_var_wake = 0;
}

void Clock_resume() {
	Clock_var_wake = 1;
}

// function to insert clock objects in. master must always be index 0!!
// obj is copied so you no need to worry about obj getting dereferenced.
void Clock_add(uint32 index, Clock_struct* clock_obj) 
{
	// sanity check
	if (index < 0 && index >= CLOCK_MAX_SCHEDULE_SIZE) {
		m_assert(false, "clock obj index out of range!\n");
	}

	if ((index == 0 && clock_obj->clock_type != Clock_type_enum::master)
	|| (index != 0 && clock_obj->clock_type == Clock_type_enum::master)) {
		m_assert(false, "only master obj can be inserted to index 0\n");
	}


	if ((Clock_arr_map >> index) & 0x1) {
		m_assert(false, "index already taken\n");
	}
	// end sanity check

	Clock_arr_map |= (0x1 << index);
	Clock_arr[index] = *clock_obj;	// copy obj

}

// replace obj without disturbing other objs
void Clock_replace(uint32 index, Clock_struct* clock_obj)
{
	// sanity check
	if (index < 0 && index >= CLOCK_MAX_SCHEDULE_SIZE) {
		m_assert(false, "clock obj index out of range!\n");
	}

	if ((index == 0 && clock_obj->clock_type != Clock_type_enum::master)
		|| (index != 0 && clock_obj->clock_type == Clock_type_enum::master)) {
		m_assert(false, "only master obj can be inserted to index 0\n");
	}

	// end sanity check

	Clock_arr_map |= (0x1 << index);
	Clock_arr[index] = *clock_obj;	// copy obj

}

// remove obj and its children objs
void Clock_remove(uint32 index) {

}

/* Clock schedule arr
*
* malloc memory
* tape is allocated and is played back in a loop
* clock-powered musical box basically
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
		m_assert(false, "could not find master clock. check if your clock obj links are properly setup\n");
	}

	return masterclock * maxval / countermax;	// percentage
}

void Clock_ready()
{
	uint32 masterclock = 0;
	uint32 tmpclock = 0;

	uint32 usec_per_tick = 0;

	memset(Clock_tickratearr, 0, CLOCK_MAX_SCHEDULE_SIZE);

	// calculate LCM
	Clock_var_maxindex = 0;	// for peri only
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
				Clock_var_maxindex = i + 1;	// for peri only
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

	Clock_schedule_vect = (uint32*)ecalloc(Clock_var_maxtickrate, sizeof(uint32));
	Clock_schedule_vect_alloc |= 0x1;

	// hammer the pins in
	/*
	* per peripheral
	*     loop schedule arr
	*
	*/
	for (uint32 i = 0; i < Clock_var_maxindex; i++) {
		if ((Clock_arr_map >> i) & 0x1) {
			switch (Clock_arr[i].clock_type) {
			case Clock_type_enum::peri:	//including cpu
				// get peri 1000 / 500 = 2
				usec_per_tick = Clock_var_maxtickrate / Clock_tickratearr[i];
				for (uint32 j = 0; j < Clock_var_maxtickrate; j++) {
					if (j - (j / usec_per_tick) * usec_per_tick == 0) {	/* j % usec_per_tick */
						Clock_schedule_vect[j] |= (0x1 << i);	// mark
					}
				}
				break;
			default:
				break;
			}
		}
	}

	/*
	* hint for cycles
	* 1. 1000
	* 2. 300
	* 
	* lcm = 3000
	* 
	* 1  4  7  10 13 16 19 22 25 28
	* 100100100100100100100100100100...
	* 100000000010000000001000000000...
	* 1         11        21
	* 
	* 2s cycle = 3000 / 300 = 10
	* 1s cycle = 3000 / 1000 = 3
	* 
	* current cycle = 13
	* 
	* 2s cycle + 1 - (current cycle % 2s cycle) =  remaining cycle before other peri
	* 
	* 10 + 1 - (13 % 10) = 11 - 3 = 8
	* 
	* we need: array of cycles for all peri (divided by lcm)
	* we only need to calculate this just before running each obj
	* 
	*/
	// generate array of cycles for all peri (divided by lcm)
	for (uint32 i = 0; i < Clock_var_maxindex; i++) {
		if ((Clock_arr_map >> i) & 0x1) {
			switch (Clock_arr[i].clock_type) {
			case Clock_type_enum::peri:	//including cpu
				Clock_div_arr[i] = Clock_var_maxtickrate / Clock_tickratearr[i];
				break;
			default:
				break;
			}
		}
	}

	/*
	* check if the bumps are too far away from each other
	* if they are over 1 slot wide, tape playback is kind of inefficient
	* if so, we create an additional link vector for skip
	*/
	// Clock_var_vectormode is to be switched here.
	uint32 intervalcontinue = 0;
	for (uint32 i = 0; i < Clock_var_maxtickrate; i++) {
		if (Clock_schedule_vect[i] != 0) {
			// do nothing
		}
		// empty space, start counting space
		else {
			intervalcontinue += 1;
		}

		if (intervalcontinue >= 2) {
			Clock_var_vectormode = 1;
			break;
		}
	}

	// debug
	//Clock_var_vectormode = 0;

	/*
	* create a new linkvector
	*/
	if (Clock_var_vectormode == 1) {
		Clock_schedule_linkvect = (uint32*)ecalloc(Clock_var_maxtickrate, sizeof(uint32));
		Clock_schedule_vect_alloc |= 0x2;
		uint32 front_loc = 0;	// last index is 0 (indicator that the tape ends after this)
		// do this in reverse
		for (uint32 x = Clock_var_maxtickrate; x > 0; x--) {
			uint32 i = x - 1;
			if (Clock_schedule_vect[i] != 0) {
				Clock_schedule_linkvect[i] = front_loc;
				front_loc = i;
			}
		}

		printf("linkvect generated\n");
	}

	Clock_var_poweron = 1;	// ready to run.
	Clock_var_poweron_count += 1;
}

uint32 Clock_currenttime() {
	/* return in ms
	* a second 1000
	* Clock_var_tick = internal tick 123456
	* Clock_var_maxtickrate * Clock_var_tickratemul = total of internal tick per second 21000
	* 123456000 / 21000 = 5878 ms
	* 
	*/
	return Clock_var_tick * 1000 / Clock_var_totalsimclock;

}