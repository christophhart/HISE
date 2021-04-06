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


SimpleRingBuffer::SimpleRingBuffer()
{
	getUpdater().addEventListener(this);

	setRingBufferSize(1, RingBufferSize);
	
	clear();
}

void SimpleRingBuffer::clear()
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getDataLock()))
	{
		internalBuffer.clear();
		numAvailable = 0;
		writeIndex = 0;
		updateCounter = 0;
	}
}

int SimpleRingBuffer::read(AudioSampleBuffer& b)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getDataLock()))
	{
		while (isBeingWritten.load()) // busy wait, but OK
			;

		int thisWriteIndex = writeIndex.load();
		int numBeforeIndex = thisWriteIndex;
		int offsetBeforeIndex = internalBuffer.getNumSamples() - numBeforeIndex;

		jassert(b.getNumSamples() == internalBuffer.getNumSamples() && b.getNumChannels() == internalBuffer.getNumChannels());

		int numSamples = b.getNumSamples();
		int numChannels = b.getNumChannels();

		for (int i = 0; i < numChannels; i++)
		{
			auto buffer = internalBuffer.getReadPointer(i);
			auto dst = b.getWritePointer(i);

			FloatVectorOperations::copy(dst + offsetBeforeIndex, buffer, numBeforeIndex);
			FloatVectorOperations::copy(dst, buffer + thisWriteIndex, offsetBeforeIndex);
			FloatSanitizers::sanitizeArray(dst, numSamples);
		}

		int numToReturn = numAvailable;
		numAvailable = 0;
		return numToReturn;
	}
}

void SimpleRingBuffer::write(double value, int numSamples)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getDataLock()))
	{
		if (numSamples == 1)
		{
			isBeingWritten = true;

			for (int i = 0; i < internalBuffer.getNumChannels(); i++)
				internalBuffer.setSample(i, writeIndex, (float)value);

			if (++writeIndex >= internalBuffer.getNumSamples())
				writeIndex.store(0);

			++numAvailable;

			isBeingWritten = false;

			if (updateCounter++ >= 1024)
			{
				getUpdater().sendDisplayChangeMessage((int)numAvailable, sendNotificationAsync, true);
				updateCounter = 0;
			}
		}
		else
		{
			isBeingWritten = true;

			int numBeforeWrap = jmin(numSamples, internalBuffer.getNumSamples() - writeIndex);
			int numAfterWrap = numSamples - numBeforeWrap;

			if (numBeforeWrap > 0)
			{
				for (int i = 0; i < internalBuffer.getNumChannels(); i++)
				{
					auto buffer = internalBuffer.getWritePointer(i);
					FloatVectorOperations::fill(buffer + writeIndex, (float)value, numBeforeWrap);
				}
			}

			writeIndex += numBeforeWrap;

			if (numAfterWrap > 0)
			{
				for (int i = 0; i < internalBuffer.getNumChannels(); i++)
				{
					auto buffer = internalBuffer.getWritePointer(i);
					FloatVectorOperations::fill(buffer, (float)value, numAfterWrap);
				}

				writeIndex = numAfterWrap;
			}

			numAvailable += numSamples;
			isBeingWritten = false;

			getUpdater().sendDisplayChangeMessage((int)numAvailable, sendNotificationAsync, true);
		}
	}
}

void SimpleRingBuffer::write(const float** data, int numChannels, int numSamples)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getDataLock()))
	{
		isBeingWritten = true;

		jassert(numChannels == internalBuffer.getNumChannels());

		int numBeforeWrap = jmin(numSamples, internalBuffer.getNumSamples() - writeIndex);
		int numAfterWrap = numSamples - numBeforeWrap;

		if (numBeforeWrap > 0)
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto buffer = internalBuffer.getWritePointer(i);
				FloatVectorOperations::copy(buffer + writeIndex, data[i], numBeforeWrap);
			}
		}

		writeIndex += numBeforeWrap;

		if (numAfterWrap > 0)
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto buffer = internalBuffer.getWritePointer(i);
				FloatVectorOperations::copy(buffer, data[i], numAfterWrap);
			}

			writeIndex = numAfterWrap;
		}

		numAvailable += numSamples;
		isBeingWritten = false;

		getUpdater().sendDisplayChangeMessage((int)numAvailable, sendNotificationAsync, true);
	}
}

void SimpleRingBuffer::write(const AudioSampleBuffer& b, int startSample, int numSamples)
{
	if (startSample != 0)
	{
		auto numBytes = sizeof(float*)*b.getNumChannels();
		const float** channels = (const float**)alloca(numBytes);

		memcpy(channels, b.getArrayOfReadPointers(), numBytes);
		for (int i = 0; i < b.getNumChannels(); i++)
			channels[i] += startSample;

		write(channels, b.getNumChannels(), b.getNumSamples());
	}

	write(b.getArrayOfReadPointers(), b.getNumChannels(), b.getNumSamples());
}

void SimpleRingBuffer::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var n)
{
	if(t == ComplexDataUIUpdaterBase::EventType::ContentRedirected)
		setupReadBuffer(externalBuffer);
	else
	{
		read(externalBuffer);

		if (transformFunction && getReferenceCount() > 1)
		{
			Ptr p(this);
			jassert(p->getReferenceCount() > 1);
			transformFunction(p);
		}
	}
}

ModPlotter::ModPlotter()
{
	setOpaque(true);
	setInterceptsMouseClicks(false, false);
}

void ModPlotter::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF313131));
	g.setColour(Colour(0xFF717171));
	g.drawRect(getLocalBounds(), 1);
	g.setColour(Colour(0xFF999999));
	g.fillRectList(rectangles);
}

int ModPlotter::getSamplesPerPixel(float rectangleWidth) const
{
	float offset = 2.0f;

	auto width = (float)getWidth() - 2.0f * offset;

	int samplesPerPixel = SimpleRingBuffer::RingBufferSize / jmax((int)(width / rectangleWidth), 1);
	return samplesPerPixel;
}

void RingBufferComponentBase::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType e, var newValue)
{
	refresh();
}

void RingBufferComponentBase::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	if (rb != nullptr)
		rb->getUpdater().removeEventListener(this);

	rb = dynamic_cast<SimpleRingBuffer*>(newData);

	if (rb != nullptr)
	{
		rb->getUpdater().addEventListener(this);
	}
}

void ModPlotter::refresh()
{
	if (rb != nullptr)
	{
		jassert(getSamplesPerPixel(1.0f) > 0);
		
		float offset = 2.0f;

		float rectangleWidth = 1.0f;

		rectangleWidth = std::ceil((float)getWidth() / 256.0f);
		rectangleWidth = jmax(1.0f, rectangleWidth);

		auto width = (float)getWidth() - 2.0f * offset;
		auto maxHeight = (float)getHeight() - 2.0f * offset;

		int samplesPerPixel = getSamplesPerPixel(rectangleWidth);
		rectangles.clear();
		int sampleIndex = 0;
		const auto& buffer = rb->getReadBuffer();

		for (float i = 0.0f; i < width; i += rectangleWidth)
		{
			auto numThisTime = jmin(samplesPerPixel, buffer.getNumSamples() - sampleIndex);

			float maxValue = jlimit(0.0f, 1.0f, buffer.getMagnitude(0, sampleIndex, numThisTime));
			FloatSanitizers::sanitizeFloatNumber(maxValue);
			float height = maxValue * maxHeight;
			float y = offset + maxHeight - height;

			sampleIndex += samplesPerPixel;

			rectangles.add({ i + offset, y, rectangleWidth, height });
		}

		repaint();
	}
}



} // namespace hise