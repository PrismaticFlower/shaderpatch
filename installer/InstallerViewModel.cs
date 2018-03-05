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

      public List<string> PossibleGameLocations
      {
         get
         {
            return possibleGameLocations;
         }
      }

      public void SearchForInstalls()
      {
         possibleGameLocations = installerModel.SearchForInstallPaths();
      }

      public void BrowseForInstallPath()
      {
         var path = installerModel.BrowseForInstallPath();

         if (path != null)
         {
            possibleGameLocations.Add(path);
         }
      }

      public void Install(string path)
      {
         installerModel.Install(path);
      }
   }
}
