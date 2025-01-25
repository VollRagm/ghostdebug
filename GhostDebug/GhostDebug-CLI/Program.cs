using GhostConnect;
using System;
using System.Diagnostics;
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

            
            var suggestedPid = Process.Start("C:\\Users\\vr\\Documents\\Visual Studio 2022\\Projects\\ghostdebug\\GhostDebug\\x64\\Release\\TestTarget.exe").Id;
            Console.WriteLine($"Suggested PID for testing: {suggestedPid}");
            

            while (true)
            {
                Console.Write("> ");
                string command = Console.ReadLine();
                Console.WriteLine();
                var result = debugClient.RunCommand(command).Result;
                Console.WriteLine(result ? "" : "Unable to parse \"" + command +"\"");
            }
        }
    }
}
