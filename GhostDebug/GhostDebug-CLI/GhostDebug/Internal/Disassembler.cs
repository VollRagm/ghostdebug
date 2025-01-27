using Iced.Intel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace GhostDebug.Internal
{
    public class Disassembler
    {
        private MemTools memory;
        public Disassembler(MemTools mem)
        {
            memory = mem;
        }

        public List<Instruction> DisassembleRegion(ulong address, int instructionsBefore, int instructionsCount, out byte[] codeBytes)
        {
            // read enough bytes to disassemble
            uint offset = (uint)(instructionsBefore + 1) * 15;
            uint readSize = (uint)(instructionsBefore + instructionsCount + 1) * 15;
            List<byte> bytes = memory.ReadBytes(address - offset, readSize).ToList();

            // Try disassembling until we hit the original address as a valid instruction
            List<Instruction> instructions;
            uint currentTry = 0;
            do
            {
                instructions = DisassembleBuffer(bytes.ToArray(), address - offset + currentTry);
                currentTry += 1;
                bytes.RemoveAt(0);

                if(bytes.Count == 0)
                {
                    // if we run out of bytes to disassemble, just disassemble from address
                    codeBytes = memory.ReadBytes(address, readSize).ToArray();
                    instructions = DisassembleBuffer(codeBytes, address);
                    // limit to instructionsCount
                    return instructions.Take(instructionsCount).ToList();
                }

            }while(!IsInstructionAtAddress(address, instructions));
            
            codeBytes = bytes.ToArray();
            // slice to instructionsBefore and instructionsCount
            var position = instructions.FindIndex(i => i.IP == address);
            return instructions.Skip(position - instructionsBefore).Take(instructionsCount).ToList();
        }

        public List<string> DumpInstructions(List<Instruction> instructions, byte[] codeBytes, ulong address, out int addressLineIndex)
        {
            var formatter = new NasmFormatter();
            formatter.Options.DigitSeparator = "";
            formatter.Options.FirstOperandCharIndex = 10;
            var output = new StringOutput();

            addressLineIndex = 0;

            // pretty print with address and instruction
            List<string> lines = new List<string>();
            foreach (var instruction in instructions)
            {
                if(instruction.IP == address)
                    addressLineIndex = lines.Count;
                string line = "";
                formatter.Format(instruction, output);
                line += instruction.IP.ToString("X16") + " ";
                int instrLen = instruction.Length;
                int byteBaseIndex = (int)(instruction.IP - instructions[0].IP);
                for (int i = 0; i < instrLen; i++)
                    line += codeBytes[byteBaseIndex + i].ToString("X2");
                int missingBytes = 10 - instrLen;
                for (int i = 0; i < missingBytes; i++)
                    line += "  ";
                line += " " + output.ToStringAndReset();
                lines.Add(line);
            }

            return lines;
        }

        private bool IsInstructionAtAddress(ulong address, List<Instruction> instructions)
        {
            foreach (var instruction in instructions)
            {
                if (instruction.IP == address)
                    return true;
                if(instruction.IP > address)
                    return false;
            }
            return false;
        }

        private List<Instruction> DisassembleBuffer(byte[] buffer, ulong rip)
        {
            List<Instruction> instructions = new List<Instruction>();
            var codeReader = new ByteArrayCodeReader(buffer);
            var decoder = Decoder.Create(64, codeReader);
            decoder.IP = rip;
            ulong endRip = decoder.IP + (uint)buffer.Length;

            while (decoder.IP < endRip)
                instructions.Add(decoder.Decode());

            return instructions;
        }

    }
}
