using System;
using System.Collections.Generic;

namespace WinBtrfsService
{
	class VolumeEntry
	{
		MountOptions mountOptions = new MountOptions();
		byte[] fsUUID = new byte[0x10];
	}

	class MountOptions
	{
		bool optSubvol = false, optSubvolID = false, optDump = false, optTestRun = false;
		string mountPoint = "", subvolName = "", dumpFile = "";
		ulong subvolID = 256;
		List<string> devices = new List<string>();
	}
	
	static class VolumeManager
	{
		public static List<VolumeEntry> volumeTable = new List<VolumeEntry>();
	}
}
