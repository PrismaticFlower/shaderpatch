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
         UnusedOldFiles = new Dictionary<string, bool>(installInfo.installedFiles);
      }

      public void InstallFile(string sourceFilePath)
      {
         var fileRelativePath = PathHelpers.GetRelativePath(sourceFilePath, installerDirectory);
         var destFilePath = Path.Combine(installInfo.installPath, fileRelativePath);
         var destExists = File.Exists(destFilePath);
         var preexisting = UnusedOldFiles.ContainsKey(fileRelativePath);

         installInfo.installedFiles.TryGetValue(fileRelativePath, out var backedUp);

         Directory.CreateDirectory(Path.GetDirectoryName(destFilePath));

         if (destExists && !preexisting)
         {
            BackupFile(fileRelativePath);
            backedUp = true;
         }
         
         File.Copy(sourceFilePath, destFilePath, true);
         installInfo.installedFiles[fileRelativePath] = backedUp;

         if (preexisting)
         {
            UnusedOldFiles.Remove(fileRelativePath);
         }
      }

      public Dictionary<string, bool> UnusedOldFiles { get; private set; }

      private InstallInfo installInfo;
      private readonly string installerDirectory;

      private void BackupFile(string filePath)
      {
         var srcFilePath = Path.Combine(installInfo.installPath, filePath);
         var destFilePath = Path.Combine(installInfo.backupPath, filePath);

         Directory.CreateDirectory(Path.GetDirectoryName(destFilePath));
         File.Copy(srcFilePath, destFilePath);
      }
   }
}
