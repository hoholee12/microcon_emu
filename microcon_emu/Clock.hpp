#pragma once
#include "Proxy.hpp"


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


#define CLOCK_MAX_SCHEDULE_SIZE (sizeof(uint32) * 8)	// 1 byte(8b) * sizeof 32bit value


static const uint32 control_fps = 60;
static const uint32 control_sync = 5;
static const uint32 control_longsync = 5;
static const uint32 control_syncinterval = control_fps / control_sync;	// 12 frames (~0.2s)
static const uint32 control_syncinterval_long = control_fps / control_syncinterval * control_longsync; // interval of 25(5s) to calc sync

/* maxtickrate based on LCM of all objects (simulated tickrate) */
extern uint32 Clock_var_maxtickrate;
extern uint32 Clock_var_tickratemul;	// multiplier for maxtickrate
extern uint32 Clock_totalsimclock;	// multiply of these two
extern int Clock_var_sleepfor;
extern uint32 Clock_var_vectormode;	// shows what mode the scheduler is running with. 0 is tape, 1 is vectorarr

extern uint32 Clock_var_poweron; // 0 when regen, 1 when ready to run

/* scheduler - hint for how many cycles it can use before handing off to another peri */
extern uint32 Clock_var_availcycles[CLOCK_MAX_SCHEDULE_SIZE];
/* user - acknowledge and send cycles actually used, so that the scheduler can skip next iterations */
/* always set this 1 by default */
extern uint32 Clock_var_usecycles;
extern uint32 Clock_var_availcycles_idx;
#define CLOCK_GET_AVAILABLE_CYCLES() Clock_var_availcycles[Clock_var_availcycles_idx]
#define CLOCK_SET_USE_CYCLES Clock_var_usecycles

/* simple freerunning tick */
extern uint32 Clock_var_tick;

extern uint32 Clock_var_wake;

enum Clock_type_enum{master, midobj, peri};
struct Clock_struct {
	uint32 linked_by; // linked by index
	uint32 groupid;	// TODO: for optimization (reduce timer syscall maybe)
	uint32 multiplier; // only for midobj. used as percentage (1.0 -> 100, 0.25 -> 25)
	uint32 baseclock; // only for master. 1 -> 1hz (uint32 4Ghz possible)
	void (*objfunc)(void);	// linked object function. only for peri(including cpu).
	Clock_type_enum clock_type;	// clock type (if midobj -> multiplier is used.)
};

extern uint32 Clock_arr_map;
extern struct Clock_struct Clock_arr[CLOCK_MAX_SCHEDULE_SIZE];

extern uint32 Clock_var_maxindex;
extern uint32 Clock_tickratearr[CLOCK_MAX_SCHEDULE_SIZE];
extern uint32 Clock_div_arr[CLOCK_MAX_SCHEDULE_SIZE];


/* Clock schedule vector 
* 
* malloc memory
* one long cylinder that plays instruments with its pins
* 
* uint32 (32bits) -> 32 functions possible at one tick
*/
extern uint32* Clock_schedule_vect;	// dynamically allocated
extern uint32* Clock_schedule_linkvect;	// used when vectormode enabled

// master_index: index for peri to be checked
static inline uint32 Clock_hintavail_cycle(uint32 master_index, uint32 n, uint32 i) {

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
	uint32 current_cycle = (n * Clock_var_maxtickrate) + i;
	uint32 closest_peri_remaining_ticks = 0xFFFF;
	uint32 closest_tmp = 0xFFFF;
	for (uint32 i = 0; i < Clock_var_maxindex; i++) {
		if ((Clock_arr_map >> i) & 0x1) {
			switch (Clock_arr[i].clock_type) {
			case Clock_type_enum::peri:	//including cpu
				if (i == master_index) {
					break;	// if the index is pointing at me, skip
				}
				
				 closest_tmp = Clock_div_arr[i] + 1 - 
					(current_cycle - (current_cycle / Clock_div_arr[i]) * Clock_div_arr[i]); /* current_cycle % Clock_div_arr[i] */
				if (closest_tmp < closest_peri_remaining_ticks) {
					// get closest peri cycle
					closest_peri_remaining_ticks = closest_tmp;
				}
				break;
			default:
				break;
			}
		}
	}

	return closest_peri_remaining_ticks;
}

// static instead of extern, this is only to be used in Clock.
static inline uint32 Clock_gcd(uint32 a, uint32 b) {
	while (b != 0) {
		// (a - (a / b) * b) -> a % b
		uint32 temp = b;
		b = a - (a / b) * b;
		a = temp;
	}
	return a;
}
static inline uint32 Clock_lcm(uint32 a, uint32 b) {
	return (a / Clock_gcd(a, b)) * b;
}

extern void Clock_pause();
extern void Clock_resume();

extern void Clock_init();

// main body for counting ticks
extern void Clock_body_main();
// sub body for launching objfuncs
extern void Clock_body_sub(int _cyclecountdown, uint32 *tn, uint32 *ti);

// function to insert clock objects in. master must always be index 0!!
// obj is copied so you no need to worry about obj getting dereferenced.
extern void Clock_add(uint32 index, Clock_struct* clock_obj);
extern void Clock_replace(uint32 index, Clock_struct* clock_obj);
extern void Clock_remove(uint32 index);

// calculate LCM and generate clock schedule arr for run
extern void Clock_ready();

// telemetry
extern uint32 Clock_currenttime();
