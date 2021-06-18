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

	setDelayTime(delayTimeSeconds * 1000.0);
}

void fix_delay::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(fix_delay, DelayTime);
		p.setRange({ 0.0, 1000.0, 0.1 });
		p.setSkewForCentre(100.0);
		p.setDefaultValue(100.0);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(fix_delay, FadeTime);
		p.setRange({ 0.0, 1024.0, 1.0 });
		p.setDefaultValue(512.0);
		data.add(std::move(p));
	}
}

void fix_delay::reset() noexcept
{
	for (auto d : delayLines)
		d->clear();
}

void fix_delay::setDelayTime(double newValue)
{
	delayTimeSeconds = newValue * 0.001;

	for (auto d : delayLines)
		d->setDelayTimeSeconds(delayTimeSeconds);
}

void fix_delay::setFadeTime(double newValue)
{
	for (auto d : delayLines)
		d->setFadeTimeSamples((int)newValue);
}

}


}