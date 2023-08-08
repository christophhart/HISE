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

#ifndef GLOBALMODULATORS_H_INCLUDED
#define GLOBALMODULATORS_H_INCLUDED

 namespace hise { using namespace juce;

class GlobalModulatorContainer;

/** A modulator that connects to a global modulator and uses its signal.
	@ingroup modulator
	
	If you need to apply a modulation signal to multiple targets, use a GlobalModulatorContainer where you 
	define the actual modulation and create instances of this class for each target.
*/
class GlobalModulator: public LookupTableProcessor,
					   public Chain::Handler::Listener
{
public:

	/** Special Parameters for the Global Modulators */
	enum Parameters
	{
		UseTable = 0,
		Inverted,
		numTotalParameters
	};

	enum ModulatorType
	{
		VoiceStart,
		TimeVariant,
		StaticTimeVariant,
		Envelope,
		numTypes
	};

	virtual ModulatorType getModulatorType() const = 0;
	virtual ~GlobalModulator();

    void referenceShared(ExternalData::DataType, int) override
    {
        table = getTableUnchecked(0);
    }
    
	Modulator *getOriginalModulator();
	const Modulator *getOriginalModulator() const;
	GlobalModulatorContainer *getConnectedContainer();

	void processorChanged(EventType t, Processor* p) override;

	const GlobalModulatorContainer *getConnectedContainer() const;

	StringArray getListOfAllModulatorsWithType();

	void disconnect();

	bool connectToGlobalModulator(const String &itemEntry);

	bool isConnected() const { return getConnectedContainer() != nullptr && getOriginalModulator() != nullptr; };

	static String getItemEntryFor(const GlobalModulatorContainer *c, const Processor *p);

	void saveToValueTree(ValueTree &v) const;
	void loadFromValueTree(const ValueTree &v);

    void connectIfPending();
    
protected:

	GlobalModulator(MainController *mc);

	SampleLookupTable* table;

	bool useTable = false;
	bool inverted = false;

private:

    String pendingConnection;
    
	WeakReference<Processor> connectedContainer;

	WeakReference<Modulator> originalModulator;

	Array<WeakReference<GlobalModulatorContainer>> watchedContainers;
};

/** Deactivates Globals (this is used in Global Containers. */
class NoGlobalsConstrainer : public FactoryType::Constrainer
{
	String getDescription() const override { return "No global modulators"; }

	bool allowType(const Identifier &typeName) override
	{
        return true;//!typeName.toString().startsWith("Global");
	}
};

/** Deactivates Global Envelopes (this is used in Global Containers. */
class NoGlobalEnvelopeConstrainer : public FactoryType::Constrainer
{
	bool allowType(const Identifier &typeName) override
	{
		return !typeName.toString().startsWith("GlobalEnvelope");
	}

	String getDescription() const override { return "No global modulators"; }
};

/** A modulator that connects to a global VoiceStartModulator (eg. Velocity).
	@ingroup modulatorTypes	
*/
class GlobalVoiceStartModulator : public VoiceStartModulator,
								  public GlobalModulator
{
public:

	SET_PROCESSOR_NAME("GlobalVoiceStartModulator", "Global Voice Start Modulator", "A modulator that connects to a global VoiceStartModulator (eg. Velocity).");

	GlobalModulator::ModulatorType getModulatorType() const override { return GlobalModulator::VoiceStart; };

	GlobalVoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	~GlobalVoiceStartModulator();

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

    void prepareToPlay(double sampleRate, int blockSize) override
    {
        VoiceStartModulator::prepareToPlay(sampleRate, blockSize);
        connectIfPending();
    }
    
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getAttribute(int parameterIndex) const override;;

	/** Calculates a new random value. If the table is used, it is converted to 7bit.*/
	float calculateVoiceStartValue(const HiseEvent& ) override;;
};

/** A voice start modulator that connects to a global TimeVariantModulator (eg. LFO).
	@ingroup modulatorTypes

	Note that this is a special class that uses the current value from the given time-variant modulator
	at the time of the voice start for a constant / per voice modulation.
*/
class GlobalStaticTimeVariantModulator : public VoiceStartModulator,
										 public GlobalModulator
{
public:

	SET_PROCESSOR_NAME("GlobalStaticTimeVariantModulator", "Global Static Time Variant Modulator", "A voice start modulator that connects to a global TimeVariantModulator (eg. LFO).");

	GlobalModulator::ModulatorType getModulatorType() const override { return GlobalModulator::StaticTimeVariant; };

	GlobalStaticTimeVariantModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	~GlobalStaticTimeVariantModulator();

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

    void prepareToPlay(double sampleRate, int blockSize) override
    {
        VoiceStartModulator::prepareToPlay(sampleRate, blockSize);
        connectIfPending();
    }
    
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getAttribute(int parameterIndex) const override;;

	/** Calculates a new random value. If the table is used, it is converted to 7bit.*/
	float calculateVoiceStartValue(const HiseEvent&) override;;
};

/** A modulator that connects to a global TimeVariantModulator (eg. LFO).
@ingroup modulatorTypes
*/
class GlobalTimeVariantModulator : public TimeVariantModulator,
								   public GlobalModulator
{
public:

	SET_PROCESSOR_NAME("GlobalTimeVariantModulator", "Global Time Variant Modulator", "A modulator that connects to a global TimeVariantModulator (eg. LFO).");

	GlobalModulator::ModulatorType getModulatorType() const override { return GlobalModulator::TimeVariant; };

	GlobalTimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m);

	~GlobalTimeVariantModulator() { };

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getAttribute(int parameterIndex) const override;;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final { return nullptr; };

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final { return nullptr; };

	virtual int getNumChildProcessors() const override final { return 0; };

	void calculateBlock(int startSample, int numSamples) override;

	void invertBuffer(int startSample, int numSamples);

	/** sets the new target value if the controller number matches. */
	void handleHiseEvent(const HiseEvent &/*m*/) override {};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
        connectIfPending();
		TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
	};
	
private:

	float inputValue;

	float currentValue;

};

class GlobalEnvelopeModulator : public EnvelopeModulator,
								public GlobalModulator
{
public:

	SET_PROCESSOR_NAME("GlobalEnvelopeModulator", "Global Envelope Modulator", "A modulator that connects to a global EnvelopeModulator (eg. AHDSR).");

	GlobalModulator::ModulatorType getModulatorType() const override { return GlobalModulator::Envelope; };

	GlobalEnvelopeModulator(MainController *mc, const String &id, Modulation::Mode m, int numVoices);;

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getAttribute(int parameterIndex) const override;;

	float startVoice(int voiceIndex) override;

	void stopVoice(int voiceIndex) override;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final { return nullptr; };

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final { return nullptr; };

	virtual int getNumChildProcessors() const override final { return 0; };

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	virtual bool isPlaying(int voiceIndex) const override;

	virtual ModulatorState * createSubclassedState(int voiceIndex) const override;

	void calculateBlock(int startSample, int numSamples) override;

	uint8 active[NUM_POLYPHONIC_VOICES];
};


} // namespace hise

#endif  // GLOBALMODULATORS_H_INCLUDED
