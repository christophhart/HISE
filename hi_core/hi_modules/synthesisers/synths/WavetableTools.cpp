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





struct ResynthesisHelpers
{
    static constexpr int MinTableLength = 128;
    
	struct SimpleNoteConversionData
	{
		File sampleFile;
		double rootRatio;
		int noteNumber;
        Range<int> noteRange;
		Range<int> veloRange;
	};

	static void writeRootAndPitch(ValueTree& s, double sampleRate, int wavetableLength)
	{
		int rootNote = -1;
		int detune = 0;

		auto freq = sampleRate / (double)wavetableLength;

		for (int i = 0; i < 128; i++)
		{
			auto ratio = freq / MidiMessage::getMidiNoteInHertz(i);

			detune = conversion_logic::pitch2cent().getValue(ratio);

			if (hmath::abs(detune) < 50)
			{
				rootNote = i;
				break;
			}
		}

		if (rootNote != -1)
		{
			s.setProperty(SampleIds::Root, rootNote, nullptr);
			s.setProperty(SampleIds::Pitch, -1 * detune, nullptr);
		}
	}

	static int getWavetableLength(int noteNumber, double sampleRate, bool findNextPowerOfTwo=true)
	{
		const double freq = MidiMessage::getMidiNoteInHertz(noteNumber);
		int sampleNumber = (int)(sampleRate / freq);

		if (findNextPowerOfTwo)
		{
			auto power = hmath::log((double)sampleNumber) / hmath::log(2.0);
			power = hmath::range(hmath::ceil(power), 7.0, 11.0);
			sampleNumber = roundToInt(hmath::pow(2.0, power));
		}

		return sampleNumber;
	};
    
	static int tryToGuessCycleLength(const AudioSampleBuffer& b)
	{
		auto numTotal = b.getNumSamples();
		span<int, 5> lengthValues = { 128, 256, 512, 1024, 2048 };
		span<double, 5> sumValues;

		for (int lenghtIndex = 0; lenghtIndex < 5; lenghtIndex++)
		{
			auto thisLength = lengthValues[lenghtIndex];
			double numLoop = (double)numTotal / (double)thisLength;

			if (hmath::fmod(numLoop, 1.0) != 0.0)
			{
				// If it's not dividable by the length, then it's certainly not the cycle length
				sumValues[lenghtIndex] = 100000.0;
				continue;
			}

			for (int i = 0; i < thisLength; i++)
			{
				auto thisSum = 0.0;
				auto thisPtr = b.getReadPointer(0, i);
				float sig = 1.0f;

				for (int j = 0; j < numLoop; j++)
				{
					thisSum += thisPtr[j * thisLength] * sig;
					sig *= -1.0f;
				}

				sumValues[lenghtIndex] += thisSum;
			}

			sumValues[lenghtIndex] = hmath::abs(sumValues[lenghtIndex]) / (double)thisLength;
		}

		auto rv = 0;

		for (int i = 0; i < 5; i++)
		{
			if (sumValues[i] < 0.1)
			{
				rv = lengthValues[i];
				break;
			}
		}

		return rv;
	}

	static void removeHarmonicsAboveNyquist(const float* source, float* target, int numThisTime, int targetNoteNumber, double sampleRate)
	{
		juce::dsp::FFT fft(log2(numThisTime));

		HeapBlock<juce::dsp::Complex<float>> input, output;

		input.calloc(numThisTime);
		output.calloc(numThisTime);

		auto realLength = ResynthesisHelpers::getWavetableLength(targetNoteNumber, sampleRate, false);

		auto realRatio = (double)numThisTime / (double)realLength;

		for (int i = 0; i < numThisTime; i++)
			input[i] = { source[i], 0.0f };

		fft.perform(input, output, false);

		int maxBand = roundToInt(numThisTime / realRatio) / 2;

		for (int i = maxBand; i < numThisTime - maxBand; i++)
			output[i] = { 0.0f, 0.0f };

		fft.perform(output, input, true);

		for (int i = 0; i < numThisTime; i++)
		{
			target[i] = input[i].real();
		}
	}

	static void createWavetableFromHarmonicSpectrum(ThreadController* threadController, const float* hmx, int numHarmonics, float* data, int noteNumber, double sampleRate = 48000.0, const float* phaseData = nullptr)
	{
		auto length = getWavetableLength(noteNumber, sampleRate);

		FloatVectorOperations::clear(data, length);


		auto rootFreq = MidiMessage::getMidiNoteInHertz(noteNumber);

		double rootDelta = 2.0 * double_Pi / (double)length;

		auto offsets = (double*)alloca(sizeof(double)*numHarmonics);

		FloatVectorOperations::clear(offsets, numHarmonics);

		if (phaseData != nullptr)
		{
			for (int i = 0; i < numHarmonics; i++)
			{
				offsets[i] = (double)phaseData[0] * (double)(i + 1) - (double)i * double_Pi * 0.5;
			}

			for (int i = 0; i < numHarmonics; i++)
			{
				auto p = (double)phaseData[i];

				auto& thisOffset = offsets[i];

				thisOffset -= p;

				thisOffset = hmath::fmod(offsets[i] + 1000.0 * double_Pi, double_Pi * 2.0);

				if (thisOffset > double_Pi)
					thisOffset -= 2.0 * double_Pi;

				if (hmath::abs(thisOffset) < 0.000001 ||
					hmx[i] == 0.0f)
				{
					thisOffset = 0.0;
				}
				
				//thisOffset = LIVE_EXPRESSION3(double, "thisOffset", thisOffset, i, numHarmonics);
			}
		}

		for (int h = 0; h < numHarmonics; h++)
		{
			if (auto s = ThreadController::ScopedStepScaler(threadController, h, numHarmonics))
			{
				float harmonicGain = hmx[h];

				// make a smooth rollof at nyquist
				auto nyquistGain = 1.0f - hmath::smoothstep(rootFreq * (double)(h + 1), 18000.0, 22000.0);

				harmonicGain *= nyquistGain;

				// the rest of the harmonics will also be above nyquist...
				if (nyquistGain == 0.0f)
					break;

				if (harmonicGain <= 0.0001f)
					continue;

				double uptime = 0.0;

				if (phaseData != nullptr)
				{
					uptime = offsets[h];
				}


				for (int i = 0; i < length; i++)
				{
					data[i] += harmonicGain * (float)sin(uptime);
					uptime += rootDelta * (double)(h + 1);
				}
			}
		}

		auto range = FloatVectorOperations::findMinAndMax(data, length);

		auto maxLevel = jmax<float>(fabsf(range.getStart()), fabsf(range.getEnd()));

        if(maxLevel != 0.0)
        {
            FloatVectorOperations::multiply(data, 1.0f / maxLevel, length);
        }
	}

};

