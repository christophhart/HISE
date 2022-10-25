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


#ifndef MODULATORSYNTHGROUP_H_INCLUDED
#define MODULATORSYNTHGROUP_H_INCLUDED

namespace hise { using namespace juce;

#define NUM_MAX_UNISONO_VOICES 16

constexpr int getNumInts()
{
#if NUM_POLYPHONIC_VOICES < 64
		return 1;
#else
		return NUM_POLYPHONIC_VOICES / 64;
#endif
}

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

	struct Iterator
	{
		Iterator(ModulatorSynthGroupVoice* v_):
			v(v_)
		{
			numSize = v->childSynths.size();
			mod = v->getFMModulator();
		}

		ModulatorSynth* getNextActiveChildSynth();

	private:

		ModulatorSynthGroupVoice* v;

		ModulatorSynth* mod;

		int i = 0;
		int numSize = 0;
	};

	ModulatorSynthGroupVoice(ModulatorSynth *ownerSynth);;

	bool canPlaySound(SynthesiserSound *) override;;


	/** This stores a reference of the child synths and a reference of the voice of the child processor with the same voice index. */
	void addChildSynth(ModulatorSynth *childSynth);;

	/** This removes reference of the child synths and a reference of the voice of the child processor with the same voice index. */
	void removeChildSynth(ModulatorSynth *childSynth);;

	/** Calls the base class startNote() for the group itself and all child synths.  */
	void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int) override;

	ModulatorSynthVoice* startNoteInternal(ModulatorSynth* childSynth, int childVoiceIndex, const HiseEvent& e);

	/** Calls the base class stopNote() for the group itself and all child synths. */
	void stopNote(float, bool) override;

	void stopNoteInternal(ModulatorSynth * childSynth, int childVoiceIndex);

	void checkRelease();

	void resetVoice() override;

	void resetInternal(ModulatorSynth * childSynth, int childVoiceIndex);

	void calculateBlock(int startSample, int numSamples) override;

	void calculateNoFMBlock(int startSample, int numSamples);

	void calculateNoFMVoiceInternal(ModulatorSynth* childSynth, int unisonoIndex, int startSample, int numSamples, const float * voicePitchValues, bool& isFirst);

	void calculatePitchValuesForChildVoice(ModulatorSynth* childSynth, ModulatorSynthVoice * childVoice, int startSample, int numSamples, const float * voicePitchValues, bool applyDetune=true);

	void calculateDetuneMultipliers(int childVoiceIndex);

	void calculateFMBlock(ModulatorSynthGroup * group, int startSample, int numSamples);

	void calculateFMCarrierInternal(ModulatorSynthGroup * group, int childVoiceIndex, int startSample, int numSamples, const float * voicePitchValues, bool& isFirst);

	int getChildVoiceAmount() const;

	

private:

	friend class ModulatorSynthGroup;

	class ChildVoiceContainer
	{
	public:

		ChildVoiceContainer()
		{
			clear();
		}

		void addVoice(ModulatorSynthVoice* v)
		{
			jassert(numVoices < 8);
			voices[numVoices++] = v;
		}

		bool removeVoice(ModulatorSynthVoice* v)
		{
			for (int i = 0; i < numVoices; i++)
			{
				if (voices[i] == v)
				{
					for (int j = i; j < numVoices-1; j++)
					{
						voices[j] = voices[j + 1];
					}

					voices[numVoices--] = nullptr;
					return true;
				}
			}

			return false;
		}

		ModulatorSynthVoice* getVoice(int index)
		{
			if (index < numVoices)
			{
				return voices[index];
			}
			else
			{
				jassertfalse;
				return nullptr;
			}
		}

		int size() const
		{
			return numVoices;
		}

		void clear()
		{
			memset(voices, 0, sizeof(ModulatorSynthGroupVoice*) * 8);
			numVoices = 0;
		}

	private:

		ModulatorSynthVoice* voices[8];
		int numVoices = 0;
	};

	ChildVoiceContainer& getChildContainer(int childVoiceIndex)
	{
		return startedChildVoices[childVoiceIndex % NUM_MAX_UNISONO_VOICES];
	}

	ChildVoiceContainer startedChildVoices[NUM_MAX_UNISONO_VOICES];

	DetuneValues detuneValues;

	ModulatorSynth* getFMModulator();

	ModulatorSynth* getFMCarrier();

	struct UnisonoState
	{
		static constexpr int bitsPerNumber = 64;

		uint64 getIndex(int index) const
		{
			return static_cast<uint64>(index % bitsPerNumber);
		}

		uint64 getOffset(int index) const
		{
			return static_cast<uint64>(index / bitsPerNumber);
		}

		UnisonoState()
		{
			clear();
		}

		bool anyActive() const noexcept
		{
			bool active = false;

			for (int i = 0; i < getNumInts(); i++)
			{
				active |= state[i] != 0;
			}

			return active;
		}

		void clearBit(int index)
		{
			auto i = getIndex(index);
			auto offset = getOffset(index);
			auto v = ~(uint64(1) << i);

			state[offset] &= v;
		}

		bool isBitSet(int index) const
		{
			auto i = getIndex(index);
			auto offset = getOffset(index);
			auto v = uint64(1) << i;

			return (state[offset] & v) != 0;
		}

		void setBit(int index)
		{
			auto i = getIndex(index);
			auto offset = getOffset(index);

			auto v = uint64(1) << i;

			state[offset] |= v;
		}

		void clear()
		{
			memset(state, 0, sizeof(uint64) * getNumInts());
		}

		

		uint64 state[getNumInts()];
	} unisonoStates;

	Random startOffsetRandomizer;

	int numUnisonoVoices = 1;

	bool useFMForVoice = false;

	struct ChildSynth
	{
		ChildSynth() :
			synth(nullptr),
			isActiveForThisVoice(false)
		{}

		ChildSynth(ModulatorSynth* synth_) :
			synth(synth_),
			isActiveForThisVoice(false)
		{}

		ChildSynth(const ChildSynth& other) :
			synth(other.synth),
			isActiveForThisVoice(other.isActiveForThisVoice)
		{};

		
		bool operator== (const ChildSynth& other) const
		{
			return synth == other.synth;
		}

		ModulatorSynth* synth;
		bool isActiveForThisVoice = false;
	};

	Array<ChildSynth> childSynths;

	float fmModBuffer[2048];
	
	void handleActiveStateForChildSynths();
};





