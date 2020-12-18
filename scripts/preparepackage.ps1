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
md .\packages\shaderpatch\data\shaderpatch\shaders\
md .\packages\shaderpatch\data\shaderpatch\bin\

copy .\LICENSE ".\packages\shaderpatch\shader patch license.txt"
copy .\third_party.md ".\packages\shaderpatch\shader patch acknowledgements.txt"

# Build
msbuild /t:Build /p:Configuration=Release /p:Platform=x86
msbuild /t:Build /p:Configuration=Release /p:Platform=x64

copy .\bin\Release\*.dll .\packages\shaderpatch\
copy ".\tools\manager\bin\Release\Shader Patch Installer.exe" ".\packages\shaderpatch\Shader Patch Installer.exe"

# Copy Assets
copy '.\assets\shader patch.yml' .\packages\shaderpatch\
copy '.\assets\shader patch user readme.txt' '.\packages\shaderpatch\shader patch readme.txt'
copy '.\assets\core\*.lvl' .\packages\shaderpatch\data\shaderpatch\
copy '.\assets\core\definitions\' .\packages\shaderpatch\data\shaderpatch\shaders\definitions\ -Recurse
copy '.\assets\core\src\' .\packages\shaderpatch\data\shaderpatch\shaders\src\ -Recurse

# Package tools.
md .\packages\shaderpatch-tools

copy '.\assets\rendertype_descriptions\' '.\packages\shaderpatch-tools\rendertype_descriptions\' -Recurse

copy .\third_party.md ".\packages\shaderpatch-tools\shader patch acknowledgements.txt"
copy .\tools\bin\x64\Release\*.dll .\packages\shaderpatch-tools\
copy .\tools\bin\x64\Release\*.exe .\packages\shaderpatch-tools\

# Create Shader Cache
Write-Host "Creating shader cache..."
Start-Process -FilePath ".\bin\Release\shader_cache_primer.exe" -WorkingDirectory ".\packages\shaderpatch" -NoNewWindow -Wait
del ".\packages\shaderpatch\shader patch.log"

# Create Archives
Compress-Archive -Path ".\packages\shaderpatch\*" -DestinationPath ".\packages\shaderpatch.zip" -CompressionLevel Optimal -Force
Compress-Archive -Path ".\packages\shaderpatch-tools\*" -DestinationPath ".\packages\shaderpatch-tools.zip" -CompressionLevel Optimal -Force
