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


#ifndef COMPRESSIONHELPERS_H_INCLUDED
#define COMPRESSIONHELPERS_H_INCLUDED

namespace hlac { using namespace juce; 

#define DUMP(x) CompressionHelpers::dump(x);

#if HLAC_DEBUG_LOG
#define LOG(x) DBG(x)
#else
#define LOG(x)
#endif

class HiseSampleBuffer;

struct CompressionHelpers
{

	/** This structure will hold the information about the normalization amount needed to be applied to the 16 bit signal. */
	struct NormaliseMap
	{
		enum Mode
		{
			NoNormalisation = 0,
			StaticNormalisation,
			RangeBasedNormalisation,
			numModes
		};

		NormaliseMap()
		{
			memset(preallocated, 0, 16);
		}

		void normalisedInt16ToFloat(float* destination, const int16* src, int start, int numSamples) const;

		/** You can use a fixed amount of normalisation instead of splitting it into chunks of 1024 samples.
		*
		*	Doing so will create a compromise between file size and audio quality.
		*/
		void setUseStaticNormalisation(uint8 staticNormalisationAmount);

		bool writeNormalisationHeader(OutputStream& output);

		void setMode(uint8 newMode)
		{
			normalisationMode = newMode;
			active = newMode > 0;
		}

		/** Sets 4 values at once. Used by the HLAC decoder. */
		void setNormalisationValues(int readOffset, int normalisedValues);

		/** This applies normalisation. */
		void normalise(const float* src, int16* dst, int numSamples);

		void allocateTableIndexes(int numSamples);

		void copyNormalisationTable(NormaliseMap& dst, int startSampleSource, int startSampleDst, int numSamples) const;

		void copyIntBufferWithNormalisation(const NormaliseMap& srcMap, const int16* srcWithoutOffset, int16* dstWithoutOffset, int startSampleSource, int startSampleDst, int numSamples, bool overwriteTable);

		int getOffset() const { return firstOffset; }

		/** If the buffers are not aligned to a 1024 sample boundary, you can set the first range length here in order to synchronise it. */
		void setOffset(int offsetToUse);

		void setThreshold(uint8 newThreshhold)
		{
			minNormalisation = newThreshhold;
		}

	private:

		uint8 minNormalisation = 0;

		friend class HiseSampleBuffer;

		int16 size() const;

		void internalNormalisation(const float* src, int16* dst, int numSamples, uint8 amount) const;

		const uint16 getIndexForSamplePosition(int samplePosition, bool roundUp=false) const
		{
			if (roundUp)
			{
				auto value = std::ceil((double)samplePosition / (double)normaliseBlockSize);
				return (uint16)roundToInt(value);
			}
			else
			{
				return (uint16)((samplePosition) / normaliseBlockSize);
			}

			
		}

		const uint8 * getTableData() const
		{
			if (allocated != nullptr)
				return allocated;
			else
				return preallocated;
		}

		uint8 * getTableData()
		{
			if (allocated != nullptr)
				return allocated;
			else
				return preallocated;
		}

		static constexpr int PreallocatedSize = 16;

		static constexpr int normaliseBlockSize = 1024;

		uint8 normalisationMode = Mode::NoNormalisation;
		int firstOffset = 0;

		uint8 preallocated[PreallocatedSize];
		uint16 numAllocated = 0;
		HeapBlock<uint8> allocated;
		bool active = false;
	};


	/** A normalized 16 bit integer buffer
	*
	*	The conversion between AudioSampleBuffers and this type will normalize the float values to retain the full 16bit range.
	*	The data is 16byte aligned.
	*/
	struct AudioBufferInt16
	{
		AudioBufferInt16(AudioSampleBuffer& b, int channelToUse, uint8 normalisationMode, uint8 normalisationThreshold=0);
		AudioBufferInt16(int16* externalData_, int numSamples);
		AudioBufferInt16(const int16* externalData_, int numSamples);
		AudioBufferInt16(int size_=0);

		AudioBufferInt16(AudioBufferInt16&& other)
		{
			deAllocate();

			size = other.size;
			data = other.data;

			other.data = nullptr;

			isReadOnly = other.isReadOnly;
			externalData = other.externalData;
			map = std::move(other.map);
		}

		AudioBufferInt16& operator= (AudioBufferInt16&& other)
		{
			deAllocate();

			size = other.size;
			data = other.data;
			other.data = nullptr;
			isReadOnly = other.isReadOnly;
			externalData = other.externalData;
			map = std::move(other.map);

			return *this;
		}

