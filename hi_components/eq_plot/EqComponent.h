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

#ifndef __MAINCOMPONENT_H_5C0D5756__
#define __MAINCOMPONENT_H_5C0D5756__

namespace hise { using namespace juce;

class ModulatorSynth;

struct AnalyserRingBuffer: public AsyncUpdater
{
	void handleAsyncUpdate()
	{
		if (auto l = SingleWriteLockfreeMutex::ScopedWriteLock(lock))
		{
			analyseBufferSize = currentSize;
			indexInBuffer = 0;

			internalBuffer.setSize(2, analyseBufferSize);
			internalBuffer.clear();
		}
		else
			triggerAsyncUpdate();
	}

	bool isActive() const
	{
		return currentSize != 0;
	}

	void setAnalyserBufferSize(int newValue)
	{
		jassert(isPowerOfTwo(newValue));

		currentSize = newValue;

		handleAsyncUpdate();
	}

	void pushSamples(AudioSampleBuffer& b, int startSample, int numSamples)
	{
		if (internalBuffer.getNumSamples() == 0)
			return;

		if (auto sl = SingleWriteLockfreeMutex::ScopedReadLock(lock))
		{
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
	}

	mutable SingleWriteLockfreeMutex lock;

	int indexInBuffer = 0;
	int currentSize = 0;
	int analyseBufferSize = -1;

	AudioSampleBuffer internalBuffer;
};

class OscilloscopeBase
{
protected:

	OscilloscopeBase(const AnalyserRingBuffer& ringBuffer_) :
		ringBuffer(ringBuffer_)
	{};

    virtual ~OscilloscopeBase() {};
    
	virtual Colour getColourForAnalyserBase(int colourId) = 0;

	void drawWaveform(Graphics& g);

private:

	const AnalyserRingBuffer& ringBuffer;

	void drawPath(const float* l_, int numSamples, int width, Path& p)
	{
		int stride = roundToInt((float)numSamples / width);
		stride = jmax<int>(1, stride * 2);

		if (numSamples != 0)
		{
			p.clear();
			p.startNewSubPath(0.0f, 0.0f);
			p.lineTo(0.0f, 1.0f);
			p.lineTo(0.0f, -1.0f);

			for (int i = stride; i < numSamples; i += stride)
			{
				const int numToCheck = jmin<int>(stride, numSamples - i);

				auto value = jmax<float>(0.0f, FloatVectorOperations::findMaximum(l_ + i, numToCheck));

				p.lineTo((float)i, -1.0f * value);

			};

			for (int i = numSamples - 1; i > 0; i -= stride)
			{
				const int numToCheck = jmin<int>(stride, numSamples - i);

				auto value = jmin<float>(0.0f, FloatVectorOperations::findMinimum(l_ + i, numToCheck));

				p.lineTo((float)i, -1.0f * value);
			};

			p.closeSubPath();
		}
		else
		{
			p.clear();
		}
	}

	void drawOscilloscope(Graphics &g, const AudioSampleBuffer &b)
	{
		AudioSampleBuffer b2(2, b.getNumSamples());

		auto dataL = b2.getWritePointer(0);
		auto dataR = b2.getWritePointer(1);
		int size = b.getNumSamples();

		auto readIndex = ringBuffer.indexInBuffer;

		int numBeforeWrap = size - readIndex;
		int numAfterWrap = size - numBeforeWrap;

		FloatVectorOperations::copy(dataL, b.getReadPointer(0, readIndex), numBeforeWrap);
		FloatVectorOperations::copy(dataL + numBeforeWrap, b.getReadPointer(0, 0), numAfterWrap);

		FloatVectorOperations::copy(dataR, b.getReadPointer(1, readIndex), numBeforeWrap);
		FloatVectorOperations::copy(dataR + numBeforeWrap, b.getReadPointer(1, 0), numAfterWrap);

		auto asComponent = dynamic_cast<Component*>(this);

		drawPath(dataL, b.getNumSamples(), asComponent->getWidth(), lPath);
		drawPath(dataR, b.getNumSamples(), asComponent->getWidth(), rPath);

		lPath.scaleToFit(0.0f, 0.0f, (float)asComponent->getWidth(), (float)(asComponent->getHeight() / 2), false);
		rPath.scaleToFit(0.0f, (float)(asComponent->getHeight() / 2), (float)asComponent->getWidth(), (float)(asComponent->getHeight() / 2), false);

		g.fillPath(lPath);
		g.fillPath(rPath);
	}

	Path lPath;
	Path rPath;


};

class FFTDisplayBase
{
protected:

	FFTDisplayBase(const AnalyserRingBuffer& ringBuffer_):
		ringBuffer(ringBuffer_)
#if USE_IPP
		, fftObject(IppFFT::DataType::RealFloat)
#endif
	{}

#if USE_IPP
	IppFFT fftObject;
#endif

	virtual Colour getColourForAnalyserBase(int colourId) = 0;
	virtual double getSamplerate() const = 0;

    virtual ~FFTDisplayBase() {};
    
	
	

	void drawSpectrum(Graphics& g);

	const AnalyserRingBuffer& ringBuffer;

	Path lPath;
	Path rPath;

	std::function<float(float)> freqToXFunction;
	

	AudioSampleBuffer windowBuffer;
	AudioSampleBuffer fftBuffer;
};

//==============================================================================
class EqComponent   : public Component,
							   public SliderListener
{
public:
    //==============================================================================
    EqComponent(ModulatorSynth *ownerSynth);
    ~EqComponent();

	void sliderValueChanged (Slider* sliderThatWasMoved);
    void paint (Graphics&);
    void resized();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqComponent)

	ScopedPointer<FilterGraph> f;
	ScopedPointer<Slider> gainSlider;
    ScopedPointer<Slider> freqSlider;
    ScopedPointer<Slider> qSlider;

	WeakReference<ModulatorSynth> owner;

};

} // namespace hise;
#endif  // __MAINCOMPONENT_H_5C0D5756__
