using System;
using System.Collections.Generic;
using System.Diagnostics;
using static GhostDebug.Internal.Win32;

namespace GhostDebug.Internal
{
    public class MemTools
    {
        public IntPtr ProcessHandle;
        private Process process;
        public long MainModuleBaseAddress
        {
            get => (long)process.MainModule.BaseAddress;
        }

        public MemTools(Process proc)
        {
            process = proc;
            ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, false, proc.Id);
        }

        public byte[] ReadBytes(long address, uint size)
        {
            byte[] lpBuffer = new byte[size];
            ReadProcessMemory(ProcessHandle, address, lpBuffer, size, 0);
            return lpBuffer;
        }

        public void WriteBytes(long address, byte[] bytes)
        {
            WriteProcessMemory(ProcessHandle, address, bytes, (uint)bytes.Length, 0);
        }
        public long AllocateExecutableMemory(uint size)
        {
            return (long)VirtualAllocEx(ProcessHandle, IntPtr.Zero, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        }
    }
}
