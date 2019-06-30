param([string]$OuputPath,
      [string]$InputPath)

function read-shader 
{ 
   $bytes = [System.IO.File]::ReadAllBytes("$InputPath")

   foreach ($byte in $bytes)
   {
      Write-Output "$byte,"
   }
}

"constexpr char assao_shader[]={" | Out-File -FilePath "$OuputPath" 

read-shader | Out-File -FilePath "$OuputPath" -Append

"};" | Out-File -FilePath "$OuputPath" -Append