void SampleMapToWavetableConverter::HarmonicMap::clear(int numSlices, int numHarmonics)
{
	pitchDeviations.calloc(numSlices);
	harmonicGains.setSize(numSlices, numHarmonics);
	harmonicGains.clear();
	harmonicGainsRight.setSize(numSlices, numHarmonics);
	harmonicGainsRight.clear();

	gainValues.clear();
	gainValues.setSize(2, numSlices);
}


void SampleMapToWavetableConverter::checkIfShouldExit()
{
	if (!*threadController)
		throw Result::fail("Cancelled");
}

template <typename T> bool isBetween(T valueToCheck, T lowerLimit, T upperLimit)
{
	return valueToCheck > lowerLimit && valueToCheck < upperLimit;
}

SampleMapToWavetableConverter::SampleMapToWavetableConverter(ModulatorSynthChain* mainSynthChain) :
	chain(mainSynthChain),
	waveTableTree("wavetableData")
{
	afm.registerBasicFormats();
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);

	s2dParameters = new Spectrum2D::Parameters();
	s2dParameters->Spectrum2DSize = 8192;
	s2dParameters->currentWindowType = FFTHelpers::WindowType::FlatTop;
	s2dParameters->minDb = 90;
	s2dParameters->order = 13;
	s2dParameters->oversamplingFactor = 4;
}


void SampleMapToWavetableConverter::rebuildPreviewBuffersInternal()
{
	logFunction("Rebuild preview buffers");

	originalSpectrum = {};

	if (auto currentMap = getCurrentMap())
	{
		if (auto ps = ThreadController::ScopedRangeScaler(threadController.get(), 0.0, 0.3))
		{
			ScopedValueSetter<double> svs(sampleRate, chain->getSampleRate());
			readSample(originalBuffer, currentMap->index.sampleIndex, currentMap->rootNote);
		}
		
		if(auto ps = ThreadController::ScopedRangeScaler(threadController.get(), 0.3, 1.0))
		{
			ValueTree previewTree("preview");

			if (phaseMode == PhaseMode::Resample)
			{
				{
					ScopedValueSetter<double> svs(sampleRate, chain->getSampleRate());
					readSample(previewBuffer, currentMap->index.sampleIndex, currentMap->rootNote);
					previewBuffer.clear();
				}

				ScopedValueSetter<ValueTree> svs2(waveTableTree, previewTree);
				renderAllWavetablesFromSingleWavetables(currentMap->index.sampleIndex);
				checkIfShouldExit();
			}
			else
			{
				if (!currentMap->analysed)
					calculateHarmonicMap();

				checkIfShouldExit();

				int numSamplesToCalculate = chain->getSampleRate() * currentMap->sampleLengthSeconds;
				previewBuffer.setSize(2, numSamplesToCalculate);
				previewBuffer.clear();

				StoreData sd;
				sd.sample = currentMap->index;
				sd.numChannels = currentMap->isStereo ? 2 : 1;
				sd.parent = previewTree;
				sd.sampleRate = 48000.0;

				if (phaseMode == PhaseMode::DynamicPhase)
				{
					auto& map = *currentMap;
					auto targetLength = jmin(ResynthesisHelpers::getWavetableLength(map.rootNote, map.fileSampleRate), map.wavetableLength);
					auto notePitch = (double)targetLength / (double)map.wavetableLength;
					notePitch *= map.lorisResynRatio;
					sd.dataBuffer = getResampledLorisBuffer(map.lorisResynBuffer, notePitch, targetLength, map.rootNote);

					checkIfShouldExit();

					sd.numParts = sd.dataBuffer.getNumSamples() / targetLength;
				}
				else
				{
					sd.dataBuffer = calculateWavetableBank(*currentMap);
					sd.numParts = numParts;

					jassert(currentMap->wavetableLength * numParts == sd.dataBuffer.getNumSamples());
				}

				storeData(sd);
			}

			checkIfShouldExit();

			auto ws = new WavetableSound(previewTree.getChild(0), chain);
			ws->calculatePitchRatio(chain->getSampleRate());

			sound = ws;

			ws->calculatePitchRatio(chain->getSampleRate());

			double uptimeDelta = ws->getPitchRatio(currentMap->rootNote);
			double voiceUptime = 0.0;

			if (phaseMode == PhaseMode::Resample)
			{
				auto detune = (int)sampleMap.getChild(currentIndex)[SampleIds::Pitch];
				uptimeDelta *= conversion_logic::cent2pitch().getValue(-1.0 * detune);
			}

			WavetableSound::RenderData r(previewBuffer, 0, previewBuffer.getNumSamples(), uptimeDelta, nullptr, true);

			r.render(ws, voiceUptime, [&](int startSample) { return jlimit(0.0f, 1.0f, (float)startSample / (float)previewBuffer.getNumSamples()); });

			if (!currentMap->isStereo)
				FloatVectorOperations::copy(previewBuffer.getWritePointer(1), previewBuffer.getReadPointer(0), previewBuffer.getNumSamples());

			checkIfShouldExit();

			applyNoiseBuffer(*currentMap, previewBuffer);

			getPreviewBuffers(false);
		}
	}
}

