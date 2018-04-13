param([string]$binpath=";" + (Resolve-Path "../bin/").Path)

$old_path = $env:Path

$env:Path += $binpath

# Make sure we have a place to put checksums and munged files.
mkdir -Force build/ > $null
mkdir -Force munged/ > $null

foreach ($file in Get-ChildItem -File -Path definitions\* -Include *.json)
{
   $srcname = $file.Name -replace ".json", ".fx"
   $outname = $file.Name -replace ".json", ".shader"

   shader_compiler $file.FullName "munged\$outname" "src\$srcname" 
}

foreach ($file in Get-ChildItem -File -Path patch_definitions\* -Include *.json)
{
   $outname = $file.Name -replace ".json", ".shader"

   shader_compiler -p $file.FullName "munged\$outname"
}

lvl_pack ".\core.lvldef" "munged\" ".\core.lvl"

$env:Path = $old_path