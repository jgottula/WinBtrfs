﻿using System;
using System.Collections.Generic;
using System.IO.Pipes;

namespace WinBtrfsCLI
{
	class Program
	{
		static void Main(string[] args)
		{
			Console.WriteLine("WinBtrfsCLI: .NET Variant");

			ParseArgs(args);
		}

		static void ParseArgs(string[] args)
		{
			if (args.Length == 0)
				UsageError("You didn't specify a command!");

			switch (args[0].ToLower()) // case insensitive
			{
			case "mount":
				MountArgs(args);
				break;
			default:
				UsageError("'" + args[0] + "' is not a recognized command!");
				break;
			}
		}

		static void MountArgs(string[] args)
		{
			bool argCommand = false, argMountPoint = false, argDevice = false;
			bool optSubvol = false, optSubvolID = false, optDump = false, optTestRun = false;
			string mountPoint = "", subvolName = "", dumpFile = "";
			ulong subvolID = 256;
			var devices = new List<string>();

			foreach (string arg in args)
			{
				if (!argCommand) // skip the first arg (the command)
				{
					argCommand = true;
					continue;
				}
				
				if (arg[0] == '-') // option
				{
					if (arg.Substring(0, 9) == "--subvol=")
					{
						if (optSubvol)
							UsageError("You specified --subvol more than once!");
						else if (optSubvolID)
							UsageError("You can't specify both a subvol name and an ID; pick just one!");

						if (arg.Length == 9)
							UsageError("You specified --subvol but didn't supply a subvolume name!");

						optSubvol = true;

						subvolName = arg.Substring(9);
					}
					else if (arg.Substring(0, 12) == "--subvol-id=")
					{
						if (optSubvolID)
							UsageError("You specified --subvol-id more than once!");
						else if (optSubvol)
							UsageError("You can't specify both a subvol name and an ID; pick just one!");

						if (arg.Length == 12)
							UsageError("You specified --subvol-id but didn't supply a subvolume ID!");

						optSubvolID = true;

						if (!ulong.TryParse(arg.Substring(12), out subvolID))
							UsageError("Could not read the value you specified for --subvol-id!");
					}
					else if (arg.Substring(0, 7) == "--dump=")
					{
						if (optDump)
							UsageError("You specified --dump more than once!");

						if (arg.Length == 7)
							UsageError("You specified --dump but didn't supply a file to which to dump!");

						optDump = true;

						dumpFile = arg.Substring(7);
					}
					else if (arg.Substring(0, 10) == "--test-run")
					{
						if (optTestRun)
							UsageError("You specified --test-run more than once!");

						optTestRun = true;
					}
					else
						UsageError("'" + arg + "' is not a valid option for the mount command!");
				}
				else
				{
					if (!argMountPoint)
					{
						argMountPoint = true;

						mountPoint = arg;
					}
					else
					{
						if (!argDevice)
							argDevice = true;

						devices.Add(arg);
					}
				}
			}

			if (!argMountPoint)
				UsageError("You didn't specify a mount point!");
			else if (!argDevice)
				UsageError("You didn't specify one or more devices to mount!");

			string msg = "Mount\n";

			if (optSubvol)
				msg += "Option|Subvol|" + subvolName.Length + "|" + subvolName + "\n";

			if (optSubvolID)
				msg += "Option|SubvolID|" + subvolID.ToString() + "\n";

			if (optDump)
				msg += "Option|Dump|" + dumpFile.Length + "|" + dumpFile + "\n";

			if (optTestRun)
				msg += "Option|TestRun\n";

			msg += "MountPoint|" + mountPoint.Length + "|" + mountPoint + "\n";

			foreach (string device in devices)
				msg += "Device|" + device.Length + "|" + device + "\n";

			SendMessage(msg);
		}

		static void SendMessage(string msg)
		{
			var pipeClient = new NamedPipeClientStream(".", "WinBtrfsService", PipeDirection.InOut, PipeOptions.None);

			try
			{
				pipeClient.Connect(1000);
			}
			catch (TimeoutException)
			{
				Error("Could not communicate with WinBtrfsService (timed out).", 2);
			}
			catch
			{
				Error("Could not communicate with WinBtrfsService.", 2);
			}

			if (!pipeClient.IsConnected)
				Error("Lost connection to WinBtrfsService.", 3);

			byte[] msgBytes = System.Text.Encoding.Unicode.GetBytes(msg);
			pipeClient.Write(msgBytes, 0, msgBytes.Length);

			byte[] reply = new byte[102400];
			int replyLen = pipeClient.Read(reply, 0, reply.Length);

			Console.Write("Reply: " + System.Text.Encoding.Unicode.GetString(reply, 0, replyLen));
		}

		static void Error(string error, int exitCode)
		{
			Console.Error.Write(error + "\n");

			Environment.Exit(exitCode);
		}

		static void UsageError(string error)
		{
			Console.Error.Write(error);

			Usage();
		}

		static void Usage()
		{
			Console.Error.Write("\n\nUsage: WinBtrfsCLI.exe <command> [parameters]\n\n" +
				"Commands: mount, [more to be added]\n\n" +
				"WinBtrfsCLI.exe mount [options] <mount point> <device> [<device> ...]\n" +
				"Options:\n" +
				"--subvol=<name>  mount the subvolume with the given name (case-sensitive)\n" +
				"--subvol-id=<id> mount the subvolume with the given ID\n" +
				"--dump=<file>    dump filesystem trees to the given file\n" +
				"--test-run       stop just short of actually mounting the volume\n");

			Environment.Exit(1);
		}
	}
}
