// dllmain.cpp : Defines the entry point for the DLL application.
#include "debugger.hpp"
#include "communication.hpp"
#include "pipe.hpp"

void attach_debugger()
{
    LOG("GhostDebug attached");

    bool success = pipe::create("\\\\.\\pipe\\ghostdebug") != INVALID_HANDLE_VALUE;

    if (!success)
    {
        LOG("Failed to create pipe!");
        return;
    }

    communication::start_listening();

    LOG("Initialized communication!");

	success = debugger::init();

    if(!success)
		LOG("Failed to initialize VEH handler!");
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        attach_debugger();
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

