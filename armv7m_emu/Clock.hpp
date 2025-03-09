#pragma once
#include "Proxy.hpp"


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


static const uint32 control_fps = 60;
static const uint32 control_syncinterval = 12;	// 12 frames (~0.2s)
static const uint32 control_longsync = 5;
static const uint32 control_syncinterval_long = control_fps / control_syncinterval * control_longsync; // interval of 25(5s) to calc sync

/* maxtickrate based on LCM of all objects (simulated tickrate) */
extern uint32 Clock_var_maxtickrate;
extern uint32 Clock_var_tickratemul;	// multiplier for maxtickrate
extern uint32 Clock_totalsimclock;	// multiply of these two
extern int Clock_var_sleepfor;

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

#define CLOCK_MAX_SCHEDULE_SIZE (sizeof(uint32) * 8)

extern uint32 Clock_arr_map;
extern struct Clock_struct Clock_arr[CLOCK_MAX_SCHEDULE_SIZE];


/* Clock schedule arr 
* 
* malloc memory
* one long cylinder that plays instruments with its pins
* 
* uint32 (32bits) -> 32 functions possible at one tick
*/
extern uint32* Clock_schedule_arr;	// dynamically allocated


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
extern void Clock_body_sub(uint32 _cyclecountdown, uint32 *tn, uint32 *ti);

// function to insert clock objects in. master must always be index 0!!
// obj is copied so you no need to worry about obj getting dereferenced.
extern void Clock_add(uint32 index, Clock_struct* clock_obj);

// calculate LCM and generate clock schedule arr for run
extern void Clock_ready();

// telemetry
extern uint32 Clock_currenttime();
