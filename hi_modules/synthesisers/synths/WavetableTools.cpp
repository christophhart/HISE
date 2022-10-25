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

namespace hise {
using namespace juce;

#if USE_BACKEND


WavetableConverter::WavetableConverter()
{
	data = ValueTree("wavetableData");

	for (int i = 0; i < 128; i++)
		dataBuffers.add(new AudioSampleBuffer());

	wavetableSizes.insertMultiple(0, -1, 128);
}

juce::ValueTree WavetableConverter::convertDirectoryToWavetableData(const String &directoryPath_)
{
	WavAudioFormat waf;
	double sampleRate = -1.0;
	directoryPath = File(directoryPath_);
	jassert(directoryPath.isDirectory());
	for(auto f: RangedDirectoryIterator(directoryPath, false, "*.wav"))
	{
		File wavFile = f.getFile();
		const String noteName = wavFile.getFileNameWithoutExtension().upToFirstOccurrenceOf("_", false, false);
		const int noteNumber = getMidiNoteNumber(noteName);

		if (noteNumber != -1)
		{
			juce::AudioSampleBuffer *bufferOfNote = dataBuffers[noteNumber];
			const int wavetableNumber = wavFile.getFileNameWithoutExtension().fromFirstOccurrenceOf("_", false, false).getIntValue();
			jassert(wavetableNumber >= 0);

			reader = waf.createMemoryMappedReader(wavFile);
			reader->mapEntireFile();
			sampleRate = reader->sampleRate;
			const int numSamples = (int)reader->lengthInSamples;

			if (bufferOfNote->getNumSamples() != numSamples * 64)
			{
				jassert(wavetableSizes[noteNumber] == -1);

				bufferOfNote->setSize(2, numSamples * 64);
				bufferOfNote->clear();
			}

			jassert(wavetableSizes[noteNumber] == numSamples || wavetableSizes[noteNumber] == -1);
			wavetableSizes.set(noteNumber, numSamples);
			reader->read(bufferOfNote, numSamples * wavetableNumber, numSamples, 0, true, true);
		}
	}


	for (int i = 0; i < 128; i++)
	{
		if (wavetableSizes[i] == -1) continue;

		jassert(dataBuffers[i]->getNumSamples() == wavetableSizes[i] * 64);
		jassert(dataBuffers[i]->getMagnitude(0, 0, dataBuffers[i]->getNumSamples()) != 0.0f);

		ValueTree child = ValueTree("wavetable");

		child.setProperty("noteNumber", i, nullptr);
		child.setProperty("amount", 64, nullptr);
		child.setProperty("sampleRate", sampleRate, nullptr);

		MemoryBlock mb(dataBuffers[i]->getReadPointer(0, 0), dataBuffers[i]->getNumSamples() * sizeof(float));
		var binaryData(mb);
		child.setProperty("data", binaryData, nullptr);
		data.addChild(child, -1, nullptr);
	}

	return data;
}

int WavetableConverter::getWavetableLength(int noteNumber, double sampleRate)
{
	const double freq = MidiMessage::getMidiNoteInHertz(noteNumber);
	const int sampleNumber = (int)(sampleRate / freq);

	return sampleNumber;
}

int WavetableConverter::getMidiNoteNumber(const String &midiNoteName)
{
	for (int i = 0; i < 127; i++)
	{
		if (MidiMessage::getMidiNoteName(i, true, true, 3) == midiNoteName)
		{
			return i;
		}

	}

	return -1;
}


struct ResynthesisHelpers
{
	struct SimpleNoteConversionData
	{
		File sampleFile;
		double rootRatio;
		int noteNumber;
		Range<int> veloRange;
	};

	static double getRootFrequencyInBuffer(const AudioSampleBuffer &buffer, double sampleRate, double estimatedFreq)
	{
		float pitch = (float)PitchDetection::detectPitch(buffer, 0, buffer.getNumSamples(), sampleRate);
		FloatSanitizers::sanitizeFloatNumber(pitch);
		return (double)pitch;
	}

