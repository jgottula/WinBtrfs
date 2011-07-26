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

			try
			{
				entry.drvProc = Process.Start("WinBtrfsDrv.exe",
					"--pipe-name=WinBtrfsService --parent-pid=" + Process.GetCurrentProcess().Id.ToString());
			}
			catch (Win32Exception e)
			{
				Program.eventLog.WriteEntry("Win32Exception on Process.Start for WinBtrfsDrv.exe:\n" +
					e.Message, EventLogEntryType.Error);
				return "Error\nCould not start WinBtrfsDrv.exe: " + e.Message;
			}

			volumeTable.Add(entry);

			return "OK";
		}
	}
}
