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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
namespace control
{
using namespace hise;
using namespace juce;


void tempo_sync::prepare(PrepareSpecs ps)
{
	jassert(ps.voiceIndex != nullptr);

	tempoSyncer = ps.voiceIndex->getTempoSyncer();
	tempoSyncer->registerItem(this);
}

tempo_sync::tempo_sync():
	no_mod_normalisation(getStaticId())
{}

tempo_sync::~tempo_sync()
{
	if(tempoSyncer != nullptr)
		tempoSyncer->deregisterItem(this);
}


void tempo_sync::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(tempo_sync, Tempo);
		p.setParameterValueNames(TempoSyncer::getTempoNames());
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(tempo_sync, Multiplier);
		p.setRange({ 1, 16, 1.0 });
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(tempo_sync, Enabled);
		p.setRange({ 0.0, 1.0, 1.0 });
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(tempo_sync, UnsyncedTime);
		p.setRange({ 0.0, 1000.0, 0.1 });
		p.setDefaultValue(200.0);
		data.add(std::move(p));
	}
}

void tempo_sync::tempoChanged(double newTempo)
{
	bpm = newTempo;
	refresh();
}



bool tempo_sync::handleModulation(double& max)
{
	if (lastTempoMs != currentTempoMilliseconds)
	{
		lastTempoMs = currentTempoMilliseconds;
		max = currentTempoMilliseconds;

		return true;
	}

	return false;
}

void tempo_sync::setTempo(double newTempoIndex)
{
	currentTempo = (TempoSyncer::Tempo)jlimit<int>(0, TempoSyncer::numTempos - 1, (int)newTempoIndex);
	refresh();
}

void tempo_sync::setMultiplier(double newMultiplier)
{
	multiplier = jlimit(1.0, 32.0, newMultiplier);
	refresh();
}

void tempo_sync::setEnabled(double v)
{
	enabled = v > 0.5;
	refresh();
}

void tempo_sync::setUnsyncedTime(double v)
{
	unsyncedTime = v;
	refresh();
}

void tempo_sync::refresh()
{
	if (enabled)
		currentTempoMilliseconds = TempoSyncer::getTempoInMilliSeconds(this->bpm, currentTempo) * multiplier;
	else
		currentTempoMilliseconds = unsyncedTime;
}

}
}