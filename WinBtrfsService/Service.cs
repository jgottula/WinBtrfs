using System;
using System.Diagnostics;
using System.IO.Pipes;
using System.ServiceProcess;
using System.Threading;

namespace WinBtrfsService
{
	public partial class Service : ServiceBase
	{
		NamedPipeServerStream pipeServer;
		Thread threadPipeLoop;
		ManualResetEvent eventTerm, eventPipeConnection;
		
		public Service()
		{
			InitializeComponent();
		}

		protected override void OnStart(string[] args)
		{
			pipeServer = new NamedPipeServerStream("WinBtrfsService", PipeDirection.InOut, 1,
				PipeTransmissionMode.Message, PipeOptions.Asynchronous);

			eventTerm = new ManualResetEvent(false);
			eventPipeConnection = new ManualResetEvent(false);

			threadPipeLoop = new Thread(PipeLoop);
			threadPipeLoop.Start();

			Program.eventLog.WriteEntry("Service started.", EventLogEntryType.Information);
		}

		protected override void OnStop()
		{
			eventTerm.Set();
			
			Program.eventLog.WriteEntry("Service stopped.", EventLogEntryType.Information);
		}

		private void PipeLoop()
		{
			while (true)
			{
				var result = pipeServer.BeginWaitForConnection(_ => eventPipeConnection.Set(), null);
				
				WaitHandle[] events = { eventTerm, eventPipeConnection };

			wait:
				switch (WaitHandle.WaitAny(events))
				{
				case 0: // eventTerm
					return;
				case 1: // eventPipeConnection
					if (result.IsCompleted)
					{
						pipeServer.EndWaitForConnection(result);
						GotConnection();
						break;
					}
					else
						goto wait;
				}
			}
		}

		private void GotConnection()
		{
			byte[] buffer = new byte[102400];
			int bufferLen = pipeServer.Read(buffer, 0, buffer.Length);

			if (pipeServer.IsMessageComplete)
			{
				String str = System.Text.Encoding.Unicode.GetString(buffer, 0, bufferLen);
				Program.eventLog.WriteEntry("Got a pipe connection. Message: " + str, EventLogEntryType.Information);

				byte[] response = System.Text.Encoding.Unicode.GetBytes("OK");
				pipeServer.Write(response, 0, response.Length);
			}
			else
			{
				Program.eventLog.WriteEntry("A message larger than 100K arrived; discarding.",
					EventLogEntryType.Warning);
			}

			pipeServer.Disconnect();
		}
	}
}
