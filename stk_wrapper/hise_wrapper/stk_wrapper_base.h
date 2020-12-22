
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

template <class StkType, size_t DataSize> struct SampleRateWrapper
{
	SampleRateWrapper();

	~SampleRateWrapper()
	{
		deleteObject();
	}

	void deleteObject();

	void setNewWithSampleRate(StkType* newObject, double newSampleRate)
	{
		deleteObject();

		sampleRate = newSampleRate;

		object = newObject;
		object->ignoreSampleRateChange();
	}

	StkType* getObject() { return object; }

	void* getDataPtr() { return data; }

	double getSampleRate() const { return sampleRate; }

	void setRelease(float newRelease)
	{
		release = newRelease;
	}

	float getRelease() const { return release; }

private:

	float release = 0.0f;

	uint8 data[DataSize];

	double sampleRate = 0.0;
	StkType* object = nullptr;
};



template <class T, size_t DataSize, class StkType, int NumChannels, int NV> class WrapperBase: public HiseDspBase
{
public:

	static constexpr int NumVoices = NV;
	using ObjectType = SampleRateWrapper<StkType, DataSize>;
	using PolyObjectType = PolyData<ObjectType, NumVoices>;

	SET_HISE_POLY_NODE_ID(T::getId());
	SN_GET_SELF_AS_OBJECT(WrapperBase);

	WrapperBase();

	void clearObjects(bool deleteObjects=true);

	PolyObjectType* begin() const
	{
		return const_cast<PolyObjectType*>(objects);
	}

	PolyObjectType* end() const
	{
		return begin() + NumChannels;
	}

	void createParameters(ParameterDataList& data) override;
	void reset();
	void prepare(PrepareSpecs ps);

	void setParameter0(void* obj, double newValue);
	void setParameter1(void* obj, double newValue);
	void setParameter2(void* obj, double newValue);
	void setParameter3(void* obj, double newValue);
	
	
	bool handleModulation(double&) { return false; }
	

protected:
	
	double paramValues[8];

	bool voiceRenderingActive() const;

	int unused = -1;
	double sr = 0.0;
	PolyObjectType objects[NumChannels];
};


template <class T, size_t DataSize, class StkType, int NumChannels, int NV> 
class EffectWrapper : public WrapperBase<T, DataSize, StkType, NumChannels, NV>
{
public:

	EffectWrapper();

	void process(ProcessDataDyn& d);
	//void processFrame(FrameType& data);
	void handleHiseEvent(HiseEvent& ) { }
};

template <class T, size_t DataSize, class StkType, int NumChannels, int NV>
class InstrumentWrapper : public WrapperBase<T, DataSize, StkType, NumChannels, NV>
{
public:

	InstrumentWrapper();

	void process(ProcessDataDyn& d);
	//void processFrame(FrameType& d);
	void handleHiseEvent(HiseEvent& e);
};



}
