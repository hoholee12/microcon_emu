#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define PLATFORM_WINDOWS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#elif defined(__APPLE__)
    #define PLATFORM_MACOS
#endif

// MSVC specific
#if defined(_MSC_VER)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <stdint.h> // for UINT32_MAX
#define m_assert(expr, msg) assert(( (void)(msg), (expr) ))

// Platform-specific includes
#if defined(_MSC_VER) && defined(PLATFORM_WINDOWS)
    // MSVC on Windows
    #include <windows.h>
    #include <profileapi.h>	// for QueryPerformanceCounter
    #include <intrin.h>
    typedef HANDLE thread_handle_t;
    typedef DWORD thread_return_t;
    #define THREAD_CALL WINAPI
#else
    // GCC/Clang on Cygwin, Linux, or macOS
    #include <sys/types.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <time.h>
    #include <sys/time.h>
    #ifdef PLATFORM_WINDOWS
        #include <windows.h>  // For Sleep on Cygwin
    #endif
    typedef pthread_t thread_handle_t;
    typedef void* thread_return_t;
    #define THREAD_CALL
#endif

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
	void (*func)(void);
	uint32 param1;
	uint32 param2;
};

#define UINT24_MAX 0xFFFFFF

// Platform-independent thread functions
extern thread_return_t THREAD_CALL ThreadFunc(void* data);
extern thread_handle_t make_thread(Thread_data* mydata);
extern void wait_thread(thread_handle_t thread);

// Platform-independent timing functions
extern uint32 Clock_gettime_msec();
extern void Clock_sleep(uint32 msec);

#ifndef NULL
#define NULL 0
#endif

#include "EmuPool.hpp"