#pragma once

#include "common.hpp"
#include "util.hpp"

namespace debugger
{
	bool init();
	void add_breakpoint(uintptr_t address);
	void remove_breakpoint(uintptr_t address);
}