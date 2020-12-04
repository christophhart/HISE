
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

#define TP template <class StkType, size_t DataSize>
#define SAMPLERATE_WRAPPER SampleRateWrapper<StkType, DataSize>


TP SAMPLERATE_WRAPPER::SampleRateWrapper()
{
	// use this to query the size if the static_assert fires...
	//static constexpr int d = sizeof(Guitar);
	
	//static_assert(sizeof(StkType) == DataSize, "Data size mismatch");

	memset(data, 0, DataSize);
}


TP void SAMPLERATE_WRAPPER::deleteObject()
{
	if (object != nullptr)
	{
		jassert(reinterpret_cast<void*>(object) == reinterpret_cast<void*>(&data));
		object->~StkType();
		memset(data, 0, DataSize);
	}

	object = nullptr;
}

#undef TP
#undef SAMPLERATE_WRAPPER

#define TP template <class T, size_t DataSize, class StkType, int NumChannels, int NV>
#define WRAPPER_BASE stk::WrapperBase<T, DataSize, StkType, NumChannels, NV>

template <class HelperClass, int NumChannels> constexpr int getNumChannelsToProcess()
{
	return HelperClass::NumChannelsPerObject * NumChannels;
}

TP WRAPPER_BASE::WrapperBase()
{
	DBG("Size: " + String(sizeof(StkType)));

	memset(paramValues, 0, sizeof(double) * 8);

	PrepareSpecs ps;
	ps.blockSize = 1;
	ps.sampleRate = Stk::sampleRate();
	ps.numChannels = NumChannels;
	ps.voiceIndex = &unused;
	prepare(ps);
}

TP void WRAPPER_BASE::clearObjects(bool deleteObjects/*=true*/)
{
	for (int i = 0; i < NumChannels; i++)
	{
		for (auto& o : objects[i])
			o.deleteObject();
	}
}


TP void WRAPPER_BASE::reset()
{
	if (voiceRenderingActive())
		for (auto& o : *this) o.get().getObject()->clear();
	else
		for (auto& o : *this)
		{
			for (auto& ob : o)
				ob.getObject()->clear();
		}
}


TP bool WRAPPER_BASE::voiceRenderingActive() const
{
	return objects[0].isVoiceRenderingActive();
}

TP void WRAPPER_BASE::setParameter0(double newValue)
{
	paramValues[0] = newValue;

	if (voiceRenderingActive())
		for (auto& o : *this) T::template setParameter<0>(o.get(), newValue);
	else
	{
		for (auto& o : *this)
		{
			for (auto& ob : o)
				T::template setParameter<0>(ob, newValue);
		}
	}
}

TP void WRAPPER_BASE::setParameter1(double newValue)
{
	paramValues[1] = newValue;

	if (voiceRenderingActive())
		for (auto& o : *this) T::template setParameter<1>(o.get(), newValue);
	else
	{
		for (auto& o : *this)
		{
			for (auto& ob : o)
				T::template setParameter<1>(ob, newValue);
		}
	}
}

TP void WRAPPER_BASE::setParameter2(double newValue)
{
	paramValues[2] = newValue;

	if (voiceRenderingActive())
		for (auto& o : *this) T::template setParameter<2>(o.get(), newValue);
	else
		{
		for (auto& o : *this)
		{
			for (auto& ob : o)
				T::template setParameter<2>(ob, newValue);
		}
	}
}

TP void WRAPPER_BASE::setParameter3(double newValue)
{
	paramValues[3] = newValue;

	if (voiceRenderingActive())
		for (auto& o : *this) T::template setParameter<3>(o.get(), newValue);
	else
	{
		for (auto& o : *this)
		{
			for (auto& ob : o)
				T::template setParameter<3>(ob, newValue);
		}
	}
}




TP void WRAPPER_BASE::createParameters(Array<ParameterData>& data)
{
	if (T::NumParameters >= 1)
		T::template addParameter<0>(data, BIND_MEMBER_FUNCTION_1(WrapperBase::setParameter0));
	if (T::NumParameters >= 2)
		T::template addParameter<1>(data, BIND_MEMBER_FUNCTION_1(WrapperBase::setParameter1));
	if (T::NumParameters >= 3)
		T::template addParameter<2>(data, BIND_MEMBER_FUNCTION_1(WrapperBase::setParameter2));
	if (T::NumParameters >= 4)
		T::template addParameter<3>(data, BIND_MEMBER_FUNCTION_1(WrapperBase::setParameter3));
}

