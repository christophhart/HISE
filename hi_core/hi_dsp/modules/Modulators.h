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

#ifndef MODULATORS_H_INCLUDED
#define MODULATORS_H_INCLUDED

namespace hise { using namespace juce;

#pragma warning( push )
#pragma warning( disable: 4589 )


class Plotter;

class ModulatorChain;
class ModulatorEditor;

/** This is the base class for all modulation behaviour. 
*
*	@ingroup modulator
*/
class Modulation
{
public:

	

	static String getDomainAsMidiRange(float input);;

	static String getDomainAsPitchBendRange(float input);;

	static String getDomainAsMidiNote(float input);;

	static String getValueAsDecibel(float input);;

	static String getValueAsSemitone(float input);

	/** There are two modes that Modulation can work: GainMode and PitchMode */
	enum Mode
	{
		/** The supplied buffer is multiplied with the modulation value (like a gain effect). */
		GainMode = 0, 
		/** The modulation value is added to the supplied buffer, which is assumed to be used as temporary storage for pitch modulation. 
		*
		*	The range is supposed to be -1.0 ... 1.0 and must be converted using convertToFreqRatio directly before using it.
		**/
		PitchMode,
		/** Range is -1.0 ... 1.0 */
		PanMode,
		/** Range is -1.0 ... 1.0 and the intensity will be ignored. */
		GlobalMode,
		numModes
	};

	static void applyModulationValue(Mode m, float& target, const float modValue);

	Modulation(Mode m);;

    virtual ~Modulation();;

	virtual Processor *getProcessor() = 0;

	/** returns the mode the Modulator is operating. */
	Mode getMode() const noexcept;;

	/** This applies the intensity to the given value and returns the applied value. 
	*
	*	- In GainMode the input is supposed to be between 0.0 and 1.0. The output will be between 0.0 and 1.0 (and 1.0 if the intensity is 0.0)
	*	- in PitchMode the input is supposed to be between 0.0 and 1.0. The output will be between -1.0 and 1.0 (and 0.0 if the intensity is 0.0)
	*/
	float calcIntensityValue(float calculatedModulationValue) const noexcept;;

	inline float calcGainIntensityValue(float calculatedModulationValue) const noexcept;

	inline float calcPitchIntensityValue(float calculatedModulationValue) const noexcept;

	inline float calcPanIntensityValue(float calculatedModulationValue) const noexcept;

	inline float calcGlobalIntensityValue(float calculatedModulationValue) const noexcept;

	/** This applies the previously calculated value to the supplied destination value depending on the modulation mode (adding or multiplying). */
	void applyModulationValue(float calculatedModulationValue, float &destinationValue) const noexcept;;

	/** Sets the intensity of the modulation. The intensity is multiplied with the outcome or added depending on the TargetMode of the owner ModulatorChain. 
	*
	*	In GainMode, the Intensity is between 0.0 and 1.0. In PitchMode, the Intensity can be between 0.5 and 2.0.
	*/
	void setIntensity(float newIntensity) noexcept;;

	/** Use this method to set the intensity from the ModulatorEditorHeader's intensity slider converting linear -12 ... 12 to log 0.5 ... 2. */
	void setIntensityFromSlider(float sliderValue) noexcept;;

	bool isBipolar() const noexcept;;

	void setIsBipolar(bool shouldBeBiPolar) noexcept;;

	float getInitialValue() const noexcept;

	/** Returns the intensity. This is used by the modulator chain to either multiply or add the outcome of the Modulation. 
	*
	*	You can subclass this method if you modulate the intensity. In this case, don't change the intensity directly (or you can't change it in the GUI anymore),
	*	but save the modulation value elsewhere and return the product:
	*
	*		virtual float getIntensity() const
	*		{
	*			return intensityModulation * Modulator::getIntensity();
	*		};
	*
	*/
	float getIntensity() const noexcept;;

	/** Returns the actual intensity of the Modulation. Use this for GUI displays, since getIntensity() could be overwritten and behave funky. */
	float getDisplayIntensity() const noexcept;;

	struct PitchConverters
	{
		static inline float octaveRangeToSignedNormalisedRange(float octaveValue);

		/** [-1 ... 1] => [0.5 ... 2.0] */
		static inline float normalisedRangeToPitchFactor(float range);

		/** [0.5 ... 2.0] => [-1 ... 1] */
		static inline float pitchFactorToNormalisedRange(float pitchFactor);

		static inline float octaveRangeToPitchFactor(float octaveValue);

		static inline void octaveRangeToSignedNormalisedRange(float* octaveValues, int numValues);

		static void normalisedRangeToPitchFactor(float* rangeValues, int numValues);

		static inline void octaveRangeToPitchFactor(float* octaveValues, int numValues);
	};

	public:


	void setPlotter(Plotter *targetPlotter);

	bool isPlotted() const;

	void pushPlotterValues(const float* b, int startSample, int numSamples);

	virtual bool shouldUpdatePlotter() const;;

	void deactivateIntensitySmoothing();

	virtual void setMode(Mode newMode, NotificationType n=dontSendNotification);

	LambdaBroadcaster<int> modeBroadcaster;
    LambdaBroadcaster<float> intensityBroadcaster;
    
protected:

	Mode modulationMode;

	LinearSmoothedValue<float> smoothedIntensity;

private:

    
    
	Component::SafePointer<Plotter> attachedPlotter;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Modulation)

	float intensity;

	bool bipolar;
};



/** A modulator is a Processor that encapsulates modulation behaviour and returns a float value 
    between 0.0 and 1.0.
	@ingroup core

	A Modulator should be always inside a ModulatorChain, which handles the processing according to the characteristics of each Modulator.
	
	In order to create a modulator, subclass one of the two subclasses, VoiceStartModulator or TimeVariantModulator (never subclass this directly),	and overwrite the following methods:

		const String getType () const;
		ModulatorEditor *createEditor();
		float calculateVoiceStartValue(const MidiMessage &messageThatStartedVoice);
		XmlElement *getDescription () const;
		float getAttribute (int parameter_index) const;
		void setAttribute (int parameter_index, float newValue);
		float getDisplayValue() const;
	
	There are two handy features to debug a modulator: If plotThisModulator() is set to true, the Modulator will print its output on a popup plotter.

	@ingroup modulator
	@see ModulatorChain, ModulatorEditor
*/
class Modulator: public Processor
{
public:

	// =================================================================================================
	// Constructors / Destructor

	/**	Creates a new modulator with the given Identifier. */
	Modulator(MainController *m, const String &id, int numVoices);
	virtual ~Modulator();

	float getValueForTextConverter(float valueToConvert) const;;


	// ====================================================================================================
	// Virtual functions

	/** If the modulator uses Midi events, you can specify the behaviour here. 
	*
	*	This is likely to be used with midi messages that do not trigger a voice start like cc-messages.
	*	For the handling of note-on messages better use the calculateVoiceStartValue() method instead.
	*/
	virtual void handleHiseEvent(const HiseEvent& ) = 0;

	// ===============================================================================================
	// Class methods

	/** Normally a Modulator has no child processors, you can overwrite it if you use internal chains. */
	virtual int getNumChildProcessors() const override;;

	/** Sets the colour of the modulator. */
	virtual void setColour(Colour c);;

	virtual Colour getColour() const override;;

	

	UpdateMerger editorUpdater;

private:

	WeakReference<Modulator>::Master masterReference;
    friend class WeakReference<Modulator>;

	Colour colour;

	
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Modulator)
	
};

/** If a modulator subclasses this, you can calculate varying modulation values over a time. 
*
*	@ingroup modulator
*/
class TimeModulation: virtual public Modulation
{
public:

	
    
    virtual ~TimeModulation();;


	/** This calculates the time modulated values and stores them in the internal buffer. 
	*/
	virtual void calculateBlock(int startSample, int numSamples) = 0;

	/** This applies the intensity to the calculated values.
	*
	*	By default, it applies one intensity value to all calculated values, but you can override it if your Modulator has a internal Chain for the intensity.
	*/
	void applyTimeModulation(float* destinationBuffer, int startIndex, int samplesToCopy);

	

	/** Returns a read pointer to the calculated values. This is used by the global modulator system. */
	virtual const float *getCalculatedValues(int /*voiceIndex*/);

	void setScratchBuffer(float* scratchBuffer, int numSamples);

protected:

	TimeModulation(Mode m);

	/** Creates the internal buffer with double the size of the expected buffer block size.
    */
	virtual void prepareToModulate(double /*sampleRate*/, int samplesPerBlock);;

	/** Checks if the prepareToPlay method has been called. */
	virtual bool isInitialized();

	/** a vectorized version of the calcIntensityValue() and applyModulationValue() for Gain modulation with a fixed intensity value. */
	void applyGainModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, int numValues) const noexcept;

	/** A vectorized version of the calcIntensityValue() and applyModulationValue() for Gain modulation with varying intensities. 
	*
	*	@param fixedIntensity the intensity of the modulator. Normally you will pass the subclasses getIntensity() here.
	*	@param intensityValues a pointer to the precalculated intensity array. It changes the array, so don't use it afterwards!
	*/
	void applyGainModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, const float *intensityValues, int numValues) const noexcept;;

	/** A vectorized version of the calcIntensityValue() and applyModulationValue() for Pitch modulation with varying intensities. 
	*
	*	@param fixedIntensity the intensity of the modulator. Normally you will pass the subclasses getIntensity() here.
	*	@param intensityValues a pointer to the precalculated intensity array. It changes the array, so don't use it afterwards!
	*/
	void applyPitchModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, const float *intensityValues, int numValues) const noexcept;;

	/** a vectorized version of the calcIntensityValue() and applyModulationValue() for Pitch modulation with a fixed intensity value.
	*
	*	If you need varying intensity, use the other function and pass an array with the calculated intensity values.
	*/
	void applyPitchModulation(float* calculatedModulationValues, float *destinationValues, float fixedIntensity, int numValues) const noexcept;;

	void applyPanModulation(float * calculatedModValues, float * destinationValues, float fixedIntensity, float* intensityValues, int numValues) const noexcept;
	void applyPanModulation(float * calculatedModValues, float * destinationValues, float fixedIntensity, int numValues) const noexcept;

	void applyGlobalModulation(float * calculatedModValues, float * destinationValues, float fixedIntensity, float* intensityValues, int numValues) const noexcept;
	void applyGlobalModulation(float * calculatedModValues, float * destinationValues, float fixedIntensity, int numValues) const noexcept;

	void applyIntensityForGainValues(float* calculatedModulationValues, float fixedIntensity, int numValues) const;

	void applyIntensityForPitchValues(float* calculatedModulationValues, float fixedIntensity, int numValues) const;

	void applyIntensityForGainValues(float* calculatedModulationValues, float fixedIntensity, const float* intensityValues, int numValues) const;

	void applyIntensityForPitchValues(float* calculatedModulationValues, float fixedIntensity, const float* intensityValues, int numValuse) const;

#if 0
	// Prepares the buffer for the processing. The buffer is cleared and filled with 1.0.

	static void initializeBuffer(AudioSampleBuffer &bufferToBeInitialized, int startSample, int numSamples);
	{
		jassert(bufferToBeInitialized.getNumChannels() == 1);
		jassert(bufferToBeInitialized.getNumSamples() >= startSample + numSamples);

		float *writePointer = bufferToBeInitialized.getWritePointer(0, startSample);

		FloatVectorOperations::fill(writePointer, 1.0f, numSamples);
	}
#endif

	AudioSampleBuffer internalBuffer;

	double getControlRate() const noexcept;;

private:

	double controlRate = 0.0;

	float lastConstantValue = 1.0f;

	
};


/** If a Modulator is subclassed with VoiceModulation, it can handle multiple states for different voices. 
*
*	@ingroup modulator
*/
class VoiceModulation: virtual public Modulation
{
public:
    
    virtual ~VoiceModulation();;

	/** Implement the startVoice logic here. */
	virtual float startVoice(int voiceIndex) = 0;

	/** Implement the stopVoice logic here. */
	virtual void stopVoice(int voiceIndex) = 0;	

	virtual void allNotesOff();

	/** If you subclass a Modulator from this class, it can handle multiple voices. */
	class PolyphonyManager
	{
	public:

		// You should never create one of these directly...
		PolyphonyManager(int voiceAmount_);;

		/** Returns the amount of voices the Modulator can handle. */
		int getVoiceAmount() const;;

		/** This sets the current voice. Call this before you process the modulator! 
		*
		*	A call to this function must always be preceded by clearCurrentVoice() or a assertion is thrown!
		*/
		void setCurrentVoice(int newCurrentVoice) noexcept;;

		void setLastStartedVoice(int voiceIndex);


		int getLastStartedVoice() const;

		/** Call this when you finished the processing to clear the current voice. */
		void clearCurrentVoice() noexcept;;

		int getCurrentVoice() const noexcept;;

	private:

		int lastStartedVoice;

		int currentVoice;
		const int voiceAmount;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyphonyManager)
	};

	PolyphonyManager polyManager;

protected:

	

	VoiceModulation(int numVoices, Modulation::Mode m);;

	
};

/** A Modulator that calculates its value only at the start of a voice.
*
*	@ingroup dsp_base_classes
*
*	If a midi note on comes in, it calculates a value and temporarily saves the value until startNote() is called.
*	The value is then stored using the supplied voice index and can be retrieved using getVoiceStartValue().
*	
*/
class VoiceStartModulator: public Modulator,
						   public VoiceModulation
{
public:

	VoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);
	
	/** When the startNote function is called, a previously calculated value (by the handleMidiMessage function) is stored using the supplied voice index. */
	virtual float startVoice(int voiceIndex) override;;

    /** Only used in Global Modulator Containers. */
    void setResetUnsavedValue(bool shouldReset);

	static Path getSymbolPath();

	Path getSpecialSymbol() const override;

	/** A VoiceStartModulator has no child processors. */
	virtual Processor *getChildProcessor(int /*processorIndex*/) override final;;

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final;;

	virtual int getNumChildProcessors() const override final;;

	virtual ValueTree exportAsValueTree() const override;;

	virtual void restoreFromValueTree(const ValueTree &v) override;;

	Processor *getProcessor() override;;

	virtual void stopVoice(int voiceIndex) override;

	/** Returns the previously calculated voice start value. */
	virtual float getVoiceStartValue(int voiceIndex) const noexcept;;

	/**	If a note on is received, the voice start value is calculated and stored temporarily until startNote() is called. */
	virtual void handleHiseEvent(const HiseEvent &m) override;;
    
    float getUnsavedValue() const;

protected:

	/** Overwrite this method to calculate the voice start value. */
	virtual float calculateVoiceStartValue(const HiseEvent &m) = 0;
	
private:

	friend class JavascriptVoiceStartModulator;

	/** Checks if the last value that was calculated by a note on message was saved. */
	bool lastValueWasSaved() const;;

    bool resetUnsavedValue = false;
	float unsavedValue;

	Array<float> voiceValues;
};


/** A Modulator that calculates its return value for every processed sample. 
*
*	@ingroup dsp_base_classes
*
**/
class TimeVariantModulator: public Modulator,
							public TimeModulation
{

public:

	void prepareToPlay(double sampleRate, int samplesPerBlock);;

	static Path getSymbolPath();

	Path getSpecialSymbol() const override;;

	void render(float* monoModulationValues, float* scratchBuffer, int startSample, int numSamples);

	float getLastConstantValue() const noexcept;

protected:

	TimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m);;

	virtual ValueTree exportAsValueTree() const override;;

	virtual void restoreFromValueTree(const ValueTree &v) override;

	Processor *getProcessor() override;;

private:
	
	float lastConstantValue = 1.0f;
};


/** A EnvelopeModulator is a base class for all envelope-type modulators.
*
*	@ingroup dsp_base_classes
*
*	It is basically a TimeVariantModulator with two additional features:
*
*	1. Polyphony - multiple states are allowed. See ModulatorStates for more information.
*	2. Release Trail - prevent note from stopping until the release phase is finished.
*
*/
class EnvelopeModulator: public Modulator,
						 public VoiceModulation,
						 public TimeModulation
{
public:

	enum Parameters
	{
		Monophonic = Processor::SpecialParameters::numParameters,
		Retrigger,
		numParameters
	};

	
	virtual ~EnvelopeModulator();;

	static Path getSymbolPath();

	Path getSpecialSymbol() const override;;


	//	=========================================================================================================
	//	PURE VIRTUAL METHODS

    /** Checks if the Envelope is active for the given voice. Overwrite this and return true as long as you want the envelope to sound. */
	virtual bool isPlaying(int voiceIndex) const = 0;

	float getAttribute(int parameterIndex) const override;

	float getDefaultValue(int parameterIndex) const override;

	void setInternalAttribute(int parameterIndex, float newValue) override;

	ValueTree exportAsValueTree() const override;;

	void restoreFromValueTree(const ValueTree &v) override;

	/** Overwrite this to reset the envelope. If you want to have the display resetted, call this method from your subclass. */
	virtual void reset(int voiceIndex);

	void handleHiseEvent(const HiseEvent &m);

	Processor *getProcessor() override;;

	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	bool isInMonophonicMode() const;

	float startVoice(int /*voiceIndex*/) override;

	void stopVoice(int /*voiceIndex*/) override;

	bool shouldUpdatePlotter() const override;

	void render(int voiceIndex, float* voiceBuffer, float* scratchBuffer, int startSample, int numSamples);

protected:

	int getNumPressedKeys() const;


	EnvelopeModulator(MainController *mc, const String &id, int voiceAmount_, Modulation::Mode m);

	/** A ModulatorState is a container for Modulator states in a polyphonic TimeVariantModulator.
	*
	*	If the modulator should be polyphonic:
    *   - subclass this class and put all state variables (!= not attribute variables) into this container.
	*	- overwrite createSubclassedState() and call setVoiceAmount() as soon as you know how many voices are needed.
    *   - use getState(int voiceIndex) to get the corresponding ModulatorState and store / get all needed variables from this object.
    *   
	*/
	struct ModulatorState
	{
	public:

		/** Creates a state.
		*
		*	You can pass the voiceIndex (it isn't used yet).
		*/
		ModulatorState(int voiceIndex);;

		virtual ~ModulatorState();;

		int index;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorState)
	};

	/** Overwrite this method and return a newly created ModulatorState of the desired subclass. It will be owned by the Modulator.	*/
	virtual ModulatorState * createSubclassedState (int /*voiceIndex*/) const = 0;
	
	/** Use this array to access the state. */
	OwnedArray<ModulatorState> states;

	ScopedPointer<ModulatorState> monophonicState;

	bool isMonophonic = false;
	bool shouldRetrigger = true;


protected:

private:

	struct MidiBitmap
	{
		MidiBitmap();

		void clear();

		int getNumSetBits() const;

		void clearBit(uint8 index);

		void setBit(uint8 index);

		uint64 data[2];
		int8 numBitsSet = 0;
	} monophonicKeymap;

	JUCE_DECLARE_WEAK_REFERENCEABLE(EnvelopeModulator);
};

/** This class only exists for the Iterator. */
class MonophonicEnvelope: public EnvelopeModulator
{

};




/**	Allows creation of VoiceStartModulators.
*
*	@ingroup factory
*/
class VoiceStartModulatorFactoryType: public FactoryType
{
	// private enum for handling
	enum
	{
		constantModulator = 0,
		velocityModulator,
		keyModulator,
		randomModulator,
		globalVoiceStartModulator,
		globalStaticTimeVariantModulator,
		arrayModulator,
		scriptVoiceStartModulator,
		eventDataStartModulator
	};

public:

	VoiceStartModulatorFactoryType(int numVoices_, Modulation::Mode m, Processor *p):
		FactoryType(p),
		numVoices(numVoices_),
		mode(m)
	{
		fillTypeNameList();
	};

	void fillTypeNameList();

	Processor *createProcessor(int typeIndex, const String &id) override;
	
	const Array<ProcessorEntry> & getTypeNames() const override
	{
		return typeNames;
	};

private:

	Array<ProcessorEntry> typeNames;

	Modulation::Mode mode;
	int numVoices;
};

/**	Allows creation of EnvelopeModulators.
*
*	@ingroup factory
*/
class EnvelopeModulatorFactoryType: public FactoryType
{
	// private enum for handling
	enum
	{
		simpleEnvelope = 0,
		ahdsrEnvelope,
		tableEnvelope,
		scriptEnvelope,
		mpeModulator,
		voiceKillEnvelope,
		globalEnvelope,
		eventDataEnvelope
	};

public:

	EnvelopeModulatorFactoryType(int numVoices_, Modulation::Mode m, Processor *p):
		FactoryType(p),
		numVoices(numVoices_),
		mode(m)
	{
		fillTypeNameList();
	};

	void fillTypeNameList();

	Processor *createProcessor(int typeIndex, const String &id) override;
	
	const Array<ProcessorEntry> & getTypeNames() const override
	{
		return typeNames;
	};

private:

	Array<ProcessorEntry> typeNames;

	Modulation::Mode mode;
	int numVoices;
};

/**	Allows creation of TimeVariantModulators.
*
*	@ingroup factory
*/
class TimeVariantModulatorFactoryType: public FactoryType
{
	// private enum for handling
	enum
	{
		lfoModulator = 0,
		controlModulator,
		pitchWheel,
		macroModulator,
		globalTimeVariantModulator,
		scriptTimeVariantModulator,
        hardcodedTimeVariantModulator
	};

public:

	TimeVariantModulatorFactoryType(Modulation::Mode m, Processor *p):
		FactoryType(p),
		mode(m)
	{
		fillTypeNameList();
	};

	void fillTypeNameList();

	Processor *createProcessor(int typeIndex, const String &id) override;
	
	const Array<ProcessorEntry> & getTypeNames() const override
	{
		return typeNames;
	};

private:

	Array<ProcessorEntry> typeNames;

	Modulation::Mode mode;
};

#pragma warning (pop)

} // namespace hise

#endif  // MODULATORS_H_INCLUDED