juce::AudioSampleBuffer SampleMapToWavetableConverter::removeHarmonicsAboveNyquistWithLoris(double ratio)
{
#if HISE_INCLUDE_LORIS
	auto& map = *getCurrentMap();
	jassert(ratio > 2.0);

	auto fileToLoad = PoolReference(chain->getMainController(), sampleMap.getChild(map.index.sampleIndex)[SampleIds::FileName].toString(), FileHandlerBase::Samples).getFile();

	auto lorisManager = dynamic_cast<BackendProcessor*>(chain->getMainController())->getLorisManager();

	LorisManager::AnalyseData ad;
	ad.file = fileToLoad;
	ad.rootFrequency = MidiMessage::getMidiNoteInHertz(map.rootNote);;

	lorisManager->analyse({ ad });


	lorisManager->processCustom(fileToLoad, [&](LorisManager::CustomPOD& d)
	{
		if (!*threadController)
			return true;

		int harmonicIndex = roundToInt(d.frequency / d.rootFrequency);
		d.frequency = (double)harmonicIndex * d.rootFrequency;

		auto nyquistGain = 1.0f - hmath::smoothstep(d.frequency * ratio, 18000.0, 22000.0);

		d.gain *= nyquistGain;

		return false;
	});


	auto sb = lorisManager->synthesise(fileToLoad);

	AudioSampleBuffer b(2, sb[0].getBuffer()->size);

	FloatVectorOperations::copy(b.getWritePointer(0), sb[0].getBuffer()->buffer.getReadPointer(0), b.getNumSamples());

	FloatVectorOperations::copy(b.getWritePointer(1), sb[map.isStereo ? 1 : 0].getBuffer()->buffer.getReadPointer(0), b.getNumSamples());

	return b;
#else
	return {};
#endif
}

float* SampleMapToWavetableConverter::getPhaseData(const HarmonicMap& map, int sliceIndex, bool getRight)
{
	if (phaseMode == PhaseMode::ZeroPhase)
		return nullptr;

	auto phaseMapIndex = jmin(harmonicMaps.size()-1, harmonicMaps.size() / 2);

	if (phaseMode == PhaseMode::DynamicPhase)
		phaseMapIndex = harmonicMaps.indexOf(&map);

	auto sliceOffset = jmin(numParts - 1, numParts / 4);

	if (phaseMode == PhaseMode::DynamicPhase)
		sliceOffset = sliceIndex;


	auto mapToUse = harmonicMaps[phaseMapIndex];

	if (!mapToUse->analysed)
	{
		mapToUse = getCurrentMap();
	}

	auto& arrayToUse = getRight ? mapToUse->harmonicPhaseRight : mapToUse->harmonicPhase;

	if (isPositiveAndBelow(sliceOffset, arrayToUse.getNumChannels()))
		return arrayToUse.getWritePointer(sliceOffset);
	else
		return nullptr;

}

void SampleMapToWavetableConverter::parseSampleMap(const ValueTree& sampleMapTree)
{
	currentIndex = -1;

    sampleMap = sampleMapTree;
    
    harmonicMaps.clear();
    
	for (auto c : sampleMap)
	{
		auto newMap = new HarmonicMap(numParts);

		auto root = (int)c[SampleIds::Root];

		newMap->rootNote = root;
		newMap->index.noteNumber = root;
		newMap->index.sampleIndex = sampleMap.indexOf(c);
		newMap->noteRange = { (int)c[SampleIds::LoKey], (int)c[SampleIds::HiKey] };

		harmonicMaps.add(newMap);
	}
    
	setCurrentIndex(0, sendNotificationSync);
}

hise::Spectrum2D::Parameters::Ptr SampleMapToWavetableConverter::getParameters() const
{

	return s2dParameters;
}

void SampleMapToWavetableConverter::rebuild()
{
    if(sampleMap.isValid())
    {
        parseSampleMap(sampleMap);
        calculateHarmonicMap();
    }
}

void SampleMapToWavetableConverter::exportAll()
{
	discardAllScans();

	if (exportAsHwt)
		waveTableTree = ValueTree("wavetableData");
	else
	{
		jassert(phaseMode != PhaseMode::Resample);
		waveTableTree = ValueTree("samplemap");
		waveTableTree.setProperty(SampleIds::ID, sampleMap[SampleIds::ID].toString() + getPrefixFromNoiseMode(-1), nullptr);
	}

	if (phaseMode == PhaseMode::Resample)
		renderAllWavetablesFromSingleWavetables();
	else
		renderAllWavetablesFromHarmonicMaps();
}

juce::AudioSampleBuffer SampleMapToWavetableConverter::getResampledLorisBuffer(AudioSampleBuffer sourceBuffer, double r, int thisCycleLength, int realNoteNumber)
{
	auto thisRatio = conversion_logic::st2pitch().getValue(realNoteNumber) / conversion_logic::st2pitch().getValue(getCurrentMap()->rootNote);

	if ( thisRatio > 1.8)
	{
		sourceBuffer = removeHarmonicsAboveNyquistWithLoris(thisRatio);
		checkIfShouldExit();
	}

	auto numSamples = (double)sourceBuffer.getNumSamples() * r;
	numSamples -= hmath::fmod(numSamples, (double)thisCycleLength);

	AudioSampleBuffer b(2, roundToInt(numSamples));

	auto numParts = numSamples / (double)thisCycleLength;
    ignoreUnused(numParts);
    
	b.clear();

	checkIfShouldExit();

	juce::LagrangeInterpolator ip;
	ip.process(1.0 / r, sourceBuffer.getReadPointer(0), b.getWritePointer(0), b.getNumSamples());

	checkIfShouldExit();

	ip.reset();
	ip.process(1.0 / r, sourceBuffer.getReadPointer(1), b.getWritePointer(1), b.getNumSamples());

	checkIfShouldExit();

	jassert(hmath::fmod((double)numParts, 1.0) == 0.0);
	return b;
}

