# Setup developer environment.
.\scripts\setupdeveloperenvironment.ps1

if (Test-Path .\packages) {
  Remove-Item -Path .\packages -Force -Recurse
}

md .\packages

# Package Shader Patch.
md .\packages\shaderpatch
md .\packages\shaderpatch\data\
md .\packages\shaderpatch\data\shaderpatch\
md .\packages\shaderpatch\data\shaderpatch\bin\

copy .\LICENSE ".\packages\shaderpatch\shader patch license.txt"
copy .\third_party.md ".\packages\shaderpatch\shader patch acknowledgements.txt"

# Copy over redistributable
$installationPath = vswhere.exe -prerelease -latest -property installationPath

copy "${installationPath}\VC\Redist\MSVC\14.*\VCRedist_x86.exe" ".\packages\shaderpatch\data\shaderpatch\bin\VCRedist_x86.exe"

# Build
msbuild /t:Build /p:Configuration=Release /p:Platform=x86 /m shader_patch.sln
msbuild /t:Build /p:Configuration=Release /p:Platform=x64 /m shader_patch.sln

copy .\bin\Release\*.dll .\packages\shaderpatch\
copy ".\bin\Release\shader patch installer.exe" ".\packages\shaderpatch\Shader Patch Installer.exe"

# Copy Assets
copy '.\assets\shader patch.yml' .\packages\shaderpatch\
copy '.\assets\shader patch user readme.txt' '.\packages\shaderpatch\shader patch readme.txt'
copy '.\assets\core\*.lvl' .\packages\shaderpatch\data\shaderpatch\

# Package tools.
md .\packages\shaderpatch-x86-tools
md .\packages\shaderpatch-x64-tools

copy '.\assets\rendertype_descriptions\' '.\packages\shaderpatch-x86-tools\rendertype_descriptions\' -Recurse
copy '.\assets\rendertype_descriptions\' '.\packages\shaderpatch-x64-tools\rendertype_descriptions\' -Recurse

copy .\third_party.md ".\packages\shaderpatch-x86-tools\shader patch acknowledgements.txt"
copy .\tools\bin\Win32\Release\*.dll .\packages\shaderpatch-x86-tools\
copy .\tools\bin\Win32\Release\*.exe .\packages\shaderpatch-x86-tools\
del .\packages\shaderpatch-x86-tools\shader_compiler.exe

copy .\third_party.md ".\packages\shaderpatch-x64-tools\shader patch acknowledgements.txt"
copy .\tools\bin\x64\Release\*.dll .\packages\shaderpatch-x64-tools\
copy .\tools\bin\x64\Release\*.exe .\packages\shaderpatch-x64-tools\
del .\packages\shaderpatch-x64-tools\shader_compiler.exe
