#include "main.hpp"

int main(int argc, char** argv) {

	/*if (argc < 2) {
		printf("needs a bin file to launch");

		return 1;
	}*/

	Thread_data mydata;

	Core_start(&mydata);

	thread_handle_t thread = make_thread(&mydata);

	if (thread) {
		/*
		while (1) {
			Clock_sleep(1000);
			Core_pause();
			Clock_sleep(1000);
			Core_resume();
			Clock_sleep(500);
			Core_pause();
			Clock_sleep(500);
			Core_resume();
		}
		*/

		// Wait for thread to complete (pthread_join equivalent)
		wait_thread(thread);
	}

	return 0;
}