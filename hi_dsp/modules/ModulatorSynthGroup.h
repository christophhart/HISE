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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#ifndef MODULATORSYNTHGROUP_H_INCLUDED
#define MODULATORSYNTHGROUP_H_INCLUDED


// TODO: Fix unisono kill voice when > 8 voices.

class ModulatorSynthGroupSound : public ModulatorSynthSound
{
public:
	ModulatorSynthGroupSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) { return true; }
	bool appliesToChannel(int /*midiChannel*/) { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override { return true; }
};



/** This class acts as wrapper in a ModulatorSynthGroup for all child synth voices. */
class ModulatorSynthGroupVoice : public ModulatorSynthVoice
{
	struct DetuneValues
	{
		float multiplier = 1.0f;
		float gainFactor = 1.0f;
		float balanceLeft = 1.0f;
		float balanceRight = 1.0f;

		float detuneModValue = 1.0f;
		float spreadModValue = 1.0f;

		float getGainFactor(bool getRightChannel)
		{
			return gainFactor * (getRightChannel ? balanceRight : balanceLeft);
		}
	};

public:

	ModulatorSynthGroupVoice(ModulatorSynth *ownerSynth);;

	bool canPlaySound(SynthesiserSound *) override;;


	/** This stores a reference of the child synths and a reference of the voice of the child processor with the same voice index. */
	void addChildSynth(ModulatorSynth *childSynth);;

	/** This removes reference of the child synths and a reference of the voice of the child processor with the same voice index. */
	void removeChildSynth(ModulatorSynth *childSynth);;

	/** Calls the base class startNote() for the group itself and all child synths.  */
	void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int) override;

	ModulatorSynthVoice* startNoteInternal(ModulatorSynth* childSynth, int childVoiceIndex, int midiNoteNumber, float velocity);

	/** Calls the base class stopNote() for the group itself and all child synths. */
	void stopNote(float, bool) override;

	void stopNoteInternal(ModulatorSynth * childSynth, int childVoiceIndex);

	void checkRelease();

	void resetInternal(ModulatorSynth * childSynth, int childVoiceIndex);

	void calculateBlock(int startSample, int numSamples) override;

	void calculateNoFMBlock(int startSample, int numSamples);

	void calculateNoFMVoiceInternal(ModulatorSynth * childSynth, int childVoiceIndex, int startSample, int numSamples, const float * voicePitchValues);

	void calculateDetuneMultipliers(int childVoiceIndex);

	void calculateFMBlock(ModulatorSynthGroup * group, int startSample, int numSamples);

	void calculateFMCarrierInternal(ModulatorSynthGroup * group, int childVoiceIndex, int startSample, int numSamples, const float * voicePitchValues);

private:

	DetuneValues detuneValues;

	ModulatorSynth* getFMModulator();

	Random startOffsetRandomizer;

	int numUnisonoVoices = 1;

	Array<ModulatorSynth*> childSynths;
	float fmModBuffer[2048];
};





