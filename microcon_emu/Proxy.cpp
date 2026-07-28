#include "Proxy.hpp"

// Thread function - platform independent
thread_return_t THREAD_CALL ThreadFunc(void* data) {
	// thread function. call Core_mainThread here.
	Thread_data* mydata = (Thread_data*)data;

	// core runs here
	mydata->func();

#if defined(_MSC_VER) && defined(PLATFORM_WINDOWS)
	return 0;
#else
	return NULL;
#endif
}

// Create thread - platform specific implementations
thread_handle_t make_thread(Thread_data* mydata) {
#if defined(_MSC_VER) && defined(PLATFORM_WINDOWS)
	// Windows MSVC
	HANDLE thread;
	// thread attrib, stacksize, funcaddr, param, creationflag, tid
	thread = CreateThread(NULL, 0, ThreadFunc, (void*)mydata, 0, NULL);
	return thread;
#else
	// POSIX threads (Cygwin, Linux, macOS)
	pthread_t thread;
	pthread_attr_t attr;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	if (pthread_create(&thread, &attr, ThreadFunc, (void*)mydata) != 0) {
		pthread_attr_destroy(&attr);
		return 0;
	}
	
	pthread_attr_destroy(&attr);
	return thread;
#endif
}

// Wait for thread to complete
void wait_thread(thread_handle_t thread) {
#if defined(_MSC_VER) && defined(PLATFORM_WINDOWS)
	// Windows MSVC
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
#else
	// POSIX threads
	pthread_join(thread, NULL);
#endif
}

// Get time in milliseconds
uint32 Clock_gettime_msec() {
#if defined(_MSC_VER) && defined(PLATFORM_WINDOWS)
	// Windows MSVC
	return GetTickCount();
#elif defined(PLATFORM_WINDOWS)
	// Cygwin on Windows
	return GetTickCount();
#elif defined(PLATFORM_LINUX)
	// Linux
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#elif defined(PLATFORM_MACOS)
	// macOS
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint32)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#else
	// Fallback
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint32)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

// Sleep for specified milliseconds
void Clock_sleep(uint32 msec) {
#if defined(PLATFORM_WINDOWS)
	// Windows (both MSVC and Cygwin)
	Sleep(msec);
#else
	// POSIX (Linux, macOS)
	struct timespec ts;
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	nanosleep(&ts, NULL);
#endif
}
