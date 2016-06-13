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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#include "ClarinetData.cpp"

ProcessorEditorBody* WavetableSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new WavetableBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


WavetableSynthVoice::WavetableSynthVoice(ModulatorSynth *ownerSynth):
	ModulatorSynthVoice(ownerSynth),
	wavetableSynth(dynamic_cast<WavetableSynth*>(ownerSynth)),
	octaveTransposeFactor(1),
	currentSound(nullptr),
	hqMode(true)
{
		
};

float WavetableSynthVoice::getGainValue(float modValue)
{
	return wavetableSynth->getGainValueFromTable(modValue);
}

const float *WavetableSynthVoice::getTableModulationValues(int startSample, int numSamples)
{
	

	dynamic_cast<WavetableSynth*>(getOwnerSynth())->calculateTableModulationValuesForVoice(voiceIndex, startSample, numSamples);



	return dynamic_cast<WavetableSynth*>(getOwnerSynth())->getTableModValues(voiceIndex);
}

void WavetableSynthVoice::stopNote(float velocity, bool allowTailoff)
{

	ModulatorSynthVoice::stopNote(velocity, allowTailoff);

	ModulatorChain *c = static_cast<ModulatorChain*>(getOwnerSynth()->getChildProcessor(WavetableSynth::TableIndexModulation));

	c->stopVoice(voiceIndex);
}

int WavetableSynthVoice::getSmoothSize() const
{
	return wavetableSynth->getMorphSmoothing();
}