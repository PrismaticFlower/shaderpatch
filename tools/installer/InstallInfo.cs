using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text;
using System.Threading.Tasks;

namespace installer
{
   [Serializable]
   public class InstallInfo
   {
      public string installPath;
      public string backupPath;
      public SerializableDictionary<string, bool> installedFiles = new SerializableDictionary<string, bool>();
   }
}
