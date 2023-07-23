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
        echo "Need admin rights to install!"
    } else {
        $action = New-ScheduledTaskAction -Execute "$pwd\for-asus-bright-ctrl.exe" -WorkingDirectory "$pwd"
        $trigger = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
        $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries
        Register-ScheduledTask -Action $action -Trigger $trigger -TaskName "for-asus-bright-ctrl" -Description "This task runs a third-party program to control ASUS Flicker-Free Dimming with hot keys" -Settings $settings -Force
        Start-ScheduledTask -TaskName "for-asus-bright-ctrl"

        Start-Process powershell.exe -Verb RunAs -ArgumentList ('-ExecutionPolicy Bypass -noprofile -File "{0}" -installDir "{1}" -elevated' -f ($myinvocation.MyCommand.Definition),($pwd))
    }
    exit
}

$action = New-ScheduledTaskAction -Execute "regedit" -Argument "/s install.reg" -WorkingDirectory "$installDir"
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries
Register-ScheduledTask -Action $action -Trigger $trigger -TaskName "for-asus-bright-ctrl regedit" -Description "This task edits the registry to allow the third-party program to control ASUS Flicker-Free Dimming with hot keys" -Settings $settings -Force -RunLevel Highest
Start-ScheduledTask -TaskName "for-asus-bright-ctrl regedit"
