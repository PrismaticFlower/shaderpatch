using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Security.AccessControl;
using System.Security.Principal;
using System.IO;

namespace installer
{
   class PathHelpers
   {
      static public string GetRelativePath(string filePath, string directoryPath)
      {
         var stringBuilder = new StringBuilder(260);

         if (!PathRelativePathTo(stringBuilder, 
            directoryPath, Microsoft.VisualBasic.FileAttribute.Directory, 
            filePath, Microsoft.VisualBasic.FileAttribute.Normal))
         {
            throw new ExternalException(string.Format("Failed to get relative path of {0} to {1}!", 
               filePath, directoryPath));
         }

         return stringBuilder.ToString();
      }

      [DllImport("shlwapi.dll", CharSet = CharSet.Auto)]
      static extern bool PathRelativePathTo(
       [Out] StringBuilder pszPath,
       [In] string pszFrom,
       [In] Microsoft.VisualBasic.FileAttribute dwAttrFrom,
       [In] string pszTo,
       [In] Microsoft.VisualBasic.FileAttribute dwAttrTo);

      static public bool CheckDirectoryAccess(string path)
      {
         try
         {
            var currentUser = WindowsIdentity.GetCurrent();
            var rootDirInfo = new DirectoryInfo(path);

            if (!CheckFileSystemAccess(rootDirInfo, currentUser)) return false;

            foreach (var dirInfo in rootDirInfo.GetDirectories("*", SearchOption.AllDirectories))
            {
               if (!CheckFileSystemAccess(dirInfo, currentUser)) return false;
            }
         }
         catch (AccessViolationException)
         {
            return false;
         }

         return true;
      }

      static private bool CheckFileSystemAccess(DirectoryInfo info, WindowsIdentity windowsIdentity)
      {
         DirectorySecurity dirSecurity = info.GetAccessControl(AccessControlSections.Access);

         foreach (FileSystemAccessRule fsar in dirSecurity.GetAccessRules(true, true, typeof(System.Security.Principal.NTAccount)))
         {
            if (fsar.IdentityReference.Value != windowsIdentity.Name) continue;

            const FileSystemRights desired = 
               FileSystemRights.FullControl & ~(FileSystemRights.TakeOwnership | FileSystemRights.ChangePermissions);
            
            if ((fsar.FileSystemRights & desired) != desired) return false;
         }

         return true;
      }
   }
}
