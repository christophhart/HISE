# How to use Faust on macOS

Integrating Faust on macOS is a bit special because the precompiled Faust releases are not universal binaries while the default build configuration for HISE is now compiling a universal binary for both architectures.

### Installing Faust in this folder

So what we need to do is to download the Faust release from here:

https://github.com/grame-cncm/faust/releases

Make sure you download the correct archive for the architecture on your system: 

- `Faust-VERSION-x64.dmg` if you're running a Intel based mac and 
- `Faust-VERSION-arm64.dmg` if you're working on an Apple Silicon machine

Then open the .dmg file and extract all folders into this subdirecty. At the end there should be these folders:

```
HISE_ROOT/tools/faust/include
HISE_ROOT/tools/faust/bin
HISE_ROOT/tools/faust/lib
HISE_ROOT/tools/faust/share
```

alongside the existing `fakelib` folder (which contains an empty static library that is used when faust isn't enabled).

> The latest versions of Faust will work with HISE, but if you want to keep on using the older `2.50.6` version you need to enable the `FAUST_NO_WARNING_MESSAGES` flag of the `hi_faust_types` module in the Projucer, otherwise it will crash at compilation.

### Building HISE with Faust

#### Change the build architecture to only your machine

Before you build HISE, you must remove the architecture target that doesn't match your machine, otherwise the link process with the Faust library will fail. If this is the case, you will see error message like this:

```
Warning: Ignoring file ../../../../tools/faust/lib/libfaust.dylib, building for macOS-x86_64 but attempting to link with file built for macOS-arm64

Undefined symbol: generateSHA1(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > const&)
Undefined symbol: deleteDSPFactory(llvm_dsp_factory*)
Undefined symbol: expandDSPFromFile(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > const&, int, char const**, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >&)
Undefined symbol: getAllDSPFactories()
Undefined symbol: expandDSPFromString(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > const&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > const&, int, char const**, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >&)
Undefined symbol: getDSPMachineTarget()
```

If that happens, you need to open the Projucer project of HISE (either standalone or plugin), then go to the **Exporters -> XCode (macOS)** tab on the left. Scroll down to the **Valid Architectures** item list and **untick the architecture that doesn't match your system.** So if you're running on a Apple Silicon CPU, untick `x86_x64` and if you're on a Intel machine, untick `arm64` and `arm64e`. 

#### Build HISE with the Faust scheme

Then click on Save and Open in IDE to open the HISE Standalone project in XCode. Change the scheme via *XCode Menu -> Product -> Scheme -> Edit Scheme -> Build Configuration* to either **Debug with Faust** or **Release with Faust**. 

You don't need to change any flags as the two configurations will contain all required build settings.

If the compilation went through, you should see a text label in the HISE top bar indicating that Faust is enabled.

When the build succeeded you need to go into the HISE Settings and set the `FaustPath` folder to the root folder of your faust installation (`HISE_ROOT/tools/faust/`), otherwise it won't find the libraries and any Faust compilation will fail.

> If you're running it for the first time, the macOS gatekeeper might complain about the unsigned faust libraries, so you need to go into the security settings and manually allow the execution in there.
