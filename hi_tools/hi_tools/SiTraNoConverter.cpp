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

#if USE_IPP
#define USE_IPP_MEDIAN_FILTER 1
#else
#define USE_IPP_MEDIAN_FILTER 0
#endif

#include <set>

namespace hise { using namespace juce;;

#if USE_IPP_MEDIAN_FILTER
struct IppMedianFilter: public MedianFilter::Base
{
	IppMedianFilter(int windowSize):
	  M(windowSize)
	{
		
	}

	~IppMedianFilter() override
	{
		if(pSrc != nullptr)
		{
			ippsFree(pSrc);
			ippsFree(pDst);
			ippsFree(pDlySrc);
			ippsFree(pDlyDst);
			ippsFree(pWork);
		}
	}

	void reset()
	{
		auto pad = M/2 + 1;
		FloatVectorOperations::clear(pDlySrc, M);
		FloatVectorOperations::clear(pDlyDst, M);
		FloatVectorOperations::clear(pDst + N, pad);
	}

	void prepare(int N_)
	{
		N = N_;
		auto pad = M/2 + 1;
		pSrc = ippsMalloc_32f(N);
		pDst = ippsMalloc_32f(N + pad);
		pDlySrc = ippsMalloc_32f(M);
		pDlyDst = ippsMalloc_32f(M);

		int bufferSize;
		ippsFilterMedianGetBufferSize(M, IppDataType::ipp32f, &bufferSize);
		pWork = ippsMalloc_8u(bufferSize);

		reset();
	}

	void apply(const float* in, float* out, int N)
	{
		// You need to call prepare before calling this method
		jassert(pSrc != nullptr);

		FloatVectorOperations::copy(pSrc, in, N);
		ippsFilterMedian_32f(pSrc, pDst, N, M, pDlySrc, pDlyDst, pWork);
		auto offset = M/2 + 1;
		FloatVectorOperations::copy(out, pDst + offset, N);
	}

private:

	const int M;
	int N = 0;

	Ipp32f* pSrc = nullptr;
	Ipp32f* pDst = nullptr;
	Ipp32f* pDlySrc = nullptr;
	Ipp32f* pDlyDst = nullptr;
	Ipp8u* pWork = nullptr;
};
#endif

/** Calculates the median for the given array. */
struct FallbackMedianFilter: public MedianFilter::Base
{
	FallbackMedianFilter(int M_): M(M_) {}

	void reset()
	{
		up.clear();
		low.clear();
	}

	void apply(const float* v, float* out, int numSamples)
	{
		// you need to call prepare
		jassert(numSamples == N);

		low.insert(get(v, 0, N));

		for (int i = 1; i < M; i++)
			insert(get(v, i, N));

		out[0] = *low.rbegin();

		for (auto i = M; i < N; i++) 
		{
			if (M == 1) 
			{
				insert(get(v, i, N));
				erase(get(v, i-M, N));
			}
			else 
			{
				erase(get(v, i - M, N));
				insert(get(v, i, N));
			}

			auto x = *low.rbegin();
			out[i - M] = x;
		}
	}

	void prepare(int numSamples)
	{
		N = numSamples;
	}

private:

	float get(const float* v, int index, int N) const
	{
		if(index < M/2 || index >= N - M/2)
			return float();

		return v[index - M/2];
	}

	void insert(float val)
	{
		float a = *low.rbegin();

		if (a < val) 
		{
			up.insert(val);

			if (up.size() > M / 2) 
			{
				low.insert(*up.begin());
				up.erase(up.begin());
			}
		}
		else 
		{
			low.insert(val);

			if (low.size() > (M + 1) / 2) 
			{
				up.insert(*low.rbegin());
				low.erase(--low.end());
			}
		}
	}

	void erase(float val)
	{  
		if (up.find(val) != up.end()) 
			up.erase(up.find(val));
		else 
			low.erase(low.find(val));

		if (low.empty()) 
		{
			low.insert(*up.begin());
			up.erase(up.begin());
		}
	}

	std::multiset<float> up;
	std::multiset<float> low;

