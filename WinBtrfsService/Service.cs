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
						pipeServer.Disconnect();
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
			int bufferLen = 0;

			try
			{
				bufferLen = pipeServer.Read(buffer, 0, buffer.Length);
			}
			catch (ObjectDisposedException)
			{
				Program.eventLog.WriteEntry("The pipe closed unexpectedly when reading.",
					EventLogEntryType.Warning);
				return;
			}
			catch (InvalidOperationException)
			{
				Program.eventLog.WriteEntry("The pipe was in an unexpected state at read time.",
					EventLogEntryType.Warning);
				return;
			}
			catch (IOException e)
			{
				Program.eventLog.WriteEntry("Caught an IOException at read time: " + e.Message,
					EventLogEntryType.Warning);
				return;
			}

			if (pipeServer.IsMessageComplete)
			{
				String str = System.Text.Encoding.Unicode.GetString(buffer, 0, bufferLen);
				Program.eventLog.WriteEntry("Got a pipe connection. Message: " + str, EventLogEntryType.Information);

				byte[] response = System.Text.Encoding.Unicode.GetBytes("OK");
				try
				{
					pipeServer.Write(response, 0, response.Length);

					/* don't disconnect prematurely */
					pipeServer.WaitForPipeDrain();
				}
				catch (ObjectDisposedException)
				{
					Program.eventLog.WriteEntry("The pipe closed unexpectedly when writing.",
						EventLogEntryType.Warning);
					return;
				}
				catch (InvalidOperationException)
				{
					Program.eventLog.WriteEntry("The pipe was in an unexpected state at write time.",
						EventLogEntryType.Warning);
					return;
				}
				catch (IOException e)
				{
					Program.eventLog.WriteEntry("Caught an IOException at write time: " + e.Message,
						EventLogEntryType.Warning);
					return;
				}
			}
			else
			{
				Program.eventLog.WriteEntry("A message larger than 100K arrived; discarding.",
					EventLogEntryType.Warning);
			}
		}
	}
}
