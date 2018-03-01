﻿
if (Test-Path .\packaged) {
  Remove-Item -Path .\packaged -Force -Recurse
}

# Create packaged folder
md .\packaged

copy .\LICENSE ".\packaged\shader patch license.txt"
copy .\third_party.md ".\packaged\shader patch acknowledgements.txt"

# Find and import Developer Command Prompt environment.
$env:path += ";${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\"

$installationPath = vswhere.exe -prerelease -latest -property installationPath

if ($installationPath -and (test-path "$installationPath\Common7\Tools\vsdevcmd.bat")) {
  & "${env:COMSPEC}" /s /c "`"$installationPath\Common7\Tools\vsdevcmd.bat`" -no_logo && set" | foreach-object {
    $name, $value = $_ -split '=', 2
    set-content env:\"$name" $value
  }
}

# Build
msbuild /t:Build /p:Configuration=Release /m shader_patch.sln

copy .\bin\Release\dinput8.dll .\packaged\

# Copy Assets
copy '.\assets\shader patch.ini' .\packaged\

md .\packaged\data\
md .\packaged\data\shaderpatch\
md .\packaged\data\shaderpatch\textures

Copy-Item -Path .\assets\textures\* -Destination .\packaged\data\shaderpatch\textures -Recurse

md .\packaged\data\_lvl_pc

Write-Host "shader patch packaged prepared, remember to copy core.lvl over from the shader toolkit!"
