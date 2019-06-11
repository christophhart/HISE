/*
  ==============================================================================

  The Synthesis ToolKit in C++ (STK) is a set of open source audio
  signal processing and algorithmic synthesis classes written in the
  C++ programming language. STK was designed to facilitate rapid
  development of music synthesis and audio processing software, with
  an emphasis on cross-platform functionality, realtime control,
  ease of use, and educational example code.  STK currently runs
  with realtime support (audio and MIDI) on Linux, Macintosh OS X,
  and Windows computer platforms. Generic, non-realtime support has
  been tested under NeXTStep, Sun, and other platforms and should
  work with any standard C++ compiler.

  STK WWW site: http://ccrma.stanford.edu/software/stk/src/

  The Synthesis ToolKit in C++ (STK)
  Copyright (c) 1995-2011 Perry R. Cook and Gary P. Scavone

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  Any person wishing to distribute modifications to the Software is
  asked to send the modifications to the original developer so that
  they can be incorporated into the canonical version.  This is,
  however, not a binding provision of this license.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  ==============================================================================
*/

#ifdef __STK_STKHEADER__
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#include "stk_wrapper.h"


#if JUCE_LITTLE_ENDIAN && ! defined (__LITTLE_ENDIAN__)
#define __LITTLE_ENDIAN__
#endif

#if JUCE_MAC
#define __MACOSX_CORE__
#endif

#if JUCE_MSVC
#pragma warning (push)
#pragma warning (disable: 4127 4702 4244 4305 4100 4996 4309)
#endif

 //=============================================================================



#include "stk/include/ADSR.h"
#include "stk/include/Asymp.h"
#include "stk/include/BandedWG.h"
#include "stk/include/BeeThree.h"
#include "stk/include/BiQuad.h"
#include "stk/include/Blit.h"
#include "stk/include/BlitSaw.h"
#include "stk/include/BlitSquare.h"
#include "stk/include/BlowBotl.h"
#include "stk/include/BlowHole.h"
#include "stk/include/Bowed.h"
#include "stk/include/BowTable.h"
#include "stk/include/Brass.h"
#include "stk/include/Chorus.h"
#include "stk/include/Clarinet.h"
#include "stk/include/Cubic.h"
#include "stk/include/Delay.h"
#include "stk/include/DelayA.h"
#include "stk/include/DelayL.h"
#include "stk/include/Drummer.h"
#include "stk/include/Echo.h"
#include "stk/include/Effect.h"
#include "stk/include/Envelope.h"
#include "stk/include/FileLoop.h"
#include "stk/include/FileRead.h"
#include "stk/include/FileWrite.h"
#include "stk/include/FileWvIn.h"
#include "stk/include/FileWvOut.h"
#include "stk/include/Filter.h"
#include "stk/include/Fir.h"
#include "stk/include/Flute.h"
#include "stk/include/FM.h"
#include "stk/include/FMVoices.h"
#include "stk/include/FormSwep.h"
#include "stk/include/Function.h"
#include "stk/include/Generator.h"
#include "stk/include/Granulate.h"
#include "stk/include/Guitar.h"
#include "stk/include/HevyMetl.h"
#include "stk/include/Iir.h"
#include "stk/include/Instrmnt.h"
#include "stk/include/JCRev.h"
#include "stk/include/JetTable.h"
#include "stk/include/LentPitShift.h"
#include "stk/include/Mandolin.h"
#include "stk/include/Mesh2D.h"
#include "stk/include/MidiFileIn.h"
#include "stk/include/Modal.h"
#include "stk/include/ModalBar.h"
#include "stk/include/Modulate.h"
#include "stk/include/Moog.h"
#include "stk/include/Noise.h"
#include "stk/include/NRev.h"
#include "stk/include/OnePole.h"
#include "stk/include/OneZero.h"
#include "stk/include/PercFlut.h"
#include "stk/include/Phonemes.h"
#include "stk/include/PitShift.h"
#include "stk/include/Plucked.h"
#include "stk/include/PoleZero.h"
#include "stk/include/PRCRev.h"
#include "stk/include/ReedTable.h"
#include "stk/include/Resonate.h"
#include "stk/include/Rhodey.h"
#include "stk/include/Sampler.h"
#include "stk/include/Saxofony.h"
#include "stk/include/Shakers.h"
#include "stk/include/Simple.h"
#include "stk/include/SineWave.h"
#include "stk/include/SingWave.h"
#include "stk/include/Sitar.h"
#include "stk/include/Skini.h"
#include "stk/include/Sphere.h"
#include "stk/include/StifKarp.h"
#include "stk/include/Stk.h"
#include "stk/include/TapDelay.h"
#include "stk/include/TubeBell.h"
#include "stk/include/Twang.h"
#include "stk/include/TwoPole.h"
#include "stk/include/TwoZero.h"
#include "stk/include/Vector3D.h"
#include "stk/include/Voicer.h"
#include "stk/include/VoicForm.h"
#include "stk/include/Whistle.h"
#include "stk/include/Wurley.h"
#include "stk/include/WvIn.h"
#include "stk/include/WvOut.h"