	/** Analyses the given buffer and fills the given buffer with the normalized harmonic spectrum.
	*
	*	@param buffer: the input buffer
	*	@param sampleRate: the sample rate
	*	@param harmonicSpectrum: the audio buffer that will contain the harmonic spectrum. Just pass in an empty one
	*							 and it will resize it to the number of available harmonics
	*	@param debugString: a pointer to a string that is used for debugging (if not nullptr).
	*	@returns: true if the root frequency could be successfully detected, false otherwise
	*/
	static bool calculateHarmonicSpectrum(const AudioSampleBuffer &buffer, double sampleRate, AudioSampleBuffer& harmonicSpectrum, double pitch, SampleMapToWavetableConverter::WindowType windowType)
	{
		ignoreUnused(buffer, sampleRate, pitch, harmonicSpectrum, windowType);

		if (pitch != 0.0)
		{
			int size = buffer.getNumSamples();

            auto order = log2(size);
            
            auto fft = juce::dsp::FFT(order);
            
            HeapBlock<float> dl_;
            HeapBlock<float> dr_;
            
            dl_.calloc(size * 2);
            dr_.calloc(size * 2);
            
            float* dl = dl_.get();
            float* dr = dl_.get();

            FloatVectorOperations::copy(dl, buffer.getReadPointer(0), size);
			FloatVectorOperations::copy(dr, buffer.getReadPointer(1), size);

			float* w = (float*)alloca(sizeof(float)*size);

            FloatVectorOperations::fill(w, 1.0f, size);
            FFTHelpers::applyWindow(windowType, w, size);
			
			FloatVectorOperations::multiply(dl, w, size);
			FloatVectorOperations::multiply(dr, w, size);

            fft.performRealOnlyForwardTransform(dl);
            
            HeapBlock<float> magL_, magR_;
            
            magL_.calloc(size/2);
            magR_.calloc(size/2);
            
            float* magL = magL_.get();
            float* magR = magR_.get();

			for (int i = 0; i < size / 2; i++)
			{
				magL[i] = dl[2 * i] * dl[2 * i] + dl[2 * i + 1] * dl[2 * i + 1];
				magL[i] = sqrt(magL[i]);

				magR[i] = dr[2 * i] * dr[2 * i] + dr[2 * i + 1] * dr[2 * i + 1];
				magR[i] = sqrt(magR[i]);
			}

			auto halfSize = (double)size / 2.0;
			int numHarmonics = harmonicSpectrum.getNumSamples();

			float* ampL = harmonicSpectrum.getWritePointer(0);
			float* ampR = harmonicSpectrum.getWritePointer(1);

			FloatVectorOperations::clear(ampL, numHarmonics);
			FloatVectorOperations::clear(ampR, numHarmonics);

			for (int i = 0; i < numHarmonics; i++)
			{
				auto hmFreq = pitch * double(i + 1);
				auto index = roundToInt(hmFreq / (sampleRate / 2.0) * halfSize);

				index = jmax<int>(0, index);

				if (index >= size / 2)
				{
					
					ampL[i] = 0.0f;
					ampR[i] = 0.0f;
				}
				else
				{
					ampL[i] = magL[index];
					ampR[i] = magR[index];
				}
			}

			auto maxL = FloatVectorOperations::findMaximum(ampL, numHarmonics);
			auto maxR = FloatVectorOperations::findMaximum(ampR, numHarmonics);

			if (maxL > 0.0f)
				FloatVectorOperations::multiply(ampL, 1.0f / maxL, numHarmonics);
			else
				FloatVectorOperations::clear(ampL, numHarmonics);

			if (maxR > 0.0f)
				FloatVectorOperations::multiply(ampR, 1.0f / maxR, numHarmonics);
			else
				FloatVectorOperations::clear(ampR, numHarmonics);

			return true;
		}
		else
		{
			return false;
		}
	}

	static int getWavetableLength(int noteNumber, double sampleRate)
	{
		const double freq = MidiMessage::getMidiNoteInHertz(noteNumber);
		const int sampleNumber = (int)(sampleRate / freq);

		return sampleNumber;
	};

	static AudioSampleBuffer loadAndResampleAudioFilesForVelocity(const Array<SimpleNoteConversionData>& data, double sampleRate)
	{
		AudioSampleBuffer output;

		if (data.isEmpty())
			return output;

		auto noteNumber = data.getFirst().noteNumber;

		auto totalSize = getWavetableLength(noteNumber, sampleRate) * data.size();

		output.setSize(2, totalSize);
		output.clear();

		int index = 0;

		for (auto s : data)
		{
			auto resampled = loadAndResampleAudioFile(s.sampleFile, noteNumber, sampleRate);

			int numThisTime = resampled.getNumSamples();

			output.copyFrom(0, index, resampled, 0, 0, numThisTime);

			if(resampled.getNumChannels() == 2)
				output.copyFrom(1, index, resampled, 1, 0, numThisTime);

			index += numThisTime;
		}

		return output;
	}

    static float cubic(const float x,const float L1,const float L0,const
                             float H0,const float H1)
    {
        return
        L0 +
        .5f*
        x*(H0-L1 +
           x*(H0 + L0*(-2) + L1 +
              x*( (H0 - L0)*9 + (L1 - H1)*3 +
                 x*((L0 - H0)*15 + (H1 -  L1)*5 +
                    x*((H0 - L0)*6 + (L1 - H1)*2 )))));
    }
    