	const int M;
	int N = -1;
};

MedianFilter::Base* MedianFilter::createBestInstance(int windowSize)
{
#if USE_IPP_MEDIAN_FILTER
	return new IppMedianFilter(windowSize);
#else
	return new FallbackMedianFilter(windowSize);
#endif
}

	
SiTraNoConverter::ConfigData::ConfigData(const var& obj):
	slowFFTOrder(jlimit(5, 15, (int)obj.getProperty("SlowFFTOrder", 13))),
	fastFFTOrder(jlimit(5, 15, (int)obj.getProperty("FastFFTOrder", 9))),
	freqResolution((double)obj.getProperty("FreqResolution", 500.0)),
	timeResolution((double)obj.getProperty("TimeResolution", 0.2)),
	calculateTransients((bool)obj.getProperty("CalculateTransients", true))
{
	auto st = obj.getProperty("SlowTransientTreshold", var());
	auto ft = obj.getProperty("FastTransientTreshold", var());

	if(st.isArray() && st.size() == 2)
	{
		slowTransientThreshold[0] = jlimit(0.0, 1.0, (double)st[0]);
		slowTransientThreshold[1] = jlimit(0.0, 1.0, (double)st[1]);
		if(slowTransientThreshold[0] < slowTransientThreshold[1])
			std::swap(slowTransientThreshold[0], slowTransientThreshold[1]);
	}

	if(ft.isArray() && st.size() == 2)
	{
		fastTransientThreshold[0] = jlimit(0.0, 1.0, (double)ft[0]);
		fastTransientThreshold[1] = jlimit(0.0, 1.0, (double)ft[1]);
		if(fastTransientThreshold[0] < fastTransientThreshold[1])
			std::swap(fastTransientThreshold[0], fastTransientThreshold[1]);
	}
}

var SiTraNoConverter::ConfigData::toVar() const
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty("SlowFFTOrder", slowFFTOrder);
	obj->setProperty("FastFFTOrder", fastFFTOrder);
	obj->setProperty("FreqResolution", freqResolution);
	obj->setProperty("TimeResolution", timeResolution);
	obj->setProperty("CalculateTransients", calculateTransients);

	Array<var> st, ft;

	st.add(slowTransientThreshold[0]);
	st.add(slowTransientThreshold[1]);
	ft.add(fastTransientThreshold[0]);
	ft.add(fastTransientThreshold[1]);

	obj->setProperty("SlowTransientTreshold", var(st));
	obj->setProperty("FastTransientTreshold", var(ft));

	return var(obj.get());
}


SiTraNoConverter::SiTraNoConverter(double sampleRate, const ConfigData& cd_):
	cd(cd_),
	slowFFT(cd_.slowFFTOrder, sampleRate),
	fastFFT(cd_.fastFFTOrder, sampleRate)
{}

const AudioSampleBuffer& SiTraNoConverter::getSignalComponent(SignalComponent c) const
{
	return outputBuffers[(int)c];
}

void SiTraNoConverter::setThreadController(ThreadController::Ptr tc)
{
	p = tc;
}

void SiTraNoConverter::process(const AudioSampleBuffer& input)
{
	int numSamples = getInternalBufferSize(input);

	for(auto& o: outputBuffers)
	{
		o.setSize(input.getNumChannels(), numSamples);
		o.clear();
	}

	originalBuffer = input;
	    
	workBuffer.setSize(input.getNumChannels(), numSamples);
	workBuffer.clear();

	signalNormaliseGains.resize(input.getNumChannels());

	for(int i = 0; i < input.getNumChannels(); i++)
	{
		prepareChannel(input.getReadPointer(i), i, input.getNumSamples());
	}

	for(int i = 0; i < input.getNumChannels(); i++)
	{
		ThreadController::ScopedStepScaler cs(p.get(), i, input.getNumChannels());

		if(!cs && p.get() != nullptr)
			break;

		{
			ThreadController::ScopedStepScaler sine(p.get(), 0, 2);

			if(!sine && p.get() != nullptr)
				break;

			performFFT(i, false);
			applyMedianFilters(false);
			calculateSinusoidalPart(i);
		}

		ThreadController::ScopedStepScaler noise(p.get(), 1, 2);

		if(!noise && p.get() != nullptr)
			break;

		if(cd.calculateTransients)
		{
			performFFT(i, true);
			applyMedianFilters(true);
			calculateTransientNoise(i);
		}
	}

	int offset = slowFFT.hopSize;

	for(auto& ob: outputBuffers)
	{
		for(int c = 0; c < ob.getNumChannels(); c++)
		{
			auto src = ob.getReadPointer(c, offset);
			auto dst = ob.getWritePointer(c, 0);
			auto g = 1.0f;;// / signalNormaliseGains[c];

			int numToMove = ob.getNumSamples() - offset;

			for(int i = 0; i < numToMove; i++)
			{
				*dst = *src * g;
				dst++;
				src++;
			}

			FloatVectorOperations::clear(ob.getWritePointer(c, numToMove), offset);
		}
	}

	for(int c = 0; c < originalBuffer.getNumChannels(); c++)
	{
		const float* o = originalBuffer.getReadPointer(c, 0);
		auto s = outputBuffers[(int)SignalComponent::Sinusoidal].getReadPointer(c);
		auto n = outputBuffers[(int)SignalComponent::Noise].getReadPointer(c);
		auto t = outputBuffers[(int)SignalComponent::Transient].getReadPointer(c);

		auto dst = outputBuffers[(int)(cd.calculateTransients ? SignalComponent::Transient : SignalComponent::Noise)].getWritePointer(c);

		for(int i = 0; i < originalBuffer.getNumSamples(); i++)
		{
			float v = 0.0f;

			v += s[i];
			v += n[i];
			v += t[i];

			auto diff = o[i] - v;

			dst[i] += diff;
		}
	}
}

