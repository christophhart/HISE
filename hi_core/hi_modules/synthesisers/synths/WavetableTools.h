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

#ifndef WAVETABLETOOLS_H_INCLUDED
#define WAVETABLETOOLS_H_INCLUDED

namespace hise {
using namespace juce;

#if USE_BACKEND

class SampleMapToWavetableConverter : public SafeChangeBroadcaster,
									  public Spectrum2D::Holder
{
public:

    using WindowType = FFTHelpers::WindowType;
    
	enum class PhaseMode
	{
		Resample,
		ZeroPhase,
		StaticPhase,
		DynamicPhase
	};

	enum class PreviewNoise
	{
		Mute,
		Mix,
		Solo
	};

    struct WavetableIndex
	{
		bool operator==(const WavetableIndex& other) const
		{
			return noteNumber == other.noteNumber && sampleIndex == other.sampleIndex;
		}

		int sampleIndex = -1;
		int noteNumber = -1;
	};

	struct StoreData
	{
		WavetableIndex sample;
		Range<int> noteRange;

		AudioSampleBuffer dataBuffer;
		int numChannels = -1;
		ValueTree parent;
		double sampleRate = -1.0;
		int numParts = -1;
	};

	struct HarmonicMap
	{
		bool operator==(const WavetableIndex& indexToShow) const
		{
			return index.noteNumber == indexToShow.noteNumber && index.noteNumber == indexToShow.noteNumber;
		}

		bool operator ==(const HarmonicMap& other) const
		{
			return other.index == index;
		}

		HarmonicMap(int numSlices=64)
		{
			clear(numSlices);
		}

		void clear(int numSlices, int numHarmonics = 0);

		

        AudioSampleBuffer harmonicPhase;
        AudioSampleBuffer harmonicPhaseRight;
        
		AudioSampleBuffer harmonicGains;
		AudioSampleBuffer harmonicGainsRight;
		AudioSampleBuffer gainValues;
		WavetableIndex index;

		int rootNote = 0;
		HeapBlock<double> pitchDeviations;
		double lastFrequency = -1.0;
		int wavetableLength = 0;
		double sampleLengthSeconds = 0;
		bool isStereo = false;
		bool analysed = false;
		Range<int> noteRange;

		double fileSampleRate = 0.0;
		double lorisResynRatio = 1.0;
		AudioSampleBuffer lorisResynBuffer;

		AudioSampleBuffer noiseBuffer;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicMap);
	};

	class Preview;
	class SampleMapPreview;
	class StatisticPreview;

	SampleMapToWavetableConverter(ModulatorSynthChain* mainSynthChain);

	~SampleMapToWavetableConverter()
	{
		harmonicMaps.clear();
		waveTableTree = ValueTree();
	}

	
	void checkIfShouldExit();

    void parseSampleMap(const ValueTree& sampleMapTree);
    
	LambdaBroadcaster<Image*> spectrumBroadcaster;
	
	Spectrum2D::Parameters::Ptr s2dParameters;

	Spectrum2D::Parameters::Ptr getParameters() const override;

	ValueTree getValueTree()
	{
		return waveTableTree;
	}

	

	Image originalSpectrum;

	double sampleRate = 48000.0;
	int numParts = 64;
	bool reverseOrder = false;
	int mipmapSize = 12;
	bool useOriginalGain = true;
	bool channelToUse = 0;

	PreviewNoise previewNoise = PreviewNoise::Mute;

	SynthesiserSound::Ptr sound;

	void discardAllScans();

	void refreshCurrentWavetable(bool forceReanalysis = true);

	void setCurrentIndex(int index, NotificationType n);

	void moveCurrentSampleIndex(bool advance)
	{
		if (advance)
			currentIndex = jmin<int>(harmonicMaps.size() - 1, currentIndex + 1);
		else
			currentIndex = jmax<int>(0, currentIndex - 1);
	}

	int getCurrentNoteNumber() const;

	const float* getCurrentGainValues() const
	{
		if (currentIndex < harmonicMaps.size())
		{
			return harmonicMaps[currentIndex]->gainValues.getReadPointer(0);
		}
		else
			return nullptr;
	}

	AudioSampleBuffer getPreviewBuffers(bool original);

    void rebuild();
    
	static var getSampleProperty(const ValueTree& vt, const Identifier& id)
	{
		return vt.getProperty(id);
	}

    double offsetInSlice = 0.5;
    
	PhaseMode phaseMode = PhaseMode::Resample;

	int cycleLength = 0;
	bool exportAsHwt = true;
	bool useCompression = false;

	ThreadController::Ptr threadController;

	void exportAll();

	void setLogFunction(const std::function<void(const String&)>& f)
	{
		logFunction = f;
	}

	String getPrefixFromNoiseMode(int noteNumber) const;

	void setPreviewMode(PreviewNoise mode);

private:

	void rebuildPreviewBuffersInternal();

	AudioSampleBuffer previewBuffer, originalBuffer;

	AudioSampleBuffer removeHarmonicsAboveNyquistWithLoris(double ratio);

	float* getPhaseData(const HarmonicMap& map, int sliceIndex, bool getRight);
	AudioSampleBuffer calculateWavetableBank(const HarmonicMap& map, int noteNumber = -1);
	void applyNoiseBuffer(const HarmonicMap& m, AudioSampleBuffer& tonalSignal);

	std::function<void(const String&)> logFunction;

	void renderAllWavetablesFromSingleWavetables(int sampleIndex=-1);
	void renderAllWavetablesFromHarmonicMaps();

	int currentOuterStep = 0;
	int numOuterSteps = 0;

	HarmonicMap * getCurrentMap()
	{
		if (currentIndex < harmonicMaps.size())
			return harmonicMaps[currentIndex];

		return nullptr;
	}

	AudioSampleBuffer getResampledLorisBuffer(AudioSampleBuffer sourceBuffer, double r, int thisCycleLength, int realNoteNumber);

	void calculateHarmonicMap();

	

	int currentIndex = -1;

	OwnedArray<HarmonicMap> harmonicMaps;

	void storeData(StoreData& data);

	

	int getSampleIndexForNoteNumber(int noteNumber);

	void readSample(AudioSampleBuffer& buffer, int index, int noteNumber);

	ModulatorSynthChain* chain;

	ValueTree sampleMap;
	ValueTree waveTableTree;
	
	int currentSampleLength = 0;
	AudioFormatManager afm;
};

#endif

}

#endif
