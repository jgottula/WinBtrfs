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
			listViewVolumes.Items.Clear();


		}

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
			buttonRefresh_Click(sender, e);
		}
    }
}
