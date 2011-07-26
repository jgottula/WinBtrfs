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
			bool header = false;
			var entry = new VolumeEntry();

			foreach (string line in lines)
			{
				if (!header)
				{
					header = true;
					continue;
				}

				try
				{
					if (line.Length > 14 && line.Substring(0, 14) == "Option|Subvol|")
					{
						int valLen = int.Parse(line.Substring(14, line.IndexOf('|', 14) - 14));
						string valStr = line.Substring(line.IndexOf('|', 14) + 1, valLen);

						entry.mountData.optSubvol = true;
						entry.mountData.subvolName = valStr;
					}
					else if (line.Length > 16 && line.Substring(0, 16) == "Option|SubvolID|")
					{
						int valLen = int.Parse(line.Substring(16, line.IndexOf('|', 16) - 16));
						string valStr = line.Substring(line.IndexOf('|', 16) + 1, valLen);

						entry.mountData.optSubvolID = true;
						entry.mountData.subvolID = ulong.Parse(valStr);
					}
					else if (line.Length > 12 && line.Substring(0, 12) == "Option|Dump|")
					{
						int valLen = int.Parse(line.Substring(12, line.IndexOf('|', 12) - 12));
						string valStr = line.Substring(line.IndexOf('|', 12) + 1, valLen);

						entry.mountData.optDump = true;
						entry.mountData.dumpFile = valStr;
					}
					else if (line.Length == 14 && line == "Option|TestRun")
						entry.mountData.optTestRun = true;
					else if (line.Length > 11 && line.Substring(0, 11) == "MountPoint|")
					{
						int valLen = int.Parse(line.Substring(11, line.IndexOf('|', 11) - 11));
						string valStr = line.Substring(line.IndexOf('|', 11) + 1, valLen);

						entry.mountData.mountPoint = valStr;
					}
					else if (line.Length > 7 && line.Substring(0, 7) == "Device|")
					{
						int valLen = int.Parse(line.Substring(11, line.IndexOf('|', 7) - 7));
						string valStr = line.Substring(line.IndexOf('|', 7) + 1, valLen);

						entry.mountData.devices.Add(valStr);
					}
					else
						Program.eventLog.WriteEntry("Encountered an unintelligible line in Mount (ignoring):\n" +
							line, EventLogEntryType.Warning);
				}
				catch (ArgumentOutOfRangeException) // for string.Substring
				{
					Program.eventLog.WriteEntry("Caught an ArgumentOutOfRangeException in Mount (ignoring):\n" +
						line, EventLogEntryType.Warning);
				}
				catch (FormatException) // for *.Parse
				{
					Program.eventLog.WriteEntry("Caught an FormatException in Mount (ignoring):\n" +
						line, EventLogEntryType.Warning);
				}
				catch (OverflowException) // for *.Parse
				{
					Program.eventLog.WriteEntry("Caught an OverflowException in Mount (ignoring):\n" +
						line, EventLogEntryType.Warning);
				}
			}

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
