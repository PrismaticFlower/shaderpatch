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

      public void StartUninstall()
      {
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
      
      public void FinishUninstall()
      {
         using (var batch = File.CreateText("~finishShaderPatchUninstall.bat"))
         {
            batch.WriteLine("taskkill /PID {0}", Process.GetCurrentProcess().Id);
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
