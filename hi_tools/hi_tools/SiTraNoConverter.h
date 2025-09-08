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

#pragma once

namespace hise { using namespace juce;

/** This is a median filter that uses the best implementation possible:
 *
 *  - if IPP is enabled, it will use the median filter algorithm from the IPP class
 *  - if IPP is not enabled, it will use a fallback algorithm using a sliding window and std::multiset with O(n*log(n))
 *
 *  It uses zero padding for the values beyond the data. 
 */
struct MedianFilter
{
	struct Base
	{
		virtual ~Base() {};
        virtual void reset() = 0;
        virtual void prepare(int numSamples) = 0;
        virtual void apply(const float* in, float* out, int numSamples) = 0;
	};

    /** Creates a filter with the given windowSize. If windowSize is even it will be rounded up to the next odd number. */
    MedianFilter(int windowSize):
      impl(createBestInstance(windowSize))
    {}

    /** Resets the filter state. Call this in between multiple calls to this function. */
    void reset()
    {
	    impl->reset();
    }

    /** Call this with the number of samples you want to process. */
    void prepare(int numSamples)
    {
	    impl->prepare(numSamples);
    }

    /** Applies the filter and writes the median values into out. This does not allocate. */
    void apply(const float* in, float* out, int numSamples)
    {
	    impl->apply(in, out, numSamples);
    }

private:

    static Base* createBestInstance(int windowSize);

    ScopedPointer<Base> impl;
};


/** A DSP tool that splits up an audio signal into sinusoidal, noise & transient components.
 *
 *  This is a custom implementation of the algorithm presented in the paper
 *	"L. Fierro, and V. Välimäki. "SiTraNo: a MATLAB app for sines-transient-noise decomposition of audio signals"
 *
 *  Improvements:
 *  - use IPP for median filter if enabled, otherwise use faster fallback implementation with sliding window
 *  - rewritten into single class for simple usage
 *  - proper multichannel support
 *  - ConfigData object that contains all properties of the algorithm with JSON import / export
 *  - sample-perfect recreation of the original buffer (the fades at the signal start/end are added to the transient buffer)
 *  - support for ThreadController
 */
struct SiTraNoConverter
{
    struct ConfigData
    {
        ConfigData() {};

        ConfigData(const var& obj);

        var toVar() const;

        int slowFFTOrder = 13;
        int fastFFTOrder = 9;

        double freqResolution = 500.0;
        double timeResolution = 0.2;
        bool calculateTransients = true;

        double slowTransientThreshold[2] = { 0.8, 0.7};
        double fastTransientThreshold[2] = { 0.85, 0.75};
    };

    using ComplexVector = std::vector<juce::dsp::Complex<float>>;
    using Vector2D = std::vector<std::vector<float>>;

    enum class SignalComponent
    {
	    Sinusoidal,
        Transient,
        Noise,
        numComponents
    };

	/** Create a converter object with the given sample rate.
	 *
	 *	If calculateTransients_ is false, then the second step of dividing the noise
	 *	into transients & residual noise is skipped to improve performance.
	 */
    SiTraNoConverter(double sampleRate, const ConfigData& cd={});
    ~SiTraNoConverter() = default;

	/** Returns the component after processing. */
    const AudioSampleBuffer& getSignalComponent(SignalComponent c) const;

    /** You can give this class a thread controller and it will use it for communicating the process &
     *  check for abortion. */
    void setThreadController(ThreadController::Ptr tc);

    /** Call this function in order to process the input buffer and split it into separate functions. */
    void process(const AudioSampleBuffer& input);

    const std::vector<AudioSampleBuffer>& getNoiseGrains()
    {
	    return noiseGrains;
    }

    /** The signal buffers are zero padded by this amount. */
    int getPaddingOffset() const
    {
        return slowFFT.hopSize;// fft.getSize() / 4;
    }

private:

    void performFFT(int channelIndex, bool useFast);
    void applyMedianFilters(bool useFast);
    int getInternalBufferSize(const AudioSampleBuffer& input) const;
    void prepareChannel(const float* inputData, int channelIndex, int numSamplesInSignal);
    void calculateSinusoidalPart(int channelIndex);
    void calculateTransientNoise(int channelIndex);

    struct MagnitudeMatrix
    {
        MagnitudeMatrix(int fftSize_, double sampleRate_):
          fftSize(fftSize_),
          hopSize(fftSize/8),
          numBins(fftSize / 2 + 1),
          sampleRate(sampleRate_)
        {}

        void applyMedianFilter(double horizontalFrequency, double verticalFrequency);
        void calculateTransients();
        void calculateMasks(double G1, double G2);
        void appendConjugate(std::vector<float>& row);
        void setData(const HeapBlock<juce::dsp::Complex<float>>& cv, int numRows_, int fftSize);

        const Vector2D& getMask(SignalComponent c) const { return componentMask[(int)c]; }

    private:

        const int fftSize;
        const int hopSize;
        const int numBins;
        const double sampleRate;

        int numRows = 0;

	    Vector2D verticallyFiltered;
        Vector2D horizontallyFiltered;
        Vector2D transients;

        std::array<Vector2D, (int)SignalComponent::numComponents> componentMask;
    };

    struct FFTData
    {
        FFTData(int order, double sampleRate);

        void process(const float* data, int numSamples);
        void applyMedianFilters(double G1, double G2, double freqResolution, double timeResolution);
        void applyMask(SignalComponent c, AudioSampleBuffer& b, int channelIndex, std::vector<AudioSampleBuffer>* grainBuffers=nullptr);

        juce::dsp::FFT fft;
        int numBins;
	    int numColumns;
        int hopSize;

        // the chunk that is put into the FFT
        ComplexVector signalInput;
        ComplexVector complexData;
        std::vector<float> hann;
        
        // The FFT output for each frame
        HeapBlock<juce::dsp::Complex<float>> complexMatrix;
        MagnitudeMatrix magnitudeMatrix;
    };

    ConfigData cd;
    
    FFTData slowFFT;
    FFTData fastFFT;

    std::vector<float> signalNormaliseGains;

    std::vector<AudioSampleBuffer> noiseGrains;

    AudioSampleBuffer workBuffer;
    AudioSampleBuffer originalBuffer;
	std::array<AudioSampleBuffer, (int)SignalComponent::numComponents> outputBuffers;
	ThreadController::Ptr p;
};


} // namespace hise