void SampleMapToWavetableConverter::calculateHarmonicMap()
{
#if HISE_INCLUDE_LORIS

	if (!isPositiveAndBelow(currentIndex, harmonicMaps.size()))
		throw Result::fail("wrong array index");

	auto& m = *getCurrentMap();

	Result result = Result::ok();

	m.wavetableLength = ResynthesisHelpers::getWavetableLength(m.rootNote, sampleRate);

	auto lorisManager = dynamic_cast<BackendProcessor*>(chain->getMainController())->getLorisManager();

	if (lorisManager == nullptr)
		throw Result::fail("Loris is not installed");

	LorisManager::AnalyseData d;

	auto ref = sampleMap.getChild(m.index.sampleIndex).getProperty(SampleIds::FileName).toString();

	PoolReference pref(chain->getMainController(), ref, FileHandlerBase::Samples);

	d.file = pref.getFile();
	d.rootFrequency = MidiMessage::getMidiNoteInHertz(m.rootNote);

	AudioFormatManager afm;
	afm.registerBasicFormats();
	
	AudioSampleBuffer sampleContent;

	int offsetInFile = 0;
	int totalLength = 0;
    double delta = 1.0 / (double)numParts;
    double pos = 0.0;
    double startPos = 0.0;
    
	logFunction("Analyse file " + d.file.getFileName());

	if (ScopedPointer<AudioFormatReader> r = afm.createReaderFor(d.file))
	{
		m.isStereo = r->numChannels > 1;

		totalLength = (int)r->lengthInSamples;

        auto end = (int)sampleMap.getChild(m.index.sampleIndex).getProperty(SampleIds::SampleEnd);
        auto start = (int)sampleMap.getChild(m.index.sampleIndex).getProperty(SampleIds::SampleStart);
        
        if(end != 0)
            totalLength = end;
        
        if(start != 0)
            totalLength -= start;
        
		offsetInFile = start;

        m.sampleLengthSeconds = totalLength / r->sampleRate;
		sampleContent.setSize(r->numChannels, totalLength);

		m.fileSampleRate = r->sampleRate;

		checkIfShouldExit();
		
		r->read(&sampleContent, 0, totalLength, start, true, true);
        
		checkIfShouldExit();

        delta = (double)totalLength / (double)r->lengthInSamples / (double)(numParts);
        pos = start / (double)r->lengthInSamples + delta;
        startPos = 0.0;

		m.noiseBuffer.setSize(r->numChannels, totalLength);
		r->read(&m.noiseBuffer, 0, totalLength, start, true, true);
	}
	else
	{
		throw Result::fail("Can't find file " + d.file.getFullPathName());
	}

	ScopedValueSetter<std::function<void(String)>> svs(lorisManager->errorFunction, [&](const String& e)
	{
		throw Result::fail(e);
	});

	auto hoptime = m.sampleLengthSeconds / (double)numParts;

	hoptime = jmax(hoptime, 1.0 / MidiMessage::getMidiNoteInHertz(m.rootNote));
	hoptime = jmin(0.05, hoptime);
    
	auto ratio = jlimit(0.25, 4.0, std::pow(2.0, offsetInSlice * 4.0 - 2.0));

	lorisManager->set("timedomain", "0to1");
	lorisManager->set("hoptime", String(hoptime));
	lorisManager->set("croptime", String(hoptime));
	lorisManager->set("windowwidth", String(ratio));
	
	if (auto s = ThreadController::ScopedRangeScaler(threadController.get(), 0.0, 0.35))
	{
		lorisManager->threadController = threadController;

		if(auto r1 = ThreadController::ScopedRangeScaler(threadController.get(), 0.0, 0.8))
		{
			lorisManager->analyse({ d });
		}
		
		checkIfShouldExit();

		if(auto r2 = ThreadController::ScopedRangeScaler(threadController.get(), 0.8, 1.0))
		{
			var synthesised = lorisManager->synthesise(d.file);

			for (int i = 0; i < synthesised.size(); i++)
			{
				auto s = synthesised[i].getBuffer()->buffer.getReadPointer(0);
				auto n = m.noiseBuffer.getWritePointer(i);

				FloatVectorOperations::subtract(n, s, m.noiseBuffer.getNumSamples());
			}
		}

		checkIfShouldExit();
	}

	if (phaseMode == PhaseMode::DynamicPhase)
	{
		if (auto s = ThreadController::ScopedRangeScaler(threadController.get(), 0.4, 1.0))
		{
			m.wavetableLength = ResynthesisHelpers::getWavetableLength(m.rootNote, m.fileSampleRate, true);

			auto pow2Length = m.wavetableLength;
			auto realLength = m.fileSampleRate / MidiMessage::getMidiNoteInHertz(m.rootNote);

			auto numFakeSlices = roundToInt(m.noiseBuffer.getNumSamples() / realLength);
			int numFakeHarmonics = realLength * 0.5;

			auto ratio = (double)pow2Length / (double)realLength;

			lorisManager->processCustom(pref.getFile(), [&](LorisManager::CustomPOD& d)
			{
				if (!*threadController)
					return true;

				int harmonicIndex = roundToInt(d.frequency / d.rootFrequency);

				d.frequency = (double)harmonicIndex * d.rootFrequency;

				return false;
			});

			checkIfShouldExit();

			m.harmonicGains.setSize(numFakeSlices, numFakeHarmonics);

			if (m.isStereo)
				m.harmonicGainsRight.setSize(numFakeSlices, numFakeHarmonics);

			m.harmonicGains.clear();
			m.harmonicGainsRight.clear();

			for (int i = 0; i < numFakeSlices; i++)
			{
				if (auto s = ThreadController::ScopedStepScaler(threadController.get(), i, numFakeSlices))
				{
					auto t = (double)i / (double)(numFakeSlices);

					auto harmonics = lorisManager->getSnapshot(pref.getFile(), t, "gain");

					auto numHarmonics = jmin(harmonics[0].size(), numFakeHarmonics);

					for (int j = 0; j < numHarmonics; j++)
					{
						m.harmonicGains.setSample(i, j, harmonics[0][j]);

						if (m.isStereo)
							m.harmonicGainsRight.setSample(i, j, harmonics[1][j]);
					}
				}
			}

			checkIfShouldExit();

			auto gain = m.harmonicGains.getMagnitude(0, m.harmonicGains.getNumSamples());

			if (m.isStereo)
				gain = jmax(gain, m.harmonicGainsRight.getMagnitude(0, m.harmonicGainsRight.getNumSamples()));

			if (gain != 0.0f)
			{
				m.harmonicGains.applyGain(1.0f / gain);

				if (m.isStereo)
					m.harmonicGainsRight.applyGain(1.0f / gain);
			}

			auto bf = lorisManager->synthesise(pref.getFile());

			checkIfShouldExit();

			auto& original = bf[0].getBuffer()->buffer;

			auto& originalR = m.isStereo ? bf[1].getBuffer()->buffer : original;

			jassert(offsetInFile + totalLength <= original.getNumSamples());

			auto numToUse = m.noiseBuffer.getNumSamples();

			m.lorisResynRatio = ratio;
			m.lorisResynBuffer.setSize(2, numToUse);
			m.lorisResynBuffer.clear();

			FloatVectorOperations::copy(m.lorisResynBuffer.getWritePointer(0), original.getReadPointer(0, offsetInFile), numToUse);
			FloatVectorOperations::copy(m.lorisResynBuffer.getWritePointer(1), originalR.getReadPointer(0, offsetInFile), numToUse);

			m.analysed = true;
			m.gainValues.clear();

			sendChangeMessage();
		}
	}
	else
	{
		if(auto ps = ThreadController::ScopedRangeScaler(threadController.get(), 0.36, 0.95))
			lorisManager->getSnapshot(d.file, 0.5, "gain");

		if (auto ps = ThreadController::ScopedRangeScaler(threadController.get(), 0.95, 1.0))
		{
			auto nyquist = sampleRate / 2.0;

			int numHarmonics = roundToInt(nyquist / MidiMessage::getMidiNoteInHertz(m.rootNote));

			m.gainValues.setSize(m.isStereo ? 2 : 1, numParts);
			m.gainValues.clear();

			for (int i = 0; i < numParts; i++)
			{
				if (auto rs = ThreadController::ScopedStepScaler(threadController.get(), i, numParts))
				{
					var ar;
					{
						ThreadController::ScopedRangeScaler s1(threadController.get(), 0.0, 0.45);
						ar = lorisManager->getSnapshot(d.file, pos + delta * offsetInSlice, "gain");
					}

					checkIfShouldExit();

					var pr;

					if (phaseMode != SampleMapToWavetableConverter::PhaseMode::ZeroPhase)
					{
						ThreadController::ScopedRangeScaler s2(threadController.get(), 0.5, 1.0);
						pr = lorisManager->getSnapshot(d.file, pos + delta * offsetInSlice, "phase");
					}

					checkIfShouldExit();

					if (ar.isArray())
					{
						for (int c = 0; c < ar.size(); c++)
						{
							checkIfShouldExit();

							auto channelData = ar[c];

							if (channelData.isArray())
							{
								auto numHarmonicsThisTime = jmin(numHarmonics, channelData.size());

								auto& bufferToUse = c == 0 ? m.harmonicGains : m.harmonicGainsRight;
								auto& phaseToUse = c == 0 ? m.harmonicPhase : m.harmonicPhaseRight;

								bufferToUse.setSize(numParts, numHarmonicsThisTime, true, true, true);

								FloatVectorOperations::clear(bufferToUse.getWritePointer(i), bufferToUse.getNumSamples());

								for (int s = 0; s < numHarmonicsThisTime; s++)
									bufferToUse.setSample(i, s, (float)channelData[s]);

								if (phaseMode != SampleMapToWavetableConverter::PhaseMode::ZeroPhase)
								{
									auto phaseData = pr[c];

									if (phaseData.isArray())
									{
										phaseToUse.setSize(numParts, numHarmonicsThisTime, true, true, true);
										FloatVectorOperations::clear(phaseToUse.getWritePointer(i), phaseToUse.getNumSamples());

										for (int s = 0; s < numHarmonicsThisTime; s++)
											phaseToUse.setSample(i, s, (float)phaseData[s]);
									}
								}

								auto offset = roundToInt((pos - startPos) * totalLength);
								auto numToCheck = jmin(totalLength - offset, roundToInt(delta * totalLength));

								auto max = jlimit(0.0f, 1.0f, sampleContent.getMagnitude(c, offset, numToCheck));

								if (max != 0.0)
								{
									FloatVectorOperations::multiply(bufferToUse.getWritePointer(i), 1.0f / max, numHarmonicsThisTime);
								}

								m.gainValues.setSample(c, i, max);
							}
						}
					}

					pos += delta;
				}
			}

			m.analysed = true;

			sendChangeMessage();
		}
	}
#else

	throw Result::fail("You must enable HISE_INCLUDE_LORIS when building HISE in order to use this function");

#endif
}

