#include "debugger.hpp"

class breakpoint
{
public:
	uintptr_t address = 0;
	std::vector<uint8_t> original_instruction = std::vector<uint8_t>();
	bool enabled = false;

	breakpoint(uintptr_t address)
	{
		this->address = address;
	}

	breakpoint() = default;

	void enable()
	{
		if (enabled)
			return;

		util::place_int3(address, original_instruction);
		enabled = true;
	}

	void disable()
	{
		if (!enabled)
			return;

		util::remove_int3(address, original_instruction);
		enabled = false;
	}
};

namespace debugger
{
	std::unordered_map<uintptr_t, breakpoint> breakpoints;

	std::mutex breakpoint_mutex;
	std::condition_variable resume_cv;
	DEBUG_ACTION user_action = DEBUG_ACTION::NONE;
	bool breakpoint_hit = false;

	// Central exception handler
	// anything that triggers an exception (MessageBox, OutputDebugString, etc) 
	// must be treated with caution here to avoid infinite loops
	LONG WINAPI exception_handler(EXCEPTION_POINTERS* exception_info)
	{
		if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
		{
			uintptr_t address = (uintptr_t)exception_info->ExceptionRecord->ExceptionAddress;
			CONTEXT* ctx = exception_info->ContextRecord;

			// Check if the breakpoint is in our list
			if (breakpoints.find(address) != breakpoints.end())
			{
				std::unique_lock<std::mutex> lock(breakpoint_mutex);
				breakpoint bp = breakpoints[address];

				// send context to client
				communication::breakpoint_callback(exception_info);
				breakpoint_hit = true;

				resume_cv.wait(lock, [] { return user_action != DEBUG_ACTION::NONE; });

				breakpoint_hit = false;

				// handle user action
				switch (user_action)
				{
					case DEBUG_ACTION::RESUME:

						bp.disable();
						ctx->Rip = address; // Reset execution to original breakpoint address
						CLEAR_STEP_FLAG(ctx);

						return EXCEPTION_CONTINUE_EXECUTION;

						break;
					case DEBUG_ACTION::STEP_OVER:
						exception_info->ContextRecord->EFlags |= 0x100;
						break;
					case DEBUG_ACTION::STEP_IN:
						break;
					case DEBUG_ACTION::STEP_OUT:
						break;
				}

				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}


		return EXCEPTION_CONTINUE_SEARCH;
	}

	bool init()
	{
		// Catch all exceptions
		return AddVectoredExceptionHandler(1, exception_handler) != NULL;
	}

	void add_breakpoint(uintptr_t address)
	{
		breakpoints[address] = breakpoint(address);
		breakpoints[address].enable();
	}

	void remove_breakpoint(uintptr_t address)
	{
		// check if breakpoint exists
		if (breakpoints.find(address) == breakpoints.end())
			return;

		breakpoints[address].disable();
		breakpoints.erase(address);
	}

	void continue_execution(DEBUG_ACTION action)
	{
		std::lock_guard<std::mutex> lock(breakpoint_mutex);
		if (breakpoint_hit)
		{
			user_action = action;
			resume_cv.notify_one();
		}
	}
}