#pragma once

#include "util.hpp"
#include "hde64.h"

namespace util
{
	std::unordered_map<std::string, DWORD64 CONTEXT::*> RegisterMap = {
		{ "RAX", &CONTEXT::Rax },
		{ "RBX", &CONTEXT::Rbx },
		{ "RCX", &CONTEXT::Rcx },
		{ "RDX", &CONTEXT::Rdx },
		{ "RSI", &CONTEXT::Rsi },
		{ "RDI", &CONTEXT::Rdi },
		{ "RBP", &CONTEXT::Rbp },
		{ "RSP", &CONTEXT::Rsp },
		{ "RIP", &CONTEXT::Rip },
		{ "R8", &CONTEXT::R8 },
		{ "R9", &CONTEXT::R9 },
		{ "R10", &CONTEXT::R10 },
		{ "R11", &CONTEXT::R11 },
		{ "R12", &CONTEXT::R12 },
		{ "R13", &CONTEXT::R13 },
		{ "R14", &CONTEXT::R14 },
		{ "R15", &CONTEXT::R15 }
	};


	DWORD64 CONTEXT::* parse_register(std::string registerName)
	{
		if(RegisterMap.find(registerName) != RegisterMap.end())
			return RegisterMap[registerName];
		return nullptr;
	}

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