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

namespace core
{

void fix_delay::prepare(PrepareSpecs ps)
{
	if (delayLines.size() != ps.numChannels)
	{
		delayLines.clear();

		for (int i = 0; i < ps.numChannels; i++)
			delayLines.add(new DelayLine<>());
	}

	reset();

	for (auto d : delayLines)
		d->prepareToPlay(ps.sampleRate);

	setDelayTimeMilliseconds(delayTimeSeconds * 1000.0);
}

void fix_delay::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("DelayTime");
		p.db = std::bind(&fix_delay::setDelayTimeMilliseconds, this, std::placeholders::_1);
		p.range = { 0.0, 1000.0, 0.1 };
		p.range.setSkewForCentre(100.0);
		p.defaultValue = 100.0;
		data.add(std::move(p));
	}
	{
		ParameterData p("FadeTime");
		p.db = std::bind(&fix_delay::setFadeTimeMilliseconds, this, std::placeholders::_1);
		p.range = { 0.0, 1024.0, 1.0 };
		p.defaultValue = 512;
		data.add(std::move(p));
	}
}

void fix_delay::reset() noexcept
{
	for (auto d : delayLines)
		d->clear();
}

void fix_delay::process(ProcessData& d) noexcept
{
	jassert(d.numChannels == delayLines.size());

	for (int i = 0; i < delayLines.size(); i++)
	{
		delayLines[i]->processBlock(d.data[i], d.size);
	}
}

void fix_delay::processSingle(float* numFrames, int numChannels) noexcept
{
	for (int i = 0; i < numChannels; i++)
		numFrames[i] = delayLines[i]->getDelayedValue(numFrames[i]);
}

void fix_delay::setDelayTimeMilliseconds(double newValue)
{
	delayTimeSeconds = newValue * 0.001;

	for (auto d : delayLines)
		d->setDelayTimeSeconds(delayTimeSeconds);
}

void fix_delay::setFadeTimeMilliseconds(double newValue)
{
	for (auto d : delayLines)
		d->setFadeTimeSamples((int)newValue);
}

}


}