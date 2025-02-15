#include "detection.hpp"

namespace detection
{
	void CheckIsDebuggerPresent()
	{
		if (IsDebuggerPresent())
			std::cout << "IsDebuggerPresent: Debugger detected!" << std::endl;
		else
			std::cout << "IsDebuggerPresent: Debugger not detected!" << std::endl;
	}

	void CheckRemoteDebugger()
	{
		BOOL isDebuggerPresent = FALSE;
		CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent);
		if (isDebuggerPresent)
			std::cout << "CheckRemoteDebuggerPresent: Debugger detected!" << std::endl;
		else
			std::cout << "CheckRemoteDebuggerPresent: Debugger not detected!" << std::endl;
	}

	void CheckNtGlobalFlag()
	{
		uintptr_t peb = __readgsqword(0x60);
		uintptr_t ntGlobalFlag = *(uintptr_t*)(peb + 0xBC);
		if (ntGlobalFlag & 0x70)
			std::cout << "NtGlobalFlag: Debugger detected!" << std::endl;
		else
			std::cout << "NtGlobalFlag: Debugger not detected!" << std::endl;
	}

	void CheckDebugBreak()
	{
		// get start time
		auto start = std::chrono::high_resolution_clock::now();
		__try
		{
			DebugBreak();
		}
		__except (GetExceptionCode() == EXCEPTION_BREAKPOINT ?
			EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			
		}
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;

		// This is a rough estimate, but if the time is more than 2ms, then a debugger is likely attached
		if (elapsed.count() > 0.002)
			std::cout << "DebugBreak: Debugger detected!" << std::endl;
		else
			std::cout << "DebugBreak: Debugger not detected!" << std::endl;
	}

	void CheckCloseHandle()
	{
		__try
		{
			CloseHandle((HANDLE)0xDEADBEEF);
			std::cout << "CloseHandle: Debugger not detected!" << std::endl;
		}
		__except (EXCEPTION_INVALID_HANDLE == GetExceptionCode()
			? EXCEPTION_EXECUTE_HANDLER
			: EXCEPTION_CONTINUE_SEARCH)
		{
			std::cout << "CloseHandle: Debugger detected!" << std::endl;
		}
	}


#pragma section(".text")
	__declspec(allocate(".text")) const char _NtQuerySyscallStub[] =
	{
		0x4C, 0x8B, 0xD1,											// mov r10, rcx
		0xB8, 0x19, 0x00, 0x00, 0x00,								// mov eax, 19h
		0x0F, 0x05,													// syscall
		0xC3 														// ret
	};

	typedef NTSTATUS(NTAPI* TNtQueryInformationProcess)(
		IN HANDLE           ProcessHandle,
		IN DWORD ProcessInformationClass,
		OUT PVOID           ProcessInformation,
		IN ULONG            ProcessInformationLength,
		OUT PULONG          ReturnLength
		);

#pragma comment(lib, "ntdll.lib")
	extern "C" NTSTATUS DECLSPEC_IMPORT NTAPI NtSetInformationProcess(HANDLE, PROCESS_INFORMATION_CLASS, PVOID, ULONG);

	using PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION = struct _PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION
	{
		ULONG Version;
		ULONG Reserved;
		void* Callback;
	};

	void CheckNtQueryInformationProcess()
	{
		// Remove any instrumentation callbacks that may filter direct syscalls
		PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION callback = { 0 };
		// Reserved is always 0
		callback.Reserved = 0;
		callback.Version = 0;
		callback.Callback = nullptr;

		// Remove callback
		NtSetInformationProcess(GetCurrentProcess(), (PROCESS_INFORMATION_CLASS)0x28, &callback, sizeof(callback));


		// NtQueryInformationProcess
		auto NtQueryInformationProcess = (TNtQueryInformationProcess)(char*)_NtQuerySyscallStub;
		HANDLE hProcessDebugObject = 0, dwReturned;
		const DWORD ProcessDebugObjectHandle = 0x1e;

		// Query ProcessDebugObjectHandle
		HANDLE hDebugObject;
		NTSTATUS status = NtQueryInformationProcess(GetCurrentProcess(),
			ProcessDebugObjectHandle, &hDebugObject, sizeof(hDebugObject), NULL);

		if (status >= 0 && hDebugObject)
			std::cout << "ProcessDebugObjectHandle: Debugger detected!" << std::endl;
		else
			std::cout << "ProcessDebugObjectHandle: Debugger not detected!" << std::endl;
	}


	void CheckAll()
	{
		CheckIsDebuggerPresent();
		CheckRemoteDebugger();
		CheckNtGlobalFlag();
		CheckNtQueryInformationProcess();
		CheckDebugBreak();
		CheckCloseHandle();
	}
}