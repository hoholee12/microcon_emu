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


/* maxtickrate based on LCM of all objects (simulated tickrate) */
extern int Clock_var_maxtickrate;

/* target tickrate(actual tickrate while running) */
extern int Clock_var_tick;

enum Clock_type_enum{master, midobj, peri};
struct Clock_struct {
	uint32 linked_by; // linked by index
	uint32 linked_to; // linked to index
	uint32 multiplier; // only for midobj. used as percentage (1.0 -> 100, 0.25 -> 25)
	uint32 baseclock; // only for master. 1 -> 1hz (uint32 4Ghz possible)
	void (*objfunc)(void);	// linked object function. only for peri(including cpu).
	Clock_type_enum clock_type;	// clock type (if midobj -> multiplier is used.)
};

#define CLOCK_MAX_SCHEDULE_SIZE (sizeof(uint32) * 8)

extern uint32 Clock_arr_map;
struct Clock_struct Clock_arr[CLOCK_MAX_SCHEDULE_SIZE];


/* Clock schedule arr 
* 
* malloc memory
* one long cylinder that plays instruments with its pins
* 
* uint32 (32bits) -> 32 functions possible at one tick
*/
extern uint32* Clock_schedule_arr;	// dynamically allocated

// clockspeed: real base (replaced by master), virtual_clockspeed: simulated
extern void Clock_init(uint32 clockspeed, uint32 virtual_clockspeed);

// main body for counting ticks and launching objfuncs.
extern void Clock_body();
// function to insert clock objects in. master must always be index 0!!
extern void Clock_add(uint32 index, Clock_struct clock_obj);

// calculate LCM and generate clock schedule arr for run
extern void Clock_ready();