    static void interpolateCubic(dsp::AudioBlock<float>& upsampledBuffer, int numInputSamples, int numOutputSamples, float latencyInSamples)
    {
		double delta = (double)(numInputSamples) / (double)numOutputSamples;
        double uptime = (double)latencyInSamples;
        
		AudioSampleBuffer scratchBuffer((int)upsampledBuffer.getNumChannels(), numOutputSamples);
        
        for (int i = 0; i < numOutputSamples; i++)
        {
            int l0i = (int)uptime;
            int h0i = (l0i + 1) % numInputSamples;
            int h1i = (h0i + 1) % numInputSamples;
            int l1i = (l0i - 1);
            if(l1i < 0)
                l1i += numInputSamples;
            
            auto alpha = (float)fmod(uptime, 1.0);
            
            auto l1 = upsampledBuffer.getSample(0, l1i);
            auto l0 = upsampledBuffer.getSample(0, l0i);
            auto h0 = upsampledBuffer.getSample(0, h0i);
            auto h1 = upsampledBuffer.getSample(0, h1i);
            
            auto v = cubic(alpha, l1, l0, h0, h1);
            
			scratchBuffer.setSample(0, i, v);
            
            uptime += delta;
        }
        
		upsampledBuffer.clear();
		upsampledBuffer.copyFrom(scratchBuffer, 0, 0, numOutputSamples);

        jassert(std::abs(uptime - (double)numInputSamples - (double)latencyInSamples) < 0.001);
    }
    
	static void interpolateLinear(const AudioSampleBuffer& input, AudioSampleBuffer& output)
	{
		double delta = (double)(input.getNumSamples()) / (double)output.getNumSamples();
		double uptime = 0.0;


		for (int i = 0; i < output.getNumSamples(); i++)
		{
			int first = (int)uptime;
			int next = (first + 1) % input.getNumSamples();
			auto alpha = (float)fmod(uptime, 1.0);

			auto firstSample = input.getSample(0, first);
			auto nextSample = input.getSample(0, next);

			auto v = Interpolator::interpolateLinear(firstSample, nextSample, alpha);

			output.setSample(0, i, v);

			uptime += delta;
		}

		
		jassert(std::abs(uptime - (double)input.getNumSamples()) < 0.001);
	}

	using Oversampler = juce::dsp::Oversampling<float>;
	static constexpr int OversamplingFactor = 16;

	static Oversampler* createOversampler(int cycleLength)
	{
		auto os = new Oversampler(1, (int)std::log2(OversamplingFactor), Oversampler::FilterType::filterHalfBandPolyphaseIIR, false);

		os->initProcessing(cycleLength + (int)(std::ceil(os->getLatencyInSamples())));
		
		return os;
	}

	static dsp::AudioBlock<float> extendAndUpsample(Oversampler* os, const AudioSampleBuffer& oneCycle, int numInputSamples)
	{
		auto oneSize = oneCycle.getNumSamples();

		AudioSampleBuffer three(oneCycle.getNumChannels(), oneSize * 3);

		three.copyFrom(0, oneSize * 0, oneCycle, 0, 0, oneSize);
		three.copyFrom(0, oneSize * 1, oneCycle, 0, 0, oneSize);
		three.copyFrom(0, oneSize * 2, oneCycle, 0, 0, oneSize);

		os->reset();

		return os->processSamplesUp(dsp::AudioBlock<float>(three));
	}

	static AudioSampleBuffer downsampleAndChop(Oversampler* os, int numChannels, int oneCycleLength)
	{
		int threeCycles = oneCycleLength * 3;

		AudioSampleBuffer three(numChannels, threeCycles);

		dsp::AudioBlock<float> b(three);

		os->processSamplesDown(b);

		AudioSampleBuffer one(numChannels, oneCycleLength);
		
		one.copyFrom(0, 0, three, 0, oneCycleLength, oneCycleLength);

		if(numChannels == 2)
			one.copyFrom(1, 0, three, 1, oneCycleLength, oneCycleLength);

		return one;
	}

	static AudioSampleBuffer loadAndResampleAudioFile(const File& f, int noteNumber, double sampleRate)
	{
		AudioFormatManager afm;
		afm.registerBasicFormats();

		ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(f);

		AudioSampleBuffer b;

		if (reader != nullptr)
		{
			// You'll usually don't find longer wavetables than this....
			jassert(reader->lengthInSamples < 8192);

			int inputSize = (int)reader->lengthInSamples;
			int outputSize = getWavetableLength(noteNumber, sampleRate);

#if 0
			b.setSize(1, inputSize);
			b.clear();

			reader->read(&b, 0, inputSize, 0, true, true);

			AudioSampleBuffer output(1, outputSize);

			interpolateLinear(b, output);

			return output;

			
#else
			auto workingBufferSize = jmax<int>(outputSize, inputSize) * 3 ;

			ScopedPointer<Oversampler> os = createOversampler(workingBufferSize);

			auto extraSamplesForLatency = (int)std::ceil(os->getLatencyInSamples());

			b.setSize(1, workingBufferSize + extraSamplesForLatency);
			b.clear();

			reader->read(&b, 0, inputSize, 0, true, true);
			reader->read(&b, inputSize, inputSize, 0, true, true);
			reader->read(&b, inputSize * 2, inputSize, 0, true, true);
			reader->read(&b, inputSize * 3, extraSamplesForLatency, 0, true, true);
			


			auto upsampled = os->processSamplesUp(dsp::AudioBlock<float>(b));

			

			interpolateCubic(upsampled, inputSize * 3 * OversamplingFactor, outputSize * 3 * OversamplingFactor, os->getLatencyInSamples());

			

			auto output =  downsampleAndChop(os, 1, outputSize);


			return output;
#endif
		}

		return b;
	}

