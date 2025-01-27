using GhostConnect;
using Newtonsoft.Json;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;

namespace GhostDebug.Internal
{
    public class CommandParser
    {
        private PipeClient debugPort;
        private DebugClient debugClient;

        public CommandParser(PipeClient debugPort, DebugClient debugClient)
        {
            this.debugPort = debugPort;
            this.debugClient = debugClient;
        }

        public ulong SymbolToAddress(string symbol)
        {
            try
            {
                // + means offset from module, for example ntdll+DEAD
                // without ntdll its from the base address
                if (symbol.Contains("+"))
                {
                    ulong baseAddress = 0;
                    ulong offset = 0;
                    if (symbol[0] == '+')
                    {
                        baseAddress = (ulong)debugClient.TargetProcess.MainModule.BaseAddress.ToInt64();
                        offset = ulong.Parse(symbol.Substring(1), System.Globalization.NumberStyles.HexNumber);
                    }
                    else
                    {
                        string moduleName = symbol.Split('+')[0];
                        ProcessModule pm = debugClient.TargetProcess.Modules.Cast<ProcessModule>().First(m => m.ModuleName == moduleName);

                        if (pm == null)
                            return 0;

                        baseAddress = (ulong)pm.BaseAddress.ToInt64();
                        offset = ulong.Parse(symbol.Split('+')[1], System.Globalization.NumberStyles.HexNumber);
                    }
                    return baseAddress + offset;
                }

                return ulong.Parse(symbol, System.Globalization.NumberStyles.HexNumber);
            }
            catch { return 0; } // Unable to parse what was given
        }

        

        public async Task<string> Parse(string command)
        {
            string[] parts = command.Split(' ');
            string cmd = parts[0].ToLower();

            switch (cmd)
            {
                case "attach":
                    if (parts.Length < 2)
                        return "Usage: attach <pid/process name>";

                    int pid = 0;
                    if(!int.TryParse(parts[1], out pid))
                    {
                        Process[] processes = Process.GetProcessesByName(parts[1]);
                        if (processes.Length == 0)
                            return "No processes found with that name.";

                        pid = processes[0].Id;
                    }
                    if(!debugClient.Attach(pid))
                        return "Unable to attach to target.";

                    return "";

                case "bp":

                    if(parts.Length < 2)
                        return "Usage: bp <symbol/address>";

                    string commandList = "";
                    if(parts.Length > 2)
                        // combine the rest of the parts into a single string
                        commandList = string.Join(" ", parts.Skip(2));

                     if(!await debugClient.SetUnsetBreakpoint(parts[1], true, commandList))
                        return "Unable to set breakpoint.";
                    return "";

                case "cl":
                    if (parts.Length < 2)
                        return "Usage: cl <symbol/address>";

                    if (!await debugClient.SetUnsetBreakpoint(parts[1], false))
                        return "Unable to clear breakpoint.";
                    return "";

                case "g":
                    await debugClient.Resume();
                    return "";

                case "t":
                    await debugClient.StepInto();
                    return "";

                case "rw":
                    if (parts.Length < 3)
                        return "Usage: rw <register> <value>";

                    await debugClient.WriteRegister(parts[1], ulong.Parse(parts[2], System.Globalization.NumberStyles.HexNumber));
                    return "";

                case "help":
                    return "Commands:\n" +
                        "attach <pid> - Attach to a process\n" +
                        "bp <symbol/address> - Set a breakpoint\n" +
                        "bp <symbol/address> <command list> - Set a breakpoint with automatic command execution (e.g. 'bp 1234 rw rax 0;g')\n" +
                        "cl <symbol/address> - Clear a breakpoint\n" +
                        "g - Resumes execution\n" +
                        "t - Step Into (single step) one instruction\n" +
                        "rw <register> <value> - Write a value into a register" +
                        "help - Display this message" +
                        "\n\n" +
                        "Info: Addresses can be provided as absolute addresses, or:\n" +
                        "Relative from main module: +1234FFFF\n" +
                        "Relative from module: ntdll+1234FFFF\n" +
                        "Symbol in main module: !my_func\n" +
                        "Symbol in module: kernel32!ExitProcess";
                default:
                    return cmd + " is not a valid command. Type help for a list of commands.";
            }
            
        }
    }
}
