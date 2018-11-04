/**@mainpage HISE C++ API

## Introduction

This API documentation is aimed towards people who use C++ to create HISE projects. It assumes a basic knowledge of the HISE architecture - if you don't know about it, I suggest you spend some time with the prebuild binaries and read / watch some of the high-level documentation to get a feeling of how it works.

## How to use the C++ side of HISE

HISE can be used to build products without touching the C++ code base once. There are many inbuilt modules, a scripting engine for customizing the MIDI processing as well as designing interfaces and a automatic export that creates C++ IDE projects and compiles them into native plugins by calling the system compilers.

However in the course of time, I got more and more requests for customizations / edge case feature requests which lead to an enormous overhead and complexity if implemented, but limit the possibilities if not implemented globally. Also the nature of a dynamic scripting language has its limitations so for bigger
projects or projects which require a higher amount of customization, I decided to open the C++ side of HISE. So if your projects require one of these things:

- dynamic signal path (adding / changing the order of effects, sound generators)
- having a lot of repetition in your UI (eg. a channel strip with 36 channels that all have the same elements)
- reuse / embed existing C++ code (either UI elements or DSP code)
- change the behaviour of the existing HISE modules
- 100% customization of the user interface

it makes a perfect candidate for using the C++ side of HISE. In this case there are basically two options:

### Use the DSP portion of HISE but build a custom UI

If you just want to change the user interface, but get along with the DSP possibilities of HISE, you can add a custom FloatingTile to your scripted interface that acts as shell around your C++ defined interface. This still leaves you the flexibility / protoyping convenience of using HISE as application, but gives you the full possibilities for UI design that JUCE offers.

  A example project that demonstrates this usage can be found here: [ExternalFloatingTileTest](https://github.com/christophhart/hise_tutorial/tree/master/ExternalFloatingTileTest)

### Write everything in C++

If you need dynamic signal paths, add modules on runtime or use other DSP code that you've either written yourself or licensed from a 3rd party, you can bypass the usage of the HISE application altogether, and create both the signal path as well as the UI completely in C++. In this case, you will still use the HISE app as project management tool as it will help you manage external resources and deployment options.

A example project that demonstrates this usage can be found here: [RawTest](https://github.com/christophhart/hise_tutorial/tree/master/RawTest)

## Getting started with HISE C++

Take a grasp look at the classes in the [core module](group__core.html). These classes are the foundation of
HISE and you should at least know what they are / do before doing anything else.

The [raw namespace](namespacehise_1_1raw.html) contains the most important tools needed to build HISE instruments and connect DSP classes to the interface. This should be your entry into the HISE world, from which you can dive into the more complex code base as needed.

A list of every available HISE module can be found [here](group__types.html).

## Create your own classes

If you want to hook up your own DSP classes to build custom modules, take a look at the base class documentation for each different Processor type [here](group__dsp__base__classes.html) which contains information on how to build custom modules.

Also for a live-example for each of the Processor types I suggest looking at the source code of one of the simpler modules of each type (eg. the SineSynth module).

## Code organisation

The HISE code base follows the JUCE coding style and uses its data structures (Array, OwnedArray) as well as some of its paradigms for handling UI / data connections (aka the Listener concept) where-ever possible. This should allow people who are acquainted with the JUCE library to catch on pretty quickly.
The LookAndFeel paradigm that encapsulates styling and logic of UI Components is not fully used, but
the most important classes will be migrated to this soon.

The code base of HISE is organized into multiple JUCE modules, which are mostly dependent from each other, however they follow a dependency chain.

### No dependencies:

- `hi_tools`: standalone tool classes
- `hi_lac`: the HISE lossless audio codec
- `hi_streaming`: the streaming classes in HISE (separated from the other sampler module)

### HISE Main modules:

- `hi_core`: the core classes of HISE
- `hi_dsp`: the DSP base classes of HISE
- `hi_components`: the components of HISE
- `hi_scripting`: all classes related to the Javascript engine of HISE
- `hi_sampler`: the sampler module of HISE
- `hi_modules`: the HISE module collection (effects, sound generators, modulators)

### HISE target-dependent modules:

- `hi_backend`: the module for the HISE application
- `hi_frontend`: the module for compiled plugins

## Usage of Preprocessor definitions

There are quite a few compile-time preprocessor flags that can change the behaviour of HISE - almost every hardcoded limitation like the max number of voices or the channel amount of multi-channel samplers are defined as preprocessor definitions and can be changed for projects that require a different value.

The two most important preprocessor macros are `USE_BACKEND` and `USE_FRONTEND` which separates code paths
between the HISE application and compiled plugins, but there are many more (mostly defined in the LibConfig file and the Macros.h file in the hi_core module).

If you want to use preprocessor customizations, you can pass in the values in the project settings in HISE (there are dedicated places for each platform / OS type).

*/
