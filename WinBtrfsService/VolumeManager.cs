using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;

namespace WinBtrfsService
{
	class VolumeEntry
	{
		public ulong instanceID;
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

		public static string List(string[] lines)
		{
			string reply = "Data\n";

			if (VolumeManager.volumeTable.Count == 0)
				reply += "No Entries";
			else
			{
				foreach (var entry in VolumeManager.volumeTable)
				{
					reply += "Entry\n";
					reply += "fsUUID|" + entry.fsUUID.ToString() + "\n";
					reply += "mountData|optSubvol|" + entry.mountData.optSubvol.ToString() + "\n";
					reply += "mountData|optSubvolID|" + entry.mountData.optSubvolID.ToString() + "\n";
					reply += "mountData|optDump|" + entry.mountData.optDump.ToString() + "\n";
					reply += "mountData|optTestRun|" + entry.mountData.optTestRun.ToString() + "\n";
					reply += "mountData|mountPoint|" + entry.mountData.mountPoint.Length + "|" +
						entry.mountData.mountPoint + "\n";
					reply += "mountData|subvolName|" + entry.mountData.subvolName.Length + "|" +
						entry.mountData.subvolName + "\n";
					reply += "mountData|dumpFile|" + entry.mountData.dumpFile.Length + "|" +
						entry.mountData.dumpFile + "\n";
					reply += "mountData|subvolID|" + entry.mountData.subvolID.ToString() + "\n";

					foreach (string device in entry.mountData.devices)
						reply += "mountData|devices|" + device.Length + "|" + device + "\n";
				}
			}

			Program.eventLog.WriteEntry("Processed a List message. Returning:\n" + reply,
				EventLogEntryType.Information);
			return reply;
		}

		public static string Mount(string[] lines)
		{
			bool uniqueID;
			var entry = new VolumeEntry();
			var rnd = new Random();

			/* assign a random, unique ID to the new entry */
			do
			{
				byte[] bytes = new byte[8];
				rnd.NextBytes(bytes);

				entry.instanceID = BitConverter.ToUInt64(bytes, 0);

				uniqueID = true;

				foreach (var otherEntry in volumeTable)
				{
					if (otherEntry.instanceID == entry.instanceID)
					{
						uniqueID = false;
						break;
					}
				}
			}
			while (!uniqueID);

			for (int i = 1; i < lines.Length; i++)
			{
				try
				{
					if (lines[i].Length > 14 && lines[i].Substring(0, 14) == "Option|Subvol|")
					{
						int valLen = int.Parse(lines[i].Substring(14, lines[i].IndexOf('|', 14) - 14));
						string valStr = lines[i].Substring(lines[i].IndexOf('|', 14) + 1, valLen);

						entry.mountData.optSubvol = true;
						entry.mountData.subvolName = valStr;
					}
					else if (lines[i].Length > 16 && lines[i].Substring(0, 16) == "Option|SubvolID|")
					{
						int valLen = int.Parse(lines[i].Substring(16, lines[i].IndexOf('|', 16) - 16));
						string valStr = lines[i].Substring(lines[i].IndexOf('|', 16) + 1, valLen);

						entry.mountData.optSubvolID = true;
						entry.mountData.subvolID = ulong.Parse(valStr);
					}
					else if (lines[i].Length > 12 && lines[i].Substring(0, 12) == "Option|Dump|")
					{
						int valLen = int.Parse(lines[i].Substring(12, lines[i].IndexOf('|', 12) - 12));
						string valStr = lines[i].Substring(lines[i].IndexOf('|', 12) + 1, valLen);

						entry.mountData.optDump = true;
						entry.mountData.dumpFile = valStr;
					}
					else if (lines[i].Length == 14 && lines[i] == "Option|TestRun")
						entry.mountData.optTestRun = true;
					else if (lines[i].Length > 11 && lines[i].Substring(0, 11) == "MountPoint|")
					{
						int valLen = int.Parse(lines[i].Substring(11, lines[i].IndexOf('|', 11) - 11));
						string valStr = lines[i].Substring(lines[i].IndexOf('|', 11) + 1, valLen);

						entry.mountData.mountPoint = valStr;
					}
					else if (lines[i].Length > 7 && lines[i].Substring(0, 7) == "Device|")
					{
						int valLen = int.Parse(lines[i].Substring(7, lines[i].IndexOf('|', 7) - 7));
						string valStr = lines[i].Substring(lines[i].IndexOf('|', 7) + 1, valLen);

						entry.mountData.devices.Add(valStr);
					}
					else
						Program.eventLog.WriteEntry("Encountered an unintelligible line in Mount (ignoring):\n[" +
							i + "] " + lines[i], EventLogEntryType.Warning);
				}
				catch (ArgumentOutOfRangeException) // for string.Substring
				{
					Program.eventLog.WriteEntry("Caught an ArgumentOutOfRangeException in Mount (ignoring):\n[" +
						i + "] " + lines[i], EventLogEntryType.Warning);
				}
				catch (FormatException) // for *.Parse
				{
					Program.eventLog.WriteEntry("Caught an FormatException in Mount (ignoring):\n[" +
						i + "] " + lines[i], EventLogEntryType.Warning);
				}
				catch (OverflowException) // for *.Parse
				{
					Program.eventLog.WriteEntry("Caught an OverflowException in Mount (ignoring):\n[" +
						i + "] " + lines[i], EventLogEntryType.Warning);
				}
			}

			var startInfo = new ProcessStartInfo("WinBtrfsDrv.exe", "--id=" + entry.instanceID.ToString("x") +
				" --pipe-name=WinBtrfsService --parent-pid=" + Process.GetCurrentProcess().Id.ToString());
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

			Program.eventLog.WriteEntry("Processed a Mount message. Returning: OK",
				EventLogEntryType.Information);
			return "OK";
		}

		public static string DrvMountData(string[] lines)
		{
			string reply = "Data\n";

			/* TODO: grab the ID out of the message;
			 * then, find the right volume entry and stringify it as thereply */

			Program.eventLog.WriteEntry("Processed a DrvMountData message. Returning:\n" + reply,
				EventLogEntryType.Information);
			return reply;
		}
	}
}
