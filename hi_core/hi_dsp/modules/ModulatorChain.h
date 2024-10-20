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

#ifndef ModulatorChainProcessor_H_INCLUDED
#define ModulatorChainProcessor_H_INCLUDED

namespace hise { using namespace juce;

#ifndef HISE_NUM_MODULATORS_PER_CHAIN
#define HISE_NUM_MODULATORS_PER_CHAIN 32
#endif

/** A chain of Modulators that can be processed serially.
*
*	@ingroup modulator
*
*	A ModulatorChain can hold three different types of Modulators:
*
*	- VoiceStartModulators that calculate their value only if the voice has been started.
*	- VariantModulators which calculate their value for each sample
*	- EnvelopeModulators which calculate their value for each sample and each voice
*
*	In order to make a ModulatorChain work, you have to 
*	- create one while specifying a Mode and the voice amount.
*	- populate it with modulators via getHandler()->addModulator().
*	- overwrite getNumChildProcessors(), getNumInternalChains() and getChildProcessor() (const and not const)
*	- 
*
*	In your code you have to call all these methods at the appropriate time / place:
*
*	- initialize it with the sample rate and the block size via 'prepareToPlay()'
*	- initialize a buffer that will store the modulation values.
*	- call the functions handleMidiEvent() and renderNextBlock() to make sure it gets the midi messages and does its processing.
*	- call the functions startVoice(), stopVoice(), renderVoice(), and reset() for every voice that should use this chain. (Skip this for monophonic modulation
*	
*	You might want to use the functions shouldBeProcessed(), isPlaying(), getVoiceValues()
*
*	For an example usage of an polyphonic ModulatorChain, take a look at the WavetableSynth class.
*	For an example usage of an monophonic ModulatorChain, the LfoModulator class is your friend.
*/
class ModulatorChain: public Chain,
					  public EnvelopeModulator
{
public:

	

	class ModChainWithBuffer
	{
	public:

		enum class Type
		{
			Normal,
			VoiceStartOnly,
			numTypes
		};

		struct ConstructionData
		{
			ConstructionData(Processor* parent_, const String& id_, Type t_ = Type::Normal, Mode m_ = Mode::GainMode);

			Processor* parent;
			String id;
			Type t;
			Mode m;
		};

		class Buffer
		{
		public:

			Buffer();;

			void setMaxSize(int maxSamplesPerBlock_);

			bool isInitialised() const noexcept;

			// this array contains the actual modulation values that can be used by external processors;
			float* voiceValues = nullptr;

			// this array contains the monophonic modulation values that will be applied to each voice
			float* monoValues = nullptr;

			// this array contains the current voice's modulation values
			float* scratchBuffer = nullptr;

			void clear();

		private:

			void updatePointers();

			HeapBlock<float> data;
			int allocated = 0;
			int maxSamplesPerBlock = 0;
		};

		

		ModChainWithBuffer(ConstructionData data);

		~ModChainWithBuffer();

		/** Sets up the modulator chain and the buffer if it's not a voice start only chain. */
		void prepareToPlay(double sampleRate, int samplesPerBlock);

		/** Returns the modulator chain. Try to avoid this method when possible. */
		ModulatorChain* getChain() noexcept;;

		/** Calls the chain method. Use this instead the direct call. */
		void resetVoice(int voiceIndex);

		/** Calls the chain method. Use this instead the direct call. */
		void stopVoice(int voiceIndex);

		/** Calls the chain method. Use this instead the direct call. */
		void startVoice(int voiceIndex);

		/** Calls the chain method. Use this instead the direct call. */
		void handleHiseEvent(const HiseEvent& m);

		/** Call this to calculate the monophonic values. 
		*
		*	Normally you don't need to use this method, as the synth / effect will take care of that
		*/
		void calculateMonophonicModulationValues(int startSample, int numSamples);

		/** Call this to calculate the monophonic values.
		*
		*	Normally you don't need to use this method, as the synth / effect will take care of that.
		*
		*	The startSample / numSample arguments passed in here are supposed to be at audio rate and will
		*	be converted to control rate internally.
		*
		*	Make sure you've expanded the values before using them.
		*/
		void calculateModulationValuesForCurrentVoice(int voiceIndex, int startSample, int numSamples);

		/** This multiplies the modulation values with the given AudioSampleBuffer. 
		*
		*	Make sure you've expanded the values before using this.
		*/
		void applyMonophonicModulationValues(AudioSampleBuffer& b, int startSample, int numSamples);

		/** Returns a read pointer to the monophonic modulation values or nullptr, if there is no active monophonic modulation.
		*
		*	You can only call this method if you didn't use them already in the voice modulation.
		*/
		const float* getMonophonicModulationValues(int startSample) const;

		/** Returns a read pointer to the current voice modulation value.
		*
		*	If the voice modulation is constant for the current sub-block, or there are no polyphonic modulators,
		*	it will return nullptr. In this case, use getConstantModulationValue() and adapt your processing logic accordingly.
		*
		*/
		const float* getReadPointerForVoiceValues(int startSample) const;

		/** Returns a read pointer to the current voice modulation value.
		*
		*	If the voice modulation is constant for the current sub-block, or there are no polyphonic modulators,
		*	it will return nullptr. In this case, use getConstantModulationValue() and adapt your processing logic accordingly.
		*
		*	The data can be overwritten, which is required if you need to further process these values, but if you don't need this,
		*	use the getReadPointerForVoiceValue() method.
		*/
		float* getWritePointerForVoiceValues(int startSample);

		/** If you're doing custom processing, use this method to get the pointer to the unexpanded values for the startSample offset.
		*
		*	Then, do your processing, and expand the values. Don't forget to store the ramp value after doing so:
		*
		*		auto modData = getWritePointerForVoiceValues(256);
		*
		*		// Process here...
		*
		*		expandValuesToAudioRate(voiceIndex, 256, 128);
		*		
		*		setCurrentRampValueForVoice(voiceIndex, lastComputedValue);
		*
		*		// Use these values as actual modulation data:
		*		auto realModData = getReadPointerForVoice(256);
		*
		*/
		float* getWritePointerForManualExpansion(int startSample);

		/** Returns the constant modulation value for the current sub block. 
		*
		*	Even if the dynamic data is quasi-constant, it will detect it and set this value to the current dynamic value.
		*/
		float getConstantModulationValue() const;

		/** Returns the first value in the modulation data or the constant value. */
		float getOneModulationValue(int startSample) const;

		float getModValueForVoiceWithOffset(int startSample) const;

		/** Returns the scratch buffer. The scratch buffer is a aligned float array that's most likely in the cache,
		*   but using this is rather hacky, so don't use it if there's another option. 
		*/
		float* getScratchBuffer();

		/** Setting this to true will allow write access to the current modulation values. Default is read-only.
		*
		*	It comes with a tiny overhead because the monophonic values have to be copied to the voice buffer so it's disabled by default.
		*	If you intend to call getWritePointerForVoiceValues(), you need to set this to true before. */
		void setAllowModificationOfVoiceValues(bool mightBeOverwritten);

		/** This makes the voice modulation include the monophonic data. Default is enabled.
		*
		*	In most cases, you want the monophonic modulation to be part of the voice modulation.
		*/
		void setIncludeMonophonicValuesInVoiceRendering(bool shouldInclude);

		/** Call this when there's nothing to do to reset the modulation chain. 
		*
		*	It clears the internal buffers and sets the constant value to the default.
		*/
		void clear();

		/** This automatically expands the control rate values to audio rate after calculation. Default is disabled.
		*
		*	If you intend to use the modulation values at audio rate, you need to enable this or manually call the expandXXX() methods.
		*	If you just want one modulation value or do custom processing before expanding, leave this at false.
		*/
		void setExpandToAudioRate(bool shouldExpandAfterRendering);

		void expandVoiceValuesToAudioRate(int voiceIndex, int startSample, int numSamples);

		void expandMonophonicValuesToAudioRate(int startSample, int numSamples);

		/** If you're doing the expansion manually, you can update the current ramp value with this method. */
		void setCurrentRampValueForVoice(int voiceIndex, float value) noexcept;

		bool isAudioRateModulation() const noexcept;

		struct Options
		{
			bool expandToAudioRate = false;
			bool includeMonophonicValues = true;
			bool voiceValuesReadOnly = true;
		};

		void setDisplayValue(float v);

		void setScratchBufferFunction(const std::function<void(int, Modulator* m, float*, int, int)>& f);

	private:

		std::function<void(int, Modulator* m, float*, int, int)> scratchBufferFunction;

		void applyMonophonicValuesToVoiceInternal(float* voiceBuffer, float* monoBuffer, int numSamples);

		void setDisplayValueInternal(int voiceIndex, int startSample, int numSamples);

		void setConstantVoiceValueInternal(int voiceIndex, float newValue);

		ScopedPointer<ModulatorChain> c;

		Type type;

		Buffer modBuffer;

		bool monoExpandChecker = false;
		bool polyExpandChecker = false;

		bool manualExpansionPending = false;

		

		Options options;
		
		float currentConstantValue = 1.0f;

		float currentMonoValue = 1.0f;
		float lastConstantVoiceValue = 1.0f;
		float currentConstantVoiceValues[NUM_POLYPHONIC_VOICES];
		float currentRampValues[NUM_POLYPHONIC_VOICES];
		
		float currentMonophonicRampValue;
		float const* currentVoiceData = nullptr;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModChainWithBuffer);
	};

	using ModulationType = ModChainWithBuffer::Type;
	using Collection = PreallocatedHeapArray<ModChainWithBuffer, ModChainWithBuffer::ConstructionData>;
	
	SET_PROCESSOR_NAME("ModulatorChain", "Modulator Chain", "chain")

	class ModulatorChainHandler;

	/** Creates a new modulator chain. You have to specify the voice amount and the Modulation::Mode */
	ModulatorChain(MainController *mc, const String &id, int numVoices, Modulation::Mode m, Processor *p);

	~ModulatorChain();;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	
	/** Returns the handler that is used to add / delete Modulators in the chain. Use this if you want to change the modulator. */
	Chain::Handler *getHandler() override;

	const Chain::Handler *getHandler() const override;;

	FactoryType *getFactoryType() const override;;

	Colour getColour() const override;;

	void setFactoryType(FactoryType *newFactoryType) override;;

	/** Sets the sample rate for all modulators in the chain and initialized the UpdateMerger. */
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	
	void setMode(Mode newMode, NotificationType n) override;

	bool hasActivePolyMods() const noexcept;
	bool hasActiveVoiceStartMods() const noexcept;
	bool hasActiveTimeVariantMods() const noexcept;
	bool hasActivePolyEnvelopes() const noexcept;
	bool hasActiveMonoEnvelopes() const noexcept;
	bool hasActiveEnvelopesAtAll() const noexcept;
	bool hasOnlyVoiceStartMods() const noexcept;

	bool hasTimeModulationMods() const noexcept;
	bool hasMonophonicTimeModulationMods() const noexcept;
	bool hasVoiceModulators() const noexcept;

	bool shouldBeProcessedAtAll() const noexcept;

	void syncAfterDelayStart(bool waitForDelay, int voiceIndex) override;

	/** Wraps the handlers method. */
	int getNumChildProcessors() const override;;

	/** Wraps the handlers method. */
	Processor *getChildProcessor(int processorIndex) override;;

	Processor *getParentProcessor() override;;

	const Processor *getParentProcessor() const override;;

	/** Wraps the handlers method. */
	const Processor *getChildProcessor(int processorIndex) const override;;

	/** Resets all envelopes. This can be used for envelopes that do not control the main gain value
	*	and therefore do not switch back to IDLE if the note stops playing before the envelope is finished.
	*
	*	Also their display is updated to clear remaining tails (that aren't cleared because of a missing update call to UpdateMerger.
	*/
	void reset(int voiceIndex) override;
	
	/** Sends the midi message all modulators. 
	*
	*	You can't use the voice index here, since it is not guaranteed if the message starts a note.
	*
	*/
	void handleHiseEvent(const HiseEvent& m) override;

	void allNotesOff() override;

	/** Checks if any of the EnvelopeModulators wants to keep the voice from being killed. 
	*
	*	If no envelopes are active, it checks if the voice was recently started or stopped using an internal array.
	*/
	bool isPlaying(int voiceIndex) const override;

	/** Calls the start voice function for all voice start modulators and envelope modulators and sends a display message. */
	float startVoice(int voiceIndex) override;;

	/** Calls the stopVoice function for all envelope modulators. */
	void stopVoice(int voiceIndex) override;

	void setInternalAttribute(int, float) override;; // nothing to do here!

	float getAttribute(int) const override;; // nothing to do here!

	float getCurrentMonophonicStartValue() const noexcept;

	ModulatorState *createSubclassedState(int) const override;; // a chain itself has no states!

	/** If you want the chain to only process voice start modulators, set this to true. */
	void setIsVoiceStartChain(bool isVoiceStartChain_);

	/** This overrides the TimeVariant::renderNextBlock method and only calculates the TimeVariant modulators.
	*
	*	It assumes that the other modulators are calculated before with renderVoice().
	*
	*	@param buffer the buffer that will be filled with the values of the timevariant modulation result.
	*/
	void newRenderMonophonicValues(int startSample, int numSamples);

	/** Does nothing (the complete renderNextBlock method is overwritten. */
	void calculateBlock(int /*startSample*/, int /*numSamples*/) override;;

	/** Iterates all voice start modulators and returns the value either between 0.0 and 1.0 (GainMode) or -1.0 ... 1.0 (Pitch Mode). */
	float getConstantVoiceValue(int voiceIndex) const;

	Table::ValueTextConverter getTableValueConverter() const;

	void setPostEventFunction(const std::function<void(Modulator*, const HiseEvent&)>& pf);

	void applyMonoOnOutputValue(float monoValue);

public:

	void setTableValueConverter(const Table::ValueTextConverter& converter);;

	/** This class handles the Modulators within the specified ModulatorChain.
	*
	*	You can get the handler for each Modulator with ModulatorChain::getHandler().
	*
	*/
	class ModulatorChainHandler : public Chain::Handler,
								  public Processor::BypassListener
	{
	public:

        struct ModSorter
        {
            ModSorter(ModulatorChainHandler& parent_);;
            
            bool operator()(Modulator* f, Modulator* s) const;;
            
            ModulatorChainHandler& parent;
        };
        
		/** Creates a Chain::Handler. */
		ModulatorChainHandler(ModulatorChain *handledChain);;

		~ModulatorChainHandler();;

		void bypassStateChanged(Processor* p, bool bypassState) override;

		/** adds a Modulator to the chain.
		*
		*	You simply pass the reference to the newly created Modulator, and the function detects
		*	the correct type and adds it to the specific chain (constant, variant or envelope).
		*
		*	If you call this method after the ModulatorChain is initialized, the Modulator's prepareToPlay will be called.
		*/
		void addModulator(Modulator *newModulator, Processor *siblingToInsertBefore);

		void add(Processor *newProcessor, Processor *siblingToInsertBefore) override;
	
		/** Deletes the Modulator. */
		void deleteModulator(Modulator *modulatorToBeDeleted, bool deleteMod);

		void remove(Processor *processorToBeRemoved, bool deleteProcessor=true) override;

		/** Returns the modulator at the specified index. */
		Modulator *getModulator(int modIndex) const;;

		/** Returns the number of modulators in the chain. */
		int getNumModulators() const;;

		Processor *getProcessor(int processorIndex);;

		const Processor *getProcessor(int processorIndex) const;;

		virtual int getNumProcessors() const;;

		void clear() override;


        Table::ValueTextConverter tableValueConverter;

		bool hasActiveEnvelopes() const noexcept;;
		bool hasActiveTimeVariantMods() const noexcept;;
		bool hasActiveVoiceStartMods() const noexcept;;
		bool hasActiveMonophoicEnvelopes() const noexcept;;
		bool hasActiveMods() const noexcept;

        UnorderedStack<VoiceStartModulator*, HISE_NUM_MODULATORS_PER_CHAIN> activeVoiceStartList;
		UnorderedStack<TimeVariantModulator*, HISE_NUM_MODULATORS_PER_CHAIN> activeTimeVariantsList;
		UnorderedStack<EnvelopeModulator*, HISE_NUM_MODULATORS_PER_CHAIN> activeEnvelopesList;
		UnorderedStack<Modulator*, HISE_NUM_MODULATORS_PER_CHAIN*3> activeAllList;
		UnorderedStack<MonophonicEnvelope*, HISE_NUM_MODULATORS_PER_CHAIN> activeMonophonicEnvelopesList;

	private:

		void checkActiveState();

		bool activeVoiceStarts = false;
		bool activeEnvelopes = false;
		bool activeTimeVariants = false;
		bool activeMonophonicEnvelopes = false;
		bool anyActive = false;

		ModulatorChain *chain;
	};

private:

