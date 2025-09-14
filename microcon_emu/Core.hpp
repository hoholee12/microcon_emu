#pragma once

#include "Proxy.hpp"
#include "Memory.hpp"
#include "Clock.hpp"

// type defines



// we assume that the Core shall be run on a separate thread, and allow it to be paused and resumed live.


// core static variables (naming convention: Core_var_xxxx)
// Core_var_status == 0 -> nothing
// Core_var_status == 1 -> launching (file received)
// Core_var_status == 2 -> pause
extern int Core_var_status;


// 0 -> do not access, 1 -> OK
extern int Core_var_Memory_init;


// core functions (naming convention: Core_xxxx)
extern void Core_mainThread();
// start running background checks befor proceeding to launch the core
extern void Core_start(Thread_data* mydata);

// for state
extern void Core_pause();
extern void Core_resume();

