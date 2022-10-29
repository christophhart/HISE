# How to use Faust on macOS

Integrating Faust on macOS is a bit special because (for now) we're running HISE under Rosetta so we need to use the x86 Faust libraries (and chances are great that the default Faust installation on an M1 machine is native).

### Installing Faust in this folder

So what we need to do instead is to download the x86-x64 Faust release from here:

https://github.com/grame-cncm/faust/releases

Make sure you download this archive: `Faust-VERSION-x64.dmg` (there's also an ARM version which you must not use). Then open the .dmg file and extract all folders into this subdirecty. At the end there should be these folders:

```
HISE_ROOT/tools/faust/include
HISE_ROOT/tools/faust/bin
HISE_ROOT/tools/faust/lib
HISE_ROOT/tools/faust/share
```

alongside the existing `fakelib` folder (which contains an empty static library that is used when faust isn't enabled).

### Building HISE with Faust

Then you need to open the HISE Standalone project in XCode and change the scheme via *XCode Menu -> Product -> Scheme -> Edit Scheme -> Build Configuration* to either **Debug with Faust** or **Release with Faust**. You don't need to change any flags as the two configurations will contain all required build settings.

If the compilation went through, you should see a text label in the HISE top bar indicating that Faust is enabled.

### Other notes

Since we're using this directory as faust folder, the requirement to set the FaustPath in the settings is removed (an in fact the FaustPath property is ignored on macOS).