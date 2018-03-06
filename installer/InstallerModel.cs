using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data.OleDb;
using System.IO;
using System.Xml.Serialization;
using System.Diagnostics;
using Microsoft.VisualBasic.FileIO;
using Microsoft.Win32;

namespace installer
{
   class InstallerModel
   {
      private void CheckWindowsSearchForPaths(ref HashSet<String> install_paths)
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

                     install_paths.Add(path);
                  }
               }

               connection.Close();
            }
         }
      }

      private void CheckRegistryForPaths(ref HashSet<String> install_paths)
      {
         var path = Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\LucasArts\Star Wars Battlefront II\1.0",
                                      "ExePath",
                                      null);

         if (path != null) install_paths.Add(path.ToString());

         path = Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\LucasArts\Star Wars Battlefront II\1.0",
                                  "ExePath",
                                  null);

         if (path != null) install_paths.Add(path.ToString());
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
      
      public void Install(string path, bool adminInstall = false)
      {
         try
         {
            Directory.CreateDirectory(Path.Combine(path, "data/shaderpatch/backup/"));

            if (!File.Exists(Path.Combine(path, "data/shaderpatch/backup/core.lvl")))
            {
               File.Move(Path.Combine(path, "data/_lvl_pc/core.lvl"),
                         Path.Combine(path, "data/shaderpatch/backup/core.lvl"));
            }

            FileSystem.CopyDirectory("./", path, true);

            List<string> files = new List<string>();

            files.Add("./data/shaderpatch/file_manifest.xml");

            foreach (string file in Directory.GetFiles("./", "*", System.IO.SearchOption.AllDirectories))
               files.Add(file);
            
            if (adminInstall)
            {
               File.WriteAllText(Path.Combine(path, "data/shaderpatch/admin install.txt"),
                                 "# Do not delete this file! #");
               files.Add("./data/shaderpatch/admin install.txt");
            }

            XmlSerializer serializer = new XmlSerializer(typeof(List<string>));

            using (var stream = File.OpenWrite(Path.Combine(path, "data/shaderpatch/file_manifest.xml")))
            {
               serializer.Serialize(stream, files);
            }
         }
         catch (UnauthorizedAccessException)
         {
            if (adminInstall) throw;

            var processInfo = new ProcessStartInfo
            {
               WindowStyle = ProcessWindowStyle.Hidden,
               CreateNoWindow = true,
               UseShellExecute = true,
               Arguments = "-install \"" + path.Replace('\\', '/') + "\"",
               Verb = "runas",
               WorkingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().CodeBase),
               FileName = Path.GetFileName(System.Reflection.Assembly.GetExecutingAssembly().CodeBase)
            };
            
            var adminProcess =  Process.Start(processInfo);

            adminProcess.WaitForExit();

            if (adminProcess.ExitCode != 0) throw;
         }

      }
   }
}
