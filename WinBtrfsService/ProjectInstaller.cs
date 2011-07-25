using System;
using System.ComponentModel;
using System.Configuration.Install;
using System.Diagnostics;

namespace WinBtrfsService
{
	[RunInstaller(true)]
	public partial class ProjectInstaller : System.Configuration.Install.Installer
	{
		public ProjectInstaller()
		{
			InitializeComponent();
		}

		private void ProjectInstaller_AfterInstall(object sender, InstallEventArgs e)
		{
			if (!EventLog.SourceExists("WinBtrfsService"))
				EventLog.CreateEventSource("WinBtrfsService", "WinBtrfsService");
		}
	}
}
