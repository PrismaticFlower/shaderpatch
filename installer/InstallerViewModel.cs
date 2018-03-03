using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace installer
{
   class InstallerViewModel
   {
      private List<string> possibleGameLocations = new List<string>();

      private InstallerModel installerModel = new InstallerModel();

      public  List<string> PossibleGameLocations
      {
         get
         {
            return possibleGameLocations;
         }
      }

      public List<string> SearchForInstalls()
      {
         return installerModel.SearchForInstallPaths();
      }

      public void Install(string path)
      {
         installerModel.Install(path);
      }
   }
}
