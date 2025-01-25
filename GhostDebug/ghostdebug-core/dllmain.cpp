// dllmain.cpp : Defines the entry point for the DLL application.
#include "debugger.hpp"
#include "communication.hpp"
#include "pipe.hpp"

void open_console()
{
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONIN$", "r", stdin);
}

void attach_debugger()
{
    // for testing purposes
    open_console();

    pipe::create("ghostdebug");
    communication::start_listening();
	debugger::init();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

