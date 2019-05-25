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

#pragma once;

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace core
{
struct fix_delay : public HiseDspBase
{
	SET_HISE_NODE_ID("fix_delay");
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	fix_delay() {};

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		if (delayLines.size() != numChannels)
		{
			delayLines.clear();

			for (int i = 0; i < numChannels; i++)
				delayLines.add(new DelayLine<>());
		}

		reset();

		for (auto d : delayLines)
			d->prepareToPlay(sampleRate);

		setDelayTimeMilliseconds(delayTimeSeconds * 1000.0);
	}

	bool handleModulation(ProcessData& d, double& data) noexcept { return false; };

	void reset() noexcept
	{
		for (auto d : delayLines)
			d->clear();
	}

	void process(ProcessData& d) noexcept
	{
		d.size == delayLines.size();

		for (int i = 0; i < delayLines.size(); i++)
		{
			delayLines[i]->processBlock(d.data[i], d.size);
		}
	}

	void processSingle(float* numFrames, int numChannels) noexcept
	{
		for (int i = 0; i < numChannels; i++)
			numFrames[i] = delayLines[i]->getDelayedValue(numFrames[i]);
	}

	void setDelayTimeMilliseconds(double newValue)
	{
		delayTimeSeconds = newValue * 0.001;

		for (auto d : delayLines)
			d->setDelayTimeSeconds(delayTimeSeconds);
	}

	void setFadeTimeMilliseconds(double newValue)
	{
		for (auto d : delayLines)
			d->setFadeTimeSamples((int)newValue);
	}

	void createParameters(Array<ParameterData>& data) override;

	OwnedArray<DelayLine<>> delayLines;
	
	double delayTimeSeconds = 0.1;
};
}



}