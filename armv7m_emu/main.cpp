#include "main.hpp"

int main(int argc, char** argv) {

	/*if (argc < 2) {
		printf("needs a bin file to launch");

		return 1;
	}*/

	Thread_data mydata;

	Core_start(&mydata);

	HANDLE thread = make_thread(&mydata);
















	if (thread) {
		while (1) {
			Sleep(1000);
			Core_pause();
			Sleep(1000);
			Core_resume();
			Sleep(500);
			Core_pause();
			Sleep(500);
			Core_resume();
		}

		//pthread_join equivalent
		WaitForSingleObject(thread, INFINITE);
	
	}






	return 0;
}