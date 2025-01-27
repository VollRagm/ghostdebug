using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;

namespace GhostDebug.Internal
{
    public enum EVENT_CODE
    {
        ADD_BREAKPOINT,
        REMOVE_BREAKPOINT,
        BP_HIT,
        PAUSE,
        RESUME,
        STEP_OVER,
        STEP_INTO,
        STEP_OUT,
        REGISTER_WRITE
    }

    public class Breakpoint
    {
        public ulong Address { get; set; }
        public string Command { get; set; }

        public Breakpoint(ulong address, string command)
        {
            Address = address;
            Command = command;
        }
    }

    public class DebugEvent
    {
        [JsonProperty("event")]
        public EVENT_CODE Event { get; set; }

        [JsonProperty("data")]
        public dynamic Data { get; set; }

        public T GetData<T>()
        {
            return JsonConvert.DeserializeObject<T>(JsonConvert.SerializeObject(Data));
        }

    }

    public class Context64
    {
        [JsonProperty("Address")]
        public ulong Address { get; set; }

        [JsonProperty("Rip")]
        public ulong Rip { get; set; }

        [JsonProperty("Rsp")]
        public ulong Rsp { get; set; }

        [JsonProperty("Rbp")]
        public ulong Rbp { get; set; }

        [JsonProperty("Rdi")]
        public ulong Rdi { get; set; }

        [JsonProperty("Rsi")]
        public ulong Rsi { get; set; }

        [JsonProperty("Rbx")]
        public ulong Rbx { get; set; }

        [JsonProperty("Rdx")]
        public ulong Rdx { get; set; }

        [JsonProperty("Rcx")]
        public ulong Rcx { get; set; }

        [JsonProperty("Rax")]
        public ulong Rax { get; set; }

        [JsonProperty("R8")]
        public ulong R8 { get; set; }

        [JsonProperty("R9")]
        public ulong R9 { get; set; }

        [JsonProperty("R10")]
        public ulong R10 { get; set; }

        [JsonProperty("R11")]
        public ulong R11 { get; set; }

        [JsonProperty("R12")]
        public ulong R12 { get; set; }

        [JsonProperty("R13")]
        public ulong R13 { get; set; }

        [JsonProperty("R14")]
        public ulong R14 { get; set; }

        [JsonProperty("R15")]
        public ulong R15 { get; set; }

        [JsonProperty("EFlags")]
        public uint EFlags { get; set; }
    }

    public class DebugEventArgs : EventArgs
    {
        public Context64 Ctx { get; set; }
    }

}
