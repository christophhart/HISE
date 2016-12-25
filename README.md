# HISE

| Linux Build | Windows Build |
| ----------- | ------------- |
| [![Build Status](https://travis-ci.org/christophhart/HISE.svg?branch=master)](https://travis-ci.org/christophhart/HISE) | [![Build status](https://ci.appveyor.com/api/projects/status/54boowuns6wy0nhd?svg=true)](https://ci.appveyor.com/project/ChristophHart/hise) |


**The open source framework for sample based instruments.**

HISE is a cross-platform open source audio application for building virtual instruments. 
It emphasizes on sampling, but includes some basic synthesis features for making hybrid instruments as well as audio effects. 
You can export the instruments as VST / AU / AAX plugins or as standalone application for Windows / macOS or iOS.

More information:

[HISE website](http://hise.audio)

## System requirements

Supported OS:

- Windows 7-10
- OSX 10.7 - 10.12
- iOS 8.0
- Linux (experimental, tested on Ubuntu 16.04 LTS)

HISE is tested on Windows and OSX with the following hosts:

- Cubase
- Ableton Live
- Logic
- Reaper
- Protools

It supports x86 and x64 on Windows, altough the 64bit version is highly recommended (it uses memory mapping for accessing samples and because of the limitations of the 32bit memory address space it needs a slower fallback solution).

## Highlights

### Sampler engine

- fast disk streaming sampler engine using memory mapped files
- Import SFZ files, Mach5 files or AIFFs with metadata from Keymap Pro
- File name parser for automapping samples
- multi mic sample support (with purging of single mic channels)
- looping with crossfades
- sample start modulation
- crossfade between samples for dynamic sustain samples
- customizable voice start behaviour
- regex parser for mapping samples
- custom monolith file format for faster loading times
- switch sample mappings dynamically

### Modulation

- complex modulation architecture for nested modulation of common parameters
- includes the most common modulators (LFO, envelopes)

### Audio Effects

- fast convolution reverb
- filters / eq
- phaser / chorus
- delay / reverb

### Javascript interpreter

- superset of Javascript built for real time usage (no allocations, low overhead function calls)
- write MIDI processing scripts
- change voice properties (volume & pitch)
- create plugin interfaces with a WYSIWYG editor
- built in IDE features (autocomplete / API reference, variable watch, console debugging)
- combine DSP routines for custom effects

### C / C++ compiler

- embedded C JIT compiler for fast prototyping of DSP routines (based on TinyCC)
- API for adding DSP modules via dynamic libraries
- one click C++ build system for building VST / AU / AAX plugins (based on JUCE) from within HISE (using msbuild / xcodebuild)

### Export

- export HISE patches as plugin or standalone application
- supported plugin architectures: AAX / VST / AU
- supported platforms: Windows / macOS / iOS (I can't offer actual Linux support because of my Linux-noobness, but feel free to try compiling it and let me know if something doesn't work)

## How to compile HISE

### Windows / OSX

1. Clone this repository. It also includes the (slightly modified) JUCE source code, so it might take a while.

3. Get all necessary 3rd party code:
	- [ASIO SDK](http://www.steinberg.net/sdk_downloads/asiosdk2.3.zip) for standalone support on Windows.
	- [VST SDK](http://www.steinberg.net/sdk_downloads/vstsdk366_27_06_2016_build_61.zip) for building VST plugins
	- [Intel Performance Primitives](https://software.intel.com/en-us/articles/free-ipp) (this is optional but heavily increases the performance of the convolution reverb)

4. Get the Projucer from ROLI. You can build it yourself or just use these compiled binaries: 
    - [macOS](https://github.com/christophhart/JUCE4HISE/releases/download/projucer/Projucer_macOS.zip)
    - [Windows](https://github.com/christophhart/JUCE4HISE/releases/download/projucer/Projucer.exe)

5. Open the Projucer and load the HISE project (either `projects/standalone/HISE Standalone.jucer` or `project/plugin/HISE.jucer`)

6. Make sure the VST / ASIO path settings is correct on your system. If you don't have IPP installed, set the USE_IPP flag in the hi_core module to 0.

7. Click on "Save Project and open in IDE" to load the project in XCode / Visual Studio. 

8. Hit compile and wait...

### Linux

1. Get these dependencies (taken from the JUCE forum):

```
sudo apt-get -y install llvm
sudo apt-get -y install clang
sudo apt-get -y install libfreetype6-dev
sudo apt-get -y install libx11-dev
sudo apt-get -y install libxinerama-dev
sudo apt-get -y install libxrandr-dev
sudo apt-get -y install libxcursor-dev
sudo apt-get -y install mesa-common-dev
sudo apt-get -y install libasound2-dev
sudo apt-get -y install freeglut3-dev
sudo apt-get -y install libxcomposite-dev
sudo apt-get -y install libcurl4-gnutls-dev
```

2. Clone this repository.

3. Open the terminal and navigate to this subdirectory: `projects/standalone/Builds/LinuxMakefile`

4. `make` and wait. 

## Licence

HISE is licenced under the GPL v3, but there will be a commercial licence for closed source usage. Every instrument you'll build will inheritate this licence so in order to release a closed source product you'll have to obtain a HISE commercial licence as well as a JUCE commercial licence. Please get in touch with me for further informations.

## Included frameworks

For FFT routines and some vector operations, it is recommended to build HISE against the Intel IPP library (not included).

Apart from the JUCE C++ library, there are some other 3rd party frameworks and libraries included in HISE, which are all non restrictively licenced (either BSD or MIT):

- **ICSTDP DSP library**: A pretty decent DSP library with some good and fast routines.   [Website](https://www.zhdk.ch/index.php?id=icst_dsplibrary)
- **Tiny C Compiler** Awesome little compiler that translates C files into machine code within milliseconds. It is embedded into HISE as development tool. The compiler is LGPL licenced, so it is linked dynamically into HISE, but for closed source plugins, the C files will be compiled by a "real" compiler anyway.
- **Kiss FFT**: A easy and C-only FFT library with a clean interface and acceptable performance. It is used as fallback FFT when the IPP library is not available.
- **WDL** (just for the convolution, it might get sorted out in the future)
- **MDA Plugins**: a collection of audio effects recently published as open source project.
- some other public domain code taken from various sources (music-dsp.org, etc.).
