/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef ANALYSEREFFECT_H_INCLUDED
#define ANALYSEREFFECT_H_INCLUDED

namespace hise {
using namespace juce;


/** A analyser that powers one of the available visualisations in HISE (FFT, Oscilloscope or Goniometer.
	@ingroup effectTypes
*/
class AnalyserEffect : public MasterEffectProcessor
{
public:

	enum Parameters
	{
		PreviewType,
		BufferSize,
		numParameters
	};

	SET_PROCESSOR_NAME("Analyser", "Analyser", "A audio analysis module");

	AnalyserEffect(MainController *mc, const String &uid) :
		MasterEffectProcessor(mc, uid)
	{
		finaliseModChains();

		parameterNames.add("PreviewType"); 
		parameterDescriptions.add("The index of the visualisation type.");
		parameterNames.add("BufferSize");
		parameterDescriptions.add("The buffer size of the internal ring buffer.");

		setAnalyserBufferSize(8192);

	};

	void setInternalAttribute(int index, float newValue)
	{
		switch (index)
		{
		case Parameters::PreviewType: currentType = (int)newValue; break;
		case Parameters::BufferSize: setAnalyserBufferSize((int)newValue); break;
		}
	}

	float getAttribute(int index) const override
	{
		switch (index)
		{
		case Parameters::PreviewType: return (float)currentType;
		case Parameters::BufferSize: return (float)analyseBufferSize;
		default: jassertfalse; return -1;
		}


	}

	~AnalyserEffect()
	{
		
	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);

		loadAttribute(BufferSize, "BufferSize");
		loadAttribute(PreviewType, "PreviewType");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();

		saveAttribute(BufferSize, "BufferSize");
		saveAttribute(PreviewType, "PreviewType");

		return v;
	}

	
	bool hasTail() const override { return false; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	Processor *getChildProcessor(int /*processorIndex*/) override
	{
		return nullptr;
	};

	void setAnalyserBufferSize(int newValue)
	{
		ScopedWriteLock sl(bufferLock);

		jassert(isPowerOfTwo(newValue));

		analyseBufferSize = newValue;
		indexInBuffer = 0;

		internalBuffer.setSize(2, analyseBufferSize);
		internalBuffer.clear();
	}

	const Processor *getChildProcessor(int /*processorIndex*/) const override
	{
		return nullptr;
	};

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
	{


		ScopedReadLock sl(bufferLock);

		auto l = b.getReadPointer(0, startSample);
		auto r = b.getReadPointer(1, startSample);

		const int numToCopy = jmin<int>(internalBuffer.getNumSamples(), numSamples);

		const int numBeforeWrap = jmin<int>(numToCopy, internalBuffer.getNumSamples() - indexInBuffer);
		
		FloatVectorOperations::copy(internalBuffer.getWritePointer(0, indexInBuffer), l, numBeforeWrap);
		FloatVectorOperations::copy(internalBuffer.getWritePointer(1, indexInBuffer), r, numBeforeWrap);
		
		const int numAfterWrap = numToCopy - numBeforeWrap;

		if (numAfterWrap > 0)
		{
			FloatVectorOperations::copy(internalBuffer.getWritePointer(0, 0), l, numAfterWrap);
			FloatVectorOperations::copy(internalBuffer.getWritePointer(1, 0), r, numAfterWrap);
		}

		indexInBuffer = (indexInBuffer + numToCopy) % internalBuffer.getNumSamples();
	}

	AudioSampleBuffer getAnalyseBuffer() const
	{
		return internalBuffer;
	}
	
	int getCurrentReadIndex() const
	{
		return indexInBuffer;
	}

	ReadWriteLock& getBufferLock() { return bufferLock; }

private:

	juce::ReadWriteLock bufferLock;

	int currentType = 1;

	int indexInBuffer = 0;

    int analyseBufferSize = -1;
    
	AudioSampleBuffer internalBuffer;

	JUCE_DECLARE_WEAK_REFERENCEABLE(AnalyserEffect);
};


class AudioAnalyserComponent : public Component,
	public Timer
{
public:

	enum ColourId
	{
		bgColour = 12,
		fillColour,
		lineColour,
		numColourIds
	};

	AudioAnalyserComponent(Processor* p) :
		processor(p)
	{
		setColour(AudioAnalyserComponent::ColourId::bgColour, Colours::transparentBlack);

		startTimer(30);
	};

	Colour getColourForAnalyser(ColourId id);

	void timerCallback() override { repaint(); }

	class Panel : public PanelWithProcessorConnection
	{
	public:

		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{
			setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF333333));
			setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF888888));
			setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colour(0xFF444444));
		};

		SET_PANEL_NAME("AudioAnalyser");

		Identifier getProcessorTypeId() const override
		{
			return AnalyserEffect::getClassType();
		}

		Component* createContentComponent(int index) override;

		void fillModuleList(StringArray& moduleList) override
		{
			fillModuleListWithType<AnalyserEffect>(moduleList);
		}

		bool hasSubIndex() const override { return true; }

		void fillIndexList(StringArray& indexList) override;
	};

protected:

	AnalyserEffect * getAnalyser();
	const AnalyserEffect* getAnalyser() const;

	WeakReference<Processor> processor;

	
};

class FFTDisplay : public AudioAnalyserComponent
{
public:

	FFTDisplay(Processor* p) :
#if USE_IPP
        AudioAnalyserComponent(p),
		fftObject(IppFFT::DataType::RealFloat)
#else
       AudioAnalyserComponent(p)
#endif
	{};

	void paint(Graphics& g) override;

private:

#if USE_IPP
	IppFFT fftObject;
#endif

	Path lPath;
	Path rPath;

	AudioSampleBuffer windowBuffer;
	AudioSampleBuffer fftBuffer;
};

class Oscilloscope : public AudioAnalyserComponent
{
public:

	Oscilloscope(Processor* p) :
		AudioAnalyserComponent(p)
	{};

	void paint(Graphics& g) override;

	Path lPath;
	Path rPath;

	void drawOscilloscope(Graphics &g, const juce::AudioSampleBuffer &buffer);
};

class Goniometer : public AudioAnalyserComponent
{
public:

	Goniometer(Processor* p) :
		AudioAnalyserComponent(p)
	{}

	void paint(Graphics& g) override;

private:

	struct Shape
	{
		Shape() {};

		Shape(const AudioSampleBuffer& buffer, Rectangle<int> size);

		RectangleList<float> points;

		static Point<float> createPointFromSample(float left, float right, float size);

		void draw(Graphics& g, Colour c);
	};

	Shape shapes[6];
	int shapeIndex = 0;

	Path oscilloscopePath;
};


} // namespace hise

#endif  // ANALYSEREFFECT_H_INCLUDED
