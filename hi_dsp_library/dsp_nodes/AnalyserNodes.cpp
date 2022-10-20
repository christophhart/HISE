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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace analyse
{
ui::simple_fft_display::simple_fft_display()
{
	fftProperties.window = FFTDisplayBase::BlackmannHarris;
}

RingBufferComponentBase* Helpers::FFT::createComponent()
{
	return new ui::simple_fft_display();
}


void Helpers::FFT::transformReadBuffer(AudioSampleBuffer& b)
{
	ScopedLock sl(fftLock);

	resizeBuffers(b.getNumSamples());

	//const auto& b = buffer->getReadBuffer();

	int size = b.getNumSamples();
	auto order = log2(size);
	auto fft = juce::dsp::FFT(order);

	AudioSampleBuffer b2(2, size * 2);
	b2.clear();

	auto data = b2.getWritePointer(0);
	FloatVectorOperations::copy(data, b.getReadPointer(0), size);

	FloatVectorOperations::multiply(data, windowBuffer.getReadPointer(0), size);

	fft.performRealOnlyForwardTransform(data, true);

	auto useFreqDomain = true;

	auto d = b.getWritePointer(0);
	int sIndex = 0;

	d[0] = 0.0f;
	d[1] = 0.0f;

	if (useFreqDomain)
	{
		for (int i = 2; i < size; i += 2)
		{
			auto re = data[i];
			auto im = data[i + 1];

			d[sIndex++] = hmath::sqrt(re * re + im * im);
			//data[i] = sqrt(data[i] * data[i] + data[i + 1] * data[i + 1]);
			//data[i + 1] = data[i];
		}
	}
	else
	{
		jassertfalse;
		auto threshhold = FloatVectorOperations::findMaximum(data, size) / 10000.0;

		data[0] = 0.0f;
		data[1] = 0.0f;

		for (int i = 2; i < size; i += 2)
		{
			auto real = data[i];
			auto img = data[i + 1];

			if (real < threshhold) real = 0.0f;
			if (img < threshhold) img = 0.0f;

			auto phase = atan2f(img, real);

			data[i] = phase;
			data[i + 1] = phase;
		}
	}

	FloatVectorOperations::multiply(d, 1.0f / (float)size, size);

	for (int i = 0; i < size; i++)
	{
		auto lastValue = lastBuffer.getSample(0, i);
		auto thisValue = d[i];

		float v;

		if (usePeakDecay)
		{
			if (thisValue > lastValue)
				v = thisValue;
			else
				v = lastValue * decay;
		}
		else
		{
			v = lastValue * decay + thisValue * (1.0f - decay);
		}

		b.setSample(0, i, v);
		lastBuffer.setSample(0, i, v);
	}
	
	FloatVectorOperations::copy(lastBuffer.getWritePointer(0, 0), b.getReadPointer(0, 0), size);
}


juce::Path Helpers::FFT::createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double) const
{
	ScopedLock sl(fftLock);

	Path lPath;

	auto data = buffer->getReadBuffer().getReadPointer(0);
	int size = buffer->getReadBuffer().getNumSamples();

	int stride = roundToInt((float)size / targetBounds.getWidth());
	stride *= 2;

	lPath.clear();

	lPath.startNewSubPath(0.0f, targetBounds.getY());
	lPath.startNewSubPath(0.0f, targetBounds.getHeight());

    auto cpy = (float*)alloca(sizeof(float)*size);

    FloatVectorOperations::copy(cpy, data, size);
    data = cpy;
    
	auto sampleRate = buffer->getSamplerate();

	if (sampleRate <= 0.0)
		sampleRate = 44100.0;

	

	for (float xPos = targetBounds.getX(); xPos < targetBounds.getWidth(); xPos += 2.0f)
	{
		auto leftFreq = FFTHelpers::getFreqForLogX(xPos, targetBounds.getWidth());
		auto rightFreq = FFTHelpers::getFreqForLogX(xPos + 2.0f, targetBounds.getWidth());

		auto leftNorm = leftFreq / (sampleRate);
		auto rightNorm = rightFreq / (sampleRate);

		auto leftBinIndex = roundToInt(leftNorm * (float)size);
		auto rightBinIndex = jmax(leftBinIndex + 1, roundToInt(rightNorm * (float)size));

		float maxValue = 0.0f;

		for (int i = leftBinIndex; i < rightBinIndex; i++)
		{
			maxValue = jmax(maxValue, data[i]);
		}

		if (!useDb)
			maxValue = maxValue;
		else if (!dbRange.isEmpty())
		{
			maxValue = Decibels::gainToDecibels(maxValue);
			maxValue = jlimit<float>(dbRange.getStart(), dbRange.getEnd(), maxValue);
			maxValue -= dbRange.getStart();
			maxValue /= dbRange.getLength();
		}

		auto yPos = std::pow(maxValue, yGamma);// lastValues[i];
		yPos = 1.0f - yPos;

		yPos *= targetBounds.getHeight();

		lPath.lineTo(xPos, yPos);
	}

	lPath.lineTo(targetBounds.getWidth(), targetBounds.getHeight());
	lPath.closeSubPath();

	return lPath;
}



