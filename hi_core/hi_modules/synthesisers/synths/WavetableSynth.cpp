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


namespace hise { using namespace juce;

hise::ProcessorMetadata WavetableSynth::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Mod = ProcessorMetadata::ModulationMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ModulatorSynth::createBaseMetadata()
		.withStandardMetadata<WavetableSynth>()
		.withDescription("A two-dimensional wavetable synthesiser that morphs between waveforms using a table index and supports audio file resynthesis.")
		.withComplexDataInterface(ExternalData::DataType::AudioFile)
		.withParameter(Par(HqMode)
			.withId("HqMode")
			.withDescription("Enables higher-quality rendering with better interpolation at the cost of more CPU usage")
			.asToggle()
			.withDefault(1.0f))
		.withParameter(Par(LoadedBankIndex)
			.withId("LoadedBankIndex")
			.withDescription("Index of the currently loaded wavetable bank from the project's wavetable directory")
			.withSliderMode(HiSlider::Discrete, Range(-1.0, 128.0, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(TableIndexValue)
			.withId("TableIndexValue")
			.withDescription("Normalized position in the wavetable where 0.0 is the first and 1.0 is the last waveform")
			.withSliderMode(HiSlider::NormalizedPercentage, {})
			.withDefault(0.0f))
		.withParameter(Par(RefreshMipmap)
			.withId("RefreshMipMap")
			.withDescription("Updates the mipmap when pitch modulation goes outside the frequency range to reduce aliasing")
			.asToggle()
			.withDefault(0.0f))
		.withModulation(Mod((int)WavetableInternalChains::TableIndexModulation)
			.withId("Table Index")
			.withDescription("Modulates the wavetable position as a scale factor on the table index value")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly)
			.withModulatedParameter(TableIndexValue))
		.withModulation(Mod((int)WavetableInternalChains::TableIndexBipolarChain)
			.withId("Table Index Bipolar")
			.withDescription("Adds a bipolar offset to the wavetable position for modulation-driven morphing")
			.withMode(scriptnode::modulation::ParameterMode::Pan)
			.withModulatedParameter(TableIndexValue))
		;
}

WavetableSynth::WavetableSynth(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices),
	AudioSampleProcessor(mc)
{
	getBuffer().addListener(this);

	modChains += {this, "Table Index"};
	modChains += {this, "Table Index Bipolar", ModulatorChain::ModulationType::Normal, Modulation::PanMode};

	finaliseModChains();

	tableIndexChain = modChains[ChainIndex::TableIndex].getChain();
	tableIndexBipolarChain = modChains[ChainIndex::TableIndexBipolar].getChain();

	updateParameterSlots();

	editorStateIdentifiers.add("TableIndexChainShown");

	for (int i = 0; i < numVoices; i++) 
		addVoice(new WavetableSynthVoice(this));

	tableIndexChain->setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff4D54B3)));
	tableIndexBipolarChain->setColour(Colour(0xff4D54B3));
}

void WavetableSynth::getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue)
{
	WavetableSynthVoice *wavetableVoice = dynamic_cast<WavetableSynthVoice*>(getLastStartedVoice());

	if (wavetableVoice != nullptr)
	{
		WavetableSound *wavetableSound = dynamic_cast<WavetableSound*>(wavetableVoice->getCurrentlyPlayingSound().get());

		if (wavetableSound != nullptr)
		{
			const int tableIndex = roundToInt(getDisplayTableValue() * ((float)wavetableSound->getWavetableAmount()-1));
			*tableValues = wavetableSound->getWaveTableData(0, tableIndex);
			numValues = wavetableSound->getTableSize();
			normalizeValue = 1.0f / wavetableSound->getMaxLevel();
		}
	}
	else
	{
		*tableValues = nullptr;
		numValues = 0;
		normalizeValue = 1.0f;
	}
}

float WavetableSynth::getTotalTableModValue(int offset)
{
	
	offset /= HISE_EVENT_RASTER;

	auto gainMod = modChains[ChainIndex::TableIndex].getModValueForVoiceWithOffset(offset);
	auto bipolarMod = modChains[ChainIndex::TableIndexBipolar].getModValueForVoiceWithOffset(offset);
	bipolarMod *= (float)modChains[ChainIndex::TableIndexBipolar].getChain()->shouldBeProcessedAtAll();
	auto mv = jlimit<float>(0.0f, 1.0f, (tableIndexKnobValue.advance() * gainMod) + bipolarMod);
	return mv * (1.0f - reversed) + (1.0f - mv) * reversed;
}

ProcessorEditorBody* WavetableSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new WavetableBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


juce::File WavetableSynth::getWavetableMonolith() const
{
	auto dir = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Samples);

	auto factoryData = dir.getChildFile("wavetables.hwm");

	if (auto e = getMainController()->getExpansionHandler().getCurrentExpansion())
	{
		dir = e->getSubDirectory(FileHandlerBase::SampleMaps);
		auto expData = dir.getChildFile("wavetables.hwm");

		if (expData.existsAsFile())
			return expData;
	}

	return factoryData;
}

juce::StringArray WavetableSynth::getWavetableList() const
{
	auto monolithFile = getWavetableMonolith();

	StringArray sa;

	if (monolithFile.existsAsFile())
	{
		FileInputStream fis(monolithFile);

#if USE_BACKEND
		auto projectName = GET_HISE_SETTING(this, HiseSettings::Project::Name).toString();
		auto encryptionKey = GET_HISE_SETTING(this, HiseSettings::Project::EncryptionKey).toString();
#else
		auto encryptionKey = FrontendHandler::getExpansionKey();
		auto projectName = FrontendHandler::getProjectName();
#endif

		auto headers = WavetableMonolithHeader::readHeader(fis, projectName, encryptionKey);

#if USE_BACKEND
		if (headers.isEmpty())
		{
			PresetHandler::showMessageWindow("Can't open wavetable monolith", "Make sure that the project name and encryption key haven't changed", PresetHandler::IconType::Error);
		}
#endif

		for (auto item : headers)
			sa.add(item.name);
	}
	else
	{
		auto dir = getMainController()->getCurrentFileHandler().getSubDirectory(ProjectHandler::SubDirectories::AudioFiles);

		Array<File> wavetables;
		dir.findChildFiles(wavetables, File::findFiles, true, "*.hwt");
		wavetables.sort();

		for (auto& f : wavetables)
			sa.add(f.getFileNameWithoutExtension());
	}

	return sa;
}

