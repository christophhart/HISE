# HISE

![](http://hise.audio/images/github.png)

**Build Status macOS / Windows (`develop` branch)**  

[![CI build macOS](https://github.com/christophhart/HISE/actions/workflows/ci_mac.yml/badge.svg?branch=develop)](https://github.com/christophhart/HISE/actions/workflows/ci_mac.yml)

**The open source framework for sample based instruments.**

HISE is a cross-platform open source audio application for building virtual instruments. 
It emphasizes on sampling, but includes some basic synthesis features for making hybrid instruments as well as audio effects. 
You can export the instruments as VST / AU / AAX plugins or as standalone application for Windows / macOS or iOS.

More information:

[HISE website](http://hise.audio)

## System requirements

Supported OS:

- Windows 7+
- OSX 10.7+
- iOS 8.0+
- Linux (tested on Ubuntu 16.04 LTS)

HISE is tested on Windows and OSX with the following hosts:

- Cubase
- Ableton Live
- Logic
- Reaper
- Protools
- REASON 10
- FL Studio
- Presonus Studio One

It supports x86 and x64 on Windows, altough the 64bit version is highly recommended (it uses memory mapping for accessing samples and because of the limitations of the 32bit memory address space it needs a slower fallback solution).

## How to compile HISE

### Windows 

1. Clone this repository. It also includes the (slightly modified) JUCE source code, so it might take a while.

2. Extract the contents of `tools/SDK/sdk.zip` to `tools/SDK`. Your `tools` folder should now contain folders named `ASIOSDK2.3` and `VST3 SDK`.

3. Open the Projucer (there are compiled versions for every supported OS in the `tools/projucer` subdirectory) and load the HISE project (either `projects/standalone/HISE Standalone.jucer` or `project/plugin/HISE.jucer`)

4. Click on "Save Project and open in IDE" to load the project in Visual Studio 2022.

5. Hit compile and wait...

### OSX

1. Clone this repository. It also includes the (slightly modified) JUCE source code, so it might take a while.

2. Extract the contents of `tools/SDK/sdk.zip` to `tools/SDK`. Your `tools` folder should now contain folders named `ASIOSDK2.3` and `VST3 SDK`.

3. Install [xcpretty](https://github.com/xcpretty/xcpretty), a formatter for xcode. You can install it from the terminal using the command `sudo gem install xcpretty`.

4. Open the Projucer (there are compiled versions for every supported OS in the `tools/projucer` subdirectory) and load the HISE project (either `projects/standalone/HISE Standalone.jucer` or `project/plugin/HISE.jucer`).

> If you hit a permission issue when launching Projucer (or an error stating that Projucer is damaged) you need to open Security & Privacy and whitelist Projucer.

5. Click on "Save Project and open in IDE" to load the project in XCode.

6. Hit compile and wait...

### Linux

1. Install the dependencies: 
```
sudo apt-get -y install build-essential make llvm clang libfreetype6-dev libx11-dev libxinerama-dev libxrandr-dev libxcursor-dev mesa-common-dev libasound2-dev freeglut3-dev libxcomposite-dev libcurl4-gnutls-dev libwebkit2gtk-4.0 libgtk-3-dev libjack-jackd2-dev
```

2. Clone this repository. It also includes the (slightly modified) JUCE source code, so it might take a while.

3. Extract the contents of `tools/SDK/sdk.zip` to `tools/SDK`. Your `tools` folder should now contain folders named `ASIOSDK2.3` and `VST3 SDK`.

4. Open the Projucer (a precompiled Linux binary can be found at `tools/projucer`). Load the project `projects/standalone/HISE Standalone.jucer` and resave the project (this will generate the Makefile with correct Linux paths).

5. Open the terminal and navigate to this subdirectory: `projects/standalone/Builds/LinuxMakefile`

6. Type `make CONFIG=Release` and wait. If you need the debug version (that is slower but allows you to jump around in the source code, use `make CONFIG=Debug`.


## License

HISE is licensed under the GPL v3, but there will be a commercial license for closed source usage. Every instrument you'll build will inheritate this license so in order to release a closed source product you'll have to obtain a HISE commercial license as well as a JUCE commercial license. Please get in touch with me for further informations.

## Included frameworks

For FFT routines and some vector operations, it is recommended to build HISE against the Intel IPP library (not included).

Apart from the JUCE C++ library, there are some other 3rd party frameworks and libraries included in HISE, which are all non restrictively licenced (either BSD or MIT):

- **ICSTDP DSP library**: A pretty decent DSP library with some good and fast routines.   [Website](https://www.zhdk.ch/en/researchproject/426390)
- **Kiss FFT**: A easy and C-only FFT library with a clean interface and acceptable performance. It is used as fallback FFT when the IPP library is not available.
- **FFTConvolver**: a library for fast, partitioned real time convolution: https://github.com/HiFi-LoFi/FFTConvolver
- **MDA Plugins**: a collection of audio effects recently published as open source project.
- some other public domain code taken from various sources (http://musicdsp.org, etc.).

## Support

The best place to get support for anything related to HISE is the user forum: https://forum.hise.audio/
