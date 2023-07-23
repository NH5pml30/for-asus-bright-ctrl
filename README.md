# Flicker-Free Dimming Brightness Controls App for MyASUS
This is a simple app that I created to control MyASUS' "Flicker-Free Dimming" brightness with hot keys instead of their GUI.

## Installation for Windows >= 10, x86_64
- Download the latest build from [releases](https://github.com/NH5pml30/for-asus-bright-ctrl/releases);
- Extract the folder. It should contain 6 files:
  - `for-asus-bright-ctrl.exe` - the app;
  - `mfc140u.dll` - MFC dll;
  - `install.ps1`, `install.reg`, `uninstall.ps1`, `uninstall.reg` - (un)installation scripts.
- run `./install.ps1` in PowerShell from the extracted directory (important). Note that script execution should be enabled, see more [here, for example](https://superuser.com/questions/106360/how-to-enable-execution-of-powershell-scripts). It will display a UAC prompt half-way through, click "Yes" or similar. It is needed to modify the Windows registry.

  This will add and start a task `for-asus-bright-ctrl` through the Windows Task Scheduler to run this program on every current user's log-in. Additionally, a task `for-asus-bright-ctrl regedit` to apply `install.reg` will be added with the same trigger and started as well. Warning: this will disable security checks that the `AsusOptimization.exe` RPC server does by setting a value in the Windows registry, so that this process can communicate with it. Also note that you shouldn't move this directory or change its contents, otherwise the task will fail and nothing will work.

## Uninstall
To uninstall, do the same things as in installation, but replace `install.ps1` and `install.reg` with `uninstall.ps1` and `uninstall.reg`. This will undo those changes.

## How Does it Work
The app tries to find the MyASUS package, locate its RPC client (the part that communicates to `AsusOptimization.exe`, which controls the driver) `.dll` file, copy it over to the working directory, and then load it and call necessary functions to control the brightness through `AsusOptimization.exe`.

## Usage
Controls are:
- `Ctrl + Shift + Win + <`: brightness down 10%;
- `Ctrl + Shift + Win + >`: brightness up 10%;
- `Ctrl + Shift + Win + /`: re-synchronize brightness, for example when changed from MyASUS app in parallel.

Also note that when using HDR these changes are not applied immediately, but it seems that they will be applied when exiting HDR. MyASUS' app in this case does not allow changing the brightness at all, which seems reasonable.

## Screenshots
A little window with the current brightness indicator should appear in the upper-left corner of the screen, when any of the hot keys are pressed:

![image](https://github.com/NH5pml30/for-asus-bright-ctrl/assets/39946761/70fc0cd9-c5c8-4dcb-bac8-01fcca26bbd6)

## Troubleshooting
You can either manually restart the app, or rerun the task from the Task Scheduler. In the same directory as the extracted `for-asus-bright-ctrl.exe` file there should be created a `log.txt` file with some logs.

## Building From Source
If you want to build this from source, you will need Visual Studio 2022 with Desktop Development with C++ and MFC libraries. Just build the project and grab the resulting `for-asus-bright-ctrl.exe` file with the scripts from `./scripts` folder in the repo.

Disclaimer: I am not affiliated, associated, authorized, endorsed by, or in any way officially connected with ASUSTek Computer Inc., or any of its subsidiaries or its affiliates. The official ASUS web site is available at https://www.asus.com.