		~AudioBufferInt16();;
		AudioSampleBuffer getFloatBuffer() const;

		void reverse(int startSample, int numSamples);
		void negate();
		void applyGainRamp(int startOffset, int rampLength, float startGain, float endGain);

		int16* getWritePointer(int startSample = 0);
		const int16* getReadPointer(int startSample = 0) const;

		int size = 0;

		const NormaliseMap& getMap() const { return map; }
		NormaliseMap& getMap() { return map; }

		/** Copies the samples from the int buffers and also copies the normalisation tables. */
		static void copyWithNormalisation(AudioBufferInt16& dst, const AudioBufferInt16& source, int startSampleDst, int startSampleSrc, int numSamples, bool overwriteTable)
		{

			dst.getMap().copyIntBufferWithNormalisation(source.getMap(), source.getReadPointer(), dst.getWritePointer(), startSampleSrc, startSampleDst, numSamples, overwriteTable);
		}

	private:

		void allocate(int newNumSamples);
		void deAllocate();

		bool isReadOnly = false;
		int16* data = nullptr;
		int16* externalData = nullptr;

		NormaliseMap map;
	};

	/** Loads a file into a AudioSampleBuffer. */
	static AudioSampleBuffer loadFile(const File& f, double& speed, double* sampleRatePtr=nullptr);;

	static float getFLACRatio(const File& f, double& speed);

	/** returns the lowest bit rate needed to store the data without losses. */
	static uint8 getPossibleBitReductionAmount(const AudioBufferInt16& b);

	/** returns the amount of blocks in the buffer using the global compression block size. */
	static int getBlockAmount(AudioSampleBuffer& b);

	/** A few operations on blocks of signed 16 bit integer data. */
	struct IntVectorOperations
	{
		/** Copies the values from src1 into dst, and calculates the normalized difference. */
		static void sub(int16* dst, const int16* src1, const int16* src2, int numValues);

		/** dst = dst - src inplace. */
		static void sub(int16* dst, const int16* src, int numValues);

		/** Adds the values from src to dst. */
		static void add(int16* dst, const int16* src, int numSamples);

		static void mul(int16*dst, const int16 value, int numSamples);

		static void div(int16* dst, const int16 value, int numSamples);

		/** Removes the dc offset and returns the value. */
		static int16 removeDCOffset(int16* data, int numValues);

		/** Returns the absolute max value in the data block. */
		static int16 max(const int16* d, int numValues);

		/** Clears the data (sets it to zero). */
		static void clear(int16* d, int numValues);
	};

	/** Gets the possible bit reduction amount for the next cycle with the given cycleLength. 
	*
	*	Don't use this directly, but use getCycleLengthWithLowestBitrate() instead. */
	static uint8 getBitrateForCycleLength(const AudioBufferInt16& block, int cycleLength, AudioBufferInt16& workBuffer);

	static void normaliseBlock(int16* data, int numSamples, int normalisationAmount, int direction, bool applyDither=false);

	/** Get the cycle length the yields the lowest bit rate for the next cycle and store the bitrate in bitRate. */
	static int getCycleLengthWithLowestBitRate(const AudioBufferInt16& block, int& bitRate, AudioBufferInt16& workBuffer);

	/** calculates the max bit reduction when applying the last cycle. */
	static uint8 getBitReductionWithTemplate(AudioBufferInt16& lastCycle, AudioBufferInt16& nextCycle, bool removeDc);

	/** Return a section b as new AudioBufferInt16 without allocating. */
	static AudioBufferInt16 getPart(AudioBufferInt16& b, int startIndex, int numSamples);

	/** Return a section b as new AudioBufferInt16 without allocating. */
	static AudioBufferInt16 getPart(const AudioBufferInt16& b, int startIndex, int numSamples);

	struct Misc
	{
		static uint64 NumberOfSetBits(uint64 i);

		static uint8 getSampleRateIndex(double sampleRate);

		static uint32 createChecksum();

		static bool validateChecksum(uint32 data);
	};

	static int getPaddedSampleSize(int samplesNeeded);

	static uint8 getBitReductionForDifferential(AudioBufferInt16& b);

	static int getByteAmountForDifferential(AudioBufferInt16& b);

	static void dump(const AudioBufferInt16& b, String fileName=String());

	static void dump(const AudioSampleBuffer& b, String fileName = String(), double sampleRate = 44100, int bitDepth = 16);

	static void fastInt16ToFloat(const void* source, float* destination, int numSamples);

