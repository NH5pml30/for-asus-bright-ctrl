$action = New-ScheduledTaskAction -Execute "$pwd\for-asus-bright-ctrl.exe" -WorkingDirectory "$pwd"
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries
Register-ScheduledTask -Action $action -Trigger $trigger -TaskName "Third-Party Flicker-Free Dimming HotKeys For MyASUS" -Description "This task runs a third-party program to control ASUS Flicker-Free Dimming with hot keys" -Settings $settings -Force
