using System;
using System.IO;
using System.IO.Pipes;
using System.Windows;

namespace WinBtrfsGUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

		private void buttonRefresh_Click(object sender, RoutedEventArgs e)
		{
			Refresh();
		}

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
			Refresh();
		}

		private void Refresh()
		{
			listViewVolumes.Items.Clear();

			string reply = IPC.SendMessage("List");

			if (reply != null)
			{
				string[] lines = reply.Split('\n');

				if (lines[0] == "Data")
					ProcessList(lines);
				else if (lines[0] == "Error")
					MessageBox.Show("WinBtrfsService returned an error:\n" +
						reply.Substring(reply.IndexOf('\n')), "Query Error",
						MessageBoxButton.OK, MessageBoxImage.Exclamation);
				else
					MessageBox.Show("WinBtrfsService replied with something unintelligible. " +
						"(This shouldn't happen!)", "Query Error Extraordinaire",
						MessageBoxButton.OK, MessageBoxImage.Error);
			}
		}

		private void ProcessList(string[] lines)
		{
			VolumeListData entry = null;
			bool first = true, ex = false;

			for (int i = 1; i < lines.Length; i++)
			{
				try
				{
					if (lines[i] == "Entry")
					{
						if (!first)
							listViewVolumes.Items.Add(entry);

						first = false;

						entry = new VolumeListData();
					}
					else if (lines[i].Substring(0, 7) == "fsUUID|")
						entry.UUID = Guid.Parse(lines[i].Substring(7));
					else if (lines[i].Substring(0, 6) == "label|")
					{
						int valLen = int.Parse(lines[i].Substring(6, lines[i].IndexOf('|', 6) - 6));
						string valStr = lines[i].Substring(lines[i].IndexOf('|', 6) + 1, valLen);

						entry.Label = valStr;
					}
					else if (lines[i].Substring(0, 21) == "mountData|mountPoint|")
					{
						int valLen = int.Parse(lines[i].Substring(21, lines[i].IndexOf('|', 21) - 21));
						string valStr = lines[i].Substring(lines[i].IndexOf('|', 21) + 1, valLen);

						entry.MountPoint = valStr;
					}
					else if (lines[i].Substring(0, 18) == "mountData|devices|")
					{
						int valLen = int.Parse(lines[i].Substring(18, lines[i].IndexOf('|', 18) - 18));
						string valStr = lines[i].Substring(lines[i].IndexOf('|', 18) + 1, valLen);

						if (entry.Devices.Length != 0)
							entry.Devices += " ";

						entry.Devices += valStr;
					}
				}
				catch (Exception) // try to be lax about data errors
				{
					ex = true;
				}
			}

			if (ex)
				MessageBox.Show("Encountered an exception during data parsing.", "Parse Error",
					MessageBoxButton.OK, MessageBoxImage.Exclamation);
		}
    }

	public class VolumeListData
	{
		public string MountPoint { get; set; }
		public string Label { get; set; }
		public Guid UUID { get; set; }
		public string Options { get; set; }
		public string Devices { get; set; }

		public VolumeListData()
		{
			MountPoint = "";
			Label = "";
			Options = "";
			Devices = "";
		}
	}
}