#if JUCE_MSVC
#pragma warning (pop)
#endif



// stops a warning with clang
#ifdef __clang__
 #pragma clang diagnostic ignored "-Wtautological-compare"
#endif

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4127 4702 4244 4305 4100 4996 4309)
#endif




#include "stk/src/ADSR.cpp"
#include "stk/src/Asymp.cpp"
#include "stk/src/BandedWG.cpp"
#include "stk/src/BeeThree.cpp"
#include "stk/src/BiQuad.cpp"
#include "stk/src/Blit.cpp"
#include "stk/src/BlitSaw.cpp"
#include "stk/src/BlitSquare.cpp"
#include "stk/src/BlowBotl.cpp"
#include "stk/src/BlowHole.cpp"
#include "stk/src/Bowed.cpp"
#include "stk/src/Brass.cpp"
#include "stk/src/Chorus.cpp"
#include "stk/src/Clarinet.cpp"
#include "stk/src/Delay.cpp"
#include "stk/src/DelayA.cpp"
#include "stk/src/DelayL.cpp"
#include "stk/src/Drummer.cpp"
#include "stk/src/Echo.cpp"
#include "stk/src/Envelope.cpp"
#include "stk/src/FileLoop.cpp"
#include "stk/src/FileRead.cpp"
#include "stk/src/FileWrite.cpp"
#include "stk/src/FileWvIn.cpp"
#include "stk/src/FileWvOut.cpp"
#include "stk/src/Fir.cpp"
#include "stk/src/Flute.cpp"
#include "stk/src/FM.cpp"
#include "stk/src/FMVoices.cpp"
#include "stk/src/FormSwep.cpp"
#include "stk/src/Granulate.cpp"
#include "stk/src/Guitar.cpp"
#include "stk/src/HevyMetl.cpp"
#include "stk/src/Iir.cpp"
#include "stk/src/JCRev.cpp"
#include "stk/src/LentPitShift.cpp"
#include "stk/src/Mandolin.cpp"
#include "stk/src/Mesh2D.cpp"
#include "stk/src/MidiFileIn.cpp"
#include "stk/src/Modal.cpp"
#include "stk/src/ModalBar.cpp"
#include "stk/src/Modulate.cpp"
#include "stk/src/Moog.cpp"
#include "stk/src/Noise.cpp"
#include "stk/src/NRev.cpp"
#include "stk/src/OnePole.cpp"
#include "stk/src/OneZero.cpp"
#include "stk/src/PercFlut.cpp"
#include "stk/src/Phonemes.cpp"
#include "stk/src/PitShift.cpp"
#include "stk/src/Plucked.cpp"
#include "stk/src/PoleZero.cpp"
#include "stk/src/PRCRev.cpp"
#include "stk/src/Resonate.cpp"
#include "stk/src/Rhodey.cpp"
#include "stk/src/Sampler.cpp"
#include "stk/src/Saxofony.cpp"
#include "stk/src/Shakers.cpp"
#include "stk/src/Simple.cpp"
#include "stk/src/SineWave.cpp"
#include "stk/src/SingWave.cpp"
#include "stk/src/Sitar.cpp"
#include "stk/src/Skini.cpp"
#include "stk/src/Sphere.cpp"
#include "stk/src/StifKarp.cpp"
#include "stk/src/Stk.cpp"
#include "stk/src/TapDelay.cpp"
#include "stk/src/TubeBell.cpp"
#include "stk/src/Twang.cpp"
#include "stk/src/TwoPole.cpp"
#include "stk/src/TwoZero.cpp"
#include "stk/src/Voicer.cpp"
#include "stk/src/VoicForm.cpp"
#include "stk/src/Whistle.cpp"
#include "stk/src/Wurley.cpp"



#if JUCE_MSVC
 #pragma warning (pop)
 #pragma warning (disable: 4127 4702 4244 4305 4100 4996 4309)
#endif

#ifdef __clang__
 #pragma pop // -Wtautological-compare
#endif



#include "hise_wrapper/stk_wrapper_base.cpp"
#include "hise_wrapper/stk_effect_wrapper.cpp"
#include "hise_wrapper/stk_instrument_wrapper.cpp"
#include "hise_wrapper/stk_factory.cpp"
