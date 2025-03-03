#include "Proxy.hpp"


DWORD WINAPI ThreadFunc(void* data) {
	// thread function. call Core_mainThread here.
	Thread_data* mydata = (Thread_data*)data;


	// core runs here
	mydata->func();


	return 0;
}

HANDLE make_thread(Thread_data* mydata) {

	HANDLE thread;

	// thread attrib, stacksize, funcaddr, param, creationflag, tid
	thread = CreateThread(NULL, 0, ThreadFunc, (void*)mydata, 0, NULL);


	return thread;
}

uint32 Clock_gettime_msec() {
	return GetTickCount();
}

void Clock_sleep(uint32 msec) {
	Sleep(msec);
}