using System;
using System.Diagnostics;
using System.ServiceProcess;

namespace WinBtrfsService
{
	static class Program
	{
		public static EventLog eventLog;

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		static void Main()
		{
			if (!EventLog.SourceExists("WinBtrfsService"))
			{
				EventLog.CreateEventSource("WinBtrfsService", "WinBtrfsService");

				/* the method above needs the application to restart, apparently */
				Console.WriteLine("EventLog source did not exist. " +
					"Run WinBtrfsService again so event logging will work.");
				return;
			}

			eventLog = new EventLog("WinBtrfsService", ".", "WinBtrfsService");
			
			ServiceBase[] ServicesToRun = new ServiceBase[]
			{
				new Service()
			};

			try
			{
				ServiceBase.Run(ServicesToRun);
			}
			catch (Exception e)
			{
				eventLog.WriteEntry("Caught an unhandled exception! Message:\n" + e.Message, EventLogEntryType.Error);
			}
		}
	}
}
