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

namespace hise { using namespace juce;

class MacroModulationSourceSound : public ModulatorSynthSound
{
public:
	MacroModulationSourceSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override   { return true; }
	bool appliesToChannel(int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override  { return true; }
};

class MacroModulationSourceVoice : public ModulatorSynthVoice
{
public:

	MacroModulationSourceVoice(ModulatorSynth *ownerSynth) :
		ModulatorSynthVoice(ownerSynth)
	{};

	void checkRelease()
 	{
		
	}

	bool canPlaySound(SynthesiserSound *) override { return true; };

	void startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
	void calculateBlock(int startSample, int numSamples) override;
};


/** A container that processes Modulator instances that can be used at different locations.
	@ingroup synthTypes
*/
class MacroModulationSource : public ModulatorSynth,
							  public Chain::Handler::Listener
{
public:

	SET_PROCESSOR_NAME("MacroModulationSource", "Macro Modulation Source", "A container that processes Modulator instances that can be used as modulation sources for the macro control system");

	int getNumActiveVoices() const override { return 0; };
    
	void processorChanged(Chain::Handler::Listener::EventType t, Processor* p) override
	{

	}

	MacroModulationSource(MainController *mc, const String &id, int numVoices);;

	~MacroModulationSource();

	int getNumChildProcessors() const override
	{
		return ModulatorSynth::numInternalChains + HISE_NUM_MACROS;
	}

	Processor* getChildProcessor(int processorIndex) override
	{
		if (processorIndex < ModulatorSynth::numInternalChains)
			return ModulatorSynth::getChildProcessor(processorIndex);
		else
			return macroChains[processorIndex - ModulatorSynth::numInternalChains];
	}

	const Processor* getChildProcessor(int processorIndex) const override
	{
		if (processorIndex < ModulatorSynth::numInternalChains)
			return ModulatorSynth::getChildProcessor(processorIndex);
		else
			return macroChains[processorIndex - ModulatorSynth::numInternalChains];
	}

	void killAllVoices()
	{
		for (auto v : voices)
		{
			static_cast<ModulatorSynthVoice*>(v)->resetVoice();
		}
	}

	int getNumInternalChains() const override { return getNumChildProcessors(); };

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	void preVoiceRendering(int startSample, int numThisTime) override;

	void addProcessorsWhenEmpty() override {};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	bool synthNeedsEnvelope() const override { return false; }

private:

	float lastValues[HISE_NUM_MACROS];

	Array<ModulatorChain*> macroChains;

	friend class MacroModulationSourceVoice;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MacroModulationSource);
};


} // namespace hise

