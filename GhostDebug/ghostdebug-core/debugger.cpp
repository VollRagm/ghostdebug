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
	// anything that triggers an exception (MessageBox, OutputDebugString, etc) 
	// must be treated with caution here to avoid infinite loops
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

				// dump out the registers
				std::cout << "RAX: 0x" << std::hex << exception_info->ContextRecord->Rax << std::endl;
				std::cout << "RBX: 0x" << std::hex << exception_info->ContextRecord->Rbx << std::endl;
				std::cout << "RCX: 0x" << std::hex << exception_info->ContextRecord->Rcx << std::endl;
				std::cout << "RDX: 0x" << std::hex << exception_info->ContextRecord->Rdx << std::endl;
				std::cout << "R8: 0x" << std::hex << exception_info->ContextRecord->R8 << std::endl;
				std::cout << "R9: 0x" << std::hex << exception_info->ContextRecord->R9 << std::endl;
				std::cout << "R10: 0x" << std::hex << exception_info->ContextRecord->R10 << std::endl;

				// Continue the program
				exception_info->ContextRecord->Rip = (DWORD64)exception_info->ExceptionRecord->ExceptionAddress;
				CLEAR_STEP_FLAG(exception_info->ContextRecord);
				
				MessageBoxA(NULL, "Done!", "Done!", MB_OK);

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

}