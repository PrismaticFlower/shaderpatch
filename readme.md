## SWBFII Shader Patch
This project is aimed at enabling the writing of more advanced shaders for Star Wars Battlefront II (2005). It does
things like expose additional shader constants, access to additional textures or performing other actions at key points
during the game's rendering process.

This repository is a counterpart to [shaderpatch](https://github.com/SleepKiller/swbfii_shader_toolkit/tree/shaderpatch)
branch of the swbfii_shader_toolkit repository.

### Building
Before anything else you'll need to install [git](https://git-scm.com/) (and made avalible on `PATH`) and
of course [Visual Studio 2017 Community](https://www.visualstudio.com/downloads/) (you'll need the Desktop C++ 
and Desktop .NET workloads).

Once you've done that all of the projects can dependencies are aquired by running `scripts/bootstrap.ps1`.
Currently by convention all scripts in `scripts/` expect to be ran from the repository's root. So instead of 
going into the `scripts/` and executing them you would start up a console in the repository's root and run
`./scripts/example.ps1`.

After running `scripts/bootstrap.ps1` you'll be good to just start building. You can use `scripts/preparepackage.ps1`
to build a release version of the patch and assemble it into a ready to use or zip package. However `core.lvl` must
manually be copied over from the [shader toolkit](https://github.com/SleepKiller/swbfii_shader_toolkit/tree/shaderpatch)
into `packaged/data/_lvl_pc` for the package to be complete.

#### Debugging
When debugging I reccomend editing the output directory of `shader_patch.vcxproj` to point to your game installation
directory and changing the debug command to launch SWBFII. This is the process I use and it works well for me, you just
have to remember to revert the output directory back before making any commits.

