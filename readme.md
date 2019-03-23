# Shader Patch
Shader Patch is a mod for SWBFII (2005) primarilly focused around two things providing sublte but meaningful, non-intrusive improvements to the game's shaders and exposing more advanced capabilties relating to shaders for modders. 


All polite feedback, bug reports and feature requets are welcomed. You can do so here or if Discord is more your speed you can use it to do so [here](https://discord.gg/x6UQvZb).

## Features Overview
There are more than this but below is a short list of some of the key features Shader Patch currently has.

* [Full Per-Pixel Lighting](http://i.cubeupload.com/Zm18nC.png)
* Improved Normal Mapping
* [Improved Water Shader](http://u.cubeupload.com/SleepKiller/watercomparison.png)
* [Custom Materials System](https://github.com/SleepKiller/swbfii-shaderpatch/wiki/Shader-Patch-Materials)
* [Modder Controllable Effects System](https://github.com/SleepKiller/swbfii-shaderpatch/wiki/Shader-Patch-Effects-System)

## Downloading
To grab yourself the latest release simply follow this [link](https://github.com/SleepKiller/swbfii-shaderpatch/releases/latest) and download the **shaderpatch.zip** file. If it's the source code your after there should be button labeled `Clone or Download` somewhere at the top of this page you can use to grab a copy of the lastest source code.

## Building
Before anything else you'll need to install [Visual Studio 2017 Community](https://www.visualstudio.com/downloads/) with the Desktop C++ 
and Desktop .NET workloads.

The project has several external dependencies, all of them are obtained through [vcpkg](https://github.com/microsoft/vcpkg). `scripts/install_vcpkgs.ps1` has a
listing of all the dependencies.

Once building you can use `scripts/preparepackages.ps1` to create ready to zip packages of Shader Patch and it's tools.

### Debugging
When debugging I reccomend editing the output directory of `shader_patch.vcxproj` to point to your game installation
directory and changing the debug command to launch SWBFII. This is the process I use and it works well for me, you just
have to remember to revert the output directory back before making any commits.

