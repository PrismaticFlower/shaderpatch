using System.IO;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace installer
{
   class FileInstaller
   {
      public FileInstaller(InstallInfo installInfo, string installerDirectory)
      {
         this.installInfo = installInfo;
         this.installerDirectory = installerDirectory;
      }

      public void InstallFile(string sourceFilePath)
      {
         var fileRelativePath = PathHelpers.GetRelativePath(sourceFilePath, installerDirectory);
         var destFilePath = Path.Combine(installInfo.installPath, fileRelativePath);
         var destExists = File.Exists(destFilePath);
         bool backedUp;
         installInfo.installedFiles.TryGetValue(fileRelativePath, out backedUp);
         
         Directory.CreateDirectory(Path.GetDirectoryName(destFilePath));

         if (destExists && !installInfo.installedFiles.ContainsKey(fileRelativePath))
         {
            BackupFile(fileRelativePath);
            backedUp = true;
         }

         File.Copy(sourceFilePath, destFilePath, true);
         installInfo.installedFiles[fileRelativePath] = backedUp;
      }

      public InstallInfo Property { get { return installInfo; } }

      private InstallInfo installInfo;
      private string installerDirectory;

      private void BackupFile(string filePath)
      {
         var srcFilePath = Path.Combine(installInfo.installPath, filePath);
         var destFilePath = Path.Combine(installInfo.backupPath, filePath);

         Directory.CreateDirectory(Path.GetDirectoryName(destFilePath));
         File.Copy(srcFilePath, destFilePath);
      }
   }
}
