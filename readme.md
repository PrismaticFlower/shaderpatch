# Shader Patch
Shader Patch is a mod for SWBFII (2005) primarilly focused around two things providing sublte but meaningful, non-intrusive improvements to the game's shaders and exposing more advanced capabilties relating to shaders for modders. 


All polite feedback, bug reports and feature requets are welcomed. You can do so here or if Discord is more your speed you can use it to do so [here](https://discord.gg/x6UQvZb).

## Features Overview
There are more than this but below is a short list of some of the key features Shader Patch currently has.

#### Core Features
* All lighting is done per-pixel.
* An Order-Independent Transparency approximation for supported GPUs.
* An optional alternative post processing path for the game that (unlike the vanilla game) gracefully handles high resolutions. 
  - An example of what this fixes being the "blue" water overlay bug. 
* An edited water shader that adds distortion of the scene beneath the water and smoother animation. 
  - This is about the only "breaking change" made by the mod, as maps that have edited their water normal maps may get incorrect results under Shader Patch. (Ideas for how to fix this and bring it inline with Shader Patch's "non-intrusive" goal/requirement are welcome.)
* Optional depth-masking for refractions. 
  - Disabled by default for performance, needs to be enabled by changing `Refraction Quality` in the user config.
* The game is drawn using D3D11 and makes use of Windows 10's support for variable refresh rate displays.

#### Modder Features
* Greater customizability of model materials through a [Custom Materials](https://github.com/SleepKiller/swbfii-shaderpatch/wiki/Shader-Patch-Materials) system.
* Access to and configuration of a completely new post processing pipeline on a per map or per mode basis. Featuring things like color grading, a more customizable and scalable bloom effect and more. So dubbed the [Effects System](https://github.com/SleepKiller/swbfii-shaderpatch/wiki/Shader-Patch-Effects-System).
* [Color Grading Regions](https://github.com/SleepKiller/shaderpatch/wiki/Shader-Patch-Color-Grading-Regions) for fine tuned control over post processing within maps.
* An ingame `BFront2.log` monitor. I know, not related to shaders at all but people asked for it and it's useful to know about.

## Downloading
To grab yourself the latest release simply follow this [link](https://github.com/SleepKiller/swbfii-shaderpatch/releases/latest) and download the **shaderpatch.zip** file. If it's the source code your after there should be button labeled `Clone or Download` somewhere at the top of this page you can use to grab a copy of the lastest source code.

## Building
Before anything else you'll need to install [Visual Studio 2019 Community](https://www.visualstudio.com/downloads/) with the Desktop C++ 
and Desktop .NET workloads.

The project has several external dependencies, all of them are obtained through [vcpkg](https://github.com/microsoft/vcpkg). `scripts/install_vcpkgs.ps1` has a
listing of all the dependencies.

Once building you can use `scripts/preparepackages.ps1` to create ready to zip packages of Shader Patch and it's tools.

### Debugging
When debugging I reccomend editing the output directory of `shader_patch.vcxproj` to point to your game installation
directory and changing the debug command to launch SWBFII. This is the process I use and it works well for me, you just
have to remember to revert the output directory back before making any commits.

