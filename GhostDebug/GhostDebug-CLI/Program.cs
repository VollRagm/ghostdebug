using GhostConnect;
using GhostDebug.Internal;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;

namespace GhostDebug
{
    internal class Program
    {
        static void Main(string[] args)
        {
            DebugClient debugClient = new DebugClient();
            
            // For testing:
            Process.GetProcessesByName("TestTarget").ToList().ForEach(p => p.Kill());

            
            var suggestedPid = Process.Start(Path.GetFullPath("..\\..\\..\\x64\\Release\\TestTarget.exe")).Id;
            Console.WriteLine($"Suggested PID for testing: {suggestedPid}");
            
            debugClient.BreakpointHit += (s, e) =>
            {
                Context64 ctx = e.Ctx;
                Console.WriteLine("\n");
                Console.WriteLine($"Breakpoint hit at {ctx.Address:X16}");
                // Print out most important registers, two per line
                Console.WriteLine($"{"RAX",-5}: {ctx.Rax:X16} {"RCX",-5}: {ctx.Rcx:X16}");
                Console.WriteLine($"{"RDX",-5}: {ctx.Rdx:X16} {"RBX",-5}: {ctx.Rbx:X16}");
                Console.WriteLine($"{"R8",-5}: {ctx.R8:X16} {"R9",-5}: {ctx.R9:X16}");
                Console.WriteLine($"{"RSP",-5}: {ctx.Rsp:X16} {"RBP",-5}: {ctx.Rbp:X16}");
                Console.WriteLine($"{"RSI",-5}: {ctx.Rsi:X16} {"RDI",-5}: {ctx.Rdi:X16}");
                Console.WriteLine($"{"RIP",-5}: {ctx.Rip:X16}");
                Console.WriteLine($"{"EFLAGS",-5}: {ctx.EFlags:X8}");

                Console.WriteLine();

                var instructions = debugClient.Disasm.DisassembleRegion(ctx.Address, 4, 10, out byte[] codeBytes);
                var lines = debugClient.Disasm.DumpInstructions(instructions, codeBytes, ctx.Address, out int addressLineIndex);

                for(int i = 0; i < lines.Count; i++)
                {
                    if (i == addressLineIndex)
                    {
                        ConsoleColor prev = Console.ForegroundColor;
                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.WriteLine($"> {lines[i]}");
                        Console.ForegroundColor = prev;
                    }
                    else
                        Console.WriteLine($"  {lines[i]}");
                }

                Console.WriteLine();
                Console.Write("> ");
            };

            debugClient.RunCommand("attach " + suggestedPid).Wait();
            Thread.Sleep(400);
            debugClient.RunCommand("bp 00007FF76F4E1EE0").Wait();

            while (true)
            {
                Console.WriteLine();
                Console.Write("> ");
                string command = Console.ReadLine();
                var result = debugClient.RunCommand(command).Result;

                if(result != "")
                    Console.WriteLine(result);
            }
        }
    }
}