struct ResynthInfo: public ControlledObject
{
	ResynthInfo(MainController* mc, const AudioSampleBuffer& fullBuffer, std::pair<int, int> lengthData):
	  ControlledObject(mc),
	  numCycles(lengthData.first),
	  sliceLength(lengthData.second),
	  totalNumSamples(fullBuffer.getNumSamples()),
	  slice(fullBuffer.getNumChannels(), sliceLength * 2),
	  magnitude(fullBuffer.getNumChannels(), sliceLength),
	  phase(fullBuffer.getNumChannels(), sliceLength),
	  firstPhase(fullBuffer.getNumChannels(), sliceLength),
	  fft(log2(sliceLength)),
	  t(new ThreadController(Thread::getCurrentThread(), &mc->getSampleManager().getPreloadProgress(), 100, lastTime))
	{}

	int getNumCycles() const { return numCycles; }

	int getSliceOffset(int sliceIndex) const
	{
		auto sliceOffset = sliceIndex * totalNumSamples / numCycles;

		if(totalNumSamples - sliceOffset < sliceLength)
			sliceOffset = totalNumSamples - sliceLength;

		return sliceOffset;
	}

	void resampleToPowerOfTwo(const AudioSampleBuffer& fullBuffer, int channelIndex, int offset, double fourCycleLength)
	{
		slice.clear(channelIndex, 0, slice.getNumSamples());
		auto ratio =  fourCycleLength / (double)(sliceLength);

		int numSamplesToFill = slice.getNumSamples();

		offset = jmin(fullBuffer.getNumSamples() - roundToInt(numSamplesToFill * ratio), offset);

		auto dst = slice.getWritePointer(channelIndex);
		auto src = fullBuffer.getReadPointer(channelIndex, offset);

		double uptime = 0.0;

		for(int i = 0; i < numSamplesToFill; i++)
		{
			auto i1 = (int)uptime;
			auto i2 = (i1 + 1);

			auto alpha = hmath::fmod((float)uptime, 1.0f);

			auto v1 = src[i1];
			auto v2 = src[i2];

			dst[i] = Interpolator::interpolateLinear(v1, v2, alpha);

			uptime += ratio;
		}

#if 0

		juce::LinearInterpolator interpolator;
		interpolator.process(1.0f / ratio, fullBuffer.getReadPointer(channelIndex, offset), slice.getWritePointer(channelIndex), sliceLength + 1);

		dyn<float> bf(slice.getWritePointer(channelIndex), slice.getNumSamples());
		std::rotate(bf.begin(), bf.begin() + roundToInt(interpolator.getBaseLatency()), bf.end());
#endif
	}

	void resynthesizeWithFFT(bool first, WavetableHelpers::PhaseMode pm)
	{
		for(int c = 0; c < slice.getNumChannels(); c++)
		{
			fft.performRealOnlyForwardTransform(slice.getWritePointer(c));

			FFTHelpers::toFreqSpectrum(slice, magnitude, c);
			FFTHelpers::toPhaseSpectrum(slice, phase, c);
			
			auto ptr = magnitude.getWritePointer(c);
			auto phasePtr = phase.getWritePointer(c);

			ptr[0] = 0.0f;

			int s = magnitude.getNumSamples();

			float phaseDelta = 0.0f;

			if(!first && pm == WavetableHelpers::PhaseMode::DynamicPhase)
			{
				// the phase of the root frequency
				auto rootOffset = firstPhase.getSample(c, 1);

				// calculate the delta to the current root frequency phase
				phaseDelta = rootOffset - phasePtr[cycleMultiplier];
			}

			for(int i = 0; i < s / (cycleMultiplier * 2); i++)
			{
				ptr[i] = ptr[i * cycleMultiplier];

				if(pm == WavetableHelpers::PhaseMode::DynamicPhase)
				{
					phasePtr[i] = phasePtr[i * cycleMultiplier];

					if(i != 0)
					{
						auto thisPhaseDelta = phaseDelta * (float)i;

						auto pv = phasePtr[i];

						pv += thisPhaseDelta;

					    const float twoPi = 2.0f * juce::MathConstants<float>::pi;

					    pv = std::fmod(pv + juce::MathConstants<float>::pi, twoPi);

					    if (pv < 0)
					        pv += twoPi;

					    pv = pv - juce::MathConstants<float>::pi;
						
						phasePtr[i] = pv;
					}
				}
				else if (pm == WavetableHelpers::PhaseMode::StaticPhase)
				{
					if(first)
						phasePtr[i] = phasePtr[i * cycleMultiplier];
					else
						phasePtr[i] = firstPhase.getSample(c, i);
				}
				else if (pm == WavetableHelpers::PhaseMode::ZeroPhase)
				{
					phasePtr[i] = 0.0f;
				}
			}

			if(first)
			{
				firstPhase.copyFrom(c, 0, phase, c, 0, phase.getNumSamples());
			}

			for(int i = (s / (cycleMultiplier*2)); i < s; i++)
			{
				ptr[i] = 0.0f;
				phasePtr[i] = 0.0f;
			}

			FFTHelpers::toComplexArray(phase, magnitude, slice, c);

			fft.performRealOnlyInverseTransform(slice.getWritePointer(c));
		}

		
	}

	void applyOffsetToStaticPhase(bool isEmpty, WavetableHelpers::PhaseMode pm)
	{
		if(pm == WavetableHelpers::PhaseMode::StaticPhase)
		{
			for(int c = 0; c < slice.getNumChannels(); c++)
			{
				auto phaseOffset = findZeroCrossingWithSlope(slice, c, 0, sliceLength, true);

				if(isEmpty)
					firstOffset[c] = phaseOffset;
				else
					phaseOffset = firstOffset[c];

				dyn<float> bf2(slice.getWritePointer(c), sliceLength);
				std::rotate(bf2.begin(), bf2.begin() + phaseOffset, bf2.end());
			}
			
		}
	}

	AudioSampleBuffer createWavetable()
	{
		AudioSampleBuffer wt(slice.getNumChannels(), sliceLength);

		for(int i = 0; i < wt.getNumChannels(); i++)
			wt.copyFrom(i, 0, slice, i, 0, sliceLength);

		return wt;
	}

	double getActualFourPeriodLength(int channelIndex, double approximate1PeriodLength)
	{
		auto x = detectPeriodicities(slice, channelIndex, sliceLength, approximate1PeriodLength);
		return (x.empty() ? approximate1PeriodLength : x[0].lag) * (double)cycleMultiplier;
	}