void SampleMapToWavetableConverter::setPreviewMode(PreviewNoise mode)
{

	previewNoise = mode;
	rebuildPreviewBuffersInternal();
}

void SampleMapToWavetableConverter::renderAllWavetablesFromSingleWavetables(int sampleIndex)
{
	waveTableTree.removeAllChildren(nullptr);

	double fileSampleRate = 0.0;

	auto detectCycles = cycleLength == 0;

	for (auto s : sampleMap)
	{
		if (sampleIndex != -1 && sampleMap.indexOf(s) != sampleIndex)
			continue;

		if (auto sms = ThreadController::ScopedStepScaler(threadController.get(), sampleMap.indexOf(s), sampleMap.getNumChildren()))
		{
			Array<AudioSampleBuffer> cycles;
			cycles.ensureStorageAllocated(sampleMap.getNumChildren());

			auto loKey = (int)s[SampleIds::LoKey];
			auto hiKey = (int)s[SampleIds::HiKey];

			double unused;
			PoolReference ref(chain->getMainController(), s[SampleIds::FileName].toString(), FileHandlerBase::Samples);

			logFunction("Create wavetable for " + ref.getFile().getFileName());

			auto fileContent = hlac::CompressionHelpers::loadFile(ref.getFile(), unused, &fileSampleRate);

			auto isStereo = fileContent.getNumChannels() == 2;

			auto thisCycleLength = cycleLength;

			if (detectCycles)
			{
				thisCycleLength = ResynthesisHelpers::tryToGuessCycleLength(fileContent);

				if (thisCycleLength != 0)
					logFunction("Detected cycle length " + String(thisCycleLength));
			}
				

			auto lengthFromRootNote = ResynthesisHelpers::getWavetableLength(s[SampleIds::Root], fileSampleRate);

			if (detectCycles)
				thisCycleLength = lengthFromRootNote;

			if (thisCycleLength == 0)
			{
				throw Result::fail("Can't detect cycle length");
			}

			auto numCycles = (float)fileContent.getNumSamples() / (float)thisCycleLength;

			if (hmath::fmod(numCycles, 1.0f) != 0.0f)
				throw Result::fail("ERROR: Sample length is not multiple of cycle size. You probably need to use a resynthesis mode instead.");

			{
				cycles.ensureStorageAllocated((int)numCycles);

				for (int i = 0; i < (int)numCycles; i++)
				{
					AudioSampleBuffer b(fileContent.getNumChannels(), thisCycleLength);

					FloatVectorOperations::copy(b.getWritePointer(0), fileContent.getReadPointer(0, i * thisCycleLength), thisCycleLength);

					if(isStereo)
						FloatVectorOperations::copy(b.getWritePointer(1), fileContent.getReadPointer(1, i * thisCycleLength), thisCycleLength);

					cycles.add(std::move(b));
				}
			}


			auto doit = [&](int i, Range<int> r)
			{
				if (auto s = ThreadController::ScopedStepScaler(threadController.get(), i - loKey, jmax(1, hiKey - loKey)))
				{
					auto length = ResynthesisHelpers::getWavetableLength(i, fileSampleRate);

					AudioSampleBuffer resampled(fileContent.getNumChannels(), length * cycles.size());

					int offset = 0;

					int cycleIndex = 0;

					for (auto& cycle : cycles)
					{
						if (auto s2 = ThreadController::ScopedStepScaler(threadController.get(), cycleIndex++, cycles.size()))
						{
							int numThisTime = cycle.getNumSamples();

							auto ratio = (double)numThisTime / (double)length;

							if (ratio != 1.0)
							{
								AudioSampleBuffer source(cycle.getNumChannels(), numThisTime * 3);

								juce::Interpolators::Lagrange ip;
								auto latency = roundToInt(ip.getBaseLatency() / ratio);

								AudioSampleBuffer target(cycle.getNumChannels(), length * 3 + latency);
								target.clear();

								if (ratio > 1.5)
								{
									ResynthesisHelpers::removeHarmonicsAboveNyquist(cycle.getReadPointer(0), source.getWritePointer(0), numThisTime, i, fileSampleRate);

									if(isStereo)
										ResynthesisHelpers::removeHarmonicsAboveNyquist(cycle.getReadPointer(1), source.getWritePointer(1), numThisTime, i, fileSampleRate);
								}
								else
								{
									FloatVectorOperations::copy(source.getWritePointer(0), cycle.getReadPointer(0), numThisTime);

									if(isStereo)
										FloatVectorOperations::copy(source.getWritePointer(1), cycle.getReadPointer(1), numThisTime);
								}

								FloatVectorOperations::copy(source.getWritePointer(0, numThisTime * 1), source.getReadPointer(0), numThisTime);
								FloatVectorOperations::copy(source.getWritePointer(0, numThisTime * 2), source.getReadPointer(0), numThisTime);

								if (isStereo)
								{
									FloatVectorOperations::copy(source.getWritePointer(1, numThisTime * 1), source.getReadPointer(1), numThisTime);
									FloatVectorOperations::copy(source.getWritePointer(1, numThisTime * 2), source.getReadPointer(1), numThisTime);
								}

								ip.process(ratio, source.getWritePointer(0), target.getWritePointer(0), length * 3);

								if (isStereo)
								{
									ip.reset();
									ip.process(ratio, source.getWritePointer(1), target.getWritePointer(1), length * 3);
								}

								auto thisOffset = length + latency;

								FloatVectorOperations::copy(resampled.getWritePointer(0, offset), target.getReadPointer(0, thisOffset), length);

								if(isStereo)
									FloatVectorOperations::copy(resampled.getWritePointer(1, offset), target.getReadPointer(1, thisOffset), length);
							}
							else
							{
								FloatVectorOperations::copy(resampled.getWritePointer(0, offset), cycle.getReadPointer(0, 0), length);

								if(isStereo)
									FloatVectorOperations::copy(resampled.getWritePointer(1, offset), cycle.getReadPointer(1, 0), length);
							}

							offset += length;
						}
					}


					StoreData sd;
					sd.sample.noteNumber = i;
					sd.noteRange = r;
					sd.dataBuffer = std::move(resampled);

					sd.numChannels = isStereo ? 2 : 1;
					
					sd.parent = waveTableTree;
					sd.sampleRate = fileSampleRate;
					sd.numParts = cycles.size();
					storeData(sd);
				}
			};

			if (sampleIndex != -1)
			{
				doit((int)s[SampleIds::Root], { loKey, hiKey });
			}
			else if (hiKey - loKey > mipmapSize)
			{
				for (int i = loKey + mipmapSize / 2; i < hiKey; i += mipmapSize)
				{
					logFunction("Create mipmap for root note " + MidiMessage::getMidiNoteName(i, true, true, 3));

					Range<int> nr(i - mipmapSize / 2, i + mipmapSize / 2 - 1);
					doit(i, nr);
				}
			}
			else
			{
				// Do not use the root not here as it might be retuned... s[SampleIds::Root]
				doit(loKey, { loKey, hiKey });
			}
		}
	}
}

