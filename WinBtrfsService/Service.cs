using System;
using System.Diagnostics;
using System.ServiceProcess;

namespace WinBtrfsService
{
	public partial class Service : ServiceBase
	{
		EventLog eventLog;
		
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
			eventLog.WriteEntry("Service started at " + DateTime.Now.ToString(), EventLogEntryType.Information);
		}

		protected override void OnStop()
		{
			eventLog.WriteEntry("Service stopped at " + DateTime.Now.ToString(), EventLogEntryType.Information);
		}
	}
}
