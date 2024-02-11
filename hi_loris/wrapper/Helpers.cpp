/*
 * This file is part of the HISE loris_library codebase (https://github.com/christophhart/loris-tools).
 * Copyright (c) 2023 Christoph Hart
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "Helpers.h"
#include "LorisState.h"


namespace loris2hise
{

juce::var Options::toJSON() const
{
	juce::DynamicObject::Ptr obj = new juce::DynamicObject();

	obj->setProperty(OptionIds::timedomain,
		getTimeDomainOptions()[(int)currentDomainType]);

	obj->setProperty(OptionIds::freqfloor, freqfloor);
	obj->setProperty(OptionIds::ampfloor, ampfloor);
	obj->setProperty(OptionIds::sidelobes, sidelobes);
	obj->setProperty(OptionIds::freqdrift, freqdrift);
	obj->setProperty(OptionIds::hoptime, hoptime);
	obj->setProperty(OptionIds::croptime, croptime);
	obj->setProperty(OptionIds::bwregionwidth, bwregionwidth);
	obj->setProperty(OptionIds::windowwidth, windowwidth);
	obj->setProperty(OptionIds::enablecache, enablecache);

	return juce::var(obj.get());
}

juce::StringArray Options::getTimeDomainOptions()
{
	static const juce::StringArray options =
	{
		"seconds",
		"samples",
		"0to1"
	};

	return options;
}

void Options::initLorisParameters()
{
	//hoptime = analyzer_getHopTime();
	//croptime = analyzer_getCropTime();
	freqfloor = analyzer_getFreqFloor();
	ampfloor = analyzer_getAmpFloor();
	sidelobes = analyzer_getSidelobeLevel();
	bwregionwidth = analyzer_getBwRegionWidth();
	windowwidth = 1.0;
	enablecache = false;
}

bool Options::update(const juce::Identifier& id, const juce::var& value)
{
	if (id == OptionIds::timedomain)
	{
		auto x = value.toString().trim().unquoted();

		auto idx = getTimeDomainOptions().indexOf(x);

		if (idx == -1)
			throw juce::Result::fail("unknown time domain option: " + value.toString());

		currentDomainType = (TimeDomainType)idx;

		return true;
	}

	if (id == OptionIds::freqfloor) { freqfloor = (double)value; if(initialised) analyzer_setFreqFloor(freqfloor); return true; }
	if (id == OptionIds::ampfloor) { ampfloor = (double)value; if (initialised) analyzer_setAmpFloor(ampfloor); return true; }
	if (id == OptionIds::sidelobes) { sidelobes = (double)value; if (initialised) analyzer_setSidelobeLevel(sidelobes); return true; }
	if (id == OptionIds::freqdrift) { freqdrift = (double)value;
        return true; }
	if (id == OptionIds::hoptime) { hoptime = (double)value; if (initialised) analyzer_setHopTime(hoptime); return true; }
	if (id == OptionIds::croptime) { croptime = (double)value; if (initialised) analyzer_setCropTime(croptime); return true; }
	if (id == OptionIds::bwregionwidth) { bwregionwidth = (double)value; if (initialised) analyzer_setBwRegionWidth(bwregionwidth); return true; }
	if (id == OptionIds::enablecache) { enablecache = (bool)value; return true; }
	if (id == OptionIds::windowwidth) { windowwidth = jlimit(0.125, 4.0, (double)value); return true;  }

	throw juce::Result::fail("Invalid option: " + id.toString());
}

void Helpers::reportError(const char* msg)
{
	if (LorisState::getCurrentInstance() != nullptr)
		LorisState::getCurrentInstance()->reportError(msg);
}

void Helpers::logMessage(const char* msg)
{
	if (LorisState::getCurrentInstance() != nullptr)
		LorisState::getCurrentInstance()->messages.add(juce::String(msg));
}

}
