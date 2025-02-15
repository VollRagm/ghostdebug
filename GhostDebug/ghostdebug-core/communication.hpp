#pragma once

enum class EVENT_CODE
{
	ADD_BREAKPOINT,
	REMOVE_BREAKPOINT,
	BP_HIT,
	PAUSE,
	RESUME,
	STEP_OVER,
	STEP_INTO,
	STEP_OUT,
	REGISTER_WRITE
};

#include "common.hpp"
#include "pipe.hpp"
#include "debugger.hpp"

namespace communication
{
	void start_listening();
	void breakpoint_callback(EXCEPTION_POINTERS* ctx);
}