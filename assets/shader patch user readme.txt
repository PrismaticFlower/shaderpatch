### Installing ###

Shader Patch should be very simple to get up and running. Download it and run the installer.
 
However you also need to make sure that you have the dependencies installed they are as
follows. 

Microsoft Visual C++ Redistributable for Visual Studio 2017 (x86)
.NET Framework 4.6 (Installer only, you can manually install Shader Patch without this.)

You can find the downloads for each of those at these locations (as of writing this).

VC++ runtime - https://www.visualstudio.com/downloads/
.NET Framework - https://www.microsoft.com/en-nz/download/details.aspx?id=48130

Once you have those you should be able to just run the installer and it'll take care of everything
for you. 

If you do not have write access to your game directory the installer may prompt you to
run it as an Administrator; it will however only do this if it has. It will depends on your OS version, 
game version and your personal configuration. In all cases you can edit the permissions of your
game folder to make sure you have write access as a normal user and it will remove the need for
admin elevation.

If you want to manually install Shader Patch you just need to copy the contents of this folder
into the GameData directory of your SWBFII installation. (Backup any duplicate files first, when using 
the installer it will do this for you.)

### Uninstalling ###

A copy of the installer will have been made in your installation directory, running this will let you 
completely remove Shader Patch and automatically restore backup files made during the install process.

To manually uninstall remove the files you copied in and restore your own backups you should've made.

### Overriding Display Mode ###

Shader Patch can override the display settings used by the game, see and edit 'shader patch.yml' in you
game directory for more information.
