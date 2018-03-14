if (Test-Path .\packaged) {
  Remove-Item -Path .\packaged -Force -Recurse
}

# Create package directory structure.
md .\packaged
md .\packaged\data\
md .\packaged\data\shaderpatch\
md .\packaged\data\shaderpatch\bin\
md .\packaged\data\shaderpatch\textures\
md .\packaged\data\shaderpatch\shaders\
md .\packaged\data\_lvl_pc\

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
msbuild /t:Build /p:Configuration=Release /p:Platform=x86 /m shader_patch.sln

copy .\bin\Release\dinput8.dll .\packaged\
copy ".\bin\Release\shader patch installer.exe" ".\packaged\Shader Patch Installer.exe"
copy ".\bin\Release\Microsoft.Expression.Drawing.dll" .\packaged\

# Copy over end-user tools.
copy ".\bin\Release\lvl_pack.exe" ".\packaged\data\shaderpatch\bin\lvl_pack.exe"
copy ".\bin\Release\shader_compiler.exe" ".\packaged\data\shaderpatch\bin\shader_compiler.exe"

# Copy Assets
copy '.\assets\shader patch.ini' .\packaged\
copy '.\assets\shader patch user readme.txt' '.\packaged\shader patch readme.txt'


Copy-Item -Path .\assets\textures\* -Destination .\packaged\data\shaderpatch\textures -Recurse

# Copy Shaders.
Copy-Item -Path .\assets\shaders\* -Destination .\packaged\data\shaderpatch\shaders -Recurse

if (Test-Path -Path ".\packaged\data\shaderpatch\shaders\build") 
{ 
   del ".\packaged\data\shaderpatch\shaders\build" -Recurse
}

del ".\packaged\data\shaderpatch\shaders\.gitignore"
del ".\packaged\data\shaderpatch\shaders\munged\.gitignore"
del ".\packaged\data\shaderpatch\shaders\munged\*.shader"

Move-Item -Path .\packaged\data\shaderpatch\shaders\core.lvl -Destination .\packaged\data\_lvl_pc\core.lvl
