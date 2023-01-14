# How to use Faust on macOS

Integrating Faust on macOS is a bit special because (for now) we're running HISE under Rosetta so we need to use the x86 Faust libraries (and chances are great that the default Faust installation on an M1 machine is native).


### Installing Faust in this folder

So what we need to do instead is to download the x86-x64 Faust release from here:

https://github.com/grame-cncm/faust/releases

> **Important**: The latest version of Faust will not work in HISE because of some build issues with LLVM (more info in [this thread](https://forum.hise.audio/topic/7026/unable-to-find-libncurses-6-dylib-when-launching-hise-faust/15?_=1673534840758)). The solution is to use a slighly older faust release until this is sorted out: 

https://github.com/grame-cncm/faust/releases/tag/2.50.6

Make sure you download this archive: `Faust-VERSION-x64.dmg` (there's also an ARM version which you must not use). Then open the .dmg file and extract all folders into this subdirecty. At the end there should be these folders:

```
HISE_ROOT/tools/faust/include
HISE_ROOT/tools/faust/bin
HISE_ROOT/tools/faust/lib
HISE_ROOT/tools/faust/share
```

alongside the existing `fakelib` folder (which contains an empty static library that is used when faust isn't enabled).

### Building HISE with Faust

Then you need to open the HISE Standalone project in XCode and change the scheme via *XCode Menu -> Product -> Scheme -> Edit Scheme -> Build Configuration* to either **Debug with Faust** or **Release with Faust**. 

> If you can't see the configurations with Faust, you need to resave the Projucer file so that it will rebuild the Xcode project to show the new configurations.

You don't need to change any flags as the two configurations will contain all required build settings.

If the compilation went through, you should see a text label in the HISE top bar indicating that Faust is enabled.

When the build succeeded you need to go into the HISE Settings and set the `FaustPath` folder to the root folder of your faust installation (`HISE_ROOT/tools/faust/`), otherwise it won't find the libraries and any Faust compilation will fail.
