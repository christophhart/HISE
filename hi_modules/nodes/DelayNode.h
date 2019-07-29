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

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace core
{
struct fix_delay : public HiseDspBase
{
	SET_HISE_NODE_ID("fix_delay");
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	GET_SELF_AS_OBJECT(fix_delay);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	fix_delay() {};

	void prepare(PrepareSpecs ps);
	bool handleModulation(double&) noexcept { return false; };
	void reset() noexcept;

	void process(ProcessData& d) noexcept;
	void processSingle(float* numFrames, int numChannels) noexcept;
	void setDelayTimeMilliseconds(double newValue);
	void setFadeTimeMilliseconds(double newValue);
	void createParameters(Array<ParameterData>& data) override;

	OwnedArray<DelayLine<>> delayLines;
	
	double delayTimeSeconds = 0.1;
};
}



}
