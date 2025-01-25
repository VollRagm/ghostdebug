using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using static GhostDebug.Internal.Win32;

namespace GhostDebug.Internal
{
    public class Injector
    {
        private MemTools mem;

        public Injector(MemTools mem)
        {
            this.mem = mem;
        }

        // Simplest LoadLibrary injector
        public bool InjectLoadLib(string dllPath)
        {
            dllPath = Path.GetFullPath(dllPath);
            if (!File.Exists(dllPath))
            {
                return false;
            }

            long loadLibraryPtr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
            long allocAddr = mem.AllocateExecutableMemory((uint)((dllPath.Length + 1) * Marshal.SizeOf(typeof(char))));
            mem.WriteBytes(allocAddr, Encoding.Default.GetBytes(dllPath));
            CreateRemoteThread(mem.ProcessHandle, IntPtr.Zero, 0, loadLibraryPtr, (IntPtr)allocAddr, 0, IntPtr.Zero);
            
            return true;
        }
    }
}
