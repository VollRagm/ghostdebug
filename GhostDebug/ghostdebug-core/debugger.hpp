#pragma once

#include "common.hpp"
#include "util.hpp"
#include "communication.hpp"

namespace debugger
{
	enum class DEBUG_ACTION
	{
		RESUME = EVENT_CODE::RESUME,
		STEP_OVER = EVENT_CODE::STEP_OVER,
		STEP_IN = EVENT_CODE::STEP_IN,
		STEP_OUT = EVENT_CODE::STEP_OUT,

		NONE
	};

	bool init();
	void add_breakpoint(uintptr_t address);
	void remove_breakpoint(uintptr_t address);
	void continue_execution(DEBUG_ACTION action);
}