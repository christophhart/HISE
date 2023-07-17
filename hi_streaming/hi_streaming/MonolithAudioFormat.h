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


#ifndef MONOLITHAUDIOFORMAT_H_INCLUDED
#define MONOLITHAUDIOFORMAT_H_INCLUDED

namespace hise { using namespace juce;

#define USE_OLD_MONOLITH_FORMAT 0

#ifndef USE_FALLBACK_READERS_FOR_MONOLITH
#if JUCE_64BIT && !JUCE_IOS
#define USE_FALLBACK_READERS_FOR_MONOLITH 0
#else
#define USE_FALLBACK_READERS_FOR_MONOLITH 1
#endif
#endif

#define DECLARE_ID(x) static const Identifier x(#x);

namespace MonolithIds
{
	DECLARE_ID(MonolithSplitAmount);
	DECLARE_ID(MonolithSplitIndex);
	DECLARE_ID(MonolithReference);
	DECLARE_ID(MonolitSplitParts);
	DECLARE_ID(MonolithLength);
	DECLARE_ID(MonolithOffset);
	DECLARE_ID(FileName);
	DECLARE_ID(SampleRate);
}

struct MonolithFileReference
{
	using FileNotFoundFunction = std::function<void(const File& f)>;

	enum class FileNotFoundBehaviour
	{
		ThrowException,
		DoNothing,
		numFileNotFoundBehaviours
	};

	MonolithFileReference(const ValueTree& v);
	MonolithFileReference(int numChannels_, int numParts_);;
	MonolithFileReference(const File& monolithFile, int numChannels_, int numParts_);

	bool isUsingMonolith() const;

	bool isMultimic() const noexcept { return numChannels > 1; };
	bool useSplitIndex() const noexcept { return numParts > 0; };
	int getNumMicPositions() const { return numChannels; }
	int getNumSplitParts() const { return numParts; }

	static juce_wchar getCharForSplitPart(int partIndex);
	static int getSplitPartFromChar(juce_wchar splitChar);
	static String getFileExtensionPrefix();

	static String getIdFromValueTree(const ValueTree& v);

	File getFile(bool checkIfFileExists);

	Array<File> getAllFiles();

	bool bumpToNextMonolith(bool allowChannelBump);

	void addSampleDirectory(const File& f) { sampleRoots.addIfNotAlreadyThere(f); }
	
	void setNumSplitPartsToCurrentIndex();

	String referenceString;
	int channelIndex = 0;
	int partIndex = 0;

	void setFileNotFoundBehaviour(FileNotFoundBehaviour fb) { fileNotFoundBehaviour = fb; }

private:

	FileNotFoundBehaviour fileNotFoundBehaviour = FileNotFoundBehaviour::ThrowException;

	Array<File> sampleRoots;
	int numParts = 0;
	int numChannels = 1;
	bool isMonolith = true;
};

#undef DECLARE_ID

/** This class will manage the HLAC files that correspond to a samplemap. 

*/
struct HlacMonolithInfo : public ReferenceCountedObject
{
public:

	bool operator==(const Identifier& sampleMapId) const;

	HlacMonolithInfo(const Array<File>& monolithicFiles_);

	~HlacMonolithInfo();

	void fillMetadataInfo(const ValueTree& sampleMap);

	String getFileName(int channelIndex, int sampleIndex) const;

	int64 getMonolithOffset(int sampleIndex) const;

	int getNumSamplesInMonolith() const;

	int64 getMonolithLength(int sampleIndex) const;

	double getMonolithSampleRate(int sampleIndex) const;

	/** This will create a reader for the given sample and channel. */
	AudioFormatReader* createReader(int sampleIndex, int channelIndex);

	/** Use this for UI rendering stuff to avoid multithreading issues. */
	AudioFormatReader* createUserInterfaceReader(int sampleIndex, int channelIndex);

	using Ptr = ReferenceCountedObjectPtr<HlacMonolithInfo>;

private:

	int getFileIndex(int channelIndex, int sampleIndex) const;

	File getFile(int channelIndex, int sampleIndex) const;

	struct SampleInfo
	{
		double sampleRate;
		int64 length;
		int64 start;
		int splitIndex = 0;
		StringArray fileNames;
	};

	AudioFormatReader* createMonolithicReader(int sampleIndex, int channelIndex);
	AudioFormatReader* createFallbackReader(int sampleIndex, int channelIndex);

	Identifier id;

	hlac::HiseLosslessAudioFormat hlaf;

	std::vector<SampleInfo> sampleInfo;

	std::vector<File> monolithicFiles;

	int numChannels = 0;
	int numSplitFiles = 0;

	OwnedArray<hlac::HiseLosslessAudioFormatReader> fallbackReaders;
	OwnedArray<hlac::HlacMemoryMappedAudioFormatReader> memoryReaders;
};



} // namespace hise
#endif  // MONOLITHAUDIOFORMAT_H_INCLUDED