void SampleMapToWavetableConverter::renderAllWavetablesFromHarmonicMaps()
{
	ScopedValueSetter svs(currentIndex, 0);

	for (const auto map_ : harmonicMaps)
	{
		if (auto ts = ThreadController::ScopedStepScaler(threadController.get(), currentIndex, harmonicMaps.size()))
		{
			checkIfShouldExit();

			auto& map = *map_;

			if (!map.analysed)
			{
				ThreadController::ScopedRangeScaler s(threadController.get(), 0.0, 0.8);
				calculateHarmonicMap();
			}
				
			checkIfShouldExit();

			ThreadController::ScopedRangeScaler s(threadController.get(), 0.8, 1.0);

			if (map.noteRange.getLength() > mipmapSize)
			{
				for (int i = map.noteRange.getStart(); i < map.noteRange.getEnd(); i += mipmapSize)
				{
					logFunction("Create mipmap for root note " + MidiMessage::getMidiNoteName(i, true, true, 3));

					ThreadController::ScopedStepScaler scaler(threadController.get(), i - map.noteRange.getStart(), map.noteRange.getLength());

					checkIfShouldExit();

					Range<int> nr(i, jmin<int>(i + mipmapSize, map.noteRange.getEnd()) - 1);
					auto midNote = i + nr.getLength() / 2;

					StoreData sd;
					sd.sample = map.index;
					sd.sample.noteNumber = midNote;
					sd.noteRange = nr;
					
					sd.numChannels = map.isStereo ? 2 : 1;
					sd.parent = waveTableTree;
					sd.sampleRate = map.fileSampleRate;

					if (phaseMode == PhaseMode::DynamicPhase)
					{
						auto targetLength = jmin(ResynthesisHelpers::getWavetableLength(midNote, map.fileSampleRate), map.wavetableLength);

						auto notePitch = (double)targetLength / (double)map.wavetableLength;
						notePitch *= map.lorisResynRatio;

						sd.dataBuffer = getResampledLorisBuffer(map.lorisResynBuffer, notePitch, targetLength, midNote);

						sd.numParts = sd.dataBuffer.getNumSamples() / targetLength;

						storeData(sd);
					}
					else
					{
						sd.dataBuffer = calculateWavetableBank(map, midNote);
						sd.numParts = numParts;

						storeData(sd);
					}
				}
			}
			else
			{
				logFunction("Create wavetable data");

				StoreData sd;
				sd.sample = map.index;
				sd.sample.noteNumber = map.rootNote;
				sd.noteRange = map.noteRange;
				sd.numChannels = map.isStereo ? 2 : 1;
				
				sd.parent = waveTableTree;

				if (phaseMode == PhaseMode::DynamicPhase)
				{
					sd.sampleRate = map.fileSampleRate;
					sd.dataBuffer = getResampledLorisBuffer(map.lorisResynBuffer, map.lorisResynRatio, map.wavetableLength, map.rootNote);
					sd.numParts = sd.dataBuffer.getNumSamples() / map.wavetableLength;

					storeData(sd);
				}
				else
				{
					auto bank = calculateWavetableBank(map);

					jassert(bank.getNumSamples() == map.wavetableLength * numParts);

					sd.dataBuffer = std::move(bank);
					sd.numParts = numParts;

					storeData(sd);
				}
			}

			currentIndex++;
		}
	}
}

