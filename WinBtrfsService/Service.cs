﻿using System;
using System.Diagnostics;
using System.ServiceProcess;
using System.Threading;

namespace WinBtrfsService
{
	public partial class Service : ServiceBase
	{
		Thread loopThread;
		EventLog eventLog;
		Object mutex;
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
			loopThread = new Thread(ServiceLoop);
			loopThread.Start();

			eventLog.WriteEntry("Service started at " + DateTime.Now.ToString() + ".", EventLogEntryType.Information);
		}

		protected override void OnStop()
		{
			lock (mutex)
				terminate = true;
			
			eventLog.WriteEntry("Service stopped at " + DateTime.Now.ToString() + ".", EventLogEntryType.Information);
		}

		private void ServiceLoop()
		{
			while (true)
			{
				lock (mutex)
				{
					if (terminate)
						break;
				}


			}
		}
	}
}