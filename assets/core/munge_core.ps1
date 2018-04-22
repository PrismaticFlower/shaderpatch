param([string]$binpath=";" + (Resolve-Path "../bin/").Path)

$old_path = $env:Path

$env:Path += $binpath

shader_compiler --outputdir "munged\" --definitioninputdir "definitions\" --hlslinputdir "src\"

lvl_pack -i "munged\" -i "premunged\" --sourcedir ".\" --outputdir ".\"

$env:Path = $old_path