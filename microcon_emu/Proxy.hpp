#pragma once

#include <stdio.h>
#include <windows.h>
#include <profileapi.h>	// for QueryPerformanceCounter
#include <setjmp.h>
#include <intrin.h>
#include <assert.h>
#include <stdint.h> // for UINT32_MAX
#define m_assert(expr, msg) assert(( (void)(msg), (expr) ))

// Debug logging macro
#define SHOW_DEBUG_LOG // Uncomment to enable debug printf statements

#ifdef SHOW_DEBUG_LOG
#define eprintf(...) printf(__VA_ARGS__)
#else
#define eprintf(...) ((void)0)
#endif

// type defines

typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
struct Thread_data {
	void(* func)(void);
	uint32 param1;
	uint32 param2;
};

// functions
extern DWORD WINAPI ThreadFunc(void* data);

extern HANDLE make_thread(Thread_data* mydata);

extern uint32 Clock_gettime_msec();

extern void Clock_sleep(uint32 msec);

#define NULL 0

#include "EmuPool.hpp"