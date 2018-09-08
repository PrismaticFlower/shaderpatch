using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml.Serialization;
using System.Threading.Tasks;
using System.Diagnostics;

namespace installer
{
   class UninstallerModel
   {
      private InstallInfo installInfo;

      private const string installInfoPath = "./data/shaderpatch/install_info.xml";

      public bool Uninstallable { get; private set; }

      public UninstallerModel()
      {
         Uninstallable = File.Exists("./data/shaderpatch/install_info.xml");
      }

      public void StartUninstall(bool adminUninstall = false)
      {
         XmlSerializer serializer = new XmlSerializer(typeof(InstallInfo));

         using (var stream = File.OpenRead(installInfoPath))
         {
            installInfo = serializer.Deserialize(stream) as InstallInfo;
         }

         if (!adminUninstall && !PathHelpers.CheckDirectoryAccess(installInfo.installPath))
         {
               StartAdminUninstall();

               Environment.Exit(0);
         }

         foreach (var file in installInfo.installedFiles.ToList())
         {
            try
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
            catch (Exception)
            {
            }
         }
         
         File.Delete(installInfoPath);
      }

      private static Process StartAdminUninstall()
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

         return Process.Start(processInfo);
      }

      public void FinishUninstall(int? parentProcessId = null)
      {
         using (var batch = File.CreateText("~finishShaderPatchUninstall.bat"))
         {
            batch.WriteLine("taskkill /PID {0}", Process.GetCurrentProcess().Id);
            if (parentProcessId != null) batch.WriteLine("taskkill /PID {0}", parentProcessId);

            batch.WriteLine("timeout /T 1 /NOBREAK");

            foreach (var file in installInfo.installedFiles.ToList())
            {
               var filePath = Path.Combine(installInfo.installPath, file.Key);

               if (file.Value)
               {
                  var backupPath = Path.Combine(installInfo.installPath, file.Key);

                  batch.WriteLine("move /Y \"{0}\" \"{1}\"", backupPath, Path.GetDirectoryName(filePath));
               }
               else
               {
                  batch.WriteLine("del \"{0}\"", filePath);
               }
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
