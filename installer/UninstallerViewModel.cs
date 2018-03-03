using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace installer
{
   class UninstallerViewModel
   {
      private UninstallerModel uninstallerModel = new UninstallerModel();
      
      public bool Uninstallable { get; private set; }

      public UninstallerViewModel()
      {
         Uninstallable = uninstallerModel.Uninstallable;
      }

      public void StartUninstall()
      {
         uninstallerModel.StartUninstall();
      }

      public void FinishUninstall()
      {
         uninstallerModel.FinishUninstall();
      }
   }
}