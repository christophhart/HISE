
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

  STK WWW site: http://ccrma.stanford.edu/software/stk/include/

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

/** This is a modified version of the STK that is compatible with HISE.
	Any changes made to the original source code are available under public domain. */

#pragma once

namespace stk
{
using namespace hise;
using namespace juce;
using namespace scriptnode;

#if JUCE_DEBUG
#define SET_OBJECT_SIZE(name, debugSize, releaseSize) static constexpr int name = debugSize;
#else
#define SET_OBJECT_SIZE(name, debugSize, releaseSize) static constexpr int name = releaseSize;
#endif


// This namespace contains the sizeof(T) value for the samplerate wrapper.
namespace objectsize
{
SET_OBJECT_SIZE(DelayA, 296, 280);
SET_OBJECT_SIZE(DelayL, 288, 272);
SET_OBJECT_SIZE(JCRev, 3376, 3168);
}

/** Definitions. */

#define STK_WRAPPER(className, node_id) using Type = stk::className; \
									    using ObjectWrapper = SampleRateWrapper<Type, objectsize::className>; \
											  static Type* create(void* data) { return new (data) Type(); } \
											  static Identifier getId() { static const Identifier id(node_id); return id; }

#define STK_NUM_CHANNELS(n) static constexpr int NumChannelsPerObject = n;
#define STK_NUM_PARAMETERS(n) static constexpr int NumParameters = n;

#define STK_TEMPLATE_WRAPPER(wrapperClass, stkClass) template class wrapperClass<helper::stkClass, objectsize::stkClass, stkClass, 1>;

#define FORWARD_DECLARE_POLY_STK_CLASS(wrapperClass, className, nodeName, polyNodeName, numChannels) \
   class className; namespace helper { class className; } \
   extern template class wrapperClass<helper::className, objectsize::className, stk::className, numChannels, 1>; \
   using nodeName = wrapperClass<helper::className, objectsize::className, stk::className, numChannels, 1>; \
   extern template class wrapperClass<helper::className, objectsize::className, stk::className, numChannels, NUM_POLYPHONIC_VOICES>; \
   using polyNodeName = wrapperClass<helper::className, objectsize::className, stk::className, numChannels, NUM_POLYPHONIC_VOICES>; 

#define FORWARD_DECLARE_STK_CLASS(wrapperClass, className, nodeName, numChannels) \
   class className; namespace helper { class className; }\
   extern template class wrapperClass<helper::className, objectsize::className, stk::className, numChannels, 1>; \
   using nodeName = wrapperClass<helper::className, objectsize::className, stk::className, numChannels, 1>;

#define DECLARE_POLY_STK_TEMPLATE(wrapperClass, className, numChannels) \
template class wrapperClass<helper::className, objectsize::className, stk::className, numChannels, 1>; \
template class wrapperClass<helper::className, objectsize::className, stk::className, numChannels, NUM_POLYPHONIC_VOICES>;

#define DECLARE_STK_TEMPLATE(wrapperClass, className, numChannels) \
template class wrapperClass<helper::className, objectsize::className, stk::className, numChannels, 1>; \

using ParameterList = Array<scriptnode::HiseDspBase::ParameterData>;

using Callback = std::function<void(double)>;

// ============================================================================================= Forward declarations


FORWARD_DECLARE_POLY_STK_CLASS(EffectWrapper, DelayA, delay_a, delay_a_poly, 1);
FORWARD_DECLARE_POLY_STK_CLASS(EffectWrapper, DelayL, delay_l, delay_l_poly, 1);
FORWARD_DECLARE_STK_CLASS(EffectWrapper, JCRev, jc_rev, 1);



}
