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
	resizeBuffers(b.getNumSamples());

	int size = removeOverlap(b.getNumSamples());

	auto delta = roundToInt(overlap * size);

	auto order = log2(size);
	auto fft = juce::dsp::FFT(order);

	AudioSampleBuffer b2(2, size * 2);

	for(int offset = 0; offset < b.getNumSamples() - (size-1); offset += delta)
	{
		b2.clear();

		auto data = b2.getWritePointer(0);
		FloatVectorOperations::copy(data, b.getReadPointer(0, offset), size);

		FloatVectorOperations::multiply(data, windowBuffer.getReadPointer(0), size);

		fft.performRealOnlyForwardTransform(data, true);

		auto useFreqDomain = true;

		auto d = b2.getWritePointer(1);
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

		auto decayToUse = decay;

		if(overlap != 0.0)
		{
			auto of = 1.0 / (1.0 - overlap);

			decayToUse = 1.0 - (1.0 - decay) / of;
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
					v = lastValue * decayToUse;
			}
			else
			{
				v = lastValue * decayToUse + thisValue * (1.0f - decayToUse);
			}

			
			lastBuffer.setSample(0, i, v);
		}
		
		

		if(delta == 0)
			break;
	}

	FloatVectorOperations::copy(b.getWritePointer(0, 0), lastBuffer.getWritePointer(0, 0), size);
}


juce::Path Helpers::FFT::createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double) const
{
    Path lPath;

	auto data = buffer->getReadBuffer().getReadPointer(0);
	int size = removeOverlap(buffer->getReadBuffer().getNumSamples());

	int stride = roundToInt((float)size / targetBounds.getWidth());
	stride *= 2;
	
	lPath.startNewSubPath(targetBounds.getX(), targetBounds.getY());
	lPath.startNewSubPath(targetBounds.getX(), targetBounds.getHeight());

    auto cpy = (float*)alloca(sizeof(float)*size);

    {
        ScopedLock sl(buffer->getReadBufferLock());
        FloatVectorOperations::copy(cpy, data, size);
        data = cpy;
    }
    
    
	auto sampleRate = buffer->getSamplerate();

	if (sampleRate <= 0.0)
		sampleRate = 44100.0;

	auto getYValue = [&](double maxValue)
	{
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

		return yPos;
	};

	auto getPixelRangeForBin = [&](int binIndex)
	{
		auto leftFreq = jmax(0.0, sampleRate * (double)(binIndex) / (double)size);
		auto rightFreq = sampleRate * (double)(binIndex + 1.0) / (double)size;

		auto lPos = FFTHelpers::getPixelValueForLogXAxis(leftFreq, targetBounds.getWidth() + targetBounds.getX());
		auto rPos = jmax(lPos + 1.0f, (FFTHelpers::getPixelValueForLogXAxis(rightFreq, targetBounds.getWidth() + targetBounds.getX())));

		return Range<float>(lPos, rPos);
	};

	lPath.preallocateSpace(5 * size-1);

	Array<Point<float>> dataPoints;

	dataPoints.ensureStorageAllocated(size);

	for(int i = 0; i < size; i++)
	{
		auto pr = getPixelRangeForBin(i);
		auto xPos = pr.getStart() + pr.getLength() * 0.5f;
		auto yPos = (float)getYValue(data[i]);

		FloatSanitizers::sanitizeFloatNumber(xPos);
		FloatSanitizers::sanitizeFloatNumber(yPos);

		dataPoints.add({jmax(0.0f, xPos), yPos});
	}

	auto lastX = -1000.0f;
	auto lastRealIndex = 0;

	

	for(int i = 0; i < dataPoints.size(); i++)
	{
		auto x = dataPoints[i].getX();

		if(x == 0.0f && dataPoints[i+1].getX() == 0.0f)
		{
			dataPoints.remove(i--);
			continue;
		}
		
		if(x - lastX < JUCE_LIVE_CONSTANT_OFF(2))
		{
			auto thisY = dataPoints[i].getY();
			auto lastRealY = dataPoints[lastRealIndex].getY();
			dataPoints.getReference(lastRealIndex).setY(jmin(thisY, lastRealY)); // jmin because of pixel domain
			dataPoints.remove(i--);
		}
		else
		{
			lastX = x;
			lastRealIndex = i;
		}
	}

	auto tolerance = JUCE_LIVE_CONSTANT_OFF(0.4f);

	for(int i = 1; i < dataPoints.size() -2; i++)
	{
		auto prev = dataPoints[i-1];
		auto current = dataPoints[i];

		if(current.getY() == targetBounds.getBottom())
			continue;

		auto next = dataPoints[i+1];

		auto midY = (prev.getY() + next.getY()) * 0.5f;

		auto deltaMax = jmax(1.0f, hmath::abs(prev.getY() - next.getY()));
		 
		auto deltaY = hmath::abs(current.getY() - midY);

		auto deltaNorm = deltaY / deltaMax;

		if(deltaNorm < tolerance)
		{
			dataPoints.remove(i--);
		}
	}


	for(int i = dataPoints.size() - 1; i >= 0.0; i--)
	{
		if(dataPoints[i].getX() > targetBounds.getRight())
			dataPoints.remove(i);
		else
			break;
	}

	if(dataPoints.isEmpty())
	{
		lPath.startNewSubPath(targetBounds.getX(), targetBounds.getBottom());
	}
	else
	{
		lPath.startNewSubPath(targetBounds.getX(), targetBounds.getBottom());
		lPath.lineTo(dataPoints[0]);

		for(int i = 1; i < dataPoints.size() - 1; i++)
		{
			auto prev = dataPoints[i-1];
			auto current = dataPoints[i];
			auto deltaX = current.getX() - prev.getX();
			auto deltaY = current.getY() - prev.getY();
			auto useCubic = hmath::abs(deltaY) > JUCE_LIVE_CONSTANT_OFF(10);

			if(useCubic)
				lPath.cubicTo(prev.translated(deltaX / 3.0f, 0.0f), current.translated(deltaX / -3.0f, 0.0f), current);
			else
				lPath.lineTo(current);
		}

		lPath.lineTo(targetBounds.getRight(), dataPoints.getLast().getY());
	}
	
	lPath.lineTo(targetBounds.getRight(), targetBounds.getBottom());
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
	int numSamples = buffer->getMaxLengthInSamples();// buffer->getReadBuffer().getNumSamples();

	auto isUsingMaxLength = numSamples != buffer->getReadBuffer().getNumSamples();

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

		bool mirrorMode = !isUsingMaxLength && stride > 100;

		for (int i = 0; i < numSamples; i += stride)
		{
			auto pos = i;

			const int numToCheck = jmin<int>(stride, numSamples - pos);

			auto value = FloatVectorOperations::findMaximum(l_ + pos, numToCheck);

			FloatSanitizers::sanitizeFloatNumber(value);

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
