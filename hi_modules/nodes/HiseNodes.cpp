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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

#include "HiseNodes.h"

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace core
{

void hise_mod_base::initialise(NodeBase* b)
{
	parentNode = dynamic_cast<ModulationSourceNode*>(b);
    
    auto& bl = parentNode->getScriptProcessor()->getMainController_()->getBlocksizeBroadcaster();
    
    bl.addListener(*this, hise_mod_base::updateBlockSize, true);
}

void hise_mod_base::prepare(PrepareSpecs ps)
{
	modValues.prepare(ps);
	uptime.prepare(ps);

	for (auto& u : uptime)
		u = 0.0;
}

bool hise_mod_base::handleModulation(double& v)
{
	return modValues.get().getChangedValue(v);
}


void hise_mod_base::handleHiseEvent(HiseEvent& e)
{
	if (e.isNoteOn())
	{
		uptime.get() = e.getTimeStamp() * uptimeDelta;
	}
}



void hise_mod_base::reset()
{
	auto startValue = getModulationValue(-1);

	for (auto& m : modValues)
		m.setModValue(startValue);
}


void pitch_mod::transformModValues(float* d, int numSamples)
{
	auto lastValue = -1.0f;
	auto lastPitchValue = 0.5f;

	for (int i = 0; i < numSamples; i++)
	{
		auto thisValue = d[i];

		if (thisValue != lastValue)
		{
			lastPitchValue = log2(thisValue);
			lastPitchValue += 1.0f;
			lastPitchValue *= 0.5f;
			lastValue = thisValue;
		}

		d[i] = lastPitchValue;
	}
}

} // namespace core


} // namespace scriptnode
