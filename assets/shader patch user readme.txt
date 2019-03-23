### Installing ###

Shader Patch should be very simple to get up and running. You can either run the installer or simply copy the contents of this folder
into your `GameData` folder.
 
The installer does require you to have .NET Framework 4.6 installed but in the majority of cases it already 
will be. If it isn't you can grab it from the link below (as of writing this).

.NET Framework 4.6 - https://www.microsoft.com/en-nz/download/details.aspx?id=48130

If you do not have write access to your game directory the installer may prompt you to
run it as an Administrator; it will however only do this if it has to. It will depend on your OS version, 
game version and your personal configuration. In all cases you can edit the permissions of your
game folder to make sure you have write access as a normal user and it will remove the need for
admin elevation.

### Configuring ###

Shader Patch comes with a number of settings that can be adjusted by the user. See and edit the config file 'shader patch.yml' in 
your game directory for a list and descriptions of them. Checking it out before using Shader Patch is highly recommended.

### Uninstalling ###

If you used the bundled installer then a copy of the installer will have been made in your installation directory, running this 
will let you completely remove Shader Patch and automatically restore backup files made during the install process.

However if you did not run the installer then you must manually delete the files you copied over. Below is a list of files and folders
you'll need to delete. (Relative to "GameData".)

   ./d3d9.dll
   ./Microsoft.Expression.Drawing.dll
   ./shader patch acknowledgements.txt
   ./Shader Patch Installer.exe
   ./shader patch license.txt
   ./shader patch readme.txt
   ./shader patch.yml
   ./yaml-cpp.dll
   ./data/shaderpatch
