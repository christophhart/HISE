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

#ifndef GLOBALMODULATORCONTAINER_H_INCLUDED
#define GLOBALMODULATORCONTAINER_H_INCLUDED
namespace hise { using namespace juce;

class GlobalModulatorContainerSound : public ModulatorSynthSound
{
public:
	GlobalModulatorContainerSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override   { return true; }
	bool appliesToChannel(int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override  { return true; }
};

class GlobalModulatorContainerVoice : public ModulatorSynthVoice
{
public:

	GlobalModulatorContainerVoice(ModulatorSynth *ownerSynth) :
		ModulatorSynthVoice(ownerSynth)
	{};

	bool canPlaySound(SynthesiserSound *) override { return true; };

	void startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
	void calculateBlock(int startSample, int numSamples) override;;

};

class GlobalModulatorData
{
public:

	GlobalModulatorData(Processor *modulator);

	/** Sets up the buffers depending on the type of the modulator. */
	void prepareToPlay(double sampleRate, int blockSize);

	void saveValuesToBuffer(int startIndex, int numSamples, int voiceIndex = 0, int noteNumber=-1);
	const float *getModulationValues(int startIndex, int voiceIndex = 0) const;
	float getConstantVoiceValue(int noteNumber);

	const Processor *getProcessor() const { return modulator.get(); }

	VoiceStartModulator *getVoiceStartModulator() { return dynamic_cast<VoiceStartModulator*>(modulator.get()); }
	const VoiceStartModulator *getVoiceStartModulator() const { return dynamic_cast<VoiceStartModulator*>(modulator.get()); }
	TimeVariantModulator *getTimeVariantModulator() { return dynamic_cast<TimeVariantModulator*>(modulator.get()); }
	const TimeVariantModulator *getTimeVariantModulator() const { return dynamic_cast<TimeVariantModulator*>(modulator.get()); }
	EnvelopeModulator *getEnvelopeModulator() { return dynamic_cast<EnvelopeModulator*>(modulator.get()); }
	const EnvelopeModulator *getEnvelopeModulator() const { return dynamic_cast<EnvelopeModulator*>(modulator.get()); }

	GlobalModulator::ModulatorType getType() const noexcept{ return type; }

	void handleVoiceStartControlledParameters(int noteNumber);

	struct ParameterConnection: public MidiControllerAutomationHandler::AutomationData
	{
		ParameterConnection(Processor* p, int parameterIndex_, const NormalisableRange<double>& range_) :
			AutomationData()
		{
			attribute = parameterIndex_;
			processor = p;
			parameterRange = range_;
			fullRange = range_;
		};

		ValueTree exportAsValueTree() const override
		{
			ValueTree av = AutomationData::exportAsValueTree();

			av.removeProperty("Controller", nullptr);

			ValueTree v("ParameterConnection");

			v.copyPropertiesFrom(av, nullptr);

			return v;
		}

		void restoreFromValueTree(const ValueTree &v) override
		{
			AutomationData::restoreFromValueTree(v);
		}

		float lastValue = 0.0f;
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(ParameterConnection);
	};

	ParameterConnection* getParameterConnection(Processor* p, int parameterIndex)
	{
		for (auto pc : connectedParameters)
		{
			if (pc->attribute == parameterIndex && pc->processor == p)
				return pc;
		}

		return nullptr;
	}

	ParameterConnection* addConnectedParameter(Processor* p, int parameterIndex, NormalisableRange<double> normalisableRange)
	{
		auto newConnection = new ParameterConnection(p, parameterIndex, normalisableRange);
		connectedParameters.addIfNotAlreadyThere(newConnection);
		return connectedParameters.getLast();
	}

	void removeConnectedParameter(Processor* p, int parameterIndex)
	{
		for (auto c : connectedParameters)
		{
			if (c->processor.get() == p && c->attribute == parameterIndex)
			{
				connectedParameters.removeObject(c, true);
				return;
			}
		}
	}

	ValueTree exportAllConnectedParameters() const
	{
		if (connectedParameters.size() == 0)
			return {};

		auto pId = getProcessor()->getId();

		ValueTree v("Modulator");
		v.setProperty("id", pId, nullptr);

		for (auto pc : connectedParameters)
		{
			auto child = pc->exportAsValueTree();
			v.addChild(child, -1, nullptr);
		}

		return v;
	}

	void restoreParameterConnections(const ValueTree& v)
	{
		connectedParameters.clear();

		for (const auto& c : v)
		{
			auto p = new ParameterConnection(nullptr, -1, {});
			p->restoreFromValueTree(c);
			connectedParameters.add(p);
		}
	}

	void handleTimeVariantControlledParameters(int startSample, int numThisTime) const;
private:

	OwnedArray<ParameterConnection> connectedParameters;
	
	WeakReference<Processor> modulator;
	GlobalModulator::ModulatorType type;

	int numVoices;
	AudioSampleBuffer valuesForCurrentBuffer;
	Array<float> constantVoiceValues;
};

class GlobalModulatorContainer : public ModulatorSynth,
								 public SafeChangeListener
{
public:

	SET_PROCESSOR_NAME("GlobalModulatorContainer", "Global Modulator Container");

	float getVoiceStartValueFor(const Processor *voiceStartModulator);

    int getNumActiveVoices() const override { return 0; };
    
	GlobalModulatorContainer(MainController *mc, const String &id, int numVoices);;

	void restoreFromValueTree(const ValueTree &v) override;

	const float *getModulationValuesForModulator(Processor *p, int startIndex, int voiceIndex = 0);
	float getConstantVoiceValue(Processor *p, int noteNumber);

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	void changeListenerCallback(SafeChangeBroadcaster *) { refreshList(); }
	void addChangeListenerToHandler(SafeChangeListener *listener);
	void removeChangeListenerFromHandler(SafeChangeListener *listener);

	void preStartVoice(int voiceIndex, int noteNumber);
	void postVoiceRendering(int startSample, int numThisTime);

	void addProcessorsWhenEmpty() override {};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void addModulatorControlledParameter(const Processor* modulationSource, Processor* processor, int parameterIndex, NormalisableRange<double> range, int macroIndex);
	void removeModulatorControlledParameter(const Processor* modulationSource, Processor* processor, int parameterIndex);
	bool isModulatorControlledParameter(Processor* processor, int parameterIndex) const;
	
	const Processor* getModulatorForControlledParameter(Processor* processor, int parameterIndex) const;

	int getNumControlledParameters() const
	{

	}

	ValueTree exportModulatedParameters() const;

	void restoreModulatedParameters(const ValueTree& v);

private:

	friend class GlobalModulatorContainerVoice;

	Array<WeakReference<GlobalModulatorData::ParameterConnection>> allParameters;

	void refreshList();

	OwnedArray<GlobalModulatorData> data;
};


} // namespace hise

#endif  // GLOBALMODULATORCONTAINER_H_INCLUDED