void SiTraNoConverter::performFFT(int channelIndex, bool useFast)
{
	ThreadController::ScopedStepScaler sss(p.get(), 0, 3);

	if(!sss && p.get() != nullptr)
		return;

	auto& fft = useFast ? fastFFT : slowFFT;
	fft.process(workBuffer.getReadPointer(channelIndex), workBuffer.getNumSamples());
}

void SiTraNoConverter::applyMedianFilters(bool useFast)
{
	ThreadController::ScopedStepScaler sss(p.get(), 1, 3);

	if(!sss && p.get() != nullptr)
		return;

	const auto& coef = useFast ? cd.fastTransientThreshold : cd.slowTransientThreshold;

	auto& fft = useFast ? fastFFT : slowFFT;
	fft.applyMedianFilters(coef[0], coef[1], cd.freqResolution, cd.timeResolution);
}

int SiTraNoConverter::getInternalBufferSize(const AudioSampleBuffer& input) const
{
	auto numSamples = input.getNumSamples();
	auto fftSize = slowFFT.fft.getSize();
	auto zeroAmount = slowFFT.hopSize;

	numSamples += zeroAmount;

	auto zeroesAfter = fftSize - numSamples % fftSize;

	numSamples += zeroesAfter;
	jassert(numSamples % fftSize == 0);
	return numSamples;
}

void SiTraNoConverter::prepareChannel(const float* inputData, int channelIndex, int numSamplesInSignal)
{
	auto preFFT = getPaddingOffset();

	FloatVectorOperations::copy(workBuffer.getWritePointer(channelIndex, preFFT), inputData, numSamplesInSignal);
	//workBuffer.applyGainRamp(channelIndex, preFFT, preFFT, 0.0f, 1.0f);
	//workBuffer.applyGainRamp(channelIndex, workBuffer.getNumSamples() - preFFT, preFFT, 1.0f, 0.0f);
	auto gain1 = workBuffer.getMagnitude(channelIndex, 0, workBuffer.getNumSamples());
	signalNormaliseGains[channelIndex] = 0.95f / gain1;
	workBuffer.applyGain(channelIndex, 0, workBuffer.getNumSamples(), signalNormaliseGains[channelIndex]);
}

void SiTraNoConverter::calculateSinusoidalPart(int channelIndex)
{
	ThreadController::ScopedStepScaler sss(p.get(), 3, 3);

	if(!sss && p.get() != nullptr)
		return;

	auto& sb = outputBuffers[(int)SignalComponent::Sinusoidal];
	slowFFT.applyMask(SignalComponent::Sinusoidal, sb, channelIndex);

	sb.applyGain(channelIndex, 0, sb.getNumSamples(), 1.0f / signalNormaliseGains[channelIndex]);

	auto& targetBuffer = cd.calculateTransients ? workBuffer : outputBuffers[(int)SignalComponent::Noise];

	targetBuffer.clear(channelIndex, 0, targetBuffer.getNumSamples());
	slowFFT.applyMask(SignalComponent::Transient, targetBuffer, channelIndex);
	slowFFT.applyMask(SignalComponent::Noise, targetBuffer, channelIndex);

	if(!cd.calculateTransients)
		targetBuffer.applyGain(channelIndex, 0, targetBuffer.getNumSamples(), 1.0f / signalNormaliseGains[channelIndex]);
	
}

