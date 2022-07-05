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

/** Converts a directory containing the individual wavetable files into a ValueTree that can be loaded by a WavetableSynth.
*
*	Simply create one of these on the stack and call convertDirectoryToValueTree.
*	It will create a ValueTree with the following structure:
*
*		<wavetableData>
*			<wavetable noteNumber="X" amount="64" data="dataAsFloat" sampleRate="48000.0"/>
*			...
*		</wavetableData>
*
*	It assumes 48kHz mono files as input.
*
*	@param directoryPath the directory where the samples are. It assumes the following file name convenience:
*						 NOTENAME_WAVETABLEINDEX.wav - with NOTENAME base octave 3 and WAVETABLEINDEX 0 - 63
*/
class WavetableConverter
{
public:

	WavetableConverter();

	ValueTree convertDirectoryToWavetableData(const String &directoryPath_);


	static int getWavetableLength(int noteNumber, double sampleRate);;
	static int getMidiNoteNumber(const String &midiNoteName);

private:

	ScopedPointer<MemoryMappedAudioFormatReader> reader;
	File directoryPath;
	ValueTree data;
	OwnedArray<AudioSampleBuffer> dataBuffers;
	Array<int> wavetableSizes;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableConverter)
};

class SampleMapToWavetableConverter : public SafeChangeBroadcaster
{
public:

    using WindowType = FFTHelpers::WindowType;
    
	struct WavetableIndex
	{
		bool operator==(const WavetableIndex& other) const
		{
			return noteNumber == other.noteNumber && sampleIndex == other.sampleIndex;
		}

		int sampleIndex = -1;
		int noteNumber = -1;
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

		const int numWavetables = 64;

		HarmonicMap()
		{
			clear();
		}

		void clear(int numHarmonics = 0);

		void replaceWithNeighbours(int harmonicIndex);

		void replaceInternal(int harmonicIndex, bool useRightChannel);

		AudioSampleBuffer harmonicGains;
		AudioSampleBuffer harmonicGainsRight;
		AudioSampleBuffer gainValues;
		WavetableIndex index;

		double pitchDeviations[64];
		double lastFrequency = -1.0;
		int wavetableLength = 0;
		bool analysed = false;
	};

	class Preview;
	class SampleMapPreview;

	SampleMapToWavetableConverter(ModulatorSynthChain* mainSynthChain);

	~SampleMapToWavetableConverter()
	{
		harmonicMaps.clear();
		leftValueTree = ValueTree();
	}

	Result parseSampleMap(const File& sampleMapFile);

    Result parseSampleMap(const ValueTree& sampleMapTree);
    
	ValueTree getValueTree(bool getLeftTree)
	{
		return getLeftTree ? leftValueTree : rightValueTree;
	}

	void renderAllWavetablesFromSingleWavetables(double& progress);

	void renderAllWavetablesFromHarmonicMaps(double& progress);

	AudioSampleBuffer calculateWavetableBank(const HarmonicMap& map);

	double sampleRate = 48000.0;
	int numParts = 64;
	int fftSize = -1;
	bool reverseOrder = true;
	bool useOriginalGain = true;
	bool channelToUse = 0;
    WindowType windowType = FFTHelpers::FlatTop;

	Result refreshCurrentWavetable(double& progress, bool forceReanalysis = true);

	void moveCurrentSampleIndex(bool advance)
	{
		if (advance)
			currentIndex = jmin<int>(harmonicMaps.size() - 1, currentIndex + 1);
		else
			currentIndex = jmax<int>(0, currentIndex - 1);
	}

	void replacePartOfCurrentMap(int index);
	int getCurrentNoteNumber() const;

	Result discardCurrentNoteAndUsePrevious();

	const float* getCurrentGainValues() const
	{
		if (currentIndex < harmonicMaps.size())
		{
			return harmonicMaps.getReference(currentIndex).gainValues.getReadPointer(0);
		}
		else
			return nullptr;
	}

	AudioSampleBuffer getPreviewBuffers(bool original);

	static var getSampleProperty(const ValueTree& vt, const Identifier& id)
	{
		return vt.getProperty(id);
	}

private:

	HarmonicMap * getCurrentMap()
	{
		if (currentIndex < harmonicMaps.size())
			return &harmonicMaps.getReference(currentIndex);

		return nullptr;
	}

	int getLowestPossibleFFTSize() const;

	Result calculateHarmonicMap();

	int currentIndex = 0;

	Array<HarmonicMap> harmonicMaps;


	void storeData(int noteNumber, float* data, ValueTree& treeToSave, int length, int numPartsToUse=-1);

	

	int getSampleIndexForNoteNumber(int noteNumber);

	Result readSample(AudioSampleBuffer& buffer, int index, int noteNumber);

	Array<AudioSampleBuffer> splitSample(const AudioSampleBuffer& buffer);

	Result loadSampleMapFromFile(File sampleMapFile);


	ModulatorSynthChain* chain;

	ValueTree sampleMap;

	ValueTree leftValueTree;
	ValueTree rightValueTree;

	int currentSampleLength = 0;

	AudioFormatManager afm;
};

#endif

}

#endif
