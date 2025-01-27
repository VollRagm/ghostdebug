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

        public event EventHandler<DebugEventArgs> BreakpointHit;


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
            debugPort.MessageRecieved += DebugPort_MessageRecieved;

            return true;
        }

        private void DebugPort_MessageRecieved(object sender, PipeMessageEventArgs e)
        {
            var debugEvent = JsonConvert.DeserializeObject<DebugEvent>(e.Message);
            if (debugEvent != null) {
                switch (debugEvent.Event)
                {
                    case EVENT_CODE.BP_HIT:
                        BreakpointHit?.Invoke(this, new DebugEventArgs() { Ctx = debugEvent.GetData<Context64>() });
                        break;
                }
            }
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

        private async Task<bool> ContinueExecution(EVENT_CODE code)
        {
            var dataPacket = new DebugEvent() { Event = code };
            var json = JsonConvert.SerializeObject(dataPacket);
            return await debugPort.Send(json);
        }


        public async Task<bool> WriteRegister(string register, ulong value)
        {
            var dataPacket = new DebugEvent()
            {
                Event = EVENT_CODE.REGISTER_WRITE,
                Data = new ExpandoObject()
            };
            register = register.ToUpper();
            dataPacket.Data.register = register;
            dataPacket.Data.value = value;

            var json = JsonConvert.SerializeObject(dataPacket);
            return await debugPort.Send(json);
        }

        public async Task<bool> Resume() => await ContinueExecution(EVENT_CODE.RESUME);

        public async Task<bool> StepOver() => await ContinueExecution(EVENT_CODE.STEP_OVER);
    }
}
