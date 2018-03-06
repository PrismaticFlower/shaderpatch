using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Windows;

namespace installer
{
   /// <summary>
   /// Interaction logic for App.xaml
   /// </summary>
   public partial class App : Application
   {
      protected override void OnStartup(StartupEventArgs e)
      {
         if (e.Args.Length < 2) return;

         if (e.Args[0] == "-install")
         {
            var installer = new InstallerModel();

            try
            {
               installer.Install(e.Args[1], true);

               Environment.Exit(0);
            }
            catch (Exception)
            {
               Environment.Exit(1);
            }
         }
         else if (e.Args[0] == "-uninstall")
         {
            var uninstaller = new UninstallerModel();

            var parentId = int.Parse(e.Args[1]);

            try
            {
               uninstaller.StartUninstall(true);
               uninstaller.FinishUninstall();

               Environment.Exit(0);
            }
            catch (Exception)
            {
               Environment.Exit(1);
            }
         }
      }
   }
}
