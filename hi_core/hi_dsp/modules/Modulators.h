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

#ifndef MODULATORS_H_INCLUDED
#define MODULATORS_H_INCLUDED



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

	/** There are two modes that Modulation can work: GainMode and PitchMode */
	enum Mode
	{
		/** The supplied buffer is multiplied with the modulation value (like a gain effect). */
		GainMode = 0, 
		/** The modulation value is added to the supplied buffer, which is assumed to be used as temporary storage for pitch modulation. 
		*
		*	The range is supposed to be -1.0 ... 1.0 and must be converted using convertToFreqRatio directly before using it.
		**/
		PitchMode 
	};

	Modulation(Mode m): 
		intensity(m == GainMode ? 1.0f : 0.0f), 
		modulationMode(m), 
		bipolar(m == PitchMode) {};
    virtual ~Modulation() {};

	virtual Processor *getProcessor() = 0;

	/** returns the mode the Modulator is operating. */
	Mode getMode() const noexcept { return modulationMode; };

	/** This applies the intensity to the given value and returns the applied value. 
	*
	*	- In GainMode the input is supposed to be between 0.0 and 1.0. The output will be between 0.0 and 1.0 (and 1.0 if the intensity is 0.0)
	*	- in PitchMode the input is supposed to be between 0.0 and 1.0. The output will be between -1.0 and 1.0 (and 0.0 if the intensity is 0.0)
	*/
	float calcIntensityValue(float calculatedModulationValue) const noexcept;;

	inline float calcGainIntensityValue(float calculatedModulationValue) const noexcept;

	inline float calcPitchIntensityValue(float calculatedModulationValue) const noexcept;

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
	virtual float getIntensity() const noexcept;;

	/** Returns the actual intensity of the Modulation. Use this for GUI displays, since getIntensity() could be overwritten and behave funky. */
	float getDisplayIntensity() const noexcept;;

	struct PitchConverters
	{
		static inline float octaveRangeToSignedNormalisedRange(float octaveValue)
		{
			return (octaveValue / 12.0f);
		}

		/** [-1 ... 1] => [0.5 ... 2.0] */
		static inline float normalisedRangeToPitchFactor(float range)
		{
			return std::exp2(range);
		}

		static inline float octaveRangeToPitchFactor(float octaveValue)
		{
			return std::exp2(octaveValue / 12.0f);
		}

		static inline void octaveRangeToSignedNormalisedRange(float* octaveValues, int numValues)
		{
			FloatVectorOperations::multiply(octaveValues, 0.1666666666f, numValues);
			FloatVectorOperations::add(octaveValues, 0.5f, numValues);
		}

		static inline void normalisedRangeToPitchFactor(float* rangeValues, int numValues)
		{
			if (numValues > 1)
			{
				float startValue = normalisedRangeToPitchFactor(rangeValues[0]);
				const float endValue = normalisedRangeToPitchFactor(rangeValues[numValues - 1]);
				float delta = (endValue - startValue);


				if (delta < 0.0003f)
				{
					FloatVectorOperations::fill(rangeValues, (startValue + endValue) * 0.5f, numValues);	
				}
				else
				{
					delta /= (float)numValues;

					while (--numValues >= 0)
					{
						*rangeValues++ = startValue;
						startValue += delta;
					}
				}
			}
			else if (numValues == 1)
			{
				rangeValues[0] = normalisedRangeToPitchFactor(rangeValues[0]);
			}
		}

		static inline void octaveRangeToPitchFactor(float* octaveValues, int numValues)
		{
			FloatVectorOperations::multiply(octaveValues, 0.1666666666f, numValues);

		}


		
	};

protected:

	const Mode modulationMode;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Modulation)

	float intensity;
	
	bool bipolar;
};



/** A modulator is a module that encapsulates modulation behaviour and returns a float value 
    between 0.0 and 1.0.

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
	Modulator(MainController *m, const String &id);
	virtual ~Modulator();;


	// ====================================================================================================
	// Virtual functions

	/** If the modulator uses Midi events, you can specify the behaviour here. 
	*
	*	This is likely to be used with midi messages that do not trigger a voice start like cc-messages.
	*	For the handling of note-on messages better use the calculateVoiceStartValue() method instead.
	*/
	virtual void handleMidiEvent(MidiMessage const &) = 0;

	// ===============================================================================================
	// Class methods

	/** Normally a Modulator has no child processors, you can overwrite it if you use internal chains. */
	virtual int getNumChildProcessors() const override {return 0;};

	/** Sets the colour of the modulator. */
	virtual void setColour(Colour c);;

	virtual Colour getColour() const override { return colour; };

	/** @brief enables the plotting of time variant modulators in the plotter popup.

		If a valid Plotter object is set, the modulator sends his results to the plotter.
	*/
	void setPlotter(Plotter *targetPlotter);
	
	bool isPlotted() const;

	/** Adds a value to the plotter. It is okay to do this on a sample level, the Plotter automatically interpolates it.
	*/
	void addValueToPlotter(float v) const;

	UpdateMerger editorUpdater;

private:

	WeakReference<Modulator>::Master masterReference;
    friend class WeakReference<Modulator>;

	Colour colour;

	Component::SafePointer<Plotter> attachedPlotter;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Modulator)
	
};

/** If a modulator subclasses this, you can calculate varying modulation values over a time. 
*
*	@ingroup modulator
*/
class TimeModulation: virtual public Modulation
{
public:

	
    
    virtual ~TimeModulation() {};

	/** This renders the next chunk of samples. It repeatedly calls calculateNewValue() until the buffer is filled.
	*
	*	
	*
	*	@param buffer the AudioSampleBuffer that the function operates on.
	*	@param startSample the start index within the buffer. If you need to change only a part of the buffer
	*					   (eg. if a voice starts in the middle), use this parameter
	*	@param numSamples the amount of samples that are processed (numSamples = buffersize - startSample)
	*
	*/
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples);;

	/** This calculates the time modulated values and stores them in the internal buffer. 
	*/
	virtual void calculateBlock(int startSample, int numSamples) = 0;

	/** This applies the intensity to the calculated values.
	*
	*	By default, it applies one intensity value to all calculated values, but you can override it if your Modulator has a internal Chain for the intensity.
	*/
	virtual void applyTimeModulation(AudioSampleBuffer &buffer, int startIndex, int samplesToCopy);;

	/** Returns a read pointer to the calculated values. This is used by the global modulator system. */
	virtual const float *getCalculatedValues(int /*voiceIndex*/);

protected:

	TimeModulation(Modulation::Mode m):
		Modulation(m)
	{};

	/** Creates the internal buffer with double the size of the expected buffer block size.
    */
	virtual void prepareToModulate(double /*sampleRate*/, int samplesPerBlock);;

	/** Picks 4 values out of the processed buffer and sends it to the plotter. 
	*
	*	This is called after the internal buffer has been filled with the Modulation values. 
	*/
	virtual void updatePlotter(const AudioSampleBuffer &processedBuffer, int startSample, int numSamples) = 0;
	
	/** Overwrite this method to determine whether the modulation values should be sent to the Gui. */
	virtual bool shouldUpdatePlotter() const = 0;

	/** Checks if the prepareToPlay method has been called. */
	virtual bool isInitialized();

	

	/** a vectorized version of the calcIntensityValue() and applyModulationValue() for Gain modulation with a fixed intensity value. */
	void applyGainModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, int numValues) const noexcept;

	/** A vectorized version of the calcIntensityValue() and applyModulationValue() for Gain modulation with varying intensities. 
	*
	*	@param fixedIntensity the intensity of the modulator. Normally you will pass the subclasses getIntensity() here.
	*	@param intensityValues a pointer to the precalculated intensity array. It changes the array, so don't use it afterwards!
	*/
	void applyGainModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, float *intensityValues, int numValues) const noexcept;;

	/** A vectorized version of the calcIntensityValue() and applyModulationValue() for Pitch modulation with varying intensities. 
	*
	*	@param fixedIntensity the intensity of the modulator. Normally you will pass the subclasses getIntensity() here.
	*	@param intensityValues a pointer to the precalculated intensity array. It changes the array, so don't use it afterwards!
	*/
	void applyPitchModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, float *intensityValues, int numValues) const noexcept;;

	/** a vectorized version of the calcIntensityValue() and applyModulationValue() for Pitch modulation with a fixed intensity value.
	*
	*	If you need varying intensity, use the other function and pass an array with the calculated intensity values.
	*/
	void applyPitchModulation(float* calculatedModulationValues, float *destinationValues, float fixedIntensity, int numValues) const noexcept;;

	// Prepares the buffer for the processing. The buffer is cleared and filled with 1.0.
	static void initializeBuffer(AudioSampleBuffer &bufferToBeInitialized, int startSample, int numSamples);;

	AudioSampleBuffer internalBuffer;

private:

};


/** If a Modulator is subclassed with VoiceModulation, it can handle multiple states for different voices. 
*
*	@ingroup modulator
*/
class VoiceModulation: virtual public Modulation
{
public:
    
    virtual ~VoiceModulation() {};

	/** Implement the startVoice logic here. */
	virtual void startVoice(int voiceIndex) = 0;

	/** Implement the stopVoice logic here. */
	virtual void stopVoice(int voiceIndex) = 0;	

	void allNotesOff();

	/** If you subclass a Modulator from this class, it can handle multiple voices. */
	class PolyphonyManager
	{
	public:

		// You should never create one of these directly...
		PolyphonyManager(int voiceAmount_):
			voiceAmount(voiceAmount_),
			currentVoice(-1),
			lastStartedVoice(0)
		{};

		/** Returns the amount of voices the Modulator can handle. */
		int getVoiceAmount() const {return voiceAmount;};

		/** This sets the current voice. Call this before you process the modulator! 
		*
		*	A call to this function must always be preceded by clearCurrentVoice() or a assertion is thrown!
		*/
		void setCurrentVoice(int newCurrentVoice) noexcept;;

		void setLastStartedVoice(int voiceIndex);


		int getLastStartedVoice() const;

		/** Call this when you finished the processing to clear the current voice. */
		void clearCurrentVoice() noexcept
		{
			jassert(currentVoice != -1);
			currentVoice = -1;
		};

		int getCurrentVoice() const noexcept
		{
			jassert (currentVoice != -1);
			return currentVoice;
		};

	private:

		int lastStartedVoice;

		int currentVoice;
		const int voiceAmount;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyphonyManager)
	};

	PolyphonyManager polyManager;

protected:

	

	VoiceModulation(int numVoices, Modulation::Mode m):
		Modulation(m),
		polyManager(numVoices)	{};

	
};

/** A Modulator that calculates its value only at the start of a voice.
*
*	@ingroup modulator
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
	virtual void startVoice(int voiceIndex) override
	{
		voiceValues.setUnchecked(voiceIndex, unsavedValue);

#if ENABLE_ALL_PEAK_METERS
		setOutputValue(unsavedValue);
#endif
	};

	static Path getSymbolPath()
	{
		Path path;

		static const unsigned char pathData[] = { 110, 109, 0, 128, 243, 66, 0, 28, 250, 67, 98, 27, 117, 238, 66, 198, 60, 250, 67, 0, 144, 234, 66, 161, 80, 251, 67, 0, 144, 234, 66, 0, 156, 252, 67, 98, 0, 144, 234, 66, 108, 148, 253, 67, 166, 196, 236, 66, 240, 101, 254, 67, 0, 0, 240, 66, 0, 208, 254, 67, 108, 0, 0, 240, 66, 0, 192, 0, 68, 98, 31, 228, 237, 66, 135, 223,
			0, 68, 27, 47, 236, 66, 184, 20, 1, 68, 0, 48, 235, 66, 0, 88, 1, 68, 108, 0, 0, 212, 66, 0, 88, 1, 68, 98, 90, 228, 210, 66, 0, 88, 1, 68, 0, 0, 210, 66, 139, 116, 1, 68, 0, 0, 210, 66, 0, 152, 1, 68, 108, 0, 0, 210, 66, 0, 88, 2, 68, 98, 0, 0, 210, 66, 117, 123, 2, 68, 90, 228, 210, 66, 0, 152, 2, 68, 0, 0, 212, 66, 0, 152, 2, 68, 108,
			0, 16, 236, 66, 0, 152, 2, 68, 98, 28, 223, 237, 66, 109, 234, 2, 68, 6, 205, 240, 66, 0, 32, 3, 68, 0, 48, 244, 66, 0, 32, 3, 68, 98, 217, 181, 249, 66, 0, 32, 3, 68, 0, 48, 254, 66, 187, 144, 2, 68, 0, 48, 254, 66, 0, 224, 1, 68, 98, 0, 48, 254, 66, 51, 116, 1, 68, 134, 141, 252, 66, 251, 21, 1, 68, 0, 0, 250, 66, 0, 220, 0, 68,
			108, 0, 0, 250, 66, 0, 176, 254, 67, 98, 167, 130, 251, 66, 163, 112, 254, 67, 253, 180, 252, 66, 193, 24, 254, 67, 0, 128, 253, 66, 0, 176, 253, 67, 108, 0, 0, 11, 67, 0, 176, 253, 67, 98, 211, 141, 11, 67, 0, 176, 253, 67, 0, 0, 12, 67, 233, 118, 253, 67, 0, 0, 12, 67, 0, 48, 253, 67, 108, 0, 0, 12, 67, 0, 176, 251, 67, 98,
			0, 0, 12, 67, 23, 105, 251, 67, 211, 141, 11, 67, 0, 48, 251, 67, 0, 0, 11, 67, 0, 48, 251, 67, 108, 0, 192, 252, 66, 0, 48, 251, 67, 98, 99, 242, 250, 66, 99, 136, 250, 67, 140, 251, 247, 66, 0, 28, 250, 67, 0, 144, 244, 66, 0, 28, 250, 67, 98, 162, 55, 244, 66, 0, 28, 250, 67, 16, 214, 243, 66, 209, 25, 250, 67, 0, 128, 243,
			66, 0, 28, 250, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}

	Path getSpecialSymbol() const override
	{
		return getSymbolPath();
	}

	/** A VoiceStartModulator has no child processors. */
	virtual Processor *getChildProcessor(int /*processorIndex*/) override final {return nullptr;};

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final {return nullptr;};

	virtual int getNumChildProcessors() const override final {return 0;};

	virtual ValueTree exportAsValueTree() const override
	{
		ValueTree v(Processor::exportAsValueTree());

		v.setProperty("Intensity", getIntensity(), nullptr);

		return v;

	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		Processor::restoreFromValueTree(v);

		setIntensity(v.getProperty("Intensity", 1.0f));
	};

	Processor *getProcessor() override { return this; };

	virtual void stopVoice(int voiceIndex) override
	{
		voiceValues.setUnchecked(voiceIndex, -1.0f);
	}

	/** Returns the previously calculated voice start value. */
	virtual float getVoiceStartValue(int voiceIndex) const noexcept { return voiceValues.getUnchecked(voiceIndex); };

	/**	If a note on is received, the voice start value is calculated and stored temporarily until startNote() is called. */
	virtual void handleMidiEvent(const MidiMessage &m) override
	{
		if(m.isNoteOnOrOff() && m.isNoteOn())
		{
			unsavedValue = calculateVoiceStartValue(m);
		}
	};

protected:

	/** Overwrite this method to calculate the voice start value. */
	virtual float inline calculateVoiceStartValue(const MidiMessage &m) = 0;
	
private:

	friend class JavascriptVoiceStartModulator;

	/** Checks if the last value that was calculated by a note on message was saved. */
	bool lastValueWasSaved() const
	{
		return unsavedValue == -1.0f;
	};

	float unsavedValue;

	Array<float> voiceValues;
};


/** A Modulator that calculates its return value for every processed sample. 
*
*	@ingroup modulator
*
**/
class TimeVariantModulator: public Modulator,
							public TimeModulation
{

public:

	void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		Processor::prepareToPlay(sampleRate, samplesPerBlock);
		TimeModulation::prepareToModulate(sampleRate, samplesPerBlock);
	};

	static Path getSymbolPath()
	{
		Path path;

		static const unsigned char pathData[] = { 110, 109, 0, 224, 39, 67, 0, 72, 9, 68, 98, 238, 132, 37, 67, 101, 76, 9, 68, 229, 139, 35, 67, 137, 194, 9, 68, 0, 72, 34, 67, 0, 60, 10, 68, 98, 215, 84, 33, 67, 227, 148, 10, 68, 207, 137, 32, 67, 50, 245, 10, 68, 0, 224, 31, 67, 0, 88, 11, 68, 98, 161, 249, 29, 67, 0, 93, 11, 68, 11, 29, 28, 67, 69, 164, 11, 68, 0, 104, 27,
			67, 0, 26, 12, 68, 98, 10, 33, 26, 67, 203, 202, 12, 68, 130, 0, 28, 67, 29, 175, 13, 68, 0, 0, 31, 67, 0, 208, 13, 68, 98, 13, 248, 33, 67, 215, 253, 13, 68, 213, 26, 37, 67, 84, 91, 13, 68, 0, 0, 37, 67, 0, 152, 12, 68, 98, 251, 1, 37, 67, 141, 94, 12, 68, 100, 188, 36, 67, 21, 37, 12, 68, 0, 72, 36, 67, 0, 244, 11, 68, 98, 84,
			14, 37, 67, 160, 122, 11, 68, 209, 7, 38, 67, 52, 7, 11, 68, 0, 104, 39, 67, 0, 166, 10, 68, 98, 130, 210, 40, 67, 36, 79, 10, 68, 190, 175, 41, 67, 197, 18, 11, 68, 0, 96, 42, 67, 0, 76, 11, 68, 98, 218, 246, 44, 67, 146, 145, 12, 68, 38, 94, 45, 67, 206, 23, 14, 68, 0, 0, 49, 67, 0, 60, 15, 68, 98, 169, 165, 50, 67, 41, 202, 15,
			68, 210, 219, 53, 67, 159, 25, 16, 68, 0, 104, 56, 67, 0, 190, 15, 68, 98, 85, 60, 59, 67, 168, 84, 15, 68, 179, 199, 60, 67, 222, 146, 14, 68, 0, 16, 62, 67, 0, 216, 13, 68, 98, 199, 18, 62, 67, 254, 215, 13, 68, 58, 21, 62, 67, 4, 216, 13, 68, 0, 24, 62, 67, 0, 216, 13, 68, 98, 215, 174, 64, 67, 165, 212, 13, 68, 122, 23, 67,
			67, 234, 66, 13, 68, 0, 0, 67, 67, 0, 152, 12, 68, 98, 157, 3, 67, 67, 31, 47, 12, 68, 207, 36, 66, 67, 18, 200, 11, 68, 0, 200, 64, 67, 0, 142, 11, 68, 98, 101, 82, 62, 67, 250, 27, 11, 68, 212, 126, 58, 67, 175, 100, 11, 68, 0, 104, 57, 67, 0, 26, 12, 68, 98, 27, 188, 56, 67, 242, 118, 12, 68, 14, 241, 56, 67, 113, 226, 12, 68,
			0, 184, 57, 67, 0, 56, 13, 68, 98, 53, 241, 56, 67, 0, 178, 13, 68, 217, 249, 55, 67, 86, 38, 14, 68, 0, 152, 54, 67, 0, 136, 14, 68, 98, 126, 45, 53, 67, 220, 222, 14, 68, 66, 80, 52, 67, 59, 27, 14, 68, 0, 160, 51, 67, 0, 226, 13, 68, 98, 38, 9, 49, 67, 110, 156, 12, 68, 218, 161, 48, 67, 50, 22, 11, 68, 0, 0, 45, 67, 0, 242, 9,
			68, 98, 52, 243, 43, 67, 113, 161, 9, 68, 186, 146, 42, 67, 223, 92, 9, 68, 0, 232, 40, 67, 0, 78, 9, 68, 98, 54, 142, 40, 67, 166, 73, 9, 68, 39, 54, 40, 67, 95, 71, 9, 68, 0, 224, 39, 67, 0, 72, 9, 68, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}

	Path getSpecialSymbol() const override
	{
		return getSymbolPath();
	};

protected:

	

	TimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m):
		Modulator(mc, id),
		TimeModulation(m),
		Modulation(m)
	{};

	

	virtual ValueTree exportAsValueTree() const override
	{
		ValueTree v(Processor::exportAsValueTree());

		v.setProperty("Intensity", getIntensity(), nullptr);

		return v;

	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		Processor::restoreFromValueTree(v);

		setIntensity(v.getProperty("Intensity", 1.0f));
	}

	Processor *getProcessor() override { return this; };

	bool shouldUpdatePlotter() const override {return ENABLE_PLOTTER == 1;};

	void updatePlotter(const AudioSampleBuffer &processedBuffer, int startSample, int numSamples) override
	{
		const float plot1 = processedBuffer.getSample(0, startSample );
		const float plot2 = processedBuffer.getSample(0, startSample + numSamples / 4);
		const float plot3 = processedBuffer.getSample(0, startSample + numSamples / 2);
		const float plot4 = processedBuffer.getSample(0, startSample + (3 * numSamples) / 4);

		addValueToPlotter(plot1);
		addValueToPlotter(plot2);
		addValueToPlotter(plot3);
		addValueToPlotter(plot4);
	};

};


/** A EnvelopeModulator is a base class for all envelope-type modulators.
*
*	@ingroup modulator
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

	virtual ~EnvelopeModulator() {};

	static Path getSymbolPath()
	{
		Path path;

		static const unsigned char pathData[] = { 110, 109, 0, 16, 34, 67, 0, 196, 248, 67, 98, 20, 77, 31, 67, 0, 196, 248, 67, 0, 16, 29, 67, 138, 226, 249, 67, 0, 16, 29, 67, 0, 68, 251, 67, 98, 0, 16, 29, 67, 175, 242, 251, 67, 19, 157, 29, 67, 132, 144, 252, 67, 0, 128, 30, 67, 0, 4, 253, 67, 108, 0, 136, 24, 67, 0, 90, 2, 68, 108, 0, 136, 24, 67, 0, 90, 2, 68, 108, 154, 124,
			24, 67, 249, 97, 2, 68, 108, 208, 114, 24, 67, 20, 106, 2, 68, 108, 167, 106, 24, 67, 76, 114, 2, 68, 108, 38, 100, 24, 67, 155, 122, 2, 68, 108, 81, 95, 24, 67, 253, 130, 2, 68, 108, 41, 92, 24, 67, 107, 139, 2, 68, 108, 179, 90, 24, 67, 224, 147, 2, 68, 108, 237, 90, 24, 67, 88, 156, 2, 68, 108, 217, 92, 24, 67, 204, 164,
			2, 68, 108, 117, 96, 24, 67, 56, 173, 2, 68, 108, 191, 101, 24, 67, 149, 181, 2, 68, 108, 179, 108, 24, 67, 222, 189, 2, 68, 108, 77, 117, 24, 67, 15, 198, 2, 68, 108, 135, 127, 24, 67, 33, 206, 2, 68, 108, 91, 139, 24, 67, 16, 214, 2, 68, 108, 194, 152, 24, 67, 215, 221, 2, 68, 108, 178, 167, 24, 67, 113, 229, 2, 68, 108,
			34, 184, 24, 67, 216, 236, 2, 68, 108, 8, 202, 24, 67, 8, 244, 2, 68, 108, 88, 221, 24, 67, 253, 250, 2, 68, 108, 6, 242, 24, 67, 178, 1, 3, 68, 108, 5, 8, 25, 67, 34, 8, 3, 68, 108, 70, 31, 25, 67, 74, 14, 3, 68, 108, 186, 55, 25, 67, 38, 20, 3, 68, 108, 83, 81, 25, 67, 177, 25, 3, 68, 108, 255, 107, 25, 67, 233, 30, 3, 68, 108,
			174, 135, 25, 67, 202, 35, 3, 68, 108, 78, 164, 25, 67, 81, 40, 3, 68, 108, 204, 193, 25, 67, 123, 44, 3, 68, 108, 22, 224, 25, 67, 69, 48, 3, 68, 108, 24, 255, 25, 67, 173, 51, 3, 68, 108, 190, 30, 26, 67, 177, 54, 3, 68, 108, 245, 62, 26, 67, 78, 57, 3, 68, 108, 167, 95, 26, 67, 132, 59, 3, 68, 108, 192, 128, 26, 67, 80, 61,
			3, 68, 108, 42, 162, 26, 67, 178, 62, 3, 68, 108, 208, 195, 26, 67, 169, 63, 3, 68, 108, 157, 229, 26, 67, 52, 64, 3, 68, 108, 123, 7, 27, 67, 82, 64, 3, 68, 108, 84, 41, 27, 67, 4, 64, 3, 68, 108, 19, 75, 27, 67, 74, 63, 3, 68, 108, 161, 108, 27, 67, 36, 62, 3, 68, 108, 234, 141, 27, 67, 148, 60, 3, 68, 108, 217, 174, 27, 67,
			153, 58, 3, 68, 108, 87, 207, 27, 67, 53, 56, 3, 68, 108, 81, 239, 27, 67, 107, 53, 3, 68, 108, 178, 14, 28, 67, 59, 50, 3, 68, 108, 102, 45, 28, 67, 167, 46, 3, 68, 108, 88, 75, 28, 67, 179, 42, 3, 68, 108, 119, 104, 28, 67, 96, 38, 3, 68, 108, 175, 132, 28, 67, 177, 33, 3, 68, 108, 239, 159, 28, 67, 170, 28, 3, 68, 108, 36,
			186, 28, 67, 77, 23, 3, 68, 108, 63, 211, 28, 67, 157, 17, 3, 68, 108, 47, 235, 28, 67, 160, 11, 3, 68, 108, 228, 1, 29, 67, 87, 5, 3, 68, 108, 80, 23, 29, 67, 201, 254, 2, 68, 108, 103, 43, 29, 67, 247, 247, 2, 68, 108, 25, 62, 29, 67, 232, 240, 2, 68, 108, 93, 79, 29, 67, 159, 233, 2, 68, 108, 38, 95, 29, 67, 33, 226, 2, 68,
			108, 106, 109, 29, 67, 115, 218, 2, 68, 108, 0, 120, 29, 67, 0, 212, 2, 68, 108, 0, 120, 29, 67, 0, 212, 2, 68, 108, 0, 184, 35, 67, 0, 160, 253, 67, 98, 53, 220, 36, 67, 57, 108, 253, 67, 30, 201, 37, 67, 5, 5, 253, 67, 0, 96, 38, 67, 0, 128, 252, 67, 108, 0, 176, 45, 67, 0, 128, 252, 67, 98, 191, 56, 46, 67, 229, 248, 252, 67,
			67, 15, 47, 67, 101, 88, 253, 67, 0, 16, 48, 67, 0, 144, 253, 67, 98, 58, 67, 48, 67, 242, 105, 254, 67, 117, 150, 48, 67, 90, 105, 255, 67, 0, 64, 49, 67, 0, 70, 0, 68, 98, 169, 11, 50, 67, 145, 244, 0, 68, 196, 57, 51, 67, 239, 169, 1, 68, 0, 64, 53, 67, 0, 62, 2, 68, 98, 60, 70, 55, 67, 17, 210, 2, 68, 208, 83, 58, 67, 0, 64,
			3, 68, 0, 0, 62, 67, 0, 64, 3, 68, 108, 0, 0, 62, 67, 0, 64, 3, 68, 108, 215, 33, 62, 67, 84, 64, 3, 68, 108, 180, 67, 62, 67, 60, 64, 3, 68, 108, 130, 101, 62, 67, 184, 63, 3, 68, 108, 42, 135, 62, 67, 199, 62, 3, 68, 108, 151, 168, 62, 67, 107, 61, 3, 68, 108, 180, 201, 62, 67, 165, 59, 3, 68, 108, 108, 234, 62, 67, 117, 57, 3,
			68, 108, 169, 10, 63, 67, 222, 54, 3, 68, 108, 87, 42, 63, 67, 224, 51, 3, 68, 108, 98, 73, 63, 67, 126, 48, 3, 68, 108, 182, 103, 63, 67, 185, 44, 3, 68, 108, 63, 133, 63, 67, 149, 40, 3, 68, 108, 235, 161, 63, 67, 20, 36, 3, 68, 108, 168, 189, 63, 67, 56, 31, 3, 68, 108, 98, 216, 63, 67, 5, 26, 3, 68, 108, 11, 242, 63, 67, 126,
			20, 3, 68, 108, 144, 10, 64, 67, 168, 14, 3, 68, 108, 226, 33, 64, 67, 132, 8, 3, 68, 108, 243, 55, 64, 67, 24, 2, 3, 68, 108, 180, 76, 64, 67, 103, 251, 2, 68, 108, 24, 96, 64, 67, 118, 244, 2, 68, 108, 18, 114, 64, 67, 73, 237, 2, 68, 108, 152, 130, 64, 67, 229, 229, 2, 68, 108, 158, 145, 64, 67, 79, 222, 2, 68, 108, 27, 159,
			64, 67, 139, 214, 2, 68, 108, 6, 171, 64, 67, 158, 206, 2, 68, 108, 88, 181, 64, 67, 142, 198, 2, 68, 108, 9, 190, 64, 67, 95, 190, 2, 68, 108, 22, 197, 64, 67, 23, 182, 2, 68, 108, 120, 202, 64, 67, 187, 173, 2, 68, 108, 44, 206, 64, 67, 81, 165, 2, 68, 108, 49, 208, 64, 67, 222, 156, 2, 68, 108, 133, 208, 64, 67, 102, 148,
			2, 68, 108, 39, 207, 64, 67, 241, 139, 2, 68, 108, 25, 204, 64, 67, 130, 131, 2, 68, 108, 92, 199, 64, 67, 32, 123, 2, 68, 108, 244, 192, 64, 67, 208, 114, 2, 68, 108, 228, 184, 64, 67, 151, 106, 2, 68, 108, 50, 175, 64, 67, 122, 98, 2, 68, 108, 228, 163, 64, 67, 127, 90, 2, 68, 108, 1, 151, 64, 67, 171, 82, 2, 68, 108, 145,
			136, 64, 67, 2, 75, 2, 68, 108, 158, 120, 64, 67, 138, 67, 2, 68, 108, 50, 103, 64, 67, 71, 60, 2, 68, 108, 88, 84, 64, 67, 63, 53, 2, 68, 108, 28, 64, 64, 67, 117, 46, 2, 68, 108, 140, 42, 64, 67, 238, 39, 2, 68, 108, 179, 19, 64, 67, 174, 33, 2, 68, 108, 163, 251, 63, 67, 185, 27, 2, 68, 108, 105, 226, 63, 67, 19, 22, 2, 68,
			108, 22, 200, 63, 67, 191, 16, 2, 68, 108, 187, 172, 63, 67, 193, 11, 2, 68, 108, 106, 144, 63, 67, 29, 7, 2, 68, 108, 51, 115, 63, 67, 212, 2, 2, 68, 108, 43, 85, 63, 67, 235, 254, 1, 68, 108, 100, 54, 63, 67, 98, 251, 1, 68, 108, 243, 22, 63, 67, 61, 248, 1, 68, 108, 234, 246, 62, 67, 126, 245, 1, 68, 108, 95, 214, 62, 67,
			38, 243, 1, 68, 108, 103, 181, 62, 67, 55, 241, 1, 68, 108, 22, 148, 62, 67, 178, 239, 1, 68, 108, 130, 114, 62, 67, 152, 238, 1, 68, 108, 193, 80, 62, 67, 234, 237, 1, 68, 108, 231, 46, 62, 67, 168, 237, 1, 68, 108, 11, 13, 62, 67, 211, 237, 1, 68, 108, 0, 0, 62, 67, 0, 238, 1, 68, 108, 0, 0, 62, 67, 0, 238, 1, 68, 98, 218, 214,
			59, 67, 0, 238, 1, 68, 196, 129, 58, 67, 239, 187, 1, 68, 0, 64, 57, 67, 0, 96, 1, 68, 98, 60, 254, 55, 67, 17, 4, 1, 68, 173, 1, 55, 67, 111, 121, 0, 68, 0, 88, 54, 67, 0, 208, 255, 67, 98, 120, 205, 53, 67, 132, 226, 254, 67, 2, 123, 53, 67, 244, 241, 253, 67, 0, 72, 53, 67, 0, 40, 253, 67, 98, 26, 89, 54, 67, 161, 178, 252, 67,
			0, 8, 55, 67, 47, 6, 252, 67, 0, 8, 55, 67, 0, 68, 251, 67, 98, 0, 8, 55, 67, 138, 226, 249, 67, 236, 202, 52, 67, 0, 196, 248, 67, 0, 8, 50, 67, 0, 196, 248, 67, 98, 40, 82, 48, 67, 0, 196, 248, 67, 203, 206, 46, 67, 87, 52, 249, 67, 0, 232, 45, 67, 0, 220, 249, 67, 108, 0, 40, 38, 67, 0, 220, 249, 67, 98, 49, 65, 37, 67, 99, 52,
			249, 67, 198, 197, 35, 67, 0, 196, 248, 67, 0, 16, 34, 67, 0, 196, 248, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}

	Path getSpecialSymbol() const override
	{
		return getSymbolPath();
	};


	//	=========================================================================================================
	//	PURE VIRTUAL METHODS

    /** Checks if the Envelope is active for the given voice. Overwrite this and return true as long as you want the envelope to sound. */
	virtual bool isPlaying(int voiceIndex) const = 0;

	virtual ValueTree exportAsValueTree() const override
	{
		ValueTree v(Processor::exportAsValueTree());

		return v.setProperty("Intensity", getIntensity(), nullptr);

	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		Processor::restoreFromValueTree(v);

		setIntensity(v.getProperty("Intensity", 1.0f));
	}

	/** Overwrite this to reset the envelope. If you want to have the display resetted, call this method from your subclass. */
	virtual void reset(int voiceIndex)
	{
#if ENABLE_ALL_PEAK_METERS
		if(voiceIndex == polyManager.getLastStartedVoice())
		{
			setOutputValue(0.0f);
		};
#else
		ignoreUnused(voiceIndex);
#endif
	}

	void handleMidiEvent(const MidiMessage &m)
	{
		if(m.isAllNotesOff())
		{
			this->allNotesOff();
		}
	}

	Processor *getProcessor() override { return this; };

	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		Processor::prepareToPlay(sampleRate, samplesPerBlock);
		TimeModulation::prepareToModulate(sampleRate, samplesPerBlock);

		globalSaveValues.setSize(globalSaveValues.getNumChannels(), samplesPerBlock);

		globalSaveValues.clear();
	}

	void saveValuesForGlobalModulator(const AudioSampleBuffer &internalBuffer, int startSample, int numSamples, int voiceIndex);

	const float *getCalculatedValues(int voiceIndex) override { return globalSaveValues.getReadPointer(voiceIndex); }

protected:

	virtual bool shouldUpdatePlotter() const override {return polyManager.getCurrentVoice() == polyManager.getLastStartedVoice(); };


	virtual void updatePlotter(const AudioSampleBuffer &processedBuffer, int startSample, int numSamples) override
	{
		const float plot1 = processedBuffer.getSample(0, startSample );
		const float plot2 = processedBuffer.getSample(0, startSample + numSamples / 4);
		const float plot3 = processedBuffer.getSample(0, startSample + numSamples / 2);
		const float plot4 = processedBuffer.getSample(0, startSample + (3 * numSamples) / 4);

		addValueToPlotter(plot1);
		addValueToPlotter(plot2);
		addValueToPlotter(plot3);
		addValueToPlotter(plot4);
		
	};

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
		ModulatorState(int voiceIndex):
			index(voiceIndex)
		{
		};

		virtual ~ModulatorState() {};

	private:
		int index;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorState)
	};

	/** Overwrite this method and return a newly created ModulatorState of the desired subclass. It will be owned by the Modulator.	*/
	virtual ModulatorState * createSubclassedState (int /*voiceIndex*/) const = 0;
	
	AudioSampleBuffer globalSaveValues;

	/** Use this array to access the state. */
	OwnedArray<ModulatorState> states;
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
		gainMatcherVoiceStartModulator,
		arrayModulator,
		scriptVoiceStartModulator
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
		globalEnvelope,
		ccEnvelope
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
		audioFileEnvelope,
		pluginParameter,
		globalTimeVariantModulator,
		gainMatcherTimeVariantModulator,
		ccDucker,
		scriptTimeVariantModulator
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

#endif  // MODULATORS_H_INCLUDED