	static void createWavetableFromHarmonicSpectrum(const float* hmx, int numHarmonics, float* data, int noteNumber, double sampleRate = 48000.0)
	{
		auto length = getWavetableLength(noteNumber, sampleRate);

		FloatVectorOperations::clear(data, length);




		double rootDelta = 2.0 * double_Pi / (double)length;

		for (int h = 0; h < numHarmonics; h++)
		{
			float harmonicGain = hmx[h];

			if (harmonicGain <= 0.0001f)
				continue;

			double uptime = 0.0;

			for (int i = 0; i < length; i++)
			{
				data[i] += harmonicGain * (float)sin(uptime);
				uptime += rootDelta * (double)(h + 1);
			}
		}

		auto range = FloatVectorOperations::findMinAndMax(data, length);

		auto maxLevel = jmax<float>(fabsf(range.getStart()), fabsf(range.getEnd()));

		FloatVectorOperations::multiply(data, 1.0f / maxLevel, length);

	}

};

void SampleMapToWavetableConverter::HarmonicMap::clear(int numHarmonics /*= 0*/)
{
	memset(pitchDeviations, 0, sizeof(double) * numWavetables);
	harmonicGains.setSize(64, numHarmonics);
	harmonicGains.clear();
	harmonicGainsRight.setSize(64, numHarmonics);
	harmonicGainsRight.clear();

	gainValues.clear();
	gainValues.setSize(2, 64);
}

void SampleMapToWavetableConverter::HarmonicMap::replaceWithNeighbours(int harmonicIndex)
{
	replaceInternal(harmonicIndex, true);
	replaceInternal(harmonicIndex, false);
	return;
}



void SampleMapToWavetableConverter::HarmonicMap::replaceInternal(int harmonicIndex, bool useRightChannel)
{
	auto& tableToUse = useRightChannel ? harmonicGainsRight : harmonicGains;

	if (harmonicGains.getNumChannels() < 2)
		return;

	int size = tableToUse.getNumSamples();

	if (harmonicIndex == 0)
	{
		auto first = tableToUse.getWritePointer(0);
		auto second = tableToUse.getReadPointer(1);

		FloatVectorOperations::copy(first, second, size);
	}
	else if (harmonicIndex == tableToUse.getNumChannels() - 1)
	{
		auto first = tableToUse.getWritePointer(harmonicIndex);
		auto second = tableToUse.getReadPointer(harmonicIndex - 1);

		FloatVectorOperations::copy(first, second, size);
	}
	else
	{
		auto target = tableToUse.getWritePointer(harmonicIndex);
		auto prev = tableToUse.getReadPointer(harmonicIndex - 1);
		auto next = tableToUse.getReadPointer(harmonicIndex + 1);

		for (int i = 0; i < size; i++)
		{
			const float interpolatedValue = (prev[i] + next[i]) / 2.0f;
			target[i] = interpolatedValue;
		}
	}
}

template <typename T> bool isBetween(T valueToCheck, T lowerLimit, T upperLimit)
{
	return valueToCheck > lowerLimit && valueToCheck < upperLimit;
}

SampleMapToWavetableConverter::SampleMapToWavetableConverter(ModulatorSynthChain* mainSynthChain) :
	chain(mainSynthChain),
	leftValueTree("wavetableData"),
	rightValueTree("wavetableData")
{
	afm.registerBasicFormats();
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
}

juce::Result SampleMapToWavetableConverter::parseSampleMap(const ValueTree& sampleMapTree)
{
    Result result = Result::ok();
    
    sampleMap = sampleMapTree;
    
    harmonicMaps.clear();
    
    for (int i = 0; i < 127; i++)
    {
        auto index = getSampleIndexForNoteNumber(i);
        
        if (index != -1)
        {
            HarmonicMap newMap;
            
            newMap.index.noteNumber = i;
            newMap.index.sampleIndex = index;
            
            harmonicMaps.add(newMap);
        }
    }
    
    currentIndex = 0;
    
    return result;
    
        
}
    
juce::Result SampleMapToWavetableConverter::parseSampleMap(const File& sampleMapFile)
{
	Result result = Result::ok();

	harmonicMaps.clear();

	// Load samplemap
	result = loadSampleMapFromFile(sampleMapFile);


	if (result.failed())
		return result;

	for (int i = 0; i < 127; i++)
	{
		auto index = getSampleIndexForNoteNumber(i);

		if (index != -1)
		{
			HarmonicMap newMap;

			newMap.index.noteNumber = i;
			newMap.index.sampleIndex = index;

			harmonicMaps.add(newMap);
		}
	}

	currentIndex = 0;

	return result;
}

int SampleMapToWavetableConverter::getLowestPossibleFFTSize() const
{
	if (currentIndex < harmonicMaps.size())
	{
		return 2 * nextPowerOfTwo(harmonicMaps.getReference(currentIndex).wavetableLength);
	}

	jassertfalse;
	return 512;
}

juce::Result SampleMapToWavetableConverter::calculateHarmonicMap()
{
	if (currentIndex > harmonicMaps.size() - 1)
	{
		jassertfalse;
		return Result::fail("wrong array index");
	}

	auto& m = harmonicMaps.getReference(currentIndex);

	Result result = Result::ok();

	// Read entire sample & repitch
	AudioSampleBuffer buffer;

	result = readSample(buffer, m.index.sampleIndex, m.index.noteNumber);

	if (result.failed())
		return result;

	m.wavetableLength = ResynthesisHelpers::getWavetableLength(m.index.noteNumber, sampleRate);

	auto parts = splitSample(buffer);

	auto nyquist = sampleRate / 2.0;

	int numHarmonics = roundToInt(nyquist / MidiMessage::getMidiNoteInHertz(m.index.noteNumber));

	m.clear(numHarmonics);

	double lastPitch = -1.0;

	int partIndex = 0;

    auto totalLength = buffer.getNumSamples() - fftSize;
    
    
    
	for (const auto& p : parts)
	{
		AudioSampleBuffer harmonics(2, numHarmonics);

        auto offsetNormalized = (double)partIndex / (double)numParts;
        auto offsetSample = roundToInt(offsetNormalized * (double)totalLength);
        
        
        
		auto estimatedPitch = MidiMessage::getMidiNoteInHertz(m.index.noteNumber);
        
        auto numSamplesRequired = PitchDetection::getNumSamplesNeeded(sampleRate, estimatedPitch * 0.9);
        
        auto numToUse = jmin(numSamplesRequired, buffer.getNumSamples() - offsetSample);
        
        if(numToUse < 0)
            break;
        
        float* d[1] = { buffer.getWritePointer(0, offsetSample) };
        
        AudioSampleBuffer pitchBuffer(d, 1, numToUse);
        
		auto thisPitch = ResynthesisHelpers::getRootFrequencyInBuffer(pitchBuffer, sampleRate, estimatedPitch);
        auto estimatedRatio = thisPitch / estimatedPitch;

		if(isBetween(estimatedRatio, 0.0, 0.33) || estimatedRatio > 3.0)
		{
			if (partIndex > 4)
			{
				thisPitch = lastPitch;
			}
			else
			{
				return Result::fail("Pitch detection error. Expected: " + String(estimatedPitch, 0) + " + Hz" + ", Actual: " + String(thisPitch, 0) + "Hz");
			}
		}
		else if (isBetween(estimatedRatio, 0.33, 0.6))
		{
			thisPitch *= 2.0;
		}
		else if (isBetween(estimatedRatio, 1.5, 3.0))
		{
			thisPitch /= 2.0;
		}

		auto lGain = p.getMagnitude(0, 0, p.getNumSamples());
		auto rGain = p.getMagnitude(1, 0, p.getNumSamples());

		m.gainValues.setSample(0, partIndex, lGain);
		m.gainValues.setSample(1, partIndex, rGain);

		if (lastPitch != -1.0)
		{
			auto pitchDelta = log2(thisPitch / lastPitch) * 1200.0;

			m.pitchDeviations[partIndex] = pitchDelta;

            if (std::abs(pitchDelta) > 100.0)
			{
				thisPitch = lastPitch;

				//return Result::fail("Pitch Deviation too big");
			}
		}

		lastPitch = thisPitch;

		if (!ResynthesisHelpers::calculateHarmonicSpectrum(p, sampleRate, harmonics, thisPitch, windowType))
			return Result::fail("Couldn't detect pitch at notenumber " + String(m.index.noteNumber));

		FloatVectorOperations::copy(m.harmonicGains.getWritePointer(partIndex), harmonics.getReadPointer(0), numHarmonics);
		FloatVectorOperations::copy(m.harmonicGainsRight.getWritePointer(partIndex), harmonics.getReadPointer(1), numHarmonics);

		partIndex++;

	}

	m.analysed = true;

	MessageManagerLock mm;
	sendSynchronousChangeMessage();

	return result;
}


void SampleMapToWavetableConverter::renderAllWavetablesFromSingleWavetables(double& progress)
{
	using Data = ResynthesisHelpers::SimpleNoteConversionData;

	for (int i = 0; i < 128; i++)
	{
		Array<Data> conversionData;

		for (auto s : sampleMap)
		{
			int loKey = s.getProperty(SampleIds::LoKey, -1);
			int hiKey = s.getProperty(SampleIds::HiKey, -1);
			int root = s.getProperty(SampleIds::Root, -1);

			if (i >= loKey && i <= hiKey)
			{
				auto thisFreq = MidiMessage::getMidiNoteInHertz(i);
				auto rootFreq = MidiMessage::getMidiNoteInHertz(root);


				auto fName = s.getProperty(SampleIds::FileName).toString();

				if (fName.isEmpty())
					s.getChild(0).getProperty(SampleIds::FileName).toString();

				PoolReference r(chain->getMainController(), fName, FileHandlerBase::Samples);

				Data data;
				data.sampleFile = r.getFile();
				data.rootRatio = thisFreq / rootFreq;
				data.noteNumber = i;
				data.veloRange.setStart((int)s.getProperty(SampleIds::LoVel, 0));
				data.veloRange.setEnd((int)s.getProperty(SampleIds::HiVel, 127) + 1);

				conversionData.insert(-1, data);
			}
		}

		progress = (double)i / 128.0;

		if (conversionData.isEmpty())
			continue;

		struct DataSorter
		{
			int compareElements(Data& first, Data& second)
			{
				if (first.veloRange.getStart() < second.veloRange.getStart())
					return -1;

				if (first.veloRange.getStart() > second.veloRange.getStart())
					return 1;

				return 0;
			}
		};

		DataSorter sorter;

		conversionData.sort(sorter);


		auto output = ResynthesisHelpers::loadAndResampleAudioFilesForVelocity(conversionData, sampleRate);

		storeData(i, output.getWritePointer(0), leftValueTree, output.getNumSamples(), conversionData.size());

		if (output.getNumChannels() == 2)
		{
			storeData(i, output.getWritePointer(1), rightValueTree, output.getNumSamples(), conversionData.size());
		}
	}
}

void SampleMapToWavetableConverter::renderAllWavetablesFromHarmonicMaps(double& progress)
{
	for (const auto& map : harmonicMaps)
	{
		if (!map.analysed)
			continue;

		progress = (double)harmonicMaps.indexOf(map) / (double)harmonicMaps.size();

		auto bank = calculateWavetableBank(map);

		float* dataL = bank.getWritePointer(0);
		float* dataR = bank.getWritePointer(1);

		storeData(map.index.noteNumber, dataL, leftValueTree, map.wavetableLength * numParts);
		storeData(map.index.noteNumber, dataR, rightValueTree, map.wavetableLength * numParts);
	}
}

AudioSampleBuffer SampleMapToWavetableConverter::calculateWavetableBank(const HarmonicMap &map)
{
	AudioSampleBuffer bank(2, map.wavetableLength * numParts);

	bank.clear();

	float* dataL = bank.getWritePointer(0);
	float* dataR = bank.getWritePointer(1);

	int offset = reverseOrder ? bank.getNumSamples() - map.wavetableLength : 0;

	int partIndex = 0;

	for (int i = 0; i < map.harmonicGains.getNumChannels(); i++)
	{
		ResynthesisHelpers::createWavetableFromHarmonicSpectrum(map.harmonicGains.getReadPointer(partIndex), map.harmonicGains.getNumSamples(), dataL + offset, map.index.noteNumber, sampleRate);

		ResynthesisHelpers::createWavetableFromHarmonicSpectrum(map.harmonicGainsRight.getReadPointer(partIndex), map.harmonicGains.getNumSamples(), dataR + offset, map.index.noteNumber, sampleRate);

		if (useOriginalGain)
		{
			const float gainL = map.gainValues.getSample(0, partIndex);
			const float gainR = map.gainValues.getSample(1, partIndex);

			FloatVectorOperations::multiply(dataL + offset, gainL, map.wavetableLength);
			FloatVectorOperations::multiply(dataR + offset, gainR, map.wavetableLength);
		}

		if (reverseOrder)
			offset -= map.wavetableLength;
		else
			offset += map.wavetableLength;

		partIndex++;
	}

	return bank;
}

juce::Result SampleMapToWavetableConverter::refreshCurrentWavetable(double& progress, bool forceReanalysis /*= true*/)
{
	if (currentIndex == -1)
		return Result::fail("Nothing loaded");

	jassert(currentIndex < harmonicMaps.size());

	if (!forceReanalysis && harmonicMaps.getReference(currentIndex).analysed)
	{
		sendChangeMessage();
		return Result::ok();
	}


	Result r = calculateHarmonicMap();

	int numAnalysed = 0;

	for (const auto& m : harmonicMaps)
	{
		if (m.analysed)
			numAnalysed++;
	}

	progress = (double)numAnalysed / (double)harmonicMaps.size();

	return r;
}

void SampleMapToWavetableConverter::replacePartOfCurrentMap(int index)
{
	if (currentIndex < harmonicMaps.size())
	{
		harmonicMaps.getReference(currentIndex).replaceWithNeighbours(index);
		
	}
}

int SampleMapToWavetableConverter::getCurrentNoteNumber() const
{
	if (currentIndex < harmonicMaps.size())
	{
		return harmonicMaps.getReference(currentIndex).index.noteNumber;
	}

	return -1;
}

juce::Result SampleMapToWavetableConverter::discardCurrentNoteAndUsePrevious()
{
	if (currentIndex > 0)
	{
		const auto& prev = harmonicMaps.getReference(currentIndex - 1);
		auto& thisMap = harmonicMaps.getReference(currentIndex);

		const auto samplesToCopy = jmin<int>(thisMap.harmonicGains.getNumSamples(), prev.harmonicGains.getNumSamples());

		for (int i = 0; i < thisMap.harmonicGains.getNumChannels(); i++)
		{
			thisMap.harmonicGains.copyFrom(i, 0, prev.harmonicGains, i, 0, samplesToCopy);
			thisMap.harmonicGainsRight.copyFrom(i, 0, prev.harmonicGainsRight, i, 0, samplesToCopy);
		}

		FloatVectorOperations::copy(thisMap.pitchDeviations, prev.pitchDeviations, 64);

		sendChangeMessage();

		return Result::ok();
	}

	return Result::fail("No previous wavetable found.");
}

juce::AudioSampleBuffer SampleMapToWavetableConverter::getPreviewBuffers(bool original)
{
	AudioSampleBuffer b;

	if (auto currentMap = getCurrentMap())
	{
		if (original)
		{
			auto actualSampleRate = sampleRate;

			sampleRate = chain->getSampleRate();

			readSample(b, currentMap->index.sampleIndex, currentMap->index.noteNumber);

			sampleRate = actualSampleRate;
		}
		else
		{
			auto bank = calculateWavetableBank(*currentMap);

			int length = currentMap->wavetableLength;

			auto numWavetables = (float)currentSampleLength / (float)length;

			int numWaveTablesPerPart = jmax<int>(1, nextPowerOfTwo(roundToInt(numWavetables / (float)numParts)));

			int newLength = numWaveTablesPerPart * length * (numParts + 1);

			b.setSize(2, newLength);

			b.clear();

			int offsetForFade = numWaveTablesPerPart * length;

			AudioSampleBuffer scratchBuffer(2, offsetForFade * 2);

			int targetOffset = 0;

			for (int i = 0; i < numParts; i++)
			{
				int bOffset = i * length;

				for (int j = 0; j < (2 * numWaveTablesPerPart); j++)
				{
					int tOffset = j * length;

					scratchBuffer.copyFrom(0, tOffset, bank.getReadPointer(0, bOffset), length);
					scratchBuffer.copyFrom(1, tOffset, bank.getReadPointer(1, bOffset), length);
				}

				if(i != 0)
					scratchBuffer.applyGainRamp(0, offsetForFade, 0.0f, 1.0f);

				scratchBuffer.applyGainRamp(offsetForFade, offsetForFade, 1.0f, 0.0f);

				b.addFrom(0, targetOffset, scratchBuffer.getReadPointer(0), scratchBuffer.getNumSamples());
				b.addFrom(1, targetOffset, scratchBuffer.getReadPointer(1), scratchBuffer.getNumSamples());

				targetOffset += offsetForFade;
			}

			if (reverseOrder)
				b.reverse(0, b.getNumSamples());

			auto playbackRate = chain->getSampleRate();

			if (playbackRate != sampleRate)
			{
				double ratio = sampleRate / playbackRate;
				int newNumSamples = roundToInt((double)b.getNumSamples() / ratio);

				LagrangeInterpolator interpolator;
				AudioSampleBuffer resampled(2, newNumSamples);

				interpolator.process(ratio, b.getReadPointer(0), resampled.getWritePointer(0), newNumSamples);
				interpolator.reset();
				interpolator.process(ratio, b.getReadPointer(1), resampled.getWritePointer(1), newNumSamples);

				return resampled;
			}

		}
	}

	return b;
}

void SampleMapToWavetableConverter::storeData(int noteNumber, float* data, ValueTree& treeToSave, int length, int numPartsToUse)
{
	ValueTree child = ValueTree("wavetable");

	if (numPartsToUse == -1)
		numPartsToUse = numParts;

	child.setProperty("noteNumber", noteNumber, nullptr);
	child.setProperty("amount", numPartsToUse, nullptr);
	child.setProperty("sampleRate", sampleRate, nullptr);

	MemoryBlock mb(length * sizeof(float));

	FloatVectorOperations::copy((float*)mb.getData(), data, length);

	var binaryData(mb);

	child.setProperty("data", binaryData, nullptr);

	treeToSave.addChild(child, -1, nullptr);
}

int SampleMapToWavetableConverter::getSampleIndexForNoteNumber(int noteNumber)
{
	for (int i = 0; i < sampleMap.getNumChildren(); i++)
	{
		auto sample = sampleMap.getChild(i);

		auto range = Range<int>(getSampleProperty(sample, SampleIds::LoKey),
			(int)getSampleProperty(sample, SampleIds::HiKey) + 1);

		if (range.contains(noteNumber))
			return i;
	}

	return -1;
}

juce::Result SampleMapToWavetableConverter::readSample(AudioSampleBuffer& buffer, int index, int noteNumber)
{
	auto sample = sampleMap.getChild(index);

	const bool isMonolith = (int)sampleMap.getProperty("SaveMode") == SampleMap::SaveMode::Monolith;

	int monoOffset = 0;

	String filePath;

	if (isMonolith)
	{
		String sampleName;

		sampleName << "{PROJECT_FOLDER}" << sampleMap.getProperty("ID").toString() << ".ch1";

		sampleName = sampleName.replace("/", "_");

		filePath = GET_PROJECT_HANDLER(chain).getFilePath(sampleName, ProjectHandler::SubDirectories::Samples);

		monoOffset = (int)sample.getProperty("MonolithOffset", 0);
	}
	else
	{
		auto fileName = getSampleProperty(sample, SampleIds::FileName);
		filePath = GET_PROJECT_HANDLER(chain).getFilePath(fileName, ProjectHandler::SubDirectories::Samples);
	}



	jassert(File::isAbsolutePath(filePath));
	File f = File(filePath);


	auto range = Range<int>((int)getSampleProperty(sample, SampleIds::SampleStart) + monoOffset,
		(int)getSampleProperty(sample, SampleIds::SampleEnd) + monoOffset);

	if (range.isEmpty() && isMonolith)
	{
		auto mOffset = (int)getSampleProperty(sample, "MonolithOffset");
		auto mLength = (int)getSampleProperty(sample, "MonolithLength");

		range = { mOffset, mOffset + mLength };
	}
		


	auto rootNote = (int)getSampleProperty(sample, SampleIds::Root);





	if (f.existsAsFile())
	{


		ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(f);



		if (reader != nullptr)
		{
			if (range.isEmpty())
				range = { 0, (int)reader->lengthInSamples };

			if (reader->sampleRate == sampleRate && rootNote == noteNumber)
			{
				buffer.setSize(2, range.getLength());

				reader->read(&buffer, 0, range.getLength(), range.getStart(), true, true);
				return Result::ok();
			}

			currentSampleLength = range.getLength();

			AudioSampleBuffer unresampled(2, range.getLength());
			reader->read(&unresampled, 0, range.getLength(), range.getStart(), true, true);

			const auto pitchFactorSampleRate = sampleRate / reader->sampleRate;


			auto pitchFactorRoot = StreamingSamplerSound::getPitchFactor(noteNumber, rootNote);
			const auto pf = pitchFactorRoot * pitchFactorSampleRate;
			const int numSamplesResampled = roundToInt((double)unresampled.getNumSamples() / pf);

			LagrangeInterpolator interpolator;
			buffer.setSize(2, numSamplesResampled);

			interpolator.process(pf, unresampled.getReadPointer(0), buffer.getWritePointer(0), numSamplesResampled);
			interpolator.reset();
			interpolator.process(pf, unresampled.getReadPointer(1), buffer.getWritePointer(1), numSamplesResampled);

			return Result::ok();
		}
		else
		{
			return Result::fail("Error opening file " + filePath);
		}
	}
	else
	{
		return Result::fail("File couldn't be found.");
	}
}

Array<juce::AudioSampleBuffer> SampleMapToWavetableConverter::splitSample(const AudioSampleBuffer& buffer)
{
	Array<AudioSampleBuffer> parts;
	parts.ensureStorageAllocated(numParts);

	int thisFFTSize = fftSize > 0 ? fftSize : getLowestPossibleFFTSize();

	int oversampledLength = thisFFTSize * 1;
	
	

	auto totalLength = buffer.getNumSamples() - thisFFTSize;

	const float* lData = buffer.getReadPointer(0);
	const float* rData = buffer.getReadPointer(1);

	if (totalLength <= 0)
	{
		// Trying to split a buffer that's smaller than the split size...
		jassertfalse;
		return parts;
	}

	for (int i = 0; i < numParts; i++)
	{
		AudioSampleBuffer p;

		auto offsetNormalized = (double)i / (double)numParts;
		auto offsetSample = roundToInt(offsetNormalized * (double)totalLength);

		p.setSize(2, oversampledLength);
		p.clear();

		FloatVectorOperations::copy(p.getWritePointer(0), lData + offsetSample, thisFFTSize);
		FloatVectorOperations::copy(p.getWritePointer(1), rData + offsetSample, thisFFTSize);

		parts.add(p);
	}

	return parts;
}

juce::Result SampleMapToWavetableConverter::loadSampleMapFromFile(File sampleMapFile)
{
	if (auto  xml = XmlDocument::parse(sampleMapFile))
	{
		sampleMap = ValueTree::fromXml(*xml);
		return Result::ok();
	}

	jassertfalse;
	return Result::fail("Error parsing Samplemap XML");
}


#endif

}
