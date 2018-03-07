if (Test-Path .\packaged) {
  Remove-Item -Path .\packaged -Force -Recurse
}

# Create package directory structure.
md .\packaged
md .\packaged\data\
md .\packaged\data\shaderpatch\
md .\packaged\data\shaderpatch\bin\
md .\packaged\data\shaderpatch\textures

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

# Copy over redistributable
copy "${installationPath}\VC\Redist\MSVC\14.*\VCRedist_x86.exe" ".\packaged\data\shaderpatch\bin\VCRedist_x86.exe"

# Build
msbuild /t:Build /p:Configuration=Release /m shader_patch.vcxproj
msbuild /t:Build /p:Configuration=Release /m tools/installer/installer.csproj

copy .\bin\Release\dinput8.dll .\packaged\
copy ".\bin\Release\shader patch installer.exe" .\packaged\
copy ".\bin\Release\Microsoft.Expression.Drawing.dll" .\packaged\

# Copy Assets
copy '.\assets\shader patch.ini' .\packaged\
copy '.\assets\shader patch user readme.txt' '.\packaged\shader patch readme.txt'


Copy-Item -Path .\assets\textures\* -Destination .\packaged\data\shaderpatch\textures -Recurse

md .\packaged\data\_lvl_pc

Write-Host "shader patch package prepared, remember to copy core.lvl over from the shader toolkit!"
