# Elevation code from https://superuser.com/a/532109

param(
    [switch]$elevated
)

function Test-Admin {
    $currentUser = New-Object Security.Principal.WindowsPrincipal $([Security.Principal.WindowsIdentity]::GetCurrent())
    $currentUser.IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
}

if ((Test-Admin) -eq $false)  {
    if ($elevated) {
        echo "Need admin rights to uninstall!"
    } else {
        echo ('Starting as admin by "{0}" at "{1}"' -f ($env:USERNAME),($pwd))
        Start-Process -Verb RunAs powershell.exe -ArgumentList ('-ExecutionPolicy Bypass -noprofile -Command Set-Location -LiteralPath \"{0}\"; & \"{1}\" -elevated' -f ($pwd),($myinvocation.MyCommand.Definition))
    }
    exit
}

echo ('Started as admin "{0}" at "{1}"' -f ($env:USERNAME),($pwd))

Stop-ScheduledTask -TaskName "for-asus-bright-ctrl"
Unregister-ScheduledTask -TaskName "for-asus-bright-ctrl" -Confirm:$false

Unregister-ScheduledTask -TaskName "for-asus-bright-ctrl regedit" -Confirm:$false
regedit /s "$installDir\uninstall.reg"