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

namespace hise { using namespace juce;

//==============================================================================
EqComponent::EqComponent(ModulatorSynth *ownerSynth)
{
    setSize (500, 400);
	
	f = new FilterGraph(3);
	f->addFilter(FilterType::LowPass);
	

	addAndMakeVisible(f);
	f->setBounds(2,0, getWidth()-5, 150);

	addAndMakeVisible (gainSlider = new Slider ("Gain 1"));
    gainSlider->setRange (-18, 18, 0.1);
	gainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gainSlider->addListener (this);
	gainSlider->setTextValueSuffix(" dB");
	ownerSynth->getMainController()->skin(*gainSlider);

    addAndMakeVisible (freqSlider = new Slider ("Freq 1"));
    freqSlider->setRange (20, 20000, 1);
    freqSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    freqSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    freqSlider->addListener (this);
	ownerSynth->getMainController()->skin(*freqSlider);
	freqSlider->setTextValueSuffix(" Hz");
	freqSlider->setSkewFactorFromMidPoint(1000.0);

    addAndMakeVisible (qSlider = new Slider ("Q 1"));
    qSlider->setRange (0.1, 5, 0.1);
    qSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    qSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    qSlider->addListener (this);
	ownerSynth->getMainController()->skin(*qSlider);
	qSlider->setSkewFactorFromMidPoint(1.0);

	gainSlider->setBounds (40, 168, 128, 48);
	freqSlider->setBounds (gainSlider->getRight(), 168, 128, 48);
    qSlider->setBounds (freqSlider->getRight(), 168, 128, 48);

}

void EqComponent::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
		//f->setEqBand(0, 44100.0, freqSlider->getValue(), qSlider->getValue(), (float)gainSlider->getValue(), BandType::HighShelf);
        //[/UserSliderCode_gainSlider]
    }
    else if (sliderThatWasMoved == freqSlider)
    {
		//f->setEqBand(0, 44100.0, freqSlider->getValue(), qSlider->getValue(), (float)gainSlider->getValue(), BandType::HighShelf);
        //[UserSliderCode_freqSlider] -- add your slider handling code here..

		f->setFilter(0, 44100, freqSlider->getValue(), FilterType::LowPass);

        //[/UserSliderCode_freqSlider]
    }
    else if (sliderThatWasMoved == qSlider)
    {
        //[UserSliderCode_qSlider] -- add your slider handling code here..

		

		//f->setEqBand(0, 44100.0, freqSlider->getValue(), qSlider->getValue(), (float)gainSlider->getValue(), BandType::HighShelf);
        //[/UserSliderCode_qSlider]
    }

	

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

EqComponent::~EqComponent()
{
}

void EqComponent::paint (Graphics& )
{
  
}

