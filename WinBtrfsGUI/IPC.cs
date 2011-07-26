using System;
using System.IO.Pipes;
using System.Threading;
using System.Windows;

namespace WinBtrfsGUI
{
	static class IPC
	{
		public static string SendMessage(string msg)
		{
			var pipeClient = new NamedPipeClientStream(".", "WinBtrfsService", PipeDirection.InOut,
				PipeOptions.Asynchronous);

			try
			{
				pipeClient.Connect(1000);
			}
			catch (TimeoutException)
			{
				MessageBox.Show("Could not communicate with WinBtrfsService (timed out).",
					"Communication Error", MessageBoxButton.OK, MessageBoxImage.Exclamation);
				return null;
			}
			catch
			{
				MessageBox.Show("Could not communicate with WinBtrfsService.",
					"Communication Error", MessageBoxButton.OK, MessageBoxImage.Exclamation);
				return null;
			}

			if (!pipeClient.IsConnected)
			{
				MessageBox.Show("Lost the connection to WinBtrfsService. Try again.",
					"Lost Connection", MessageBoxButton.OK, MessageBoxImage.Exclamation);
				return null;
			}

			byte[] msgBytes = System.Text.Encoding.Unicode.GetBytes(msg);
			pipeClient.Write(msgBytes, 0, msgBytes.Length);

			byte[] reply = new byte[102400];
			var eventGotRead = new ManualResetEvent(false);
			var result = pipeClient.BeginRead(reply, 0, reply.Length, _ => eventGotRead.Set(), null);

			if (!eventGotRead.WaitOne(1000))
			{
				MessageBox.Show("Did not receive a reply from WinBtrfsService (timed out).",
					"Communication Error", MessageBoxButton.OK, MessageBoxImage.Exclamation);
				return null;
			}

			int replyLen = pipeClient.EndRead(result);
			string replyStr = System.Text.Encoding.Unicode.GetString(reply, 0, replyLen);

			return replyStr;
		}
	}
}