void SiTraNoConverter::calculateTransientNoise(int channelIndex)
{
	ThreadController::ScopedStepScaler sss(p.get(), 3, 3);

	if(!sss && p.get() != nullptr)
		return;

	jassert(cd.calculateTransients);

	auto& tb = outputBuffers[(int)SignalComponent::Transient];
	auto& nb = outputBuffers[(int)SignalComponent::Noise];

	fastFFT.applyMask(SignalComponent::Noise, nb, channelIndex, &noiseGrains);

	fastFFT.applyMask(SignalComponent::Transient, tb, channelIndex);
	
}

void SiTraNoConverter::MagnitudeMatrix::applyMedianFilter(double horizontalFrequency, double verticalFrequency)
{
	{
		// horizontal filtering
		auto windowSize = roundToInt(horizontalFrequency * sampleRate / hopSize);

		if(windowSize % 2 == 0)
			windowSize++;

		std::vector<float> medianValues(numRows, 0.0f);
		std::vector<float> filteredValues(numRows, 0.0f);

		MedianFilter mf(windowSize);
		mf.prepare(numRows);

		for (int h = 0; h < numBins; ++h)
		{
			for (int i = 0; i < numRows; ++i)
			{
				auto v = horizontallyFiltered[i][h];
				medianValues[i] = v;
			}

			mf.reset();
			mf.apply(medianValues.data(), filteredValues.data(), (int)medianValues.size());

			for(int i = 0; i < numRows; i++)
				horizontallyFiltered[i][h] = filteredValues[i];
		}
	}
            
	{
		// vertical filtering
		int windowSize = roundToInt(verticalFrequency * fftSize / sampleRate);

		if(windowSize % 2 == 0)
			windowSize++;

		std::vector<float> medianValues(numBins, 0.0f);

		MedianFilter mf(windowSize);
		mf.prepare(numBins);

		for (int i = 0; i < numRows; ++i)
		{
			FloatVectorOperations::copy(medianValues.data(), verticallyFiltered[i].data(), numBins);
			mf.apply(medianValues.data(), verticallyFiltered[i].data(), (int)medianValues.size());
		}
	}
}

void SiTraNoConverter::MagnitudeMatrix::calculateTransients()
{
	transients.resize(numRows, std::vector<float>(numBins));

	for (int x = 0; x < numRows; x++) 
	{
		for (int y = 0; y < numBins; y++) 
		{
			auto v = verticallyFiltered[x][y];
			auto h = horizontallyFiltered[x][y];
			float denom = v + h;
			auto r = denom != 0.0f ? v / denom : 0.0f;
			FloatSanitizers::sanitizeFloatNumber(r);
			transients[x][y] = r; 
		}
	}
}

void SiTraNoConverter::MagnitudeMatrix::calculateMasks(double G1, double G2)
{
	size_t newNumCols = 2 * (numBins - 1); // New number of columns after appending the conjugate and flipped part

	for(auto& m: componentMask)
		m.resize(numRows, std::vector<float>(newNumCols));
            
	auto applyThreshold = [](double G1, double G2, float v)
	{
		float out;

		if (v >= G1)     out = 1.0f;
		else if (v < G2) out = 0.0f;
		else             out = std::sin(float_Pi * (v - G2) / (2.0f * (G1 - G2)));

		return out * out;
	};

	for (size_t row = 0; row < numRows; ++row) 
	{
		for (size_t col = 0; col < numBins; ++col) 
		{
			auto tv = transients[row][col];

			auto v = applyThreshold(G1, G2, 1.0f - tv);
			auto t = applyThreshold(G1, G2, tv);

			componentMask[(int)SignalComponent::Sinusoidal][row][col] = v;
			componentMask[(int)SignalComponent::Transient][row][col] = t;
			componentMask[(int)SignalComponent::Noise][row][col] = 1.0f - v - t;
		}

		for(auto &m: componentMask)
			appendConjugate(m[row]);
	}
}

void SiTraNoConverter::MagnitudeMatrix::appendConjugate(std::vector<float>& row)
{
	for(int x = numBins; x < row.size(); x++)
	{
		auto x_inv = row.size() - x;
		row[x] = row[x_inv];
	}
}