	static int tryToGuessCycleLength(const AudioSampleBuffer& b)
	{
		auto minLag = 128;
		auto maxLag = 2048;

		std::vector<float> scores;

		scores = computeNormalizedAutocorrelation(b, 0, minLag, maxLag, jmin(8192, b.getNumSamples()));

		auto peaks = findPeaksWithSubsampleRefinement(scores, minLag);

		// Sort peaks by strength
	    std::sort(peaks.begin(), peaks.end(), [](const CycleCandidate& a, const CycleCandidate& b) {
	        return a.score > b.score;
	    });

		auto c = suppressHarmonics(peaks);

		if(c.empty())
			return 0;

		auto x2 = c[0].lag;

		auto l2 = std::log2(x2);
		auto diff = std::abs(std::round(l2) - l2);

		if(diff < 0.003)
		{
			auto x = roundToInt(std::pow(2.0, std::round(l2)));
			return x;
		}

		return 0;
	}

	int cycleMultiplier = 4;

	void fillSlice(const AudioSampleBuffer& fullBuffer, int offset)
	{
		slice.clear();

		jassert(fullBuffer.getNumChannels() == slice.getNumChannels());

		for(int i = 0; i < fullBuffer.getNumChannels(); i++)
			slice.copyFrom(i, 0, fullBuffer, i, offset, sliceLength);
	}


	const int numCycles;
	int sliceLength;
	const int totalNumSamples;

	AudioSampleBuffer slice;
	AudioSampleBuffer magnitude;
	AudioSampleBuffer phase;
	AudioSampleBuffer firstPhase;

	juce::dsp::FFT fft;
	int firstOffset[2] = {0, 0};

	struct CycleCandidate
	{
	    float lag;
	    float score;
	};

	uint32 lastTime = 0;
	ThreadController::Ptr t;

	bool useLoris() const
	{
#if HISE_INCLUDE_LORIS
		return lm != nullptr;
#else
		return false;
#endif
	}

	void resynthesizeWithLoris(int sliceIndex, double timeOffsetSeconds, WavetableHelpers::PhaseMode pm)
	{
#if HISE_INCLUDE_LORIS
		if(auto s = ThreadController::ScopedStepScaler(t.get(), sliceIndex, numCycles))
		{
			AudioSampleBuffer half(magnitude.getNumChannels(), 2048);
			
			magnitude.clear();
			
			slice.clear();

			if(auto s = ThreadController::ScopedStepScaler(t.get(), 0, 2))
			{
				auto gains = lm->getSnapshot(lf, timeOffsetSeconds, "gain");

				for(int c = 0; c < gains.size(); c++)
				{
					if(auto x = gains[c].getArray())
					{
						for(int i = 0; i < x->size(); i++)
						{
							magnitude.setSample(c, i+1, (float)(*x)[i] * 1024.0);
						}
					}
				}
			}

			if(auto s = ThreadController::ScopedStepScaler(t.get(), 1, 2))
			{
				if(pm == WavetableHelpers::PhaseMode::DynamicPhase || sliceIndex == 0)
				{
					auto phases = lm->getSnapshot(lf, timeOffsetSeconds, "phase");

					for(int c = 0; c < phases.size(); c++)
					{
						if(auto p = phases[c].getArray())
						{
							float phaseDelta = 0.0f;

							for(int i = 0; i < p->size(); i++)
							{
								auto pv = (float)(*p)[i];

								const float twoPi = 2.0f * juce::MathConstants<float>::pi;

								if(i == 0)
								{
									phaseDelta = -1.0 * pv;

									if(phaseDelta < 0.0)
										phaseDelta += twoPi;

									pv = 0.0f;

								}
								else
								{
									auto thisPhaseDelta = phaseDelta * (float)(i+1);

									pv += thisPhaseDelta;
								    pv = std::fmod(pv + juce::MathConstants<float>::pi, twoPi);

								    if (pv < 0)
								        pv += twoPi;

								    pv = pv - juce::MathConstants<float>::pi;
								}

								phase.setSample(c, i+1, pv);
							}
						}
					}
				}
			}

			juce::dsp::FFT fft(log2(2048));

			for(int c = 0; c < phase.getNumChannels(); c++)
			{
				FFTHelpers::toComplexArray(phase, magnitude, slice, c);
				fft.performRealOnlyInverseTransform(slice.getWritePointer(c));
			}
		}
#endif
	}

	void setUseLoris(bool shouldUseLoris, const String& b64, double hoptime, double rootFrequency)
	{
#if HISE_INCLUDE_LORIS
		if(useLoris() != shouldUseLoris)
		{
			if(shouldUseLoris)
			{
				lm = new LorisManager(File(), BIND_MEMBER_FUNCTION_1(ResynthInfo::onLorisMessage));
				lm->threadController = t;

				PoolReference ref(getMainController(), b64, FileHandlerBase::AudioFiles);
				LorisManager::AnalyseData ad;
				ad.file = ref.getFile();
				lf = ad.file;
				ad.rootFrequency = rootFrequency;

				hoptime = jmax(hoptime, 1.0 / ad.rootFrequency);
				hoptime = jmin(0.05, hoptime);

				{
					// offset in slice...
					static const double items[7] = {0.0, 0.1, 0.25, 0.5, 0.66666, 0.75, 1.0 };
		            auto offsetInSlice = items[1];
					auto ratio = jlimit(0.25, 4.0, std::pow(2.0, offsetInSlice * 4.0 - 2.0));

					lm->set("hoptime", String(hoptime));
					lm->set("croptime", String(hoptime));
					lm->set("windowwidth", String(ratio));

				}

				sliceLength = 2048;

				magnitude.setSize(magnitude.getNumChannels(), sliceLength);
				phase.setSize(magnitude.getNumChannels(), sliceLength);
				slice.setSize(magnitude.getNumChannels(), sliceLength * 2);

				getMainController()->getSampleManager().getPreloadProgress() = 0.0;
				getMainController()->getSampleManager().setCurrentPreloadMessage("Analysing " + ad.file.getFileName());
				lm->analyse(ad);
			}
			else
			{
				lm = nullptr;
			}
		}
#endif
	}

	void onLorisMessage(const String& m)
	{
		jassertfalse;
		DBG(m);
	}

#if HISE_INCLUDE_LORIS
	ScopedPointer<LorisManager> lm;
#endif

private:

	File lf;

	// Step 1: Compute normalized autocorrelation scores
	static std::vector<float> computeNormalizedAutocorrelation(const AudioSampleBuffer& signalBuffer, int channelIndex, int minLag, int maxLag, int N)
	{
	    
	    std::vector<float> scores;
		scores.reserve(maxLag - minLag);

		dyn<float> signal(const_cast<float*>(signalBuffer.getReadPointer(channelIndex)), N);

	    float mean = 0.0f;

	    for (float v : signal) mean += v;
			mean /= N;

	    float variance = 0.0f;
	    for (float v : signal) variance += (v - mean) * (v - mean);
	    if (variance < 1e-10f) return scores;

	    for (int lag = minLag; lag <= maxLag; ++lag) 
		{
	        float num = 0.0f, denom1 = 0.0f, denom2 = 0.0f;

	        for (int i = 0; i < N - lag; ++i) 
			{
	            float a = signal[i]     - mean;
	            float b = signal[i+lag] - mean;
	            num += a * b;
	            denom1 += a * a;
	            denom2 += b * b;
	        }

	        float normCorr = num / (std::sqrt(denom1 * denom2) + 1e-10f);
	        scores.push_back(normCorr);
	    }

	    return scores;
	}

	// Step 2: Peak picking (local maxima)
	static std::vector<CycleCandidate> findPeaksWithSubsampleRefinement(const std::vector<float>& scores, int minLag)
	{
	    std::vector<CycleCandidate> peaks;

	    for (size_t i = 0; i < scores.size(); ++i) 
		{
			auto idx = i;
			auto next = (i + 1) % scores.size();
			auto prev = (i + scores.size() - 1) % scores.size();

	        float y_prev = scores[prev];
	        float y_curr = scores[idx];
	        float y_next = scores[next];

	        if (y_curr > y_prev && y_curr > y_next) {
	            // Parabolic interpolation
	            float denom = (y_prev - 2 * y_curr + y_next);
	            float delta = (denom == 0.0f) ? 0.0f : 0.5f * (y_prev - y_next) / denom;
	            float refined_lag = static_cast<float>(i + minLag) + delta;

	            // Evaluate peak height at vertex using interpolated y (optional refinement)
	            float refined_score = y_curr - 0.25f * (y_prev - y_next) * delta;

				// do not calculate the refined score at the wrap around as this will
				// not work
				if((next == 0 || prev == 0))
					refined_score = y_curr;

	            peaks.push_back({ refined_lag, refined_score });
	        }
	    }

	    return peaks;
	}

	// Step 3: Harmonic suppression
	static std::vector<CycleCandidate> suppressHarmonics(const std::vector<CycleCandidate>& peaks, float harmonicTolerance = 0.05f) {
	    std::vector<CycleCandidate> filtered;

	    for (const auto& candidate : peaks) {
	        bool isHarmonic = false;

	        for (const auto& accepted : filtered) {
	            float ratio = static_cast<float>(candidate.lag) / accepted.lag;
	            float invRatio = static_cast<float>(accepted.lag) / candidate.lag;

	            if ((std::abs(ratio - std::round(ratio)) < harmonicTolerance) ||
	                (std::abs(invRatio - std::round(invRatio)) < harmonicTolerance)) {
	                isHarmonic = true;
	                break;
	            }
	        }

	        if (!isHarmonic) {
	            filtered.push_back(candidate);
	        }
	    }

	    return filtered;
	}

public:

	static double getApproximatePeriodLength(const AudioSampleBuffer& signal, double sampleRate)
	{
		int maxIndex = 0;
		float maxValue = 0.0f;

		auto ptr = signal.getReadPointer(0);
		int numSamples = signal.getNumSamples();


		for(int i = 0; i < numSamples; i++)
		{
			auto mag = std::abs(ptr[i]);

			if(mag > maxValue)
			{
				maxIndex = i;
				maxValue = mag;
			}
		}

		auto startIndex = jmax(0, maxIndex - signal.getNumSamples() / 8);
		auto numToCheck = signal.getNumSamples() / 4;
		startIndex = jmin(maxIndex - numToCheck, startIndex);
		startIndex = jmax(0, startIndex);

		auto freq = PitchDetection::detectPitch(signal, startIndex, numToCheck, sampleRate);

		if(freq == 0.0)
		{
			auto maxFreq = 1500.0;
			auto minFreq = 40.0;

			auto minLag = sampleRate / maxFreq;
			auto maxLag = sampleRate / minFreq;

			auto x = computeNormalizedAutocorrelation(signal, 0, minLag, maxLag, 4096);

			auto peaks = findPeaksWithSubsampleRefinement(x, minLag);

			// Sort peaks by strength
		    std::sort(peaks.begin(), peaks.end(), [](const CycleCandidate& a, const CycleCandidate& b) {
		        return a.score > b.score;
		    });

			auto c = suppressHarmonics(peaks);

			if(c.empty())
				return 0.0;

			freq = sampleRate / c[0].lag;

			if(freq != 0.0)
				return c[0].lag;

			return 0.0;
		}
			
			

		return  sampleRate / freq;
	}

	// Find nearest zero-crossing near a given sample position with slope matching
	int findZeroCrossingWithSlope(const AudioSampleBuffer& signal, int channelIndex, int startPos, int searchRadius, bool risingSlope)
	{
	    int N = (int)signal.getNumSamples();

	    int bestPos = startPos;
	    int minDist = searchRadius + 1;

	    for (int offset = 0; offset <= searchRadius; ++offset)
	    {
	        int pos = (startPos + offset + N) % N;

	        float s1 = signal.getSample(channelIndex, pos);
	        float s2 = signal.getSample(channelIndex, (pos + 1) % N);

	        bool zeroCross = (s1 <= 0.0f && s2 > 0.0f) || (s1 >= 0.0f && s2 < 0.0f);
	        if (!zeroCross) continue;

	        float slope = s2 - s1;
	        bool slopeMatch = (risingSlope && slope > 0) || (!risingSlope && slope < 0);

	        if (slopeMatch)
	        {
	            int dist = std::abs(offset);
	            if (dist < minDist)
	            {
	                minDist = dist;
	                bestPos = pos;
	            }
	        }
	    }
	    return bestPos;
	}

	std::vector<CycleCandidate> detectPeriodicities(const AudioSampleBuffer& signal,
													int channelIndex,
													int numSamples,
													double approximatePeriodLength,
	                                                float harmonicTolerance = 0.05f)
	{
		int topK = 5;

		auto minLag = roundToInt(approximatePeriodLength * 0.8);
		auto maxLag = jmin(numSamples, roundToInt(approximatePeriodLength * 1.2));

		numSamples = jmin(numSamples, signal.getNumSamples());

	    if (numSamples < 3) return {};

	    auto scores = computeNormalizedAutocorrelation(signal, channelIndex, minLag, maxLag, numSamples);

		if(scores.empty())
		{
			std::vector<CycleCandidate> result;
			CycleCandidate dv;
			dv.lag = (float)approximatePeriodLength;
			dv.score = 1.0f;
			result.push_back(dv);
			return result;
		}

	    auto peaks = findPeaksWithSubsampleRefinement(scores, minLag);

	    // Sort peaks by strength
	    std::sort(peaks.begin(), peaks.end(), [](const CycleCandidate& a, const CycleCandidate& b) {
	        return a.score > b.score;
	    });

	    // Apply harmonic suppression
	    auto filtered = suppressHarmonics(peaks, harmonicTolerance);

	    if (filtered.size() > static_cast<size_t>(topK))
	        filtered.resize(topK);

	    return filtered;
	}

	static std::pair<int, int> getNumCyclesAndFourCycleLength(const AudioSampleBuffer& signal, double periodLength, int cycleMultiplier)
	{
		int maxNumCycles = 512;

		auto x = signal.getNumSamples() / periodLength;
		auto numCycles = jlimit(1, maxNumCycles, roundToInt(std::pow(2.0, std::round(std::log2(x)))));
		
		auto fourCycles = periodLength * (double)cycleMultiplier;
		int sliceLength = nextPowerOfTwo(fourCycles);

		return { numCycles, sliceLength };
	}
};


void WavetableSynth::loadWavetableInternal()
{
	if(currentBankIndex == AudioFileIndex)
	{
		if(getBuffer().isEmpty())
			currentBankIndex = 0;
	}

	if (currentBankIndex == 0)
	{
		clearSounds();
	}

	if(currentBankIndex == AudioFileIndex)
	{
		try
		{
			const auto& b = getAudioSampleBuffer();

			auto filename = getBuffer().toBase64String();

			int cycleLength = getBuffer().getLoopRange().getLength();

			if(!isPowerOfTwo(cycleLength) || cycleLength > 2048)
				cycleLength = 0;

			if(cycleLength == 0)
				cycleLength = ResynthInfo::tryToGuessCycleLength(b);

			WavetableHelpers::ExportData ed;

			if(loadFromCache(filename, ed))
			{
				postProcessCycles(ed);
				rebuildMipMaps(ed);
				return;
			}

			ed.numChannels = b.getNumChannels();
			//ed.fileContent = getAudioSampleBuffer();
			ed.fileSampleRate = getBuffer().sampleRate;
			
			WavetableHelpers::ConfigData cd;
			cd.reverseOrder = resynthOptions.reverseOrder;
			cd.useCompression = false;

			if(cycleLength != 0 && !resynthOptions.forceResynthesis)
			{
				WavetableHelpers::createCycles(ed.cycles, b, cycleLength);

				storeToCache(filename, ed);
				postProcessCycles(ed);
			}
			else
			{
				double periodLength = 0.0;

				if(resynthOptions.rootNote != -1)
				{
					auto midiFreq = MidiMessage::getMidiNoteInHertz(resynthOptions.rootNote);
					periodLength = getBuffer().sampleRate / midiFreq;
				}

				if(periodLength == 0.0)
					periodLength = ResynthInfo::getApproximatePeriodLength(b, getBuffer().sampleRate);

				if(periodLength == 0.0)
				{
					throw Result::fail("Cannot detect root pitch of this sample");
				}

				auto multiplier = resynthOptions.cycleMultiplier;
				auto useLoris = resynthOptions.useLoris;

				AudioSampleBuffer bToUse;

				if(resynthOptions.removeNoise && !useLoris)
				{
					getMainController()->getSampleManager().setCurrentPreloadMessage("Remove Noise for " + getBuffer().toBase64String());
					SiTraNoConverter conv(getBuffer().sampleRate, resynthOptions.sitranoSettings);
					conv.process(b);
					bToUse = conv.getSignalComponent(SiTraNoConverter::SignalComponent::Sinusoidal);
				}
				else
					bToUse = b;


				auto x = ResynthInfo::getNumCyclesAndFourCycleLength(bToUse, periodLength, multiplier);

				if(resynthOptions.numCycles != -1)
					x.first = nextPowerOfTwo(resynthOptions.numCycles);

				auto transientX = ResynthInfo::getNumCyclesAndFourCycleLength(bToUse, periodLength, 1);

				// Use the same amount of cycles
				transientX.first = x.first;

				cd.phaseMode = resynthOptions.pm;

				ResynthInfo transientInfo(getMainController(), bToUse, transientX);
				transientInfo.cycleMultiplier = 1;

				ResynthInfo info(getMainController(), bToUse, x);
				info.cycleMultiplier = multiplier;

				jassert(info.getNumCycles() == transientInfo.getNumCycles());

				
				auto hoptime = (bToUse.getNumSamples() / getBuffer().sampleRate) / (double)info.getNumCycles();

				info.setUseLoris(useLoris, getBuffer().toBase64String(), hoptime, getBuffer().sampleRate / periodLength);

				getMainController()->getSampleManager().setCurrentPreloadMessage("Resynthesising " + getBuffer().toBase64String());

				std::map<int, std::vector<double>> gains;

				getMainController()->getSampleManager().getPreloadProgress() = 0.0;

				for(int i = 0; i < info.getNumCycles(); i++)
				{
					if(!info.useLoris())
					{
						getMainController()->getSampleManager().getPreloadProgress() = (double)i / (double)info.getNumCycles();
					}
					//progress = (double)i / (double)info.getNumCycles();

					auto offset = info.getSliceOffset(i);

					

					if(offset < 0)
						break;

					auto useTransient = resynthOptions.useTransientMode && !info.useLoris() && i < 4;

					bool skipLast = false;

					if(info.useLoris())
					{
						auto timeOffset = offset / getBuffer().sampleRate;
						info.resynthesizeWithLoris(i, timeOffset, cd.phaseMode);
						info.applyOffsetToStaticPhase(ed.cycles.isEmpty(), cd.phaseMode);
					}
					else
					{
						info.fillSlice(bToUse, offset);

						if(useTransient)
							transientInfo.fillSlice(bToUse, offset);

						for(int c = 0; c < bToUse.getNumChannels(); c++)
						{
							auto offsetA = info.findZeroCrossingWithSlope(bToUse, c, offset, roundToInt(periodLength), true);
							auto fourCyclesReal = info.getActualFourPeriodLength(c, periodLength);
							

							if(offsetA + info.sliceLength > bToUse.getNumSamples())
							{
								skipLast = true;
								break;
							}

							info.resampleToPowerOfTwo(bToUse, c, offsetA, fourCyclesReal);

							if(useTransient)
							{
								auto fourCyclesTransient = transientInfo.getActualFourPeriodLength(c, periodLength);
								auto offsetB = transientInfo.findZeroCrossingWithSlope(bToUse, c, offset, roundToInt(periodLength), true);
								transientInfo.resampleToPowerOfTwo(bToUse, c, offsetB, fourCyclesTransient);
							}
						}

						if(!skipLast)
						{
							info.resynthesizeWithFFT(ed.cycles.isEmpty(), cd.phaseMode);
							info.applyOffsetToStaticPhase(ed.cycles.isEmpty(), cd.phaseMode);

							if(useTransient)
							{
								transientInfo.resynthesizeWithFFT(ed.cycles.isEmpty(), cd.phaseMode);
								transientInfo.applyOffsetToStaticPhase(ed.cycles.isEmpty(), cd.phaseMode);
							}
						}
					}

					if(!skipLast)
					{
						if(useTransient)
						{
							auto twt = transientInfo.createWavetable();

							hlac::CompressionHelpers::dump(twt, "transientsmall.wav");

							for(int c = 0; c < b.getNumChannels(); c++)
							{
								auto src = twt.getReadPointer(0);
								auto dst = info.slice.getWritePointer(0);

								for(int i = 0; i < twt.getNumSamples(); i++)
								{
									for(int j = 0; j < info.cycleMultiplier; j++)
									{
										dst[i*info.cycleMultiplier + j] = src[i];
									}
								}
							}

							ed.cycles.add(info.createWavetable());
						}
						else
						{
							ed.cycles.add(info.createWavetable());
						}
					}
				}

				storeToCache(filename, ed);
				postProcessCycles(ed);
			}

			rebuildMipMaps(ed);
			return;
		}
		catch(Result& r)
		{
			if(resynthOptions.errorHandler != nullptr)
				resynthOptions.errorHandler->handleErrorMessage(r.getErrorMessage());

			// do something here...
		}
		catch(...)
		{
			jassertfalse;
		}
	}

	auto monolithFile = getWavetableMonolith();

	if (monolithFile.existsAsFile())
	{
		FileInputStream fis(monolithFile);

#if USE_BACKEND
		auto projectName = GET_HISE_SETTING(this, HiseSettings::Project::Name).toString();
		auto encryptionKey = GET_HISE_SETTING(this, HiseSettings::Project::EncryptionKey).toString();
#else
		auto encryptionKey = FrontendHandler::getExpansionKey();
		auto projectName = FrontendHandler::getProjectName();
#endif

		auto headers = WavetableMonolithHeader::readHeader(fis, projectName, encryptionKey);

		auto dataSize = fis.readInt64();

		auto headerOffset = fis.getPosition();

        ignoreUnused(dataSize);
		

		auto itemToLoad = headers[currentBankIndex - 1];

		if (itemToLoad.name.isEmpty())
		{
			clearSounds();
			return;
		}

		auto seekPosition = itemToLoad.offset + headerOffset;

		if (fis.setPosition(seekPosition))
		{
			auto v = ValueTree::readFromStream(fis);

			if (v.isValid())
			{
				loadWaveTable(v);
				return;
			}
		}

		clearSounds();
		return;
	}
	else
	{
		auto dir = getMainController()->getCurrentFileHandler().getSubDirectory(ProjectHandler::SubDirectories::AudioFiles);

		Array<File> wavetables;

		dir.findChildFiles(wavetables, File::findFiles, true, "*.hwt");
		wavetables.sort();

		if (wavetables[currentBankIndex - 1].existsAsFile())
		{
			FileInputStream fis(wavetables[currentBankIndex - 1]);

			ValueTree v = ValueTree::readFromStream(fis);

			loadWaveTable(v);
		}
		else
		{
			clearSounds();
		}
	}
}

