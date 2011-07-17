WinBtrfs — Readme/FAQ

Last updated 2011.07.16


NOTE: WinBtrfs is in NO WAY a finished product and is NOT ready for general use. At this time, it should only really be used by people interested in testing, experimentation, or development. See [03] below for details on which features have been implemented.


[00] What is WinBtrfs?

WinBtrfs is a Windows driver for the Btrfs filesystem. Specifically, it is a read-only filesystem implementation in userspace.


[01] What is Btrfs?

Btrfs is a new Linux filesystem designed by Chris Mason that has been under development by Oracle and the Linux development community since 2007. Licensed under the GNU General Public License (GPL), it appears poised to become Linux's primary filesystem in the near future, providing such modern features as 64-bit addressing, copy-on-write snapshots, subvolumes, fliesystem-level mirroring and striping, and transparent compression. Btrfs is often compared to ZFS, another recent filesystem developed by Oracle, and in many respects they are similar, including their copy-on-write and multi-device capabilities.


[02] Why WinBtrfs?

Justin Gottula began development on WinBtrfs in May 2011 as a result of the observation that, despite Btrfs having been in the Linux kernel for a long time and the on-disk format having been similarly finalized for a decent amount of time, there was no way to mount a Btrfs volume in Windows, read-only or otherwise. This is especially disadvantageous for Linux users who want to dual-boot Windows and still access their Linux data from the Windows side. So, the WinBtrfs project was born out of the desire to reduce the cost of adopting Btrfs for individuals with the need to access their Linux filesystems from Windows.


[03] In its current version, what can WinBtrfs do? What can't it do?

As of 2011.07.16, the following features are supported:
— Mouting volumes via the command line
— Mounting subvolumes by name or ID
— Reading Btrfs volumes converted from ext4
— Drive properties
— Directory listings
— File information
— Reading file contents
— Multi-drive volumes (limited support; you will not be able to read from raid0/raid10 chunks)
— Compressed files (both zlib and lzo)

These features are NOT supported yet:
— Symlinks
— File locking
— Extended attributes
— Mounting volumes via a GUI
— Automounting volumes at startup
— Controlling volumes from a central interface
— User-friendly error messages
— Easy installation
— Binaries (you have to compile it yourself)


[04] What dependencies are required for WinBtrfs?

Building WinBtrfs requires:
— Microsoft Visual Studio 2010 (for greatest ease)
— Dokan 0.6.0 or later [http://dokan-dev.net/en/]
— The Boost C++ libraries [http://www.boost.org/]
The zlib and minilzo libraries are distributed with WinBtrfs.

Running WinBtrfs requires:
— Microsoft Visual C++ 2010 runtime [http://www.microsoft.com/downloads/en/details.aspx?FamilyID=a7b7a05e-6de6-4d3a-a423-37bf0912db84]
— Dokan 0.6.0 or later [http://dokan-dev.net/en/]


[05] How do I build WinBtrfs?

See [04] for information on the software and libraries required to build WinBtrfs. If you are using Visual Studio, load up the solution file and hit Build. Provided you have fulfilled the dependencies, it should build successfully. You will probably need to adjust the WinBtrfsLib project settings so that the include directories and linker inputs are pointed toward Dokan's and Boost's headers and libs.
If you are using a non-Microsoft compiler to build WinBtrfs, you are pretty much on your own at this point, until the developer finds the time to try out other compilers himself. There isn't a Makefile prepared, but no atypical compiler options should be necessary at this time. You will probably have to fix several instances where the developer has relied on Microsoft-specific language extensions.


[06] How do I use WinBtrfs?

Run WinBtrfsCLI.exe without any options to see usage information. Essentially, run the command in this manner:

WinBtrfsCLI.exe [options] <mount point> <device(s)>

where <device> is either the path to an image file, or a physical partition in the format \\.\HarddiskAPartitionB, where A is the hard drive (numbered from zero) and B is the partition within that drive (numbered from one); and where <mount point> is either a drive letter (e.g. "C:") or an empty directory on an NTFS drive. To figure out what the physical disk and partition numbers are for your system, open the Disk Management console in Windows and take a look at how the disks and their partitions are numbered; use these numbers when invoking WinBtrfsCLI. For Linux users, it may be helpful to know that /dev/sda1 is equivalent to \\.\Harddisk0Partition1; /dev/sdb2 is equivalent to \\.\Harddisk1Partition2; and so forth (disks are indexed from zero, and partitions are indexed from one). For multi-drive volumes, you may specify multiple devices, one after the other. These steps will become much easier in the future, once a GUI is implemented.


[07] What does the project roadmap look like?

The following major features will be developed in approximately the order specified:
— Multi-drive volumes
— Windows service
— GUI
— Installer
— Multithreading
— Speed

Lower priority objectives:
— Non-Microsoft compiler support (MinGW, Cygwin)
— UI Translations
— Old OS support


[08] Is read-write support coming in a future version?

The short answer is: no, probably not. This is for several reasons. The most significant reason is that developing a read-write implementation of Btrfs would require orders of magnitude more work than a mere read-only one; it is simply much, much easier to read the data structures from a partition that the official Btrfs driver has created than to incorporate the logic required to create, manage, AND read them in the same manner as the official driver. Essentially then, a full write-enabled driver would require porting the _entire_ Btrfs driver from the Linux kernel to Windows, which would require duplicating/reimplementing the parts of the Linux kernel upon which the driver developers built. Also, all of the userspace utilities (mkfs, btrfsctl, and others) would have to be ported and maintained as well! And, a read-write filesystem driver has the potential to destroy the _entire_ volume and all the data contained within should a serious bug crop up. So, in summary, it is both far more feasible to write a read-only driver; it means significantly less support/bug load on the developer; and there is far less potential for problems to result in the partial or total corruption of all of your data. If you are really want a read-write implementation, write one yourself—or better yet, fork WinBtrfs to get a head start.


[09] Under what licensing terms is WinBtrfs distributed?

WinBtrfs is licensed under the GNU General Public License version 2 (GPLv2), which permits you to redistribute and modify the software as you see fit, so long as you conform to the terms of the GPL. For more information, see http://www.gnu.org/licenses/gpl-2.0.html.


[0A] Can I contribute to WinBtrfs?

Absolutely. WinBtrfs is an open source project, which means that submitting bug reports, contributing patches, helping out with development, breaking open the source code to see how it works, and even forking the project to start your own derivative work are all permitted and even encouraged. See [0B] for information on how to get involved.


[0B] Where can I obtain the latest source code?

WinBtrfs is an open source project hosted on GitHub at https://github.com/jgottula/WinBtrfs. This project's page on GitHub will always have the latest source code and releases, as well as the facilities for contributing back to the project and reporting bugs or other issues.
