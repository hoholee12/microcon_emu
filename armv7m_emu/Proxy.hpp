#pragma once

#include <stdio.h>
#include <windows.h>



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

#define NULL 0
