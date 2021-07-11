
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

namespace nodes
{
using namespace juce;
using namespace hise;
using namespace scriptnode;
using namespace snex;
	
struct jcrev : public StkEffectWrapper<stk::JCRev, 2>
{
	STK_NODE(jcrev, 1);

	enum class Parameters
	{
		EffectMix,
		T60
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(EffectMix, jcrev);
		DEF_PARAMETER(T60, jcrev);
	}
	PARAMETER_MEMBER_FUNCTION;

	STK_FORWARD_TO_OBJ_METHOD(setT60);

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(jcrev, EffectMix);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(jcrev, T60);
			p.setRange({ 0.0, 16.0, 0.01 });
			data.add(std::move(p));
		}
	}
};


struct delay_a : public StkFilterWrapper<stk::DelayA, 2>
{
	STK_NODE(delay_a, 1);

	enum class Parameters
	{
		Delay,
		MaximumDelay,
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Delay, delay_a);
		DEF_PARAMETER(MaximumDelay, delay_a);
	}
	PARAMETER_MEMBER_FUNCTION;

	STK_FORWARD_TO_MULTI_OBJ_METHOD(setDelay);
	STK_FORWARD_TO_MULTI_OBJ_METHOD(setMaximumDelay);

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(delay_a, Delay);
			p.setRange({ 0.0, 100.0, 0.01 });
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(delay_a, MaximumDelay);
			p.setRange({ 0.0, 100.0, 0.01 });
			data.add(std::move(p));
		}
	}
};


struct banded_wg
{
	enum class Parameters
	{
		Dynamics,
		Pressure,
		Position,
		Active,
		FreqRatio
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Dynamics, banded_wg);
		DEF_PARAMETER(Pressure, banded_wg);
		DEF_PARAMETER(Position, banded_wg);
		DEF_PARAMETER(Active, banded_wg);
		DEF_PARAMETER(FreqRatio, banded_wg);
	}
	PARAMETER_MEMBER_FUNCTION;

	static const int NumVoices = 1;

	STK_NODE(banded_wg, NumVoices);

	HISE_EMPTY_INITIALISE;

	void prepare(PrepareSpecs ps);

	void reset();

	void handleHiseEvent(HiseEvent& e);

	void setDynamics(double newValue);

	void setPressure(double newPressure);

	void setFreqRatio(double ratio);

	void setPosition(double ratio);

	void setActive(double a);

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto& v = voices.get();

		for (auto& s : d[0])
			s += v.tick();
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		d[0] += voices.get().tick();
	}

	void createParameters(ParameterDataList& data);

	PolyData<stk::Bowed, NumVoices> voices;
};


}
}
