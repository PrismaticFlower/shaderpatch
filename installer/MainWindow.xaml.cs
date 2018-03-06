using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;
using System.Threading.Tasks;

namespace installer
{
   /// <summary>
   /// Interaction logic for MainWindow.xaml
   /// </summary>
   public partial class MainWindow : Window
   {
      private InstallerViewModel installerViewModel = new InstallerViewModel();
      private UninstallerViewModel uninstallerViewModel = new UninstallerViewModel();

      public MainWindow()
      {
         InitializeComponent();

         if (uninstallerViewModel.Uninstallable)
         {
            installGrid.Visibility = Visibility.Hidden;
            installGrid.IsEnabled = false;

            Width = 256;
            Height = 192;
         }
         else
         {
            uninstallGrid.Visibility = Visibility.Hidden;
            uninstallGrid.IsEnabled = false;

            installerViewModel.SearchForInstalls();
            gameDirList.ItemsSource = installerViewModel.PossibleGameLocations;
         }
      }

      private async void OnInstallClicked(object sender, RoutedEventArgs e)
      {
         try
         {
            var path = gameDirList.SelectedItem.ToString();

            installingMask.IsEnabled = true;
            installingMask.Visibility = Visibility.Visible;

            await Task.Run(() =>
            {
               installerViewModel.Install(path);
            });
         }
         catch (Exception exception)
         {
            installFailedMask.IsEnabled = true;
            installFailedMask.Visibility = Visibility.Visible;

            installFailedMessage.Text = string.Format("Failed to install Shader Patch. Exception message was \"{0}\".", exception.Message);
         }
         finally
         {
            installingMask.IsEnabled = false;
            installingMask.Visibility = Visibility.Hidden;
         }


         installedMask.IsEnabled = true;
         installedMask.Visibility = Visibility.Visible;
      }

      private async void OnUninstallClick(object sender, RoutedEventArgs e)
      {
         uninstallingMask.IsEnabled = true;
         uninstallingMask.Visibility = Visibility.Visible;

         await Task.Run(() =>
         {
            uninstallerViewModel.StartUninstall();
         });

         uninstallerViewModel.FinishUninstall();
      }

      private void OnBrowsePressed(object sender, RoutedEventArgs e)
      {
         installerViewModel.BrowseForInstallPath();
         gameDirList.ItemsSource = null;
         gameDirList.ItemsSource = installerViewModel.PossibleGameLocations;
      }
   }
}
