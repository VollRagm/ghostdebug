using GhostConnect;
using Newtonsoft.Json;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;

namespace GhostDebug.Internal
{
    public enum EVENT_CODE
    {
        ADD_BREAKPOINT,
        REMOVE_BREAKPOINT,
        PAUSE,
        RESUME,
        STEP_IN,
        STEP_OVER
    }

    public class DebugEvent
    {
        [JsonProperty("event")]
        public EVENT_CODE Event { get; set; }

        [JsonProperty("data")]
        public dynamic Data { get; set; }
    }

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

        

        public async Task<bool> Parse(string command)
        {
            string[] parts = command.Split(' ');
            string cmd = parts[0].ToLower();

            switch (cmd)
            {
                case "attach":
                    if (parts.Length < 2)
                        return false;

                    int pid = int.Parse(parts[1]);
                    return debugClient.Attach(pid);

                case "bp":
                    return await debugClient.SetUnsetBreakpoint(parts[1], true);

                case "ubp":
                    return await debugClient.SetUnsetBreakpoint(parts[1], false);

                default:
                    return false;
            }
            
        }
    }
}
