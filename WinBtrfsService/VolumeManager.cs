using System;
using System.Collections.Generic;

namespace WinBtrfsService
{
	class VolumeEntry
	{
		public MountData mountData = new MountData();
		public Guid fsUUID = new Guid();
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
	}
}