void WavetableSynth::loadWavetableFromIndex(int index)
{
	if (currentBankIndex != index || index == AudioFileIndex)
	{
		currentBankIndex = index;

		getMainController()->getKillStateHandler().killVoicesAndCall(this, [](Processor* p)
		{
			auto ws = static_cast<WavetableSynth*>(p);

			ws->loadWavetableInternal();

			return SafeFunctionCall::OK;
		}, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
	}
}

float WavetableSynth::getDisplayTableValue() const
{
	return (1.0f - reversed) * displayTableValue + (1.0f - displayTableValue) * reversed;
}

WavetableSynthVoice::WavetableSynthVoice(ModulatorSynth *ownerSynth):
	ModulatorSynthVoice(ownerSynth),
	wavetableSynth(dynamic_cast<WavetableSynth*>(ownerSynth)),
	octaveTransposeFactor(1),
	currentSound(nullptr),
	hqMode(true)
{
		
};

bool WavetableSynthVoice::updateSoundFromPitchFactor(double pitchFactor, WavetableSound* soundToUse)
{
    if(soundToUse == nullptr)
    {
        auto thisFreq = startFrequency * pitchFactor;
        
        if(!currentSound->getFrequencyRange().contains(thisFreq))
        {
            auto owner = getOwnerSynth();
            
            for(int i = 0; i < owner->getNumSounds(); i++)
            {
                auto ws = static_cast<WavetableSound*>(owner->getSound(i));
                
                if(ws->getFrequencyRange().contains(thisFreq))
                {
                    soundToUse = ws;
                    break;
                }
            }
        }
    }
    
    if(soundToUse == nullptr)
        return false;
    
    if(currentSound != soundToUse)
    {
        currentSound = soundToUse;
        
        tableSize = currentSound->getTableSize();
        
		uptimeDelta = currentSound->getPitchRatio(noteNumberAtStart);
        uptimeDelta *= getOwnerSynth()->getMainController()->getGlobalPitchFactor();

		if (startUptimeDelta != 0.0)
		{
			auto ratio = uptimeDelta / startUptimeDelta;
			voiceUptime *= ratio;
		}
		
		saveStartUptimeDelta();
        return true;
    }
    
    return false;
}



void WavetableSynthVoice::calculateBlock(int startSample, int numSamples)
{
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	const float *voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();
	
	auto stereoMode = currentSound->isStereo();
	auto owner = static_cast<WavetableSynth*>(getOwnerSynth());
	
	WavetableSound::RenderData r(voiceBuffer, startSample, numSamples, uptimeDelta, voicePitchValues, hqMode);

	r.render(currentSound, voiceUptime, [owner](int startSample) { return owner->getTotalTableModValue(startSample); });

	if (refreshMipmap)
	{
		auto pf = voicePitchValues != nullptr ? voicePitchValues[startIndex + samplesToCopy / 2] : (uptimeDelta / startUptimeDelta);
		updateSoundFromPitchFactor(pf, nullptr);
	}

	if (auto modValues = getOwnerSynth()->getVoiceGainValues())
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);

		if(stereoMode)
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
		else
			FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);
		
	}
	else
	{
		const float constantGain = getOwnerSynth()->getConstantGainModValue();

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), constantGain, samplesToCopy);

		if(stereoMode)
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), constantGain, samplesToCopy);
		else
			FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);
	}

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

	if (getOwnerSynth()->getLastStartedVoice() == this)
	{
		auto currentTableIndex = owner->getTotalTableModValue(startSample);
		owner->setDisplayTableValue(currentTableIndex);
		owner->triggerWaveformUpdate();

		if(owner->currentBankIndex == WavetableSynth::AudioFileIndex)
		{
			auto& bf = owner->getBuffer();

			bf.sendDisplayIndexMessage(currentTableIndex * (float)bf.getCurrentRange().getLength());
		}
	}
}

