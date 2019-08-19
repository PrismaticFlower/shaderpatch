# Find and import Developer Command Prompt environment.
$env:path += ";${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\"

$installationPath = vswhere.exe -latest -property installationPath

if ($installationPath -and (test-path "$installationPath\Common7\Tools\vsdevcmd.bat")) {
  & "${env:COMSPEC}" /s /c "`"$installationPath\Common7\Tools\vsdevcmd.bat`" -no_logo && set" | foreach-object {
    $name, $value = $_ -split '=', 2
    set-content env:\"$name" $value
  }
}


