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
		EventLog eventLog;
		NamedPipeServerStream pipeServer;
		Thread loopThread;
		bool terminate = false;
		
		public Service()
		{
			InitializeComponent();

			if (!EventLog.SourceExists("WinBtrfsService"))
			{
				EventLog.CreateEventSource("WinBtrfsService", "WinBtrfsService");

				/* the method above needs the application to restart, apparently */
				throw new Exception("EventLog source did not exist. " +
					"Run WinBtrfsService again so event logging will work.");
			}

			eventLog = new EventLog("WinBtrfsService", ".", "WinBtrfsService");
		}

		protected override void OnStart(string[] args)
		{
			pipeServer = new NamedPipeServerStream("WinBtrfsService", PipeDirection.InOut, 1,
				PipeTransmissionMode.Message, PipeOptions.None);
			
			loopThread = new Thread(ServiceLoop);
			loopThread.Start();

			eventLog.WriteEntry("Service started.", EventLogEntryType.Information);
		}

		protected override void OnStop()
		{
			lock (this)
				terminate = true;
			
			eventLog.WriteEntry("Service stopped.", EventLogEntryType.Information);
		}

		private bool CheckTerm()
		{
			lock (this)
				return terminate;
		}

		private void ServiceLoop()
		{
			var result = pipeServer.BeginWaitForConnection(null, null);
			
			while (!CheckTerm())
			{
				if (result.AsyncWaitHandle.WaitOne(100))
				{
					pipeServer.EndWaitForConnection(result);

					eventLog.WriteEntry("Got a pipe connection.", EventLogEntryType.Information);


					result = pipeServer.BeginWaitForConnection(null, null);
				}

				/*Thread.Sleep(10);*/
			}
		}

		/*private void GotConnection(IAsyncResult result)
		{

		}*/
	}
}