/** A ModulatorSynthGroup is a collection of somehow related ModulatorSynths that are processed together.
*
*	This class is designed to process related ModulatorSynths (eg. round-robin groups or multimiced samples) together. Unlike the ModulatorSynthChain,
*	this structure allows Modulators and effects to control multiple child synths.
*	In order to ensure correct functionality, the ModulatorSynthGroup assumes that all child synths start at the same time.
*
*	The ModulatorSynthGroup is rendered like a normal ModulatorSynth using a special kind of voice (the ModulatorSynthGroupVoice).
*
*	- Group start logic can be implemented here.
*	- Modulators of the ModulatorSynthGroup also control their child processors.
*
*	There are some restrictions for this group type:
*
*	- MidiProcessors are not allowed in the child processors, but you can use specialised MidiProcessors in the ModulatorSynthGroup's MidiEditor.
*	- ModulatorSynthGroups can't be nested. The child processors only collect modulation from their immediate parent.
*
*/
class ModulatorSynthGroup : public ModulatorSynth,
	public Chain
{
public:

	enum SpecialParameters
	{
		EnableFM = ModulatorSynth::numModulatorSynthParameters,
		CarrierIndex,
		ModulatorIndex,
		UnisonoVoiceAmount,
		UnisonoDetune,
		UnisonoSpread,
		numSynthGroupParameters
	};

	SET_PROCESSOR_NAME("SynthGroup", "Syntesizer Group")

		enum InternalChains
	{
		DetuneModulation = ModulatorSynth::numInternalChains,
		SpreadModulation,
		numInternalChains
	};


	ModulatorSynthGroup(MainController *mc, const String &id, int numVoices);
	~ModulatorSynthGroup();

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor) override;

	Chain::Handler *getHandler() override { return &handler; };
	const Chain::Handler *getHandler() const override { return &handler; };

	FactoryType *getFactoryType() const override { return modulatorSynthFactory; };
	void setFactoryType(FactoryType *newFactoryType) override { modulatorSynthFactory = newFactoryType; };

	/** returns the total amount of child groups (internal chains + all child synths) */
	int getNumChildProcessors() const override { return numInternalChains + handler.getNumProcessors(); };
	int getNumInternalChains() const override { return numInternalChains; };
	void setInternalAttribute(int index, float newValue) override;
	float getAttribute(int index) const override;
	float getDefaultValue(int parameterIndex) const override;

	ModulatorSynth* getFMModulator();

	/** returns the total amount of child groups (internal chains + all child synths) */
	Processor *getChildProcessor(int processorIndex) override;;

	/** returns the total amount of child groups (internal chains + all child synths) */
	const Processor *getChildProcessor(int processorIndex) const override;;

	/** Iterates over all child synths.
	*
	*	Example usage:
	*
	*		ModulatorSynth *child;
	*		ChildSynthIterator childIterator(this);
	*
	*		while(childIterator.getNextAllowedChild(child))
	*		{
	*			// Do something here
	*		}
	*/
	class ChildSynthIterator
	{
	public:

		/** This can be set to iterate either all synths or only the allowed ones. */
		enum Mode
		{
			SkipUnallowedSynths = 0,
			GetFMCarrierOnly,
			IterateAllSynths
		};

		/** Creates a new ChildSynthIterator. It is okay to create one on the stack. You have to pass a pointer to the ModulatorSynthGroup that should be iterated. */
		ChildSynthIterator(ModulatorSynthGroup *groupToBeIterated, Mode iteratorMode = SkipUnallowedSynths);;

		/** sets the passed pointer to the next ModulatorSynth in the chain that is allowed and returns false if the end of the chain is reached or if the child is a nullpointer */
		bool getNextAllowedChild(ModulatorSynth *&child);;

	private:

		friend class ModulatorSynthGroup;

		ModulatorSynthGroup &group;
		int counter;
		const int limit;
		const Mode mode;

		bool carrierWasReturned = false;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChildSynthIterator)

	};

	void allowChildSynth(int childSynthIndex, bool shouldBeAllowed);;

	/** set the state for all groups at once. */
	void setAllowStateForAllChildSynths(bool shouldBeEnabled);;

	void setUnisonoVoiceAmount(int newVoiceAmount);
	void setUnisonoDetuneAmount(float newDetuneAmount);
	void setUnisonoSpreadAmount(float newSpreadAmount);

	virtual int getNumActiveVoices() const
	{
		return ModulatorSynth::getNumActiveVoices() * unisonoVoiceAmount;
	}

	Processor *getParentProcessor() { return nullptr; };
	const Processor *getParentProcessor() const { return nullptr; };

	/** Passes the incoming MidiMessage only to the modulation chains of all child synths and NOT to the child synth's voices, as they get rendered by the ModulatorSynthGroupVoices. */
	void preHiseEventCallback(const HiseEvent &m) override;

	/** Prepares all ModulatorSynths for playback. */
	void prepareToPlay(double newSampleRate, int samplesPerBlock) override;;

	/** Clears the internal buffers of the childs and the group itself. */
	void initRenderCallback() override;;

	void preStartVoice(int voiceIndex, int noteNumber) override;;

	void preVoiceRendering(int startSample, int numThisTime) override;;
	void postVoiceRendering(int startSample, int numThisTime) override;;

	void restoreFromValueTree(const ValueTree &v) override;
	ValueTree exportAsValueTree() const override;

	/** Handles the ModulatorSynthGroup.
	*
	*	It is almost the same as Modulator, but it calls setGroup() on the ModulatorSynths that are added.
	*/
	class ModulatorSynthGroupHandler : public Chain::Handler
	{
	public:
		ModulatorSynthGroupHandler(ModulatorSynthGroup *synthGroupToHandle) :
			group(synthGroupToHandle)
		{

		};

		/** Adds a new ModulatorSynth to the ModulatorSynthGroup. It also stores a reference in each ModulatorSynthGroupVoice.
		*
		*	By default, it sets the synths allow state to 'true'.
		*/
		void add(Processor *newProcessor, Processor *siblingToInsertBefore) override;

		/** Deletes a processor from the chain. It also removes the reference in the ModulatorSynthGroupVoices. */
		virtual void remove(Processor *processorToBeRemoved) override;;

		/** Returns the processor at the index. */
		virtual Processor *getProcessor(int processorIndex);;

		const virtual Processor *getProcessor(int processorIndex) const override;;

		/** Returns the amount of processors. */
		virtual int getNumProcessors() const override;;

		void clear() override;;

	private:

		ModulatorSynthGroup *group;

	};

	String getFMState() const { return fmState; };
	void checkFmState();

	bool fmIsCorrectlySetup() const { return fmCorrectlySetup; };

	const float* calculateDetuneModulationValuesForVoice(int voiceIndex, int startSample, int numSamples)
	{
		detuneChain->renderVoice(voiceIndex, startSample, numSamples);
		float *detuneValues = detuneChain->getVoiceValues(voiceIndex);
		const float* timeVariantDetuneValues = detuneBuffer.getReadPointer(0);

		FloatVectorOperations::multiply(detuneValues + startSample, timeVariantDetuneValues + startSample, numSamples);

		return detuneChain->getVoiceValues(voiceIndex) + startSample;
	}


	const float* calculateSpreadModulationValuesForVoice(int voiceIndex, int startSample, int numSamples)
	{
		spreadChain->renderVoice(voiceIndex, startSample, numSamples);
		float *spreadValues = spreadChain->getVoiceValues(voiceIndex);
		const float* timeVariantSpreadValues = spreadBuffer.getReadPointer(0);

		FloatVectorOperations::multiply(spreadValues + startSample, timeVariantSpreadValues + startSample, numSamples);

		return spreadChain->getVoiceValues(voiceIndex) + startSample;
	}

private:

	friend class ChildSynthIterator;
	friend class ModulatorSynthGroupVoice;

	ScopedPointer<ModulatorChain> detuneChain;
	ScopedPointer<ModulatorChain> spreadChain;

	// the precalculated modvalues for LFOs & stuff
	AudioSampleBuffer modSynthGainValues;

	AudioSampleBuffer spreadBuffer;
	AudioSampleBuffer detuneBuffer;

	String fmState;

	bool fmEnabled;
	bool fmCorrectlySetup;
	int modIndex;
	int carrierIndex;

	int unisonoVoiceAmount;
	int unisonoVoiceLimit;
	double unisonoDetuneAmount;
	float unisonoSpreadAmount;

	ModulatorSynthGroupHandler handler;
	int numVoices;
	float vuValue;
	BigInteger allowStates;
	OwnedArray<ModulatorSynth> synths;
	ScopedPointer<FactoryType> modulatorSynthFactory;
};



#endif  // MODULATORSYNTHGROUP_H_INCLUDED
