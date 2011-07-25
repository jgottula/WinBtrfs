using System;
using System.Collections.Generic;

namespace WinBtrfsService
{
	class VolumeEntry
	{
		public MountOptions mountOptions = new MountOptions();
		public Guid fsUUID = new Guid();
	}

	class MountOptions
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