void WavetableSynthVoice::startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound* s, int /*currentPitchWheelPosition*/)
{
    currentSound = nullptr;
    
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	midiNoteNumber += getTransposeAmount();
    
    noteNumberAtStart = midiNoteNumber;
    
    startFrequency = MidiMessage::getMidiNoteInHertz(noteNumberAtStart);

    updateSoundFromPitchFactor(1.0, static_cast<WavetableSound*>(s));
    
    static_cast<WavetableSynth*>(getOwnerSynth())->tableIndexKnobValue.reset();
    
	voiceUptime = (double)getCurrentHiseEvent().getStartOffset() / 441.0 * (double)tableSize;
}

void WavetableSynthVoice::resetVoice()
{
	ModulatorSynthVoice::resetVoice();
	wavetableSynth->resetModChains(voiceIndex);
}

static MemoryBlock getMemoryBlockFromWavetableData(const ValueTree& v, int channelIndex)
{
	MemoryBlock mb(*v.getProperty(channelIndex == 0 ? "data" : "data1", var::undefined()).getBinaryData());
	auto useCompression = v.getProperty("useCompression", false);

	if (useCompression)
	{
		auto mis = new MemoryInputStream(std::move(mb));

		FlacAudioFormat flac;
		ScopedPointer<AudioFormatReader> reader = flac.createReaderFor(mis, true);

		MemoryBlock mb2;
		mb2.ensureSize(sizeof(float) * reader->lengthInSamples, true);

		float* d[1] = { (float*)mb2.getData() };

		reader->read(d, 1, 0, (int)reader->lengthInSamples);
		reader = nullptr;
		return mb2;
	}
	else
	{
		return mb;
	}
};

WavetableSound::WavetableSound(const ValueTree &wavetableData, Processor* parent)
{
	jassert(wavetableData.getType() == Identifier("wavetable"));

	stereo = wavetableData.hasProperty("data1");

	reversed = (float)(int)wavetableData.getProperty("reversed", false);
	
	auto mb = getMemoryBlockFromWavetableData(wavetableData, 0);

	const int numSamples = (int)(mb.getSize() / sizeof(float));

	wavetables.setSize(stereo ? 2 : 1, numSamples);



	memoryUsage = wavetables.getNumChannels() * wavetables.getNumSamples() * sizeof(float);
	storageSize = wavetableData.getProperty("data").getBinaryData()->getSize();

	if(stereo)
		storageSize += wavetableData.getProperty("data1").getBinaryData()->getSize();

	FloatVectorOperations::copy(wavetables.getWritePointer(0, 0), (float*)mb.getData(), numSamples);

	if (stereo)
	{
		auto mb2 = getMemoryBlockFromWavetableData(wavetableData, 1);
		FloatVectorOperations::copy(wavetables.getWritePointer(1, 0), (float*)mb2.getData(), numSamples);
	}

	maximum = wavetables.getMagnitude(0, numSamples);

	wavetableAmount = wavetableData.getProperty("amount", 64);

	sampleRate = wavetableData.getProperty("sampleRate", 48000.0);

	midiNotes.setRange(0, 127, false);

	if (wavetableData.hasProperty(SampleIds::Root))
		noteNumber = (int)wavetableData[SampleIds::Root];
	else
		noteNumber = wavetableData.getProperty("noteNumber", 0);

	midiNotes.setBit(noteNumber, true);

	dynamicPhase = wavetableData.getProperty("dynamic_phase", false);

    if(wavetableData.hasProperty(SampleIds::LoKey))
    {
        auto l = (int)wavetableData[SampleIds::LoKey];
        auto h = (int)wavetableData[SampleIds::HiKey];
        
        midiNotes.setRange(l, h - l+1, true);
    }
    
	wavetableSize = wavetableAmount > 0 ? numSamples / wavetableAmount : 0;

#if USE_MOD2_WAVETABLESIZE

	if (!isPowerOfTwo(wavetableSize))
	{
		debugError(parent, "Wavetable with non-power two buffer size loaded. Please recompile HISE without USE_MOD2_WAVETABLESIZE.");
	}

#endif

	emptyBuffer = AudioSampleBuffer(1, wavetableSize);
	emptyBuffer.clear();

	unnormalizedMaximum = 0.0f;

	normalizeTables();

	pitchRatio = 1.0;
    
    auto lowDelta = MidiMessage::getMidiNoteInHertz(midiNotes.findNextSetBit(0));
    auto highDelta = MidiMessage::getMidiNoteInHertz(midiNotes.getHighestBit());
                                
    
    frequencyRange = { lowDelta, highDelta };
}

const float * WavetableSound::getWaveTableData(int channelIndex, int wavetableIndex) const
{
	jassert(isPositiveAndBelow(wavetableIndex, wavetableAmount));
	jassert((wavetableIndex + 1) * wavetableSize <= wavetables.getNumSamples());
	jassert(channelIndex == 0 || isStereo());

	return wavetables.getReadPointer(channelIndex, wavetableIndex * wavetableSize);
}

void WavetableSound::calculatePitchRatio(double playBackSampleRate_)
{
    playbackSampleRate = playBackSampleRate_;
    
	const double idealCycleLength = playbackSampleRate / MidiMessage::getMidiNoteInHertz(noteNumber);

	pitchRatio = (double)wavetableSize / idealCycleLength;
}

void WavetableSound::normalizeTables()
{
	unnormalizedGainValues.calloc(wavetableAmount);

	for (int i = 0; i < wavetableAmount; i++)
	{
		const float peak = wavetables.getMagnitude(i * wavetableSize, wavetableSize);

		unnormalizedGainValues[i] = peak;

		if (peak == 0.0f) continue;

		if (peak > unnormalizedMaximum)
			unnormalizedMaximum = peak;
	}

	maximum = 1.0f;
}



