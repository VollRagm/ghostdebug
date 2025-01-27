using System;
using System.Collections.Generic;
using System.IO.Pipes;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;


namespace GhostDebug.Internal
{
    public class PipeClient
    {
        private NamedPipeClientStream pipeStream;
        public event EventHandler<PipeMessageEventArgs> MessageRecieved;

        public PipeClient(string PipeName)
        {
            pipeStream = new NamedPipeClientStream(".", PipeName, PipeDirection.InOut, PipeOptions.Asynchronous);
        }

        public bool Connect()
        {
            try
            {
                pipeStream.Connect(10000);
                pipeStream.ReadMode = PipeTransmissionMode.Message;
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
                return false;
            }
        }

        public void StartListening()
        {
            var listenThread = new Thread(ListenThread);
            listenThread.Start();
        }

        public async void ListenThread()
        {
            while (true)
            {
                List<byte> buffer = new List<byte>();
                var nBytes = 0;

                do
                {
                    byte[] buf = new byte[1024];
                    nBytes += await pipeStream.ReadAsync(buf, 0, buf.Length);
                    buffer.AddRange(buf);

                } while (!pipeStream.IsMessageComplete);

                var message = Encoding.ASCII.GetString(buffer.ToArray(), 0, nBytes);
                MessageRecieved?.Invoke(this, new PipeMessageEventArgs() { Message = message });
            }
        }

        public async Task<bool> Send(string Message)
        {
            try
            {
                if (pipeStream.IsConnected)
                {
                    byte[] buffer = Encoding.UTF8.GetBytes(Message);
                    await pipeStream.WriteAsync(buffer, 0, buffer.Length);
                    await pipeStream.FlushAsync();
                    pipeStream.WaitForPipeDrain();
                    return true;
                }
                else
                    return false;
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
                return false;
            }
        }
    }

    public class PipeMessageEventArgs : EventArgs
    {
        public string Message { get; set; }
    }
}
