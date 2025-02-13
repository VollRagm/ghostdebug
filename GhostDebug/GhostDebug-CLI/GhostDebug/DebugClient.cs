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
        public Dictionary<ulong, Breakpoint> Breakpoints;

        private CommandParser commandParser;
        private PipeClient debugPort;
        private MemTools memory;
        private Injector injector;
        public Disassembler Disasm;

        public event EventHandler<DebugEventArgs> BreakpointHit;
        public event EventHandler<string> AutoExecCommand;


        public DebugClient()
        {
            Breakpoints = new Dictionary<ulong, Breakpoint>();
            commandParser = new CommandParser(debugPort, this);
        }

        public bool Attach(int processId)
        {
            TargetProcess = Process.GetProcessById(processId);

            if (TargetProcess == null)
                return false;

            memory = new MemTools(TargetProcess);
            injector = new Injector(memory);
            Disasm = new Disassembler(memory);

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

        private async void DebugPort_MessageRecieved(object sender, PipeMessageEventArgs e)
        {
            var debugEvent = JsonConvert.DeserializeObject<DebugEvent>(e.Message);
            if (debugEvent != null) {
                switch (debugEvent.Event)
                {
                    case EVENT_CODE.BP_HIT:
                        var data = debugEvent.GetData<Context64>();

                        // check if this is an autoexec bp
                        if (Breakpoints.ContainsKey(data.Address))
                        {
                            var bp = Breakpoints[data.Address];
                            if (!string.IsNullOrEmpty(bp.Command))
                            {
                                AutoExecCommand?.Invoke(this, bp.Command);
                                if (bp.Command.Contains(";"))
                                {
                                    var commands = bp.Command.Split(';');
                                    foreach (var command in commands)
                                    {
                                        await RunCommand(command);
                                    }
                                }
                                else
                                {
                                    await RunCommand(bp.Command);
                                }
                                // if the command resumes execution, don't raise the event
                                if (bp.Command.EndsWith("g"))
                                    return;
                            }
                        }

                        BreakpointHit?.Invoke(this, new DebugEventArgs() { Ctx = data});
                        break;
                }
            }
        }

        public async Task<string> RunCommand(string command) 
            => await commandParser.Parse(command);

        public async Task<bool> SetUnsetBreakpoint(string location, bool enable, string autoExecCommand = "")
        {
            ulong address = commandParser.SymbolToAddress(location);

            bool isBreakpointActive = Breakpoints.ContainsKey(address);

            // breakpoint already in the state requested
            if (isBreakpointActive == enable)
                return false;

            var dataPacket = new DebugEvent()
            {
                Event = enable ? EVENT_CODE.ADD_BREAKPOINT : EVENT_CODE.REMOVE_BREAKPOINT,
                Data = new ExpandoObject()
            };

            dataPacket.Data.address = address;
            var json = JsonConvert.SerializeObject(dataPacket);

            await debugPort.Send(json);

            if (enable)
                Breakpoints.Add(address, new Breakpoint(address, autoExecCommand));
            else
                Breakpoints.Remove(address);

            return true;
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

        public async Task<bool> StepInto() => await ContinueExecution(EVENT_CODE.STEP_INTO);
    }
}
