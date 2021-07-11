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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace core
{


hise_mod::hise_mod()
{
	
}

void hise_mod::initialise(NodeBase* b)
{
	parentProcessor = dynamic_cast<JavascriptSynthesiser*>(b->getScriptProcessor());
	parentNode = dynamic_cast<ModulationSourceNode*>(b);
}

void hise_mod::prepare(PrepareSpecs ps)
{
	modValues.prepare(ps);
	uptime.prepare(ps);

	for (auto& u : uptime)
		u = 0.0;

	if (parentProcessor != nullptr && ps.sampleRate > 0.0)
	{
		synthBlockSize = (double)parentProcessor->getLargestBlockSize();
		uptimeDelta = parentProcessor->getSampleRate() / ps.sampleRate;
	}
	
}

bool hise_mod::handleModulation(double& v)
{
	return modValues.get().getChangedValue(v);
}


void hise_mod::handleHiseEvent(HiseEvent& e)
{
	if (e.isNoteOn())
	{
		uptime.get() = e.getTimeStamp() * uptimeDelta;
	}
}

void hise_mod::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(hise_mod, Index);
		p.setParameterValueNames({ "Pitch", "Extra 1", "Extra 2" });
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}
}

void hise_mod::setIndex(double index)
{
	auto inp = roundToInt(index);

	if (inp == 0)
	{
		modIndex = ModulatorSynth::BasicChains::PitchChain;
	}

	else if (inp == 1)
		modIndex = JavascriptSynthesiser::ModChains::Extra1;

	else if (inp == 2)
		modIndex = JavascriptSynthesiser::ModChains::Extra2;
}

void hise_mod::reset()
{
	if (parentProcessor != nullptr)
	{
		auto startValue = parentProcessor->getModValueAtVoiceStart(modIndex);

		for (auto& m : modValues)
			m.setModValue(startValue);
	}
}


} // namespace core


} // namespace scriptnode
