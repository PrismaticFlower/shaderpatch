using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Microsoft.VisualBasic.FileIO;
using System.Xml.Serialization;
using System.Threading.Tasks;
using System.Diagnostics;

namespace installer
{
   class UninstallerModel
   {
      private List<string> deferredFiles = new List<string>();

      public bool Uninstallable { get; private set; }

      public UninstallerModel()
      {
         Uninstallable = File.Exists("battlefrontii.exe");
      }

      public void StartUninstall(bool adminUninstall = false)
      {
         if (File.Exists("./data/shaderpatch/admin install.txt") && !adminUninstall)
         {
            var processInfo = new ProcessStartInfo
            {
               WindowStyle = ProcessWindowStyle.Hidden,
               CreateNoWindow = true,
               UseShellExecute = true,
               Arguments = "-uninstall " + Process.GetCurrentProcess().Id.ToString(),
               Verb = "runas",
               WorkingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().CodeBase),
               FileName = Path.GetFileName(System.Reflection.Assembly.GetExecutingAssembly().CodeBase)
            };

            Process.Start(processInfo);
            Environment.Exit(0);
         }

         List<string> files;
         
         XmlSerializer serializer = new XmlSerializer(typeof(List<string>));

         using (var stream = File.OpenRead("./data/shaderpatch/file_manifest.xml"))
         {
            files = serializer.Deserialize(stream) as List<string>;
         }
         
         foreach (var path in files)
         {
            try
            {
               File.Delete(path);
            }
            catch (Exception)
            {
               deferredFiles.Add(path);
            }
         }

         FileSystem.MoveFile("./data/shaderpatch/backup/core.lvl", "./data/_lvl_pc/core.lvl", true);

         Directory.Delete("./data/shaderpatch", true);
      }
      
      public void FinishUninstall(int? parentProcessId = null)
      {
         using (var batch = File.CreateText("~finishShaderPatchUninstall.bat"))
         {
            batch.WriteLine("taskkill /PID {0}", Process.GetCurrentProcess().Id);
            if (parentProcessId != null) batch.WriteLine("taskkill /PID {0}", parentProcessId);

            batch.WriteLine("timeout /T 1 /NOBREAK");

            foreach (var path in deferredFiles)
            {
               batch.WriteLine("del \"{0}\"", path);
            }

            batch.WriteLine("del ~finishShaderPatchUninstall.bat");
         }
         
         Process.Start(new ProcessStartInfo() {
            WindowStyle = ProcessWindowStyle.Hidden,
            CreateNoWindow = true,
            FileName = "~finishShaderPatchUninstall.bat"});
      }
   }
}
