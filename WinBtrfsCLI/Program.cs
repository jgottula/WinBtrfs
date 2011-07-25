using System;

namespace WinBtrfsCLI
{
	class Program
	{
		static void Main(string[] args)
		{
			Console.WriteLine("WinBtrfsCLI: .NET Stub");

			Usage();
		}

		static void Usage()
		{
			Console.Write("\n\nUsage: WinBtrfsCLI.exe <command> [parameters]\n\n" +
				"Commands: mount, [more to be added]\n\n" +
				"WinBtrfsCLI.exe mount [options] <mount point> <device> [<device> ...]\n" +
				"Options:\n" +
				"--subvol=<name>  mount the subvolume with the given name (case-sensitive)\n" +
				"--subvol-id=<id> mount the subvolume with the given ID\n" +
				"--dump=<file>    dump filesystem trees to the given file\n" +
				"--test-run       stop just short of actually mounting the volume\n");
			
			Environment.Exit(1);
		}

		static void UsageError(string msg)
		{
			Console.WriteLine(msg);

			Usage();
		}
	}
}