void SiTraNoConverter::MagnitudeMatrix::setData(const HeapBlock<juce::dsp::Complex<float>>& cv, int numRows_, int fftSize)
{
	numRows = numRows_;
	horizontallyFiltered.resize(numRows, std::vector<float>(numBins, 0.0f));
	verticallyFiltered.resize(numRows, std::vector<float>(numBins, 0.0f));

	for(int i = 0; i < horizontallyFiltered.size(); i++)
	{
		for(int s = 0; s < horizontallyFiltered[0].size(); s++)
		{
			auto offset = i * fftSize + s;

			auto v = cv[offset];
			auto mag = std::abs(v);// std::sqrt(v.imag() * v.imag() + v.real() * v.real());
			horizontallyFiltered[i][s] = mag;
			verticallyFiltered[i][s] = mag;
		}
	}
}

SiTraNoConverter::FFTData::FFTData(int order, double sampleRate):
	fft(order),
	hopSize(fft.getSize() / 8),
	numBins(fft.getSize() / 2 + 1),
	signalInput(fft.getSize()),
	complexData(fft.getSize()),
	hann(fft.getSize()),
	magnitudeMatrix(fft.getSize(), sampleRate)
{
	using WT = juce::dsp::WindowingFunction<float>;
	WT::fillWindowingTables(hann.data(), hann.size(), WT::hann, false);
}

void SiTraNoConverter::FFTData::process(const float* data, int numSamples)
{
	numColumns = std::floor((numSamples - fft.getSize()) / hopSize + 1);
	complexMatrix.calloc(numColumns * fft.getSize());

	int frameIndex = 0;

	for(int i = 0; (i + fft.getSize()) <= numSamples; i += hopSize)
	{
		jassert(isPositiveAndBelow(frameIndex, numColumns));

		for(int h = 0; h < fft.getSize(); h++)
		{
			signalInput[h].imag(0.0);

			auto v = data[i + h];
			v *= hann[h];
			signalInput[h].real(v);
		}

		fft.perform(signalInput.data(), complexData.data(), false);

		jassert(isPositiveAndBelow(frameIndex, numColumns));
		auto offset = frameIndex++ * fft.getSize();

		memcpy(complexMatrix.get() + offset, complexData.data(), sizeof(juce::dsp::Complex<float>) * fft.getSize());
	}

	magnitudeMatrix.setData(complexMatrix, numColumns, fft.getSize());
}

void SiTraNoConverter::FFTData::applyMedianFilters(double G1, double G2, double freqResolution, double timeResolution)
{
	magnitudeMatrix.applyMedianFilter(timeResolution, freqResolution);
	magnitudeMatrix.calculateTransients();
	magnitudeMatrix.calculateMasks(G1, G2);
}

void SiTraNoConverter::FFTData::applyMask(SignalComponent c, AudioSampleBuffer& b, int channelIndex, std::vector<AudioSampleBuffer>* grainBuffer)
{
	const auto& mask = magnitudeMatrix.getMask(c);

	auto numSamples = (numColumns - 1) * hopSize + fft.getSize();
	size_t numRows = mask.size();
	size_t numCols = mask[0].size();

	//jassert(mask.size() == complexMatrix.size());
	//jassert(mask[0].size() == complexMatrix[0].size());
	jassert(numSamples == b.getNumSamples());
	ignoreUnused(numSamples);

	HeapBlock<juce::dsp::Complex<float>> result;
	result.calloc(numColumns * fft.getSize());

	for (size_t row = 0; row < numRows; ++row) 
	{
		for (size_t col = 0; col < numCols; ++col)
		{
			auto off = row * fft.getSize() + col;
			result[off] = mask[row][col] * complexMatrix[off];
		}
	}

	auto dst = b.getWritePointer(channelIndex);
	int frameOffset = 0;

	for (int i = 0; i < (int)numColumns; ++i)
	{
		auto off = i * fft.getSize();

		fft.perform(result.get() + off, signalInput.data(), true);

		if(grainBuffer != nullptr)
		{
			if(channelIndex == 0)
			{
				grainBuffer->push_back(AudioSampleBuffer(b.getNumChannels(), fft.getSize()));
			}
			else
			{
				jassert(isPositiveAndBelow(i, grainBuffer->size()));
			}
			
			auto& gb = (*grainBuffer)[i];
			auto ptr = gb.getWritePointer(channelIndex);

			for (int j = 0; j < (int)fft.getSize(); ++j)
				ptr[j] = signalInput[j].real() * hann[j] / 3;
		}

		for (int j = 0; j < (int)fft.getSize(); ++j)
			dst[frameOffset + j] += signalInput[j].real() * hann[j] / 3;

		frameOffset += hopSize;
	}
}
} // namespace hise
