/**
  ==============================================================================

@mainpage HISE Library v.095
by Christoph Hart

**HISE** is a C++ library based on JUCE which contains classes & methods to build a modular synthesiser / sampler.

It comes shipped with a ready-to-use application / audio plugin for creating virtual instruments combining synthesis and sampling.

HISE targets end users which can use a fully working, open source sampler as well as commercial developers by allowing compiling of patches into a individual plugin or extending the system with individual sound generators

### Engine Features

- versatile @ref modulatorTypes "modulator system" that allows dynamic modulation of different parameters and the usual suspects (envelopes, LFOs...)
- basic waveform generators for synthesing sounds
- wavetable synthesiser
- @ref sampler "Sampler engine" with disk streaming, round robin, looping, sample start modulation & sfz import 
- @ref effectTypes "Basic audio effects" (delay, polyphonic filters, reverb, convolution, stereo fx)
- @ref macroControl "Macro control system" that can be connected to any parameter / modulator.
- @ref scripting "Scripting engine" for customizable instrument behaviour.
- simple copy protection scheme (based on a key file) to prevent the most stupid script kiddies from uploading your plugin on the web

### Scripting Features

- based on JUCE's Javascript Engine (not fully standard compliant, but the most basic stuff works really well).
- from simple midi filtering logic to a full grown PONG clone
- Access / change engine properties on different callbacks (midi events, timer, control events)
- Asynchronous mode for heavy tasks or synchronous mode for no-latency processing of midi messages
- scriptable interfaces with most common user interface components (knobs, labels, images etc.)
- a c++ wrapper class (HardcodedScriptProcessor) that allows hardcoding of scripts into fast c++ modules with only minimal changes to the script code.
- comfortable IDE features like X-Code style autocomplete (using the escape button) and debugging toolkit (live watch, console output)

### GUI Features

- backend editor with interface system following the tree structure of the engine.
- frontend preset player with customizable appearance using the Scripting Engine.
- additional generic components like table editor, filter response graph, audio file display, peak meters
- @ref debugComponents "debugging tools" like on screen console and data plotter for modulators.
- sample editing interface with focus on workflow.
- @ref views storable "view configurations" to allow customizable workflow for bigger patches

### API features

- code is written following the JUCE code conventions.
- tree structure for every object from sound generator to script processor
- extendable base classes for custom synthesisers / modulators.
- basic implementations of the most common synthesisers / modulators / effects
- set of utility classes like Interpolator, Ramper, TempoSyncer.

### Quick-Links

- \ref scriptingApi - The Scripting API Documentation
- \ref Processor - The base class for all synthesisers / modulators / effects
- \ref ModulatorSampler - the main sampler class

### Supported Technologies / Platforms:

- OSX & Windows
- x86 & x64
- Standalone, VST & AU plugin
- tested on many hosts (Cubase, Ableton Live, GarageBand, Sequoia)

## Licence

This class is published under the GPL license with special commercial available on request.

See http://hartinstruments.net/hise

You need also a valid JUCE license if you want to go closed source.

See http://juce.com

HISE includes the following libraries or 3rd party code, which are all non-restrictively licenced:

- WDL (fork by Oli Larkin) for the convolution engine: https://github.com/olilarkin/wdl-ol
- dywapitchtrack: http://www.schmittmachine.com/dywapitchtrack.html
- the filter response graph code is a modified version of Sean Enderby's FilterGraph class: http://sourceforge.net/projects/jucefiltergraph/

There are also many code snippets which are taken from various public domain sources (mostly musicdsp.org)

<small>Copyright (c) 2015 Christoph Hart</small>

  ==============================================================================
*/
