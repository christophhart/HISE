# HISE

![](http://hise.audio/images/github.png)

**Build status (`develop` branch)**

[![CI build macOS](https://github.com/christophhart/HISE/actions/workflows/ci_mac.yml/badge.svg?branch=develop)](https://github.com/christophhart/HISE/actions/workflows/ci_mac.yml)

**The open source framework for sample based instruments.**

HISE is a cross-platform open source audio application for building virtual instruments.
It emphasizes on sampling, but includes some basic synthesis features for making hybrid instruments as well as audio effects.
You can export the instruments as VST / AU / AAX plugins or as standalone application for Windows / macOS or iOS.

More information:

[HISE website](http://hise.audio)

## System requirements

The current `projects/standalone/HISE Standalone.jucer` and `projects/plugin/HISE.jucer` Xcode exporters set the macOS deployment target and SDK compatibility to **11.0**. Use an Xcode toolchain with a compatible macOS SDK for these builds. The standalone project also has a macOS Makefile exporter without an explicit deployment target; this is not a verified minimum for that build path or for the `hise-cli` installer.

The current Windows exporters target **x64** and use Visual Studio 2026. The project files do not declare a minimum Windows runtime version or a specific Windows SDK version; do not infer a minimum Windows OS from the installed build SDK. On Linux, use a toolchain and development packages compatible with the project; automated `hise-cli /setup` currently supports x64 Debian/Ubuntu-compatible systems with `apt-get`. Linux ARM64 is not supported by that automated setup.

For a managed source build, see [hise-cli](https://github.com/christophhart/hise-cli): run `/setup` as a normal user. Both `/setup` and `/update` select the latest CI-green `develop` commit by default.

The checked-in Windows standalone and plugin project configurations target x64. Other architectures or older operating-system versions are not established as supported by these project files.

## How to compile HISE

### Windows

1. Clone this repository with its JUCE submodule: `git clone --recurse-submodules https://github.com/christophhart/HISE.git`. The setup wizard also checks out the `juce6` branch in that submodule. For a manual build, ensure your JUCE checkout matches the project before resaving.

2. Extract `tools/SDK/sdk.zip` into `tools/SDK`. The included archive contains `ASIOSDK2.3` and `VST3 SDK`; you do not need to fetch those separately for this build. Other optional SDKs, such as AAX, may have separate requirements.

3. Use the Projucer supplied by the JUCE submodule (`JUCE/Projucer`) to open `projects/standalone/HISE Standalone.jucer` (or `projects/plugin/HISE.jucer` for the plugin project).

4. The checked-in Windows exporter is **VS2026** and its IPP setting is enabled. To build without IPP, set the exporter's **Use IPP** option to **No** before resaving. For a different installed Visual Studio version, the `hise-cli /setup` wizard can adapt the exporter.

5. Select **Save Project and Open in IDE**, then build the Release configuration. A first full build may take 30 minutes or more.

### macOS

1. Clone this repository with its JUCE submodule: `git clone --recurse-submodules https://github.com/christophhart/HISE.git`. For the wizard-matched submodule checkout, see the Windows instructions above.

2. Extract `tools/SDK/sdk.zip` into `tools/SDK`.

3. Open `projects/standalone/HISE Standalone.jucer` (or `projects/plugin/HISE.jucer`) with the JUCE submodule's Projucer. If macOS blocks Projucer, allow it in **System Settings > Privacy & Security**.

4. Select **Save Project and Open in IDE**, then build in Xcode. These Xcode project configurations target macOS 11.0; the separate Makefile build path does not declare a deployment target.

### Linux

1. Install the dependencies:

```bash
sudo apt-get update
sudo apt-get -y install build-essential make llvm clang libfreetype6-dev libx11-dev libxinerama-dev libxrandr-dev libxcursor-dev mesa-common-dev libasound2-dev freeglut3-dev libxcomposite-dev libcurl4-gnutls-dev libgtk-3-dev libjack-jackd2-dev libwebkit2gtk-4.0-dev libpthread-stubs0-dev ladspa-sdk unzip
```

> If *libwebkit2gtk-4.0-dev* is not available in your distro's repositories you can use *libwebkit2gtk-4.1-dev* instead.

> HISE can be compiled with GCC/G++ versions greater than 11, however there will be buggy behaviour with certain elements of the UI. Therefore it is advised to use GCC/G++ 11 or lower.

> If your distro's repositories contain the *mold* linker you should absolutely use it as it will greatly reduce build times. Alias it to *gold* in .bashrc and you don't need to make any changes to HISE or Projucer to get the benefit.

2. Clone this repository with its JUCE submodule: `git clone --recurse-submodules https://github.com/christophhart/HISE.git`.

3. Extract `tools/SDK/sdk.zip` into `tools/SDK`.

4. Use the JUCE submodule's Projucer to open `projects/standalone/HISE Standalone.jucer` and resave the project to generate a Makefile with the correct Linux paths.

5. Open the terminal and navigate to this subdirectory: `projects/standalone/Builds/LinuxMakefile`

6. Run `make CONFIG=Release` and wait. For a debug build, use `make CONFIG=Debug`.

## AI Development Tools

HISE includes optional tooling for AI-assisted development with [OpenCode](https://opencode.ai), [Claude Code](https://claude.ai), or other MCP/LSP-compatible AI coding agents.

### MCP Server

Gives your AI agent access to HISE documentation and, when HISE is running with the REST server enabled (**Tools > Enable REST Server**), live interaction with your project - compile scripts, capture screenshots, manipulate UI components.

**Setup:** See [`MCP Server Repository`](https://github.com/christoph-hart/hise_mcp_server)

### LSP Server

A diagnostic-only Language Server that catches HISEScript errors (API hallucinations, wrong argument counts, type mismatches) in `.js` files before you compile. Configure this **per-project** to avoid overriding the standard JavaScript LSP in non-HISE projects.

**Setup:** See [`LSP Server Repository`](https://github.com/christoph-hart/hise_lsp_server)

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
