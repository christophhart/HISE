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

#ifndef GLOBALMODULATORS_H_INCLUDED
#define GLOBALMODULATORS_H_INCLUDED

 

class GlobalModulatorContainer;


class GlobalModulator: public LookupTableProcessor,
					   public SafeChangeListener
{
public:

	/** Special Parameters for the Global Modulators */
	enum Parameters
	{
		UseTable = 0,
		numTotalParameters
	};

	enum ModulatorType
	{
		VoiceStart,
		TimeVariant,
		Envelope,
		numTypes
	};

	

	virtual ModulatorType getModulatorType() const = 0;

	virtual ~GlobalModulator();

	Table *getTable(int /*tableIndex*/) const override { return table; }

	Modulator *getOriginalModulator();

	const Modulator *getOriginalModulator() const;

	GlobalModulatorContainer *getConnectedContainer();

	void changeListenerCallback(SafeChangeBroadcaster *)
	{
		dynamic_cast<Processor*>(this)->sendSynchronousChangeMessage();
	}

	const GlobalModulatorContainer *getConnectedContainer() const;

	StringArray getListOfAllModulatorsWithType();

	void connectToGlobalModulator(const String &itemEntry);

	bool isConnected() const { return getConnectedContainer() != nullptr && getOriginalModulator() != nullptr; };

	static String getItemEntryFor(const GlobalModulatorContainer *c, const Processor *p);

	void saveToValueTree(ValueTree &v) const;

	void loadFromValueTree(const ValueTree &v);

	void removeFromAllContainers();

protected:

	GlobalModulator(MainController *mc);

	ScopedPointer<MidiTable> table;

	bool useTable;

private:

	WeakReference<Processor> connectedContainer;

	WeakReference<Processor> originalModulator;

};

/** Deactivates Globals (this is used in Global Containers. */
class NoGlobalsConstrainer : public FactoryTypeConstrainer
{
	bool allowType(const Identifier &typeName) override
	{
		return !typeName.toString().startsWith("Global");
	}
};

/** Deactivates Global Envelopes (this is used in Global Containers. */
class NoGlobalEnvelopeConstrainer : public FactoryTypeConstrainer
{
	bool allowType(const Identifier &typeName) override
	{
		return !typeName.toString().startsWith("GlobalEnvelope");
	}
};

class GlobalVoiceStartModulator : public VoiceStartModulator,
								  public GlobalModulator
{
public:

	SET_PROCESSOR_NAME("GlobalVoiceStartModulator", "Global Voice Start Modulator");

	GlobalModulator::ModulatorType getModulatorType() const override { return GlobalModulator::VoiceStart; };

	GlobalVoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	~GlobalVoiceStartModulator();

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getAttribute(int parameterIndex) const override;;

	/** Calculates a new random value. If the table is used, it is converted to 7bit.*/
	float calculateVoiceStartValue(const MidiMessage &) override;;
};

class GlobalTimeVariantModulator : public TimeVariantModulator,
								   public GlobalModulator
{
public:

	SET_PROCESSOR_NAME("GlobalTimeVariantModulator", "Global Time Variant Modulator");

	GlobalModulator::ModulatorType getModulatorType() const override { return GlobalModulator::TimeVariant; };

	GlobalTimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m);

	~GlobalTimeVariantModulator() { removeFromAllContainers(); };

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;;

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getAttribute(int parameterIndex) const override;;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final { return nullptr; };

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final { return nullptr; };

	virtual int getNumChildProcessors() const override final { return 0; };

	void calculateBlock(int startSample, int numSamples) override;

	/** sets the new target value if the controller number matches. */
	void handleMidiEvent(const MidiMessage &/*m*/) override {};

	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
	};
	
private:

	float inputValue;

	float currentValue;

};

/** This Modulator gets its values from the connected modulator in the global container chain.
*
*	This is a quite experimental feature - do not use it on other chains than the gain chain or in combination with other envelopes - as soon as the voice indexes don't match anymore, the results are unpredictable!
*
*/
class GlobalEnvelopeModulator : public EnvelopeModulator,
								public GlobalModulator
{
public:

	SET_PROCESSOR_NAME("GlobalEnvelopeModulator", "Global Envelope Modulator");

	GlobalModulator::ModulatorType getModulatorType() const override { return GlobalModulator::Envelope; };

	GlobalEnvelopeModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	~GlobalEnvelopeModulator() { removeFromAllContainers(); };

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getAttribute(int parameterIndex) const override;;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final { return nullptr; };

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final { return nullptr; };

	virtual int getNumChildProcessors() const override final { return 0; };

	void startVoice(int /*voiceIndex*/) override {};

	void stopVoice(int /*voiceIndex*/) override {};

	void calculateBlock(int startSample, int numSamples) override;;

	void reset(int voiceIndex) override
	{
		EnvelopeModulator::reset(voiceIndex);
	};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);
	}

	/** @brief returns \c true, if the envelope is not IDLE and not bypassed. */
	bool isPlaying(int voiceIndex) const override;;

	/// @brief handles note-on and note-off messages and switches the internal state
	void handleMidiEvent(MidiMessage const &/*m*/) {};

	ModulatorState *createSubclassedState(int /*voiceIndex*/) const override { return nullptr; };
};



#endif  // GLOBALMODULATORS_H_INCLUDED
