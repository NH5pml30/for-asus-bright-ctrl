# Elevation code from https://superuser.com/a/532109

param(
    [switch]$elevated,
    [string]$installDir
)

function Test-Admin {
    $currentUser = New-Object Security.Principal.WindowsPrincipal $([Security.Principal.WindowsIdentity]::GetCurrent())
    $currentUser.IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
}

if ((Test-Admin) -eq $false)  {
    if ($elevated) {
        echo "Need admin rights to uninstall!"
    } else {
        Stop-ScheduledTask -TaskName "for-asus-bright-ctrl"
        Unregister-ScheduledTask -TaskName "for-asus-bright-ctrl"

        Start-Process powershell.exe -Verb RunAs -ArgumentList ('-ExecutionPolicy Bypass -noprofile -File "{0}" -installDir "{1}" -elevated' -f ($myinvocation.MyCommand.Definition),($pwd))
    }
    exit
}

Unregister-ScheduledTask -TaskName "for-asus-bright-ctrl regedit"
regedit /s "$installDir\uninstall.reg"
