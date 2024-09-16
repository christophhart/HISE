# The payload installer

This project will compile a generic installer that will load a monolith payload exported from the multipage creator. This is an alternative way of just compiling the Installer from within the multipage creator. 

If you don't want or can't codesign your installers on Windows / macOS, using the precompiled binaries in `/bin` will prevent the nasty security popup when running unsigned installers.

> on macOS you will still need to notarize your plugin binaries with your Apple development ID.

## How to use it

In order to use this binary as an installer for your project, you need to:

1. Create your installer in the multipage creator. You can use [this](https://github.com/christophhart/vcsl_hise/blob/master/Installer/installer.json) as a starting point
2. Export it as a monolith payload file using **File -> Export as monolith payload**. You can choose any filename here and it will create a single file with the `*.dat` extension that contains all the logic, styling & assets of your installer.
3. Rename the installer binary to match your monolith file. So if you've saved the monolith as `My Installer 1_0_0.dat`, then rename the installer binary to `My Installer 1_0.0.exe` (or `My Installer 1_0_0.app`).
4. Make sure to distribute both files (the .dat file and the renamed installer binary to the end user). Ideally they end up in the same folder, then it will automatically load the payload (it checks its filename and then searches for a sibling .dat file). If this doesn't work because the end user has not put them in the correct position, they will be asked to locate the payload file. **This is a severe UX disadvantage towards the solution where you compile the assets into one installer binary, so if you are able to codesign stuff, it's recommended to do it the other way.**

> Protip: on macOS, make sure to use [DMGCanvas](https://www.araelium.com/dmgcanvas) to bundle the installer and the payload into a single DMG image.