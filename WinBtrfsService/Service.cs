using System;
using System.Diagnostics;
using System.IO;
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
			Program.eventLog.WriteEntry("Got a pipe connection.", EventLogEntryType.Information);

			byte[] buffer = new byte[1024];

			pipeServer.Read(buffer, 0, 1024);

			String str = System.Text.Encoding.ASCII.GetString(buffer);
			Program.eventLog.WriteEntry("Message on pipe: " + str, EventLogEntryType.Information);

			pipeServer.Disconnect();
		}
	}
}