AudioSampleBuffer SampleMapToWavetableConverter::calculateWavetableBank(const HarmonicMap &map, int noteNumber)
{
	if (noteNumber == -1)
		noteNumber = map.rootNote;

	auto numSamples = ResynthesisHelpers::getWavetableLength(noteNumber, sampleRate);
	auto numHarmonics = map.harmonicGains.getNumSamples();
	auto numSlices = map.harmonicGains.getNumChannels();

	AudioSampleBuffer bank(2, numSamples * numParts);

	bank.clear();

	float* dataL = bank.getWritePointer(0);
	float* dataR = bank.getWritePointer(1);

	int offset = 0;

	int partIndex = 0;

	for (int i = 0; i < numSlices; i++)
	{
		checkIfShouldExit();

		if (auto s = ThreadController::ScopedStepScaler(threadController.get(), i, numSlices))
		{
			auto phase = getPhaseData(map, i, false);

			{
				ThreadController::ScopedRangeScaler s(threadController.get(), 0.0, 0.5);

				ResynthesisHelpers::createWavetableFromHarmonicSpectrum(threadController.get(), map.harmonicGains.getReadPointer(partIndex), numHarmonics, dataL + offset, noteNumber, sampleRate, phase);

				checkIfShouldExit();
			}

			if (map.isStereo)
			{
				phase = getPhaseData(map, i, true);

				ThreadController::ScopedRangeScaler s(threadController.get(), 0.5, 1.0);

				ResynthesisHelpers::createWavetableFromHarmonicSpectrum(threadController.get(), map.harmonicGainsRight.getReadPointer(partIndex), numHarmonics, dataR + offset, noteNumber, sampleRate, phase);

				checkIfShouldExit();
			}

			if (useOriginalGain)
			{
				const float gainL = map.gainValues.getSample(0, partIndex);
				const float gainR = map.isStereo ? map.gainValues.getSample(1, partIndex) : gainL;

				FloatVectorOperations::multiply(dataL + offset, gainL, numSamples);

				if (map.isStereo)
					FloatVectorOperations::multiply(dataR + offset, gainR, numSamples);
			}

			offset += numSamples;

			partIndex++;
		}
	}

	return bank;
}

String SampleMapToWavetableConverter::getPrefixFromNoiseMode(int noteNumber) const
{
	String p;

	switch (previewNoise)
	{
	case PreviewNoise::Mix: p << "_mix"; break;
	case PreviewNoise::Mute: p << "_resyn"; break;
	case PreviewNoise::Solo: p << "_noise"; break;
	}

	if (noteNumber != -1)
		p << "_" << String(noteNumber);

	return p;
}

void SampleMapToWavetableConverter::applyNoiseBuffer(const HarmonicMap& m, AudioSampleBuffer& tonalSignal)
{
	if (previewNoise == PreviewNoise::Mute)
		return;

	auto numToCopy = jmin(tonalSignal.getNumSamples(), m.noiseBuffer.getNumSamples());

	if (previewNoise == PreviewNoise::Mix)
	{
		FloatVectorOperations::add(tonalSignal.getWritePointer(0), m.noiseBuffer.getReadPointer(0), numToCopy);
		FloatVectorOperations::add(tonalSignal.getWritePointer(1), m.noiseBuffer.getReadPointer(m.isStereo ? 1 : 0), numToCopy);
	}
	else if (previewNoise == PreviewNoise::Solo)
	{
		tonalSignal.clear();
		FloatVectorOperations::copy(tonalSignal.getWritePointer(0), m.noiseBuffer.getReadPointer(0), numToCopy);
		FloatVectorOperations::copy(tonalSignal.getWritePointer(1), m.noiseBuffer.getReadPointer(m.isStereo ? 1 : 0), numToCopy);
	}
}

void SampleMapToWavetableConverter::discardAllScans()
{
	originalSpectrum = {};

	for (auto m : harmonicMaps)
	{
		m->analysed = false;
	}
}

void SampleMapToWavetableConverter::refreshCurrentWavetable(bool forceReanalysis /*= true*/)
{
	originalSpectrum = {};

	if (phaseMode == PhaseMode::Resample)
	{
		rebuildPreviewBuffersInternal();
		return;
	}
		

	if (currentIndex == -1)
		throw Result::fail("Nothing loaded");

	jassert(currentIndex < harmonicMaps.size());

	if (!forceReanalysis && getCurrentMap()->analysed)
	{
		sendChangeMessage();
		return;
	}

	if (auto s = ThreadController::ScopedRangeScaler(threadController.get(), 0.0, 0.8))
		calculateHarmonicMap();

	if (auto s = ThreadController::ScopedRangeScaler(threadController.get(), 0.8, 1.0))
		rebuildPreviewBuffersInternal();

}

void SampleMapToWavetableConverter::setCurrentIndex(int index, NotificationType n)
{
	for (int i = 0; i < harmonicMaps.size(); i++)
	{
		if (harmonicMaps[i]->index.sampleIndex == index)
		{
			if (currentIndex == i)
				return;

			currentIndex = i;
			break;
		}
	}

	if (n != dontSendNotification)
	{
		refreshCurrentWavetable(true);
	}
}


int SampleMapToWavetableConverter::getCurrentNoteNumber() const
{
	if (currentIndex < harmonicMaps.size())
	{
		return harmonicMaps[currentIndex]->index.noteNumber;
	}

	return -1;
}


