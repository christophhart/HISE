These .dll files contain the [rLottie Library](https://github.com/Samsung/rlottie) that is required to load and play Bodymovin animations in a ScriptPanel.

The license of this library is LGPL, which means that you have to ship these .dlls along with your project and make sure they end up in the correct path:

Windows:

```
%APPDATA%/Company/Product/rlottie_x86.dll (for 32bit builds)
%APPDATA%/Company/Product/rlottie_x64.dll (for 64bit builds)
```

macOS:

```
tba
```

GNU/Linux:

```
/usr/lib/
```

