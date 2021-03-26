
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
using namespace juce;
using namespace hise;
using namespace scriptnode;
using namespace snex;

template <typename T, int NumChannels> struct StkWrapperBase
{
	T obj;

	void reset()
	{
		this->obj.clear();
	}

	void prepare(PrepareSpecs ps)
	{
		obj.setSampleRate(ps.sampleRate);
		numChannelsToProcess = jmin(ps.numChannels, NumChannels);
	}

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_MOD;

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		int i = 0;

		for (auto& s : data)
			s = obj.tick(s, i++);
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if constexpr (ProcessDataType::hasCompileTimeSize())
		{
			constexpr int NumChannelsToUse = jmin(NumChannels, d.getNumChannels());
			auto fd = d.toFrameData<NumChannels>();

			while (fd.next())
				processFrame(fd.toSpan());
		}
		else
		{
			if (d.getNumChannels() == 1)
				FrameConverters::processFix<1>(this, d);
			else
				FrameConverters::processFix<2>(this, d);
		}
	}

	int numChannelsToProcess = 0;
};

template <typename T, int NumChannels> struct MultiChannelWrapperBase
{
	span<T, NumChannels> obj;

	void reset()
	{
		for (auto& o : obj)
			o.clear();
	}

	void prepare(PrepareSpecs ps)
	{
		for (int i = 0; i < ps.numChannels; i++)
			obj[i].setSampleRate(ps.sampleRate);
		
		numChannelsToProcess = jmin(ps.numChannels, NumChannels);
	}

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_MOD;

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		int i = 0;

		for (auto& s : data)
			s = obj[i].tick(s, 0);
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if constexpr (ProcessDataType::hasCompileTimeSize())
		{
			constexpr int NumChannelsToUse = jmin(NumChannels, d.getNumChannels());
			auto fd = d.toFrameData<NumChannels>();

			while (fd.next())
				processFrame(fd.toSpan());
		}
		else
		{
			if (d.getNumChannels() == 1)
				FrameConverters::processFix<1>(this, d);
			else
				FrameConverters::processFix<2>(this, d);
		}
	}

	int numChannelsToProcess = 0;
};



template <typename T, int NumChannels> struct StkEffectWrapper: public StkWrapperBase<T, NumChannels>
{
	void setEffectMix(double v)
	{
		obj.setEffectMix((StkFloat)v);
	}
};

template <typename T, int NumChannels> struct StkFilterWrapper: public MultiChannelWrapperBase<T, NumChannels>
{
	
};

#define STK_NODE(className, numVoices) SET_HISE_NODE_ID(#className); SN_GET_SELF_AS_OBJECT(className); constexpr bool isPolyphonic() { return numVoices > 1;}
#define STK_FORWARD_TO_OBJ_METHOD(methodName) void methodName(double v) { obj.methodName((StkFloat)v); }
#define STK_FORWARD_TO_MULTI_OBJ_METHOD(methodName) void methodName(double v) { for(auto& o: obj) o.methodName((StkFloat)v); }

}
