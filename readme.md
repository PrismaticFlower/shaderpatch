# SWBFII Shader Patch
Shader Patch is a mod for SWBFII primarilly focused around two things providing sublte but meaningful, non-intrusive improvements to the game's shaders and exposing more advanced capabilties relating to shaders for modders. 


All polite feedback, bug reports and feature requets are welcomed. You can do so here or if Discord is more your speed you can use it to do so [here](https://discord.gg/x6UQvZb).

## Features Overview
There are more than this but below is a short list of some of the key features Shader Patch currently has.

* [Full Per-Pixel Lighting](http://i.cubeupload.com/Zm18nC.png)
* Improved Normal Mapping
* [Smoother Light Bloom](http://u.cubeupload.com/SleepKiller/bloomcomparison.png)
* [Improved Water Shader](http://u.cubeupload.com/SleepKiller/watercomparison.png)
* [Distortive Shield Shader](http://u.cubeupload.com/SleepKiller/shieldcomparison.png)
* [Generic Custom Materials System](https://github.com/SleepKiller/swbfii-shaderpatch/wiki/Shader-Patch-Materials)
* [Modder Controllable Effects System](https://github.com/SleepKiller/swbfii-shaderpatch/wiki/Shader-Patch-Effects-System)

## Downloading
To grab yourself the latest release simply follow this [link](https://github.com/SleepKiller/swbfii-shaderpatch/releases/latest) and download the **shaderpatch.zip** file. If it's the source code your after there should be button labeled `Clone or Download` somewhere at the top of this page you can use to grab a copy of the lastest source code.

## Building
Before anything else you'll need to install [git](https://git-scm.com/) (and made avalible on `PATH`) and
of course [Visual Studio 2017 Community](https://www.visualstudio.com/downloads/) (you'll need the Desktop C++ 
and Desktop .NET workloads).

Once you've done that all of the projects can dependencies are aquired by running `scripts/bootstrap.ps1`.
Currently by convention all scripts in `scripts/` expect to be ran from the repository's root. So instead of 
going into the `scripts/` and executing them you would start up a console in the repository's root and run
`./scripts/example.ps1`.

After running `scripts/bootstrap.ps1` you'll be good to just start building. You can use `scripts/preparepackage.ps1`
to build a release version of the patch and assemble it into a ready to use or zip package.

### Debugging
When debugging I reccomend editing the output directory of `shader_patch.vcxproj` to point to your game installation
directory and changing the debug command to launch SWBFII. This is the process I use and it works well for me, you just
have to remember to revert the output directory back before making any commits.

