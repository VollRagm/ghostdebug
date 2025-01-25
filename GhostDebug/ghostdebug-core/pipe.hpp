#pragma once

#include "common.hpp"

namespace pipe
{
	HANDLE create(std::string name);
	void send(std::string message);
	std::string receive();
}