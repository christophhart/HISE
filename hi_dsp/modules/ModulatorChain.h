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

/** A chain of Modulators that can be processed serially.
*
*	@ingroup modulatorTypes
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

	SET_PROCESSOR_NAME("ModulatorChain", "Modulator Chain")

	class ModulatorChainHandler;

	/** Creates a new modulator chain. You have to specify the voice amount and the Modulation::Mode */
	ModulatorChain(MainController *mc, const String &id, int numVoices, Modulation::Mode m, Processor *p);

	~ModulatorChain() {	allModulators.clear(); };

	

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	/** Returns the handler that is used to add / delete Modulators in the chain. Use this if you want to change the modulator. */
	Chain::Handler *getHandler() override;

	const Chain::Handler *getHandler() const override {return &handler;};

	FactoryType *getFactoryType() const override {return modulatorFactory;};

	Colour getColour() const override
	{ 
		if(Modulator::getColour() != Colours::transparentBlack) return Modulator::getColour();
		else
		{
			//return getMode() == GainMode ? Colour(0xffD9A450) : Colour(0xff628214);
			if (getMode() == GainMode)
			{
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xffbe952c));
			}
			else
			{
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xff7559a4));
			}
		}
	};

	void setFactoryType(FactoryType *newFactoryType) override {modulatorFactory = newFactoryType;};

	/** Sets the sample rate for all modulators in the chain and initialized the UpdateMerger. */
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	
	/** Checks if the chain is bypassed or contains no modulators. 
	*
	*	@param checkPolyphonicModulators if true, it searches the voice start modulators and envelopes.
	*/
	bool shouldBeProcessed(bool checkPolyphonicModulators) const;

	/** Wraps the handlers method. */
	int getNumChildProcessors() const override { return handler.getNumProcessors();	};

	/** Wraps the handlers method. */
	Processor *getChildProcessor(int processorIndex) override { return handler.getProcessor(processorIndex);	};

	Processor *getParentProcessor() override { return parentProcessor; };

	const Processor *getParentProcessor() const override { return parentProcessor; };

	/** Wraps the handlers method. */
	const Processor *getChildProcessor(int processorIndex) const override{ return handler.getProcessor(processorIndex);	};

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

	/** Checks if any of the EnvelopeModulators wants to keep the voice from being killed. 
	*
	*	If no envelopes are active, it checks if the voice was recently started or stopped using an internal array.
	*/
	bool isPlaying(int voiceIndex) const override;

	/** Calls the start voice function for all voice start modulators and envelope modulators and sends a display message. */
	void startVoice(int voiceIndex) override;;

	/** Iterates all voice start modulators and returns the value either between 0.0 and 1.0 (GainMode) or -1.0 ... 1.0 (Pitch Mode). */
	float getConstantVoiceValue(int voiceIndex) const;

	/** Calls the stopVoice function for all envelope modulators. */
	void stopVoice(int voiceIndex) override;

	void setInternalAttribute(int, float) override { jassertfalse; }; // nothing to do here!

	float getAttribute(int) const override { jassertfalse; return -1.0; }; // nothing to do here!

	/** Iterates all VoiceStartModulators and EnvelopeModulators and stores their values in the internal voice buffer.
	*
	*	You can use getVoiceValues() to retrieve these values at a later time.
	*/
	void renderVoice(int voiceIndex, int startSample, int numSamples);

	/** Returns a read pointer to the calculated voice values. The array size is supposed to be the size of the internal buffer. */
	float *getVoiceValues(int voiceIndex) noexcept
	{ return internalVoiceBuffer.getWritePointer(voiceIndex); };

	/** Returns a write pointer to the calculated voice values. You can change them and the array size should be known. */
	const float *getVoiceValues(int voiceIndex) const noexcept
	{ return internalVoiceBuffer.getReadPointer(voiceIndex); }

	/** This ocverrides the TimeVariant::renderNextBlock method and only calculates the TimeVariant modulators.
	*
	*	It assumes that the other modulators are calculated before with renderVoice().
	*
	*	@param buffer the buffer that will be filled with the values of the timevariant modulation result.
	*/
	void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override;

	/** Does nothing (the complete renderNextBlock method is overwritten. */
	void calculateBlock(int /*startSample*/, int /*numSamples*/) override
	{
	};

	ModulatorState *createSubclassedState(int) const override { jassertfalse; return nullptr; }; // a chain itself has no states!

	/** Call this function if you want to process each sample at one time. This is useful for internal ModulatorChains.
	*
	*	You have to set the voiceIndex with polyMangager.setCurrentVoice() and clear it afterwards!
	*/
	float calculateNewValue();

	/** If you want the chain to only process voice start modulators, set this to true. */
	void setIsVoiceStartChain(bool isVoiceStartChain_);

	/** This renders all modulators as they were monophonic. This is useful for ModulatorChains that are not interested in polyphony (eg internal chains of non-polyphonic Modulators, but want to process polyphonic modulators.
	*
	*	The best thing is to use this method after / or before all voices are rendered.
	*/
	void renderAllModulatorsAsMonophonic(AudioSampleBuffer &buffer, int startSample, int numSamples);

	void setTableValueConverter(const Table::ValueTextConverter& converter)
	{
		handler.tableValueConverter = converter;
	};

	/** This class handles the Modulators within the specified ModulatorChain.
	*
	*	You can get the handler for each Modulator with ModulatorChain::getHandler().
	*
	*/
	class ModulatorChainHandler : public Chain::Handler,
								  public Processor::BypassListener
	{
	public:

		/** Creates a Chain::Handler. */
		ModulatorChainHandler(ModulatorChain *handledChain);;

		~ModulatorChainHandler() {};

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
		Modulator *getModulator(int modIndex) const
		{
			jassert(modIndex < getNumModulators());
			return chain->allModulators[modIndex];
		};

		/** Returns the number of modulators in the chain. */
		int getNumModulators() const { return chain->allModulators.size(); };

		Processor *getProcessor(int processorIndex)
		{
			return chain->allModulators[processorIndex];
		};

		const Processor *getProcessor(int processorIndex) const
		{
			return chain->allModulators[processorIndex];
		};

		virtual int getNumProcessors() const
		{
			return getNumModulators();
		};

		void clear() override
		{
			notifyListeners(Listener::Cleared, nullptr);

			chain->envelopeModulators.clear();
			chain->variantModulators.clear();
			chain->voiceStartModulators.clear();
			chain->allModulators.clear();
		}

		
		Table::ValueTextConverter tableValueConverter;

		bool hasActivePolyModulators() const noexcept { return activePoly; };
		bool hasActiveMonoModulators() const noexcept { return activeMono; };

	private:

		void checkActiveState();

		bool activePoly = false;
		bool activeMono = false;

		ModulatorChain *chain;

	};

private:

	// Checks if the Modulators are initialized correctly and are set to the right voices */
	bool checkModulatorStructure();

	BigInteger activeVoices;

	

	

	CriticalSection lock;

	ScopedPointer<FactoryType> modulatorFactory;
	
	// A AudioSampleBuffer with one channel per voice
	AudioSampleBuffer internalVoiceBuffer;
	
	AudioSampleBuffer envelopeTempBuffer;

	ModulatorChainHandler handler;

	OwnedArray<VoiceStartModulator> voiceStartModulators;
	OwnedArray<EnvelopeModulator> envelopeModulators;
	OwnedArray<TimeVariantModulator> variantModulators;

	Processor *parentProcessor;

	Array<float> calculatedConstantVoiceValues;

	Array<Modulator*> allModulators;

	int blockSize;
	
	Identifier chainIdentifier;

	// the values of the envelope of the first voice is stored here to retrieve it at renderNextBlock...
	float envelopeOutputValue1 = 0.0f;
	float envelopeOutputValue2 = 0.0f;
	float envelopeOutputValue3 = 0.0f;
	float envelopeOutputValue4 = 0.0f;

	float lastVoiceValues[NUM_POLYPHONIC_VOICES];

	bool isVoiceStartChain;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorChain)

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
	
	int fillPopupMenu(PopupMenu &menu, int startIndex=1) override
	{
		int index = startIndex;

		PopupMenu voiceMenu;
		index = voiceStartFactory->fillPopupMenu(voiceMenu, index);
		menu.addSubMenu("VoiceStart", voiceMenu);

		PopupMenu timeMenu;
		index = timeVariantFactory->fillPopupMenu(timeMenu, index);
		menu.addSubMenu("TimeVariant", timeMenu);

		PopupMenu envelopes;
		index = envelopeFactory->fillPopupMenu(envelopes, index);
		menu.addSubMenu("Envelopes", envelopes);

		return index;
	}

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