void EqComponent::resized()
{
    // This is called when the EqComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void FFTDisplayBase::drawSpectrum(Graphics& g)
{
	g.fillAll(getColourForAnalyserBase(AudioAnalyserComponent::bgColour));

#if USE_IPP

	if (auto l_ = SingleWriteLockfreeMutex::ScopedReadLock(ringBuffer.lock))
	{
		const auto& b = ringBuffer.internalBuffer;

		int size = b.getNumSamples();

		if (windowBuffer.getNumSamples() == 0 || windowBuffer.getNumSamples() != size)
		{
			windowBuffer = AudioSampleBuffer(1, size);
			fftBuffer = AudioSampleBuffer(1, size);

			fftBuffer.clear();

			icstdsp::VectorFunctions::blackman(windowBuffer.getWritePointer(0), size);
		}

		AudioSampleBuffer b2(2, b.getNumSamples());

		auto data = b2.getWritePointer(0);
		auto lastValues = fftBuffer.getWritePointer(0);

		auto readIndex = ringBuffer.indexInBuffer;

		int numBeforeWrap = size - readIndex;
		int numAfterWrap = size - numBeforeWrap;

		FloatVectorOperations::copy(data, b.getReadPointer(0, readIndex), numBeforeWrap);
		FloatVectorOperations::copy(data + numBeforeWrap, b.getReadPointer(0, 0), numAfterWrap);

		FloatVectorOperations::add(data, b.getReadPointer(1, readIndex), numBeforeWrap);
		FloatVectorOperations::add(data + numBeforeWrap, b.getReadPointer(1, 0), numAfterWrap);

		FloatVectorOperations::multiply(data, 0.5f, size);
		FloatVectorOperations::multiply(data, windowBuffer.getReadPointer(0), size);

		auto sampleRate = getSamplerate();

		fftObject.realFFTInplace(data, size);

		for (int i = 2; i < size; i += 2)
		{
			data[i] = sqrt(data[i] * data[i] + data[i + 1] * data[i + 1]);
			data[i + 1] = data[i];
		}

		//fftObject.realFFT(b.getReadPointer(1), b2.getWritePointer(1), b2.getNumSamples());

		FloatVectorOperations::abs(data, b2.getReadPointer(0), size);
		FloatVectorOperations::multiply(data, 1.0f / 95.0f, size);

		auto asComponent = dynamic_cast<Component*>(this);

		int stride = roundToInt((float)size / asComponent->getWidth());
		stride *= 2;

		lPath.clear();

		lPath.startNewSubPath(0.0f, (float)asComponent->getHeight());
		//lPath.lineTo(0.0f, -1.0f);


		int log10Offset = (int)(10.0 / (sampleRate * 0.5) * (double)size + 1.0);

		auto maxPos = log10f((float)(size));

		float lastIndex = 0.0f;
		float value = 0.0f;
		int lastI = 0;
		int sumAmount = 0;

		int lastLineLog = 1;

		g.setColour(getColourForAnalyserBase(AudioAnalyserComponent::lineColour));

		int xLog10Pos = roundToInt(log10((float)log10Offset) / maxPos * (float)asComponent->getWidth());

		for (int i = log10Offset; i < size; i += 2)
		{
			auto f = (double)i / (double)size * sampleRate / 2.0;

			auto thisLineLog = (int)log10(f);

			if (thisLineLog == 0)
				continue;


			float xPos;
			
			if (freqToXFunction)
				xPos = freqToXFunction(f);
			else
				xPos = log10((float)i) / maxPos * (float)(asComponent->getWidth() + xLog10Pos) - xLog10Pos;

			auto diff = xPos - lastIndex;

			if ((int)thisLineLog != lastLineLog)
			{
				g.drawVerticalLine((int)xPos, 0.0f, (float)asComponent->getHeight());

				lastLineLog = (int)thisLineLog;
			}

			auto indexDiff = i - lastI;

			float v = fabsf(data[i]);

			v = Decibels::gainToDecibels(v);
			v = jlimit<float>(-70.0f, 0.0f, v);
			v = 1.0f + v / 70.0f;
			v = powf(v, 0.707f);

			value += v;
			sumAmount++;

			if (diff > 1.0f && indexDiff > 4)
			{
				value /= (float)(sumAmount);

				sumAmount = 0;

				lastIndex = xPos;
				lastI = i;

				value = 0.6f * value + 0.4f * lastValues[i];

				if (value > lastValues[i])
					lastValues[i] = value;
				else
					lastValues[i] = jmax<float>(0.0f, lastValues[i] - 0.02f);

				auto yPos = lastValues[i];
				yPos = 1.0f - yPos;

				yPos *= asComponent->getHeight();

				lPath.lineTo(xPos, yPos);

				value = 0.0f;
			}
		}

		lPath.lineTo((float)asComponent->getWidth(), (float)asComponent->getHeight());
		lPath.closeSubPath();

		g.setColour(getColourForAnalyserBase(AudioAnalyserComponent::fillColour));
		g.fillPath(lPath);
	}

#else

	auto asComponent = dynamic_cast<Component*>(this);
	g.setColour(Colours::grey);
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("You need IPP for the FFT Analyser", asComponent->getLocalBounds().toFloat(), Justification::centred, false);

#endif
}

void OscilloscopeBase::drawWaveform(Graphics& g)
{
	if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(ringBuffer.lock))
	{
		g.fillAll(getColourForAnalyserBase(AudioAnalyserComponent::bgColour));

		g.setColour(getColourForAnalyserBase(AudioAnalyserComponent::fillColour));

		drawOscilloscope(g, ringBuffer.internalBuffer);
	}
}

} // namespace hise;