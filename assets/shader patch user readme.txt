### Installing ###

Shader Patch should be very simple to get up and running. Just run the installer, select where SWBFII is and hit "Install!".
 
The installer does require you to have .NET Framework 4.6 installed but in the majority of cases it already 
will be. If it isn't you can grab it from the link below (as of writing this).

.NET Framework 4.6 - https://www.microsoft.com/en-nz/download/details.aspx?id=48130

If you do not have write access to your game directory the installer may prompt you to
run it as an Administrator; it will however only do this if it has to. It will depend on your OS version, 
game version and your personal configuration. In all cases you can edit the permissions of your
game folder to make sure you have write access as a normal user and it will remove the need for
admin elevation.

### Configuring ###

Shader Patch comes with a number of settings that can be adjusted by the user. See and edit 'shader patch.yml' in 
your game directory for a list and descriptions of them.

game directory for more information.

### Uninstalling ###

A copy of the installer will have been made in your installation directory, running this will let you 
completely remove Shader Patch and automatically restore backup files made during the install process.

To manually uninstall remove the files you copied in and restore your own backups you should've made.

### Manual Install ###

If you want to manually install Shader Patch you just need to copy the contents of this folder
into the GameData directory of your SWBFII installation. Backup any duplicate files first (as of 
v0.6.0 only core.lvl needs to be backed up), when using the installer it will do this for you.

You'll need the Visual Studio 2017 version of the VC++ runtime to use Shader Patch. You can 
find it (relative to this file) at "data\shaderpatch\bin\VCRedist_x86.exe" or you can download it
from Microsoft directly over here https://www.visualstudio.com/downloads/ (scroll down to the 
bottom of the page to find it).