	static void applyDithering(float* data, int numSamples);

	struct Diff
	{
		static int getNumFullValues(int bufferSize);
		static int getNumErrorValues(int bufferSize);


		static AudioBufferInt16 createBufferWithFullValues(const AudioBufferInt16& b);

		static AudioBufferInt16 createBufferWithErrorValues(const AudioBufferInt16& b, const AudioBufferInt16& packedFullValues);

		/** Applies 4x downsampling with linear interpolation for the given buffer.
		*
		*	The buffer size must be a multiple of 4. The last quadruple uses the first 
		*	and fourth value for downsampling (so that the buffer can be used without storing
		*	intermediate values.
		*/
		static void downSampleBuffer(AudioBufferInt16& b);

		/** Distributes the given samples from fullSamplesPacked to the destination buffer. */
		static void distributeFullSamples(AudioBufferInt16& dst, const uint16* fullSamplesPacked, int numSamples);

		static void addErrorSignal(AudioBufferInt16& dst, const uint16* errorSignalPacked, int numSamples);

		

	};

	static uint8 checkBuffersEqual(AudioSampleBuffer& workBuffer, AudioSampleBuffer& referenceBuffer);

	static AudioSampleBuffer getPart(HiseSampleBuffer& b, int startIndex, int numSamples);

	/** Return a section b as new AudioSampleBuffer without allocating. */
	static AudioSampleBuffer getPart(AudioSampleBuffer& b, int startIndex, int numSamples);

	static AudioSampleBuffer getPart(AudioSampleBuffer& b, int channelIndex, int startIndex, int numSamples);
};

class HlacMemoryMappedAudioFormatReader;


/** This helper class compresses a list of HLAC files into a big FLAC chunk. */
struct HlacArchiver
{
	enum class OverwriteOption
	{
		OverwriteIfNewer = 0,
		DontOverwrite,
		ForceOverwrite,
		numOverwriteOptions
	};

	enum class Flag
	{
		BeginMetadata,
		EndMetadata,
		BeginName,
		EndName,
		BeginTime,
		EndTime,
		BeginMonolithLength,
		EndMonolithLength,
		BeginMonolith,
		EndMonolith,
		SplitMonolith,
		ResumeMonolith,
		EndOfArchive,
		BeginHeaderFile,
		EndHeaderFile,
		BeginAdditionalFile,
		EndAdditionalFile,
		numFlags
	};

	struct CompressData
	{
		Array<File> fileList;
		File optionalHeaderFile;
		Array<File> additionalFiles;
		File targetFile;
		String metadataJSON;
		int64 partSize = -1;
		double* progress = nullptr;
		double* totalProgress = nullptr;
	};

	struct DecompressData
	{
		OverwriteOption option;
		bool supportFullDynamics = false;
		File sourceFile;
		File targetDirectory;
		double* progress = nullptr;
		double* partProgress = nullptr;
		double* totalProgress = nullptr;
		bool debugLogMode = false;

	};

	HlacArchiver(Thread* threadToUse) :
		thread(threadToUse)
	{}

	struct Listener
	{
        virtual ~Listener() {};
        
		virtual void logStatusMessage(const String& message) = 0;

		virtual void logVerboseMessage(const String& verboseMessage) = 0;

		virtual void criticalErrorOccured(const String& message) = 0;
	};

	/** Extracts the compressed data from the given file. */
	bool extractSampleData(const DecompressData& data);

	/** Compressed the given data using the supplied Thread. */
	void compressSampleData(const CompressData& data);

	static String getMetadataJSON(const File& sourceFile);

	var readMetadataFromArchive(const File& archiveFile);

	void setListener(Listener* l)
	{
		listener = l;
	}

private:

	FileInputStream* writeTempFile(AudioFormatReader* reader, int bitDepth=16);

	Listener* listener = nullptr;

	String getFlagName(Flag f);

	File getPartFile(const File& originalFile, int partIndex);

	bool writeFlag(FileOutputStream* fos, Flag flag);

	bool readAndCheckFlag(FileInputStream* fis, Flag flag);

	Flag readFlag(FileInputStream* fis);

	Flag currentFlag = Flag::numFlags;

	bool decompressMode = false;

	Thread* thread = nullptr;
	
	
	File tmpFile;

	double deltaPerFile = 0.1;
	double fileProgress = 0.0;

	CompressData cData;
	
	double* progress = nullptr;
	

};


} // namespace hlac

#endif  // COMPRESSIONHELPERS_H_INCLUDED