/** A ModulatorSynthGroup is a collection of tightly coupled ModulatorSynth that are processed together.
	@ingroup synthTypes
*
*	Other than the ModulatorSynthChain, it will render its children grouped on voice level which allows stuff like FM synthesis etc.
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

	ADD_DOCUMENTATION_WITH_BASECLASS(ModulatorSynth);
	
	enum ModChains
	{
		Detune = 2,
		Spread
	};

	enum SpecialParameters
	{
		EnableFM = ModulatorSynth::numModulatorSynthParameters,
		CarrierIndex,
		ModulatorIndex,
		UnisonoVoiceAmount,
		UnisonoDetune,
		UnisonoSpread,
		ForceMono,
		KillSecondVoices,
		numSynthGroupParameters
	};

	SET_PROCESSOR_NAME("SynthGroup", "Syntesizer Group", "A container for other Sound generators that allows FM and other additional synthesis types.");

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
	ModulatorSynth* getFMCarrier();

	const ModulatorSynth* getFMModulator() const;
	const ModulatorSynth* getFMCarrier() const;

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

	virtual int getNumActiveVoices() const;

	Processor *getParentProcessor() { return nullptr; };
	const Processor *getParentProcessor() const { return nullptr; };

	/** Passes the incoming MidiMessage only to the modulation chains of all child synths and NOT to the child synth's voices, as they get rendered by the ModulatorSynthGroupVoices. */
	void preHiseEventCallback(HiseEvent &m) override;

	/** Prepares all ModulatorSynths for playback. */
	void prepareToPlay(double newSampleRate, int samplesPerBlock) override;;

	/** Clears the internal buffers of the childs and the group itself. */
	void initRenderCallback() override;;

	int collectSoundsToBeStarted(const HiseEvent& m) override;

	float getDetuneModValue(int startSample) const
	{
		return modChains[ModChains::Detune].getOneModulationValue(startSample);
	}

	float getSpreadModValue(int startSample) const noexcept
	{
		return modChains[ModChains::Spread].getOneModulationValue(startSample);
	}

	void preVoiceRendering(int startSample, int numThisTime) override;;

	void handleRetriggeredNote(ModulatorSynthVoice *voice) override;

	bool handleVoiceLimit(int numVoicesToClear) override;

	void killAllVoices() override;

	void resetAllVoices() override;

	String getFMStateString() const;

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
		virtual void remove(Processor *processorToBeRemoved, bool removeSynth=true) override;;

		/** Returns the processor at the index. */
		virtual Processor *getProcessor(int processorIndex);;

		const virtual Processor *getProcessor(int processorIndex) const override;;

		/** Returns the amount of processors. */
		virtual int getNumProcessors() const override;;

		void clear() override;;

	private:

		ModulatorSynthGroup *group;
	};

	String getFMState() const { return getFMStateString(); };
	void checkFmState();

	void checkFMStateInternally();

	bool fmIsCorrectlySetup() const { return fmCorrectlySetup; };

private:

	struct SynthVoiceAmount
	{
		bool operator ==(const SynthVoiceAmount& other) const
		{
			return other.s == s;
		}

		ModulatorSynth* s = nullptr;
		int numVoicesNeeded = 0;
	};

	UnorderedStack<SynthVoiceAmount> synthVoiceAmounts;

	friend class ChildSynthIterator;
	friend class ModulatorSynthGroupVoice;

	ModulatorChain* detuneChain = nullptr;
	ModulatorChain* spreadChain = nullptr;

	// the precalculated modvalues for LFOs & stuff
	AudioSampleBuffer modSynthGainValues;

	AudioSampleBuffer spreadBuffer;
	AudioSampleBuffer detuneBuffer;

	bool forceMono = false;

	bool fmEnabled;
	bool fmCorrectlySetup;
	int modIndex;
	int carrierIndex;

	// Used to check the retrigger behaviour
	bool carrierIsSampler = false;

	int unisonoVoiceAmount;
	
	double unisonoDetuneAmount;
	float unisonoSpreadAmount;

	bool killSecondVoice = true;

	ModulatorSynthGroupHandler handler;
	int numVoices;
	float vuValue;
	BigInteger allowStates;
	OwnedArray<ModulatorSynth> synths;
	ScopedPointer<FactoryType> modulatorSynthFactory;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ModulatorSynthGroup);
};

} // namespace hise

#endif  // MODULATORSYNTHGROUP_H_INCLUDED
