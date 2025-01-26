using GhostConnect;
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;

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
            

            while (true)
            {
                Console.Write("> ");
                string command = Console.ReadLine();
                var result = debugClient.RunCommand(command).Result;
                Console.WriteLine(result == "" ? result + "\n" : "");
            }
        }
    }
}
