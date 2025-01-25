// TestTarget.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>


int debug_this(uint64_t arg1, uint64_t arg2)
{
	return arg1 + arg2;
}

int main()
{
	// Print address of debug_this
	std::cout << "Address of debug_this: " << std::hex << (void*)debug_this << std::endl;

	// wait for debugger
	std::cin.get();

	// Compiler optimizations must be disabled for this to be called
	std::cout << debug_this(0xDEADC0D3, 0xEACEACEAC);

	std::cin.get();
}