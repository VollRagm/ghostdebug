#pragma once

#include "common.hpp"

#define INT3 0xCC

#define SET_STEP_FLAG(ContextRecord) ContextRecord->EFlags |= 0x100
#define CLEAR_STEP_FLAG(ContextRecord) ContextRecord->EFlags &= ~0x100

#ifdef _DEBUG
#define LOG(x, ...) printf("[GhostDebug] "); printf(x "\n", __VA_ARGS__)
#else
#define LOG(x)
#endif


namespace util
{
	void place_int3(uintptr_t address, std::vector<uint8_t>& originalBytes);
	void remove_int3(uintptr_t address, std::vector<uint8_t>& originalBytes);
}