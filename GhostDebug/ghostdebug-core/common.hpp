#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

