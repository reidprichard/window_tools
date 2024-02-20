function prompt{"PS > "}
$Executable="./kanata_client.exe"
$Command = "$Executable --config-file=./kanata.kbd --default-layer=default --port=1337"
Clear-Host
Write-Host $Command
Invoke-Expression $Command
