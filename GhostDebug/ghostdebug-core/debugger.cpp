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

	// Central exception handler
	LONG WINAPI exception_handler(EXCEPTION_POINTERS* exception_info)
	{
		if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
		{
			// Check if the breakpoint is in our list
			if (breakpoints.find((uintptr_t)exception_info->ExceptionRecord->ExceptionAddress) != breakpoints.end())
			{
				// Disable the breakpoint
				breakpoints[(uintptr_t)exception_info->ExceptionRecord->ExceptionAddress].disable();

				std::cout << "Breakpoint hit at: " << std::hex << exception_info->ExceptionRecord->ExceptionAddress << std::endl;

				// Continue the program
				exception_info->ContextRecord->Rip = (DWORD64)exception_info->ExceptionRecord->ExceptionAddress;
				CLEAR_STEP_FLAG(exception_info->ContextRecord);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}


		return EXCEPTION_CONTINUE_SEARCH;
	}

	void init()
	{
		// Catch all exceptions
		AddVectoredExceptionHandler(1, exception_handler);
	}

	void add_breakpoint(uintptr_t address)
	{
		breakpoints[address] = breakpoint(address);
	}

	void remove_breakpoint(uintptr_t address)
	{
		// check if breakpoint exists
		if (breakpoints.find(address) == breakpoints.end())
			return;

		breakpoints[address].disable();
		breakpoints.erase(address);
	}

}