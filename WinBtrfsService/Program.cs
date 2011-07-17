using System;
using System.ServiceProcess;

namespace WinBtrfsService
{
	static class Program
	{
		static void Main()
		{
			ServiceBase[] services = new ServiceBase[]
			{
				new Service()
			};
			
			ServiceBase.Run(services);
		}
	}
}