    std::function<void(Modulator* m, const HiseEvent& e)> postEventFunction;
    
	friend class GlobalModulatorContainer;

	// Checks if the Modulators are initialized correctly and are set to the right voices */
	bool checkModulatorStructure();

	BigInteger activeVoices;

	ScopedPointer<FactoryType> modulatorFactory;
	
	ModulatorChainHandler handler;

	OwnedArray<VoiceStartModulator> voiceStartModulators;
	OwnedArray<EnvelopeModulator> envelopeModulators;
	OwnedArray<TimeVariantModulator> variantModulators;

	Processor *parentProcessor;

	Array<Modulator*> allModulators;

	int blockSize;
	
	Identifier chainIdentifier;

	float lastVoiceValues[NUM_POLYPHONIC_VOICES];
	float monophonicStartValue = 1.0f;

	bool isVoiceStartChain;

	float voiceOutputValue;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorChain)
};


struct ModBufferExpansion
{

	static bool isEqual(float rampStart, const float* data, int numElements);

	/** Expands the data found in modulationData + startsample according to the HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR.
	*
	*	It updates the rampstart and returns true if there was movement in the modulation data.
	*
	*/
	static bool expand(const float* modulationData, int startSample, int numSamples, float& rampStart);
};

/**	Allows creation of TimeVariantModulators.
*
*	This holds three different FactoryTypes and enables nice popup menus.
*	You should not bother with this class. If you write new Modulators, put it in the FactoryType of the Modulator subclass and it will be handled automatically.
*
*	@ingroup factory
*/
class ModulatorChainFactoryType: public FactoryType
{
public:

	enum Factories
	{
		VoiceStart = 0,
		Time,
		Envelope,
		numSubFactories
	};

	ModulatorChainFactoryType(int numVoices_, Modulation::Mode m, Processor *p):
		FactoryType(p),
		timeVariantFactory(new TimeVariantModulatorFactoryType(m, p)),
		voiceStartFactory(new VoiceStartModulatorFactoryType(numVoices_, m, p)),
        envelopeFactory(new EnvelopeModulatorFactoryType(numVoices_, m, p))
	{
		typeNames.addArray(voiceStartFactory->getAllowedTypes());
		typeNames.addArray(timeVariantFactory->getAllowedTypes());
		typeNames.addArray(envelopeFactory->getAllowedTypes());
	}
	
	int fillPopupMenu(PopupMenu &menu, int startIndex=1) override;

	/** Returns the desired FactoryType. Use this for popup menus. */
	FactoryType *getSubFactory(Factories f)
	{
		switch(f)
		{
		case VoiceStart:	return voiceStartFactory;
		case Time:			return timeVariantFactory;
		case Envelope:		return envelopeFactory;
		default:			jassertfalse; return nullptr;
		}
	}

	void setConstrainer(FactoryType::Constrainer *c, bool ownConstrainer) override
	{
		FactoryType::setConstrainer(c, ownConstrainer);

		voiceStartFactory->setConstrainer(c, false);
		envelopeFactory->setConstrainer(c, false);
		timeVariantFactory->setConstrainer(c, false);
	}

	Processor *createProcessor(int typeIndex, const String &id);
	

	const Array<ProcessorEntry> &getTypeNames() const override
	{
		return typeNames;		
	};

private:

	Modulation::Mode m;
	int numVoices;

	Array<ProcessorEntry> typeNames;

	ScopedPointer<VoiceStartModulatorFactoryType> voiceStartFactory;
	ScopedPointer<TimeVariantModulatorFactoryType> timeVariantFactory;
	ScopedPointer<EnvelopeModulatorFactoryType> envelopeFactory;
};

} // namespace hise
#endif  // ModulatorChainProcessor_H_INCLUDED