TP void WRAPPER_BASE::prepare(PrepareSpecs ps)
{
	auto gb = Stk::sampleRate();
	Stk::setSampleRate(ps.sampleRate);

	sr = ps.sampleRate;

	clearObjects();

	auto s = sr;

	for (int i = 0; i < NumChannels; i++)
	{
		for(auto& t: objects[i])
			t.setNewWithSampleRate(T::create(t.getDataPtr()), s);

		objects[i].prepare(ps);
	}

	reset();

	int numParameters = T::NumParameters;

	if (numParameters > 0) setParameter0(paramValues[0]);
	if (numParameters > 1) setParameter1(paramValues[1]);
	if (numParameters > 2) setParameter2(paramValues[2]);
	if (numParameters > 3) setParameter3(paramValues[3]);

	Stk::setSampleRate(gb);
}


#undef WRAPPER_BASE

#define EFFECT_WRAPPER EffectWrapper<T, DataSize, StkType, NumChannels, NV>


TP EFFECT_WRAPPER::EffectWrapper():
	WrapperBase<T, DataSize, StkType, NumChannels, NV>()
{
	
}

TP void EFFECT_WRAPPER::process(ProcessDataDyn& d)
{
	

#if 0

	constexpr int ChannelAmount = getNumChannelsToProcess<T, NumChannels>();

	auto fd = d.toFrameData<ChannelAmount>();

	while (fd.next())
	{
		processFrame(fd.begin(), ChannelAmount);
	}

	float* ch[ChannelAmount];

	for (int i = 0; i < ChannelAmount; i++)
		ch[i] = d.data[i];


	float frameData[ChannelAmount];

	ProcessData copy(ch, ChannelAmount, d.size);
	copy.allowPointerModification();

	for (int i = 0; i < d.size; i++)
	{
		copy.copyToFrameDynamic(frameData);
		processFrame(frameData, ChannelAmount);
		copy.copyFromFrameAndAdvanceDynamic(frameData);
	}
#endif
}

#if 0
TP void EFFECT_WRAPPER::processFrame(snex::Types::span<float, NumChannels>& data)
{
	//jassert(numChannels == getNumChannelsToProcess<T, NumChannels>() );

	float* ptr = frameData;

	for (int i = 0; i < NumChannels; i++)
	{
		constexpr int NumThisTime = getNumChannelsToProcess<T, 1>();

		for (int j = 0; j < NumThisTime; j++)
		{
            auto value = *ptr;
			*ptr++ = this->objects[i].get().getObject()->tick(value, j);
		}
	}
}
#endif

#undef EFFECT_WRAPPER

#define INSTRUMENT_WRAPPER stk::InstrumentWrapper<T, DataSize, StkType, NumChannels, NV>

TP INSTRUMENT_WRAPPER::InstrumentWrapper()
{

}

TP void INSTRUMENT_WRAPPER::process(ProcessDataDyn& d)
{

#if 0



	constexpr int ChannelAmount = getNumChannelsToProcess<T, NumChannels>();

	auto fd = d.toFrameData<ChannelAmount>();

	while (fd.next())
	{
		processFrame(fd);
	}
	float* ch[ChannelAmount];

	for (int i = 0; i < ChannelAmount; i++)
		ch[i] = d.data[i];


	float frameData[ChannelAmount];
	float* copyPtrs[ChannelAmount];

	memcpy(copyPtrs, d.data, sizeof(float*) * ChannelAmount);

	ProcessData copy(copyPtrs, ChannelAmount, d.size);
	copy.allowPointerModification();

	for (int i = 0; i < d.size; i++)
	{
		copy.copyToFrameDynamic(frameData);
		processFrame(frameData, ChannelAmount);
		copy.copyFromFrameAndAdvanceDynamic(frameData);
	}
#endif
}

#if 0
TP void INSTRUMENT_WRAPPER::processFrame(snex::Types::span<float, NumChannels>& data)
{
	//jassert(numChannels == getNumChannelsToProcess<T, NumChannels>() );

	jassertfalse;

#if 0

	float* ptr = frameData;

	for (int i = 0; i < NumChannels; i++)
	{
		constexpr int numThisTime = getNumChannelsToProcess<T, 1>();

		for (int j = 0; j < numThisTime; j++)
			*ptr++ += this->objects[i].get().getObject()->tick(j);
	}
#endif
}
#endif


TP void INSTRUMENT_WRAPPER::handleHiseEvent(HiseEvent& e)
{
	auto& obj = *this->objects[0].get().getObject();

	if (e.isNoteOn())
	{
		obj.noteOn(e.getFrequency(), e.getFloatVelocity());
	}
	if (e.isNoteOff())
	{
		float r = this->objects[0].get().getRelease();
		obj.noteOff(r);
	}
}

#undef INSTRUMENT_WRAPPER
#undef TP

}
