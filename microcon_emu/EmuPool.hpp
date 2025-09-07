#pragma once
#include "Proxy.hpp"


#define EMUPOOL_BUFFER_SIZE 0x100000	// 1MB buffer
// one bit per 1KB block
extern uint32 microcon_emupool[EMUPOOL_BUFFER_SIZE];
