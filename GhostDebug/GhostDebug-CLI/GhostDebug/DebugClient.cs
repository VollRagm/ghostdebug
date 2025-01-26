using GhostDebug.Internal;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Dynamic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace GhostConnect
{
    public class DebugClient
    {
        public Process TargetProcess { get; private set; }
        public List<ulong> Breakpoints;

        private CommandParser commandParser;
        private PipeClient debugPort;
        private MemTools memory;
        private Injector injector;


        public DebugClient()
        {
            Breakpoints = new List<ulong>();
            commandParser = new CommandParser(debugPort, this);
        }

        public bool Attach(int processId)
        {
            TargetProcess = Process.GetProcessById(processId);

            if (TargetProcess == null)
                return false;

            memory = new MemTools(TargetProcess);
            injector = new Injector(memory);

            if(!injector.InjectLoadLib("ghostdebug-core.dll"))
                return false;
            Thread.Sleep(1000);

            debugPort = new PipeClient("ghostdebug");
            if(!debugPort.Connect())
                return false;

            debugPort.StartListening();

            return true;
        }

        public async Task<string> RunCommand(string command) 
            => await commandParser.Parse(command);

        public async Task<bool> SetUnsetBreakpoint(string location, bool enable)
        {
            ulong address = commandParser.SymbolToAddress(location);

            bool isBreakpointActive = Breakpoints.Contains(address);

            // breakpoint already in the state requested
            if (isBreakpointActive == enable)
                return true;

            var dataPacket = new DebugEvent()
            {
                Event = enable ? EVENT_CODE.ADD_BREAKPOINT : EVENT_CODE.REMOVE_BREAKPOINT,
                Data = new ExpandoObject()
            };

            dataPacket.Data.address = address;
            var json = JsonConvert.SerializeObject(dataPacket);

            var success = await debugPort.Send(json);

            if (enable)
                Breakpoints.Add(address);
            else
                Breakpoints.Remove(address);

            return success;
        }
    }
}
