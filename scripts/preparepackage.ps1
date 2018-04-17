# Setup developer environment.
.\scripts\setupdeveloperenvironment.ps1

if (Test-Path .\packaged) {
  Remove-Item -Path .\packaged -Force -Recurse
}

# Create package directory structure.
md .\packaged
md .\packaged\data\
md .\packaged\data\shaderpatch\
md .\packaged\data\shaderpatch\bin\
md .\packaged\data\shaderpatch\textures\
md .\packaged\data\shaderpatch\core\
md .\packaged\data\_lvl_pc\

copy .\LICENSE ".\packaged\shader patch license.txt"
copy .\third_party.md ".\packaged\shader patch acknowledgements.txt"

# Copy over redistributable
$installationPath = vswhere.exe -prerelease -latest -property installationPath

copy "${installationPath}\VC\Redist\MSVC\14.*\VCRedist_x86.exe" ".\packaged\data\shaderpatch\bin\VCRedist_x86.exe"

# Build
msbuild /t:Build /p:Configuration=Release /p:Platform=x86 /m shader_patch.sln

copy .\bin\Release\dinput8.dll .\packaged\
copy ".\bin\Release\shader patch installer.exe" ".\packaged\Shader Patch Installer.exe"
copy ".\bin\Release\Microsoft.Expression.Drawing.dll" .\packaged\

# Copy over end-user tools.
copy ".\bin\Release\lvl_pack.exe" ".\packaged\data\shaderpatch\bin\lvl_pack.exe"
copy ".\bin\Release\material_munge.exe" ".\packaged\data\shaderpatch\bin\material_munge.exe"
copy ".\bin\Release\shader_compiler.exe" ".\packaged\data\shaderpatch\bin\shader_compiler.exe"
copy ".\bin\Release\sp_texture_munge.exe" ".\packaged\data\shaderpatch\bin\sp_texture_munge.exe"

# Copy Assets
copy '.\assets\material_descriptions\' '.\packaged\data\shaderpatch\bin\material_descriptions\'
copy '.\assets\shader patch.ini' .\packaged\
copy '.\assets\shader patch user readme.txt' '.\packaged\shader patch readme.txt'


Copy-Item -Path .\assets\textures\* -Destination .\packaged\data\shaderpatch\textures -Recurse

# Copy core.lvl source files.
Copy-Item -Path .\assets\core\* -Destination .\packaged\data\shaderpatch\core -Recurse

if (Test-Path -Path ".\packaged\data\shaderpatch\shaders\munged") 
{ 
   del ".\packaged\data\shaderpatch\shaders\munged" -Recurse
}

Move-Item -Path .\packaged\data\shaderpatch\core\core.lvl -Destination .\packaged\data\_lvl_pc\core.lvl
