param([string]$BinPath)

$old_path = $env:Path

$env:Path += ";" + $BinPath

sp_texture_munge --outputdir "munged\" --sourcedir "textures\"
lvl_pack -i "munged\" -i "fonts\" --sourcedir ".\" --outputdir ".\"

$env:Path = $old_path