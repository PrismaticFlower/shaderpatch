using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data.OleDb;
using System.IO;
using System.Xml.Serialization;
using System.Diagnostics;
using System.Threading.Tasks;
using Microsoft.VisualBasic.FileIO;
using Microsoft.Win32;

namespace installer
{
   class InstallerModel
   {
      private void CheckWindowsSearchForPaths(ref HashSet<String> installPaths)
      {
         using (var connection = new OleDbConnection(@"Provider=Search.CollatorDSO;Extended Properties=""Application=Windows"""))
         {

            var query = "SELECT \"System.ItemUrl\" FROM \"SystemIndex\" WHERE System.Kind = SOME ARRAY['program'] AND CONTAINS(System.FileName, '\"battlefrontii.exe\"')";

            connection.Open();

            using (var command = new OleDbCommand(query, connection))
            {

               using (var reader = command.ExecuteReader())
               {
                  while (reader.Read())
                  {
                     var path = reader[0].ToString();
                     path = path.Substring(5);
                     path = path.Replace('/', '\\');

                     installPaths.Add(path);
                  }
               }

               connection.Close();
            }
         }
      }

      private void CheckRegistryForPaths(ref HashSet<String> installPaths)
      {
         var path = Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\LucasArts\Star Wars Battlefront II\1.0",
                                      "ExePath",
                                      null);

         if (path != null) installPaths.Add(path.ToString());

         path = Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\LucasArts\Star Wars Battlefront II\1.0",
                                  "ExePath",
                                  null);

         if (path != null) installPaths.Add(path.ToString());
      }
      
      private bool Vc14RuntimeInstalled()
      {
         try
         {
            var systemPath = Environment.SystemDirectory;

            if (Environment.Is64BitOperatingSystem) systemPath = systemPath.Replace("System32", "SysWOW64");

            return File.Exists(Path.Combine(systemPath, "vcruntime140.dll")) && 
               File.Exists(Path.Combine(systemPath, "msvcp140.dll"));
         }
         catch
         {
            return false;
         }
      }

      private Task InstallVc14Runtime()
      {
         return Task.Run(() =>
         {
            var vcInstaller = Process.Start(Path.Combine(Directory.GetCurrentDirectory(), "data/shaderpatch/bin/VCRedist_x86.exe"));

            vcInstaller.WaitForExit();
         });
      }

      public List<String> SearchForInstallPaths()
      {
         HashSet<String> install_paths = new HashSet<String>();

         CheckWindowsSearchForPaths(ref install_paths);
         CheckRegistryForPaths(ref install_paths);

         return install_paths.ToList();
      }
      
      public string BrowseForInstallPath()
      {
         var dialog = new OpenFileDialog
         {
            Filter = "SWBFII Executable|battlefrontii.exe"
         };


         if (dialog.ShowDialog() == true)
         {
            return dialog.FileName;
         }

         return null;
      }
      
      public void Install(string installPath, bool adminInstall = false)
      {
         if (!adminInstall && !PathHelpers.CheckDirectoryAccess(installPath))
         {
            LaunchAdminInstall(installPath);

            return;
         }

         var installInfo = GetInstallInfo(installPath);

         try
         {
            Task runtimeInstall = null;
            
            if (!Vc14RuntimeInstalled()) runtimeInstall = InstallVc14Runtime();

            var fileInstaller = new FileInstaller(installInfo, Environment.CurrentDirectory);
            var dirInfo = new DirectoryInfo("./");
            var files = dirInfo.GetFiles("*", System.IO.SearchOption.AllDirectories);

            foreach (var file in files)
            {
               fileInstaller.InstallFile(file.FullName);
            }

            TidyInstall(installInfo, fileInstaller.UnusedOldFiles);

            XmlSerializer serializer = new XmlSerializer(typeof(InstallInfo));

            using (var stream = File.OpenWrite(Path.Combine(installPath, "data/shaderpatch/install_info.xml")))
            {
               serializer.Serialize(stream, installInfo);
            }

            if (runtimeInstall != null) runtimeInstall.Wait();
         }
         catch (Exception)
         {
            RevertInstall(installInfo);

            throw;
         }
      }
      
      private static void LaunchAdminInstall(string installPath)
      {
         var processInfo = new ProcessStartInfo
         {
            WindowStyle = ProcessWindowStyle.Hidden,
            CreateNoWindow = true,
            UseShellExecute = true,
            Arguments = "-install \"" + installPath.Replace('\\', '/') + "\"",
            Verb = "runas",
            WorkingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().CodeBase),
            FileName = Path.GetFileName(System.Reflection.Assembly.GetExecutingAssembly().CodeBase)
         };

         var adminProcess = Process.Start(processInfo);

         adminProcess.WaitForExit();

         if (adminProcess.ExitCode != 0)
         {
            throw new Exception("Failed to install using admin privileges.");
         }
      }

      private static void RevertInstall(InstallInfo installInfo)
      {
         foreach (var file in installInfo.installedFiles.ToList())
         {
            var filePath = Path.Combine(installInfo.installPath, file.Key);

            File.Delete(filePath);

            if (file.Value)
            {
               var backupFilePath = Path.Combine(installInfo.backupPath, file.Key);

               File.Move(backupFilePath, filePath);
            }

            installInfo.installedFiles.Remove(file.Key);
         }
      }

      private static void TidyInstall(InstallInfo installInfo, Dictionary<string, bool> unusuedFiles)
      {
         foreach (var file in unusuedFiles.ToList())
         {
            var filePath = Path.Combine(installInfo.installPath, file.Key);

            File.Delete(filePath);

            if (file.Value)
            {
               var backupFilePath = Path.Combine(installInfo.backupPath, file.Key);

               File.Move(backupFilePath, filePath);
            }

            if (installInfo.installedFiles.ContainsKey(file.Key))
            {
               installInfo.installedFiles.Remove(file.Key);
            }
         }
      }

      private static InstallInfo GetInstallInfo(string installPath)
      {
         var infoPath = Path.Combine(installPath, "data/shaderpatch/install_info.xml");

         if (!File.Exists(infoPath))
         {
            return new InstallInfo
            {
               installPath = installPath,
               backupPath = Path.Combine(installPath, "./data/backup")
            };
         }
         else
         {
            XmlSerializer serializer = new XmlSerializer(typeof(InstallInfo));

            using (var stream = File.OpenRead(Path.Combine(installPath, "data/shaderpatch/install_info.xml")))
            {
               return serializer.Deserialize(stream) as InstallInfo;
            }
         }
      }
   }
}
