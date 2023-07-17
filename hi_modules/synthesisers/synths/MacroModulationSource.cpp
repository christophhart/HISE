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

MacroModulationSource::MacroModulationSource(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices)
{
	for (int i = 0; i < HISE_NUM_MACROS; i++)
	{
		String s("Macro " + String(i + 1));
		modChains += { this, s};
		lastValues[i] = 0.0f;
	}

	finaliseModChains();

	auto offset = 2;

	for (int i = 0; i < HISE_NUM_MACROS; i++)
	{
		macroChains.set(i, modChains[i + offset].getChain());
		modChains[i + offset].setExpandToAudioRate(true);
		modChains[i + offset].setIncludeMonophonicValuesInVoiceRendering(true);
	}

	for (auto mChain : macroChains)
	{
		auto c = Colour(SIGNAL_COLOUR).withSaturation(JUCE_LIVE_CONSTANT_OFF(0.4f));

		mChain->setColour(c.withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(0.9f)));
		
		mChain->getHandler()->addListener(this);
	}
		

	for (int i = 0; i < numVoices; i++) addVoice(new MacroModulationSourceVoice(this));
	addSound(new MacroModulationSourceSound());

	disableChain(GainModulation, true);
	disableChain(PitchModulation, true);
	disableChain(ModulatorSynth::EffectChain, true);

}

MacroModulationSource::~MacroModulationSource()
{
}

ProcessorEditorBody* MacroModulationSource::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND
	return new EmptyProcessorEditorBody(parentEditor);
#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void MacroModulationSource::preVoiceRendering(int startSample, int numThisTime)
{
	ModulatorSynth::preVoiceRendering(startSample, numThisTime);

	// skip plugin parameter update calls
	ScopedValueSetter<bool> setter(getMainController()->getPluginParameterUpdateState(), false);

	for (int i = 0; i < HISE_NUM_MACROS; i++)
	{
		auto& mb = modChains[i+2];

		if (mb.getChain()->shouldBeProcessedAtAll())
		{
			float v = 1.0f;

			mb.expandMonophonicValuesToAudioRate(startSample, numThisTime);

			if (auto m = mb.getMonophonicModulationValues(startSample))
			{
				v *= m[0];
			}
			
			if (auto vv = mb.getWritePointerForManualExpansion(startSample))
			{
				v *= vv[0];
			}
			else
				v *= mb.getConstantModulationValue();

			if (v != lastValues[i])
				getMainController()->getMainSynthChain()->setMacroControl(i, v * 127.0f, sendNotificationAsync);

			lastValues[i] = v;

			mb.setDisplayValue(v);
		}
	}
}

void MacroModulationSource::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);
}



void MacroModulationSourceVoice::startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	voiceUptime = 0.0;
	uptimeDelta = 1.0;
}

void MacroModulationSourceVoice::calculateBlock(int startSample, int numSamples)
{

}


} // namespace hise
