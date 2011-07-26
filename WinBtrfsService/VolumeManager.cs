using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;

namespace WinBtrfsService
{
	class VolumeEntry
	{
		public MountData mountData = new MountData();
		public Guid fsUUID = new Guid();
		public Process drvProc = null;

		public void Process_OutputDataReceived(object sender, DataReceivedEventArgs e)
		{ }

		public void Process_ErrorDataReceived(object sender, DataReceivedEventArgs e)
		{ }
	}

	class MountData
	{
		public bool optSubvol = false, optSubvolID = false, optDump = false, optTestRun = false;
		public string mountPoint = "", subvolName = "", dumpFile = "";
		public ulong subvolID = 256;
		public List<string> devices = new List<string>();
	}
	
	static class VolumeManager
	{
		public static List<VolumeEntry> volumeTable = new List<VolumeEntry>();

		public static string Mount(string[] lines)
		{
			var entry = new VolumeEntry();
			/* TODO: populate the mountData fields from information in lines */

			var startInfo = new ProcessStartInfo("WinBtrfsDrv.exe",
					"--pipe-name=WinBtrfsService --parent-pid=" + Process.GetCurrentProcess().Id.ToString());
			startInfo.UseShellExecute = false;
			startInfo.CreateNoWindow = true;
			startInfo.ErrorDialog = false;
			startInfo.RedirectStandardInput = true;
			startInfo.RedirectStandardOutput = true;
			startInfo.RedirectStandardError = true;

			try
			{
				entry.drvProc = Process.Start(startInfo);
			}
			catch (Win32Exception e)
			{
				Program.eventLog.WriteEntry("Win32Exception on Process.Start for WinBtrfsDrv.exe:\n" +
					e.Message, EventLogEntryType.Error);
				return "Error\nCould not start WinBtrfsDrv.exe: " + e.Message;
			}

			entry.drvProc.OutputDataReceived += entry.Process_OutputDataReceived;
			entry.drvProc.ErrorDataReceived += entry.Process_ErrorDataReceived;

			volumeTable.Add(entry);

			return "OK";
		}
	}
}
