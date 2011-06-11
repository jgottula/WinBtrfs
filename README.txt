WinBtrfs — Readme/FAQ

Last updated 2011.06.10


NOTE: WinBtrfs is NOT EVEN CLOSE to a finished product, and is not yet ready for use by most users. See the project status question below for more details.


What is WinBtrfs?

WinBtrfs is a Windows driver for the Btrfs filesystem. Specifically, it is a read-only filesystem implementation in userspace.


What is Btrfs?

Btrfs is a new Linux filesystem developed by Chris Mason and Oracle Corporation since 2007. Licensed under the GNU General Public License (GPL), it is poised to become Linux's primary filesystem in the near future, providing such modern features as copy-on-write snapshots, subvolumes, RAID-like mirroring and striping, and many others. In many respects it is similar to ZFS, another nascent filesystem.


Why WinBtrfs?

I began development on WinBtrfs in early 2011, when I observed that although Btrfs had been in the Linux kernel for a decent amount of the time and the on-disk format was finalized, there was no way to mount a Btrfs volume in Windows. Taking after the example of several Windows ext2 and ext3 drivers, I decided to write my own read-only userspace implementation of the Btrfs filesystem so that I, as well as others, could make use of our new Btrfs volumes from Windows.


What dependencies are required for WinBtrfs?

Building WinBtrfs requires:
— Microsoft Visual Studio (for greatest ease)
— Dokan 0.6.0 or later [http://dokan-dev.net/en/]
— The Boost C++ libraries [http://www.boost.org/]
Running WinBtrfs requires:
— Microsoft Visual C++ 2010 runtime [http://www.microsoft.com/downloads/en/details.aspx?FamilyID=a7b7a05e-6de6-4d3a-a423-37bf0912db84]
— Dokan 0.6.0 or later [http://dokan-dev.net/en/]


How do I build WinBtrfs?

See the question above for information on the software and libraries required to build WinBtrfs. If you are using Visual Studio, load up the solution file and hit Build. Provided you have fulfilled the dependencies, it should build successfully. You will probably need to adjust the WinBtrfsCLI project settings so that the include directories and linker inputs are pointed toward Dokan's and Boost's headers and libs.
If you are using a non-Microsoft compiler to build WinBtrfs, you are pretty much on your own at this point, until I find time to try out other compilers myself. I don't have a Makefile prepared, but no atypical compiler options should be necessary at this time.


How do I use WinBtrfs?

Run WinBtrfsCLI.exe without any options to see usage information. Essentially, run the command in this manner:

WinBtrfsCLI.exe <device> <mount point>

where <device> is either an image file, or a physical partition in the format \\.\HarddiskAPartitionB, where A is the hard drive (numbered from zero) and B is the partition within that drive (numbered from one); and where <mount point> is either a drive letter (e.g. "C:") or an empty directory on an NTFS drive. To get a handle on physical disk and partition numbers, open the Disk Management console in Windows and take a look at how the disks are numbered. For Linux users, it may be useful to note that /dev/sda1 is equivalent to \\.\Harddisk0Partition1; /dev/sdb2 is equivalent to \\.\Harddisk1Partition2; and so forth. Identifying the proper volume and specifying a mount point will become much easier in the future, once more development time is spent on the user interface aspect of WinBtrfs.


What is the status of the project, and what does the roadmap look like?

WinBtrfs has been in development since May 25, 2011, and many important features have not yet been completed. At the time of this writing, WinBtrfs implements the Dokan callbacks required to display the Drive Properties dialog (that is, the drive label, the FS type, the total size, and the free space) and do basic directory listings (in Windows Explorer or the command prompt). On the other side of the chasm, I have been implementing the low-level functionality required to process the internal structures that make up a Btrfs filesystem. Fortunately, I am quickly approaching the point when the gap between these low-level filesystem operations and the functionality users expect (directory listings, file information, reading files) will be fully bridged. WinBtrfs can currently perform the tasks required to reach the filesystem tree, so it can get basic file and directory information. The next step will be accessing file data, which will entail another leap on the backend (primarly involving being able to read the extent tree). After these steps have been completed, the next priority will be to ensure that the more esoteric aspects of Btrfs filesystems (multi-drive filesystems, compression, subvolumes, ext4-converted volumes, etc.) work properly. Once all of that works properly, it will be time for a proper GUI so that mounting volumes isn't so damn difficult for the average user. And after all that, a proper Windows installer (almost certainly NSIS) will be made available so that Windows noobs won't get confused at what to do to install the thing.


Is read-write support coming in a future version?

The short answer is: no, probably not. This is for several reasons. The most significant reason is that developing a read-write implementation of Btrfs would require considerably more work than a mere read-only one; for analogy, compare merely reading the works of Shakespeare with being able to come up with them and write them yourself. Perhaps that's a bit over the top, but a full write-enabled driver would certainly require porting the _entire_ Btrfs driver from the Linux kernel to Windows, which would additionally require duplicating the parts of the Linux kernel upon which the driver depends. More than this, it would require significantly more support and if problems did develop, a read-write filesystem driver has the potential to destroy the _entire_ volume and all the data contained within. So, in summary, it is both far more feasible to write a read-only driver; it means significantly less support/bug load on the developer; and there is far less potential for problems to result in the partial or total corruption of all of your data.


Under what licensing terms is WinBtrfs distributed?

WinBtrfs is licensed under the GNU General Public License version 2 (GPLv2), which permits you to redistribute and modify the software as you see fit, so long as you conform to the terms of the GPL. For more information, see http://www.gnu.org/licenses/gpl-2.0.html.


Can I contribute to WinBtrfs?

Absolutely. WinBtrfs is an open source project, which means that submitting bug reports, contributing patches, helping out with development, breaking open the source code to see how it works, and even forking the project to start your own derivative work are all permitted and even encouraged. See the question on obtaining the source code below for information on how to get involved.


Where can I obtain the latest source code?

WinBtrfs is an open source project hosted on GitHub at https://github.com/jgottula/WinBtrfs. This project's page on GitHub will always have the latest source code and releases, as well as the facilities for contributing back to the project.
