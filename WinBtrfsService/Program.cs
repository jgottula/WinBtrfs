using System;
using System.Diagnostics;
using System.IO;
using System.ServiceProcess;

namespace WinBtrfsService
{
	static class Program
	{
		public static EventLog eventLog = null;

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		static void Main()
		{
			/* change the working directory to the directory containing the executable */
			string modulePath = Process.GetCurrentProcess().MainModule.FileName;
			Directory.SetCurrentDirectory(modulePath.Substring(0, modulePath.LastIndexOf('\\')));
			
			AppDomain.CurrentDomain.UnhandledException +=
				new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);
			
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

			ServiceBase.Run(ServicesToRun);
		}

		static void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
		{
			try
			{
				Exception exception = (Exception)e.ExceptionObject;

				if (eventLog != null)
					eventLog.WriteEntry("Caught an unhandled exception: " + exception.Message +
						"\n" + exception.StackTrace,
						EventLogEntryType.Error);
			}
			finally
			{
				Environment.Exit(2);
			}
		}
	}
}