juce::Path Helpers::Oscilloscope::createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double startValue) const
{
	bool isStereo = buffer->getReadBuffer().getNumChannels() == 2;

	Path p;
	
	if (isStereo)
	{
		Path l;
		Path r;
		
		auto top = targetBounds.removeFromTop(targetBounds.getHeight() / 2.0f).reduced(2.0f);
		auto bottom = targetBounds.reduced(2.0f);

		drawPath(l, top, 0);
		drawPath(r, bottom, 1);

		p.addPath(l);
		p.addPath(r);
	}
	else
	{
		drawPath(p, targetBounds.reduced(2.0f), 0);
	}
	
	return p;
}

RingBufferComponentBase* Helpers::Oscilloscope::createComponent()
{
	return new ui::simple_osc_display();
}

void Helpers::Oscilloscope::drawPath(Path& p, Rectangle<float> bounds, int channelIndex) const
{
	int numSamples = buffer->getReadBuffer().getNumSamples();
	auto width = (int)bounds.getWidth();

	auto l_ = buffer->getReadBuffer().getReadPointer(channelIndex);

	int stride = roundToInt((float)numSamples / width);
	stride = jmax<int>(1, stride * 2);

	if (numSamples != 0)
	{
		p.clear();

		p.startNewSubPath(0.0f, 1.0f);
		p.startNewSubPath(0.0f, -1.0f);
		p.startNewSubPath(0.0f, 0.0f);

		float x = 0.0f;

		bool mirrorMode = stride > 100;

		for (int i = 0; i < numSamples; i += stride)
		{
			const int numToCheck = jmin<int>(stride, numSamples - i);

			auto value = FloatVectorOperations::findMaximum(l_ + i, numToCheck);

			if (mirrorMode)
				value = jmax(0.0f, value);

			x = (float)i;
			p.lineTo(x, -1.0f * value);
		};

		if (mirrorMode)
		{
			for (int i = numSamples - 1; i >= 0; i -= stride)
			{
				const int numToCheck = jmin<int>(stride, numSamples - i);

				auto value = jmin<float>(0.0f, FloatVectorOperations::findMinimum(l_ + i, numToCheck));

				x = (float)i;

				p.lineTo(x, -1.0f * value);
			};

			p.lineTo(x, 0.0f);
		}
		else
		{
			p.lineTo(x, 0.0f);
		}

		p.scaleToFit(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false);
		p.scaleToFit(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false);
	}
	else
	{
		p.clear();
	}


}

RingBufferComponentBase* Helpers::GonioMeter::createComponent()
{
	return new ui::simple_gon_display();
}
}





}