juce::AudioSampleBuffer SampleMapToWavetableConverter::getPreviewBuffers(bool original)
{
	auto& bToUse = original ? originalBuffer : previewBuffer;

	if (originalSpectrum.isNull())
	{
		Spectrum2D s2d(this, bToUse);

		s2d.parameters = getParameters();
		
		auto lb = s2d.createSpectrumBuffer(false);

		originalSpectrum = s2d.createSpectrumImage(lb);
		spectrumBroadcaster.sendMessage(sendNotificationAsync, &originalSpectrum);
	}

	return bToUse;
}

void SampleMapToWavetableConverter::storeData(StoreData& data)
{
	ValueTree child(exportAsHwt ? "wavetable" : "sample");

	if (data.numChannels == -1)
		data.numChannels = data.dataBuffer.getNumChannels();

	if (data.numParts == -1)
		data.numParts = numParts;

	if (data.sampleRate == -1.0)
		data.sampleRate = 48000.0;

	if (!exportAsHwt)
	{
		applyNoiseBuffer(*getCurrentMap(), data.dataBuffer);

		auto originalSample = sampleMap.getChild(data.sample.sampleIndex);
		child.copyPropertiesFrom(originalSample, nullptr);

		PoolReference ref(chain->getMainController(), originalSample[SampleIds::FileName].toString(), FileHandlerBase::Samples);

		auto originalFile = ref.getFile();

		auto wavetableLength = (double)data.dataBuffer.getNumSamples() / (double)data.numParts;

		jassert(hmath::fmod(wavetableLength, 1.0) == 0.0);

		// We don't need the rescaling
		if (previewNoise == PreviewNoise::Solo)
			child.setProperty(SampleIds::Root, getCurrentMap()->rootNote, nullptr);
		else
			ResynthesisHelpers::writeRootAndPitch(child, data.sampleRate, wavetableLength);

		auto resynCopy = originalFile.getSiblingFile(originalFile.getFileNameWithoutExtension() + getPrefixFromNoiseMode(data.sample.noteNumber)).withFileExtension(originalFile.getFileExtension());

		AudioFormatManager afm;
		afm.registerBasicFormats();

		bool ok = false;

		if (ScopedPointer<AudioFormatReader> originalReader = afm.createReaderFor(originalFile))
		{
			jassert(data.sampleRate == originalReader->sampleRate);

			if (auto format = afm.findFormatForFileExtension(originalFile.getFileExtension()))
			{
				auto fos = new FileOutputStream(resynCopy);
				if (ScopedPointer<AudioFormatWriter> writer = format->createWriterFor(fos, originalReader->sampleRate, originalReader->getChannelLayout(), originalReader->bitsPerSample, originalReader->metadataValues, 5))
				{
					writer->writeFromAudioSampleBuffer(data.dataBuffer, 0, data.dataBuffer.getNumSamples());
					ok = true;

					PoolReference copyRef(chain->getMainController(), resynCopy.getFullPathName(), FileHandlerBase::Samples);

					child.setProperty(SampleIds::FileName, copyRef.getReferenceString(), nullptr);
					child.setProperty(SampleIds::SampleStart, 0, nullptr);
					child.removeProperty(SampleIds::SampleEnd, nullptr);
				}
			}
		}
	}
	
	child.setProperty(SampleIds::LoKey, data.noteRange.getStart(), nullptr);
	child.setProperty(SampleIds::HiKey, data.noteRange.getEnd(), nullptr);

	if (exportAsHwt)
	{
		child.setProperty(SampleIds::Root, data.sample.noteNumber, nullptr);
		child.setProperty("amount", data.numParts, nullptr);
		child.setProperty("sampleRate", data.sampleRate, nullptr);
		child.setProperty("reversed", reverseOrder, nullptr);
		child.setProperty("dynamic_phase", phaseMode == PhaseMode::DynamicPhase, nullptr);

		child.setProperty("useCompression", useCompression, nullptr);

		

		for (int i = 0; i < data.numChannels; i++)
		{

			MemoryBlock mb;

			if (useCompression)
			{
				ScopedPointer<MemoryOutputStream> mos = new MemoryOutputStream(mb, false);

				FlacAudioFormat fm;

				if (ScopedPointer<AudioFormatWriter> writer = fm.createWriterFor(mos, data.sampleRate, AudioChannelSet::mono(), 24, {}, 5))
				{
					mos.release();

					float* d[1] = { data.dataBuffer.getWritePointer(i) };

					writer->writeFromFloatArrays(d, 1, data.dataBuffer.getNumSamples());

					writer->flush();
					writer = nullptr;
				}
			}
			else
			{
				mb = MemoryBlock(data.dataBuffer.getNumSamples() * sizeof(float));
				FloatVectorOperations::copy((float*)mb.getData(), data.dataBuffer.getReadPointer(i), data.dataBuffer.getNumSamples());
			}

			var binaryData(mb);

			String s = "data";

			if (i != 0)
				s << String(i);

			child.setProperty(s, binaryData, nullptr);
		}
	}
	
	data.parent.addChild(child, -1, nullptr);
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

void SampleMapToWavetableConverter::readSample(AudioSampleBuffer& buffer, int index, int noteNumber)
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

				checkIfShouldExit();

				return;
			}

			currentSampleLength = range.getLength();

			AudioSampleBuffer unresampled(2, range.getLength());
			reader->read(&unresampled, 0, range.getLength(), range.getStart(), true, true);

			checkIfShouldExit();

            const auto pitchFactorSampleRate = reader->sampleRate / sampleRate;// / reader->sampleRate;


			auto pitchFactorRoot = StreamingSamplerSound::getPitchFactor(noteNumber, rootNote);
			const auto pf = pitchFactorRoot * pitchFactorSampleRate;
			const int numSamplesResampled = roundToInt((double)unresampled.getNumSamples() / pf);

			LagrangeInterpolator interpolator;
			buffer.setSize(2, numSamplesResampled);

			interpolator.process(pf, unresampled.getReadPointer(0), buffer.getWritePointer(0), numSamplesResampled);

			checkIfShouldExit();

			interpolator.reset();
			interpolator.process(pf, unresampled.getReadPointer(1), buffer.getWritePointer(1), numSamplesResampled);

			checkIfShouldExit();
		}
		else
		{
			throw Result::fail("Error opening file " + filePath);
		}
	}
	else
	{
		throw Result::fail("File couldn't be found.");
	}
}




#endif

}