String WavetableSound::getMarkdownDescription() const
{
	String s;
	String nl = "\n";

	auto printProperty = [&s, &nl](const String& name, const var& value)
	{
		s << "**" << name << "**: `" << value.toString() << "`  " << nl;
	};

	s << "### Wavetable Data" << nl;

	
	printProperty("Wavetable Length", getTableSize());
	printProperty("Wavetable Amount", getWavetableAmount());
	printProperty("RootNote", MidiMessage::getMidiNoteName(getRootNote(), true, true, 3));
	printProperty("Max Level", String(Decibels::gainToDecibels(getUnnormalizedMaximum()), 2) + " dB");
	printProperty("Stereo", isStereo());
	printProperty("Reversed", (bool)(int)isReversed());
	printProperty("Storage Size", String(storageSize / 1024) + " kB");
	printProperty("Memory Usage", String(memoryUsage / 1024) + " kB");

	return s;
}

void WavetableSound::RenderData::render(WavetableSound* currentSound, double& voiceUptime, const TableIndexFunction& tf)
{
	auto numTables = currentSound->getWavetableAmount();
	auto stereoMode = currentSound->isStereo();
	auto tableSize = currentSound->getTableSize();

	dynamicPhase = currentSound->dynamicPhase;

	while (--numSamples >= 0)
	{
		int index = (int)voiceUptime;

		span<int, 4> i;

#if USE_MOD2_WAVETABLESIZE
		i[0] = (index + tableSize - 1) & (tableSize - 1);
		i[1] = index & (tableSize - 1);
		i[2] = (index + 1) & (tableSize - 1);
		i[3] = (index + 2) & (tableSize - 1);
#else
		i[1] = index % (tableSize);
		i[2] = i[1] + 1;
		i[0] = i[1] - 1;
		i[3] = i[1] + 2;

		if (i[1] == 0)         i[0] = tableSize - 1;
		if (i[2] >= tableSize) i[2] = 0;
		if (i[3] >= tableSize) i[3] = 0;

#endif

		const float tableModValue = tf(startSample);
		const float tableValue = tableModValue * (float)(numTables - 1);

		const int lowerTableIndex = (int)(tableValue);

		const float tableDelta = tableValue - (float)lowerTableIndex;
		jassert(0.0f <= tableDelta && tableDelta <= 1.0f);

		const int upperTableIndex = jmin(numTables - 1, lowerTableIndex + 1);

		auto lowerTable = currentSound->getWaveTableData(0, lowerTableIndex);
		auto upperTable = currentSound->getWaveTableData(0, upperTableIndex);
		const float alpha = float(voiceUptime) - (float)index;

		auto l = calculateSample(lowerTable, upperTable, i, alpha, tableDelta);

		// Stereo mode assumed
		b.setSample(0, startSample, l);

		if (stereoMode)
		{
			auto lowerTableR = currentSound->getWaveTableData(1, lowerTableIndex);
			auto upperTableR = currentSound->getWaveTableData(1, upperTableIndex);

			auto r = calculateSample(lowerTableR, upperTableR, i, alpha, tableDelta);
			b.setSample(1, startSample, r);
		}

		jassert(voicePitchValues == nullptr || voicePitchValues[startSample] > 0.0f);

		const double delta = (uptimeDelta * (voicePitchValues == nullptr ? 1.0 : voicePitchValues[startSample]));

		voiceUptime += delta;

		++startSample;
	}
}

float WavetableSound::RenderData::calculateSample(const float* lowerTable, const float* upperTable, const span<int, 4>& i, float alpha, float tableAlpha) const
{
	float l0 = lowerTable[i[0]];
	float l1 = lowerTable[i[1]];
	float l2 = lowerTable[i[2]];
	float l3 = lowerTable[i[3]];

	float sample;

	if (lowerTable != upperTable)
	{
		float u0 = upperTable[i[0]];
		float u1 = upperTable[i[1]];
		float u2 = upperTable[i[2]];
		float u3 = upperTable[i[3]];

		float upperSample, lowerSample;

		if (hqMode)
		{
			upperSample = Interpolator::interpolateCubic(u0, u1, u2, u3, alpha);
			lowerSample = Interpolator::interpolateCubic(l0, l1, l2, l3, alpha);
		}
		else
		{
			upperSample = Interpolator::interpolateLinear(u1, u2, alpha);
			lowerSample = Interpolator::interpolateLinear(l1, l2, alpha);
		}

		sample = Interpolator::interpolateLinear(lowerSample, upperSample, tableAlpha);
	}
	else
	{
		sample = hqMode ? Interpolator::interpolateCubic(l0, l1, l2, l3, alpha) :
			Interpolator::interpolateLinear(l1, l2, alpha);
	}

	return sample;
}

void WavetableMonolithHeader::writeProjectInfo(OutputStream& output, const String& projectName, const String& encryptionKey)
{
	auto projectLength = projectName.length();

	if (encryptionKey.isNotEmpty())
	{
		BlowFish bf(encryptionKey.getCharPointer().getAddress(), encryptionKey.length());

		char buffer[512];
		memset(buffer, 0, 512);
		memcpy(buffer, projectName.getCharPointer().getAddress(), projectName.length());

		auto numEncrypted = bf.encrypt(buffer, projectLength, 512);
		output.writeBool(true);
		output.writeByte(numEncrypted);
		output.write(buffer, numEncrypted);
	}
	else
	{
		output.writeBool(false);
		output.writeByte(projectLength + 1);
		output.writeString(projectName);
	}
}

bool WavetableMonolithHeader::checkProjectName(InputStream& input, const String& projectName, const String& encryptionKey)
{
	auto shouldBeEncrypted = input.readBool();

	if (shouldBeEncrypted && encryptionKey.isEmpty())
		return false;

	char buffer[512];
	memset(buffer, 0, 512);
	String s;

	if (shouldBeEncrypted)
	{
		BlowFish bf(encryptionKey.getCharPointer().getAddress(), encryptionKey.length());
		
		auto numEncrypted = input.readByte();

		
		input.read(buffer, numEncrypted);

		auto numDecrypted = bf.decrypt(buffer, numEncrypted);

		s = String(buffer, numDecrypted);
	}
	else
	{
		auto length = input.readByte();
		input.read(buffer, length);
		s = String(buffer, length);
	}

	return projectName.compare(s) == 0;
}

Array<hise::WavetableMonolithHeader> WavetableMonolithHeader::readHeader(InputStream& input, const String& projectName, const String& encryptionKey)
{
	Array<WavetableMonolithHeader> headers;

	auto headerSize = input.readInt64();

	if (!checkProjectName(input, projectName, encryptionKey))
		return headers;

	char buffer[512];

	while (input.getPosition() < headerSize)
	{
		memset(buffer, 0, 512);
		auto numBytes = (uint8)input.readByte();
		input.read(buffer, numBytes);

		WavetableMonolithHeader header;

		header.name = String(buffer, numBytes);
		header.offset = input.readInt64();
		header.length = input.readInt64();
		headers.add(header);
	}

	return headers;
}

} // namespace hise
