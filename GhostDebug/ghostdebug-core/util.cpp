#pragma once

#include "util.hpp"
#include "hde64.h"

namespace util
{
	void place_int3(uintptr_t address, std::vector<uint8_t>& originalBytes)
	{
		hde64s hs;
		// Disassemble the instruction at the address to get the length of the instruction
		hde64_disasm((void*)address, &hs);

		// Store the original instruction
		originalBytes.resize(hs.len);
		memcpy(originalBytes.data(), (void*)address, hs.len);

		// Write the breakpoint
		DWORD oldProtect;
		VirtualProtect((void*)address, hs.len, PAGE_EXECUTE_READWRITE, &oldProtect);
		
		((uint8_t*)address)[0] = INT3;

		VirtualProtect((void*)address, hs.len, oldProtect, &oldProtect);
	}

	void remove_int3(uintptr_t address, std::vector<uint8_t>& originalBytes)
	{
		DWORD oldProtect;
		VirtualProtect((void*)address, originalBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);

		memcpy((void*)address, originalBytes.data(), originalBytes.size());

		VirtualProtect((void*)address, originalBytes.size(), oldProtect, &oldProtect);
	}
}