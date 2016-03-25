/*
  ==============================================================================

    CurveEq.h
    Created: 30 Sep 2014 11:15:36pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef CURVEEQ_H_INCLUDED
#define CURVEEQ_H_INCLUDED


#define FFT_SIZE_FOR_EQ 4096

/** A parametriq equalizer with unlimited bands and FFT display. 
*	@ingroup effectTypes
*
*/
class CurveEq: public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("CurveEq", "Parametriq EQ")

	enum Parameters
	{
		numEffectParameters
	};

	/** The types of filters for every band. */
	enum FilterType
	{
		LowPass = 0, ///< a 1-Pole Lowpass
		HighPass, ///< a 1-Pole Highpass
		LowShelf, ///< a shelving eq for the low end
		HighShelf, ///< a shelving eq for the high end
		Peak, ///< a peak eq
		numFilterTypes
	};

	/** The parameters for each band. */
	enum BandParameter
	{
		Gain = 0, ///< the gain (not available on HP/LP)
		Freq, ///< the center frequency
		Q, ///< the q factor (not available on HP/LP)
		Enabled, ///< enables / disables the band
		Type, ///< defines the type of the band @see FilterType
		numBandParameters
	};

	class StereoFilter
	{
	public:

		StereoFilter():
			sampleRate(44100.0),
			frequency(1000.0),
			gain(1.0),
			q(1.0),
			type(Peak),
			enabled(true)
		{
			updateCoefficients();
		};

		void setEnabled(bool shouldBeEnabled)
		{
			ScopedLock sl(lock);

			enabled = shouldBeEnabled;
		}

		bool isEnabled() const
		{
			return enabled;
		}

		void setType(int newType)
		{
			ScopedLock sl(lock);

			type = (FilterType)newType;
			updateCoefficients();
		}

		double getFrequency() const {return frequency; };

		double getGain() const {return (double)gain; };

		double getQ() const { return q; };

		void setFrequency(double newFrequency)
		{
			ScopedLock sl(lock);

			frequency = newFrequency;

			updateCoefficients();
		};


		void setGain(double newGain)
		{
			ScopedLock sl(lock);

			gain = (float)newGain;
			updateCoefficients();
		}

		void setQ(double newQ)
		{
			ScopedLock sl(lock);

			q = newQ;
			updateCoefficients();
		}
		
		void setSampleRate(double newSampleRate)
		{
			ScopedLock sl(lock);

			sampleRate = newSampleRate;
			updateCoefficients();
			leftFilter.reset();
			rightFilter.reset();
		}

		void process(AudioSampleBuffer &b, int startSample, int numSamples)
		{
			if(!enabled) return;

			ScopedLock sl(lock);

			leftFilter.processSamples(b.getWritePointer(0, startSample), numSamples);
			rightFilter.processSamples(b.getWritePointer(1, startSample), numSamples);
		}

		IIRCoefficients getCoefficients() const
		{
			return currentCoefficients;
		};

		int getFilterType() const
		{
			return type;
		}

	private:

		CriticalSection lock;

		void updateCoefficients()
		{
			switch(type)
			{
			case LowPass:		currentCoefficients = IIRCoefficients::makeLowPass(sampleRate, frequency); break;
			case HighPass:		currentCoefficients = IIRCoefficients::makeHighPass(sampleRate, frequency); break;
			case LowShelf:		currentCoefficients = IIRCoefficients::makeLowShelf(sampleRate, frequency, q, gain); break;
			case HighShelf:		currentCoefficients = IIRCoefficients::makeHighShelf(sampleRate, frequency, q, gain); break;
			case Peak:			currentCoefficients = IIRCoefficients::makePeakFilter(sampleRate, frequency, q, gain); break;
                case numFilterTypes: break;
			}

			leftFilter.setCoefficients(currentCoefficients);
			rightFilter.setCoefficients(currentCoefficients);
		};

		IIRCoefficients currentCoefficients;

		double sampleRate;
		double frequency;
		float gain;
		double q;
		bool enabled;
		FilterType type;

		IIRFilter leftFilter;
		IIRFilter rightFilter;
	};

	CurveEq(MainController *mc, const String &id):
		MasterEffectProcessor(mc, id),
		fftBufferIndex(0)
	{
		parameterNames.add("Gain");
		parameterNames.add("Freq");
		parameterNames.add("Q");
		parameterNames.add("Enabled");
		parameterNames.add("Type");
		parameterNames.add("BandOffset");

		FloatVectorOperations::fill(fftData, 0.0f, 256);
	};

	int getParameterIndex(int filterIndex, int parameterType) const
	{
		return filterIndex * numBandParameters + parameterType;
	}

	float getAttribute(int index) const override;;

	void setInternalAttribute(int index, float newValue) override;;

	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		ScopedLock sl(lock);

		for(int i = 0; i < filterBands.size(); i++)
		{
			filterBands[i]->process(buffer, startSample, numSamples);
		}

		if(fftBufferIndex < FFT_SIZE_FOR_EQ)
		{
			const int numSamplesToCopy = jmin<int>(numSamples, FFT_SIZE_FOR_EQ - fftBufferIndex);

			FloatVectorOperations::copyWithMultiply(fftData + fftBufferIndex, buffer.getReadPointer(0, startSample), 0.5f, numSamplesToCopy);
			FloatVectorOperations::addWithMultiply(fftData + fftBufferIndex, buffer.getReadPointer(1, startSample), 0.5f, numSamplesToCopy);

			fftBufferIndex += numSamplesToCopy;

		}
	};

	IIRCoefficients getCoefficients(int filterIndex)
	{
		return filterBands[filterIndex]->getCoefficients();
	};

	int getNumFilterBands() const
	{
		return filterBands.size();
	};

	StereoFilter *getFilterBand(int filterIndex)
	{
		return filterBands[filterIndex];
	}

	void addFilterBand(double freq, double gain)
	{
		ScopedLock sl(lock);

		StereoFilter *f = new StereoFilter();

		f->setSampleRate(getSampleRate());

		f->setType(CurveEq::Peak);

		f->setGain(gain);

		f->setFrequency(freq);

		filterBands.add(f);

		sendChangeMessage();
	}

	void removeFilterBand(int filterIndex)
	{
		filterBands.remove(filterIndex);

		sendChangeMessage();
	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		ScopedLock sl(lock);

		EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

		for(int i = 0; i < filterBands.size(); i++)
		{
			filterBands[i]->setSampleRate(sampleRate);
		}
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();

		v.setProperty("NumFilters", filterBands.size(), nullptr);

		for(int i = 0; i < filterBands.size() * numBandParameters; i++)
		{
			v.setProperty("Band" + String(i), getAttribute(i), nullptr);
		}

		return v;
	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);

		filterBands.clear();

		const int numFilters = v.getProperty("NumFilters", 0);

		for(int i = 0; i < numFilters; i++)
		{
			filterBands.add(new StereoFilter());
		}

		for(int i = 0; i < numFilters * numBandParameters; i++)
		{
#if HI_USE_BACKWARD_COMPATIBILITY
            if(v.hasProperty(String(i)))
            {
                const float value = v.getProperty(String(i), 0.0f);
                setAttribute(i, value, dontSendNotification);

            }
            else
            {
                const float value = v.getProperty("Band" + String(i), 0.0f);
                setAttribute(i, value, dontSendNotification);
            }
#else
            const float value = v.getProperty("Band" + String(i), 0.0f);
            setAttribute(i, value, dontSendNotification);
#endif
		}

	}

	struct AlignedDouble
	{
		double data;
		double padding;
	};

	const double *getExternalData()
	{
		ScopedLock sl(lock);

		for(int i = 0; i < FFT_SIZE_FOR_EQ; i++)
		{
			externalFftData[i] = (float)fftData[i];
		}

		fftBufferIndex = 0;

		return externalFftData;
	};

	bool hasTail() const override {return false;};

	int getNumChildProcessors() const override { return 0; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

private:

	float fftData[FFT_SIZE_FOR_EQ];

	double externalFftData[FFT_SIZE_FOR_EQ];

	

	int fftBufferIndex;

	CriticalSection lock;

	OwnedArray<StereoFilter> filterBands;

};


#endif  // CURVEEQ_H_INCLUDED
