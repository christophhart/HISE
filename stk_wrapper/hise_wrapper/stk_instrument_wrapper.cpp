
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

namespace stk
{
using namespace hise;
using namespace juce;
using namespace scriptnode;


namespace helper
{

class Plucked
{
public:
	STK_WRAPPER(Plucked, "plucked");
	STK_NUM_PARAMETERS(2);
	STK_NUM_CHANNELS(1);

	template <int Index> static void addParameter(ParameterDataList& data)
	{
		if (Index == 0)
		{
			parameter::data p("Frequency");
			p.callback = parameter::inner<Plucked, 0>(*this);
			p.range = { 20.0, 20000.0, 0.1 };
			p.range.setSkewForCentre(1000.0);
			data.add(std::move(p));
		}
		if (Index == 1)
		{
			parameter::data p("Release");
			p.callback = parameter::inner<Plucked, 1>(*this);
			p.range = { 0.0, 1.0, 0.1 };
			data.add(std::move(p));
		}
	}

	template <int Index> static void setParameter(void* obj, double newValue)
	{
		ObjectWrapper& d = *reinterpret_cast<ObjectWrapper*>(obj);

		if (Index == 0)
			d.getObject()->setFrequency(newValue);
		if (Index == 1)
			d.setRelease(jlimit(0.0f, 1.0f, (float)newValue));
	}
};



class Guitar
{
public:
	STK_WRAPPER(Guitar, "guitar");
	STK_NUM_PARAMETERS(2);
	STK_NUM_CHANNELS(1);

	template <int Index> static void addParameter(ParameterDataList& data)
	{
		if (Index == 0)
		{
			parameter::data p("Frequency");
			p.callback = parameter::inner<Guitar, 0>(*this);
			p.range = { 20.0, 20000.0, 0.1 };
			p.range.setSkewForCentre(1000.0);
			data.add(std::move(p));
		}
		if (Index == 1)
		{
			parameter::data p("Release");
			p.callback = parameter::inner<Guitar, 1>(*this);
			p.range = { 0.0, 1.0, 0.1 };
			data.add(std::move(p));
		}
	}

	template <int Index> static void setParameter(void* obj, double newValue)
	{
		ObjectWrapper& d = *reinterpret_cast<ObjectWrapper*>(obj);

		if (Index == 0)
			d.getObject()->setFrequency(newValue);
		if (Index == 1)
			d.setRelease(jlimit(0.0f, 1.0f, (float)newValue));
	}
};



}

DECLARE_POLY_STK_TEMPLATE(InstrumentWrapper, Plucked, 1);
DECLARE_POLY_STK_TEMPLATE(InstrumentWrapper, Guitar, 1);
    
}
