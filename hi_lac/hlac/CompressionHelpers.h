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

namespace hlac {

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

		void normalisedInt16ToFloat(float* destination, const int16_t* src, int start, int numSamples) const;

		/** You can use a fixed amount of normalisation instead of splitting it into chunks of 1024 samples.
		*
		*	Doing so will create a compromise between file size and audio quality.
		*/
		void setUseStaticNormalisation(uint8_t staticNormalisationAmount);

		bool writeNormalisationHeader(juce::OutputStream& output);

		void setMode(uint8_t newMode)
		{
			normalisationMode = newMode;
			active = newMode > 0;
		}

		/** Sets 4 values at once. Used by the HLAC decoder. */
		void setNormalisationValues(int readOffset, int normalisedValues);

		/** This applies normalisation. */
		void normalise(const float* src, int16_t* dst, int numSamples);

		void allocateTableIndexes(int numSamples);

		void copyNormalisationTable(NormaliseMap& dst, int startSampleSource, int startSampleDst, int numSamples) const;

		void copyIntBufferWithNormalisation(const NormaliseMap& srcMap, const int16_t* srcWithoutOffset, int16_t* dstWithoutOffset, int startSampleSource, int startSampleDst, int numSamples, bool overwriteTable);

		int getOffset() const { return firstOffset; }

		/** If the buffers are not aligned to a 1024 sample boundary, you can set the first range length here in order to synchronise it. */
		void setOffset(int offsetToUse);

		void setThreshold(uint8_t newThreshhold)
		{
			minNormalisation = newThreshhold;
		}

	private:

		uint8_t minNormalisation = 0;

		friend class HiseSampleBuffer;

		int16_t size() const;

		void internalNormalisation(const float* src, int16_t* dst, int numSamples, uint8_t amount) const;

		const uint16_t getIndexForSamplePosition(int samplePosition, bool roundUp=false) const
		{
			if (roundUp)
			{
				auto value = std::ceil((double)samplePosition / (double)normaliseBlockSize);
				return (uint16_t)juce::roundToInt(value);
			}
			else
			{
				return (uint16_t)((samplePosition) / normaliseBlockSize);
			}

			
		}

		const uint8_t * getTableData() const
		{
			if (allocated != nullptr)
				return allocated;
			else
				return preallocated;
		}

		uint8_t * getTableData()
		{
			if (allocated != nullptr)
				return allocated;
			else
				return preallocated;
		}

		static constexpr int PreallocatedSize = 16;

		static constexpr int normaliseBlockSize = 1024;

		uint8_t normalisationMode = Mode::NoNormalisation;
		int firstOffset = 0;

		uint8_t preallocated[PreallocatedSize];
		uint16_t numAllocated = 0;
        juce::HeapBlock<uint8_t> allocated;
		bool active = false;
	};


	/** A normalized 16 bit integer buffer
	*
	*	The conversion between AudioSampleBuffers and this type will normalize the float values to retain the full 16bit range.
	*	The data is 16byte aligned.
	*/
	struct AudioBufferInt16
	{
		AudioBufferInt16(juce::AudioSampleBuffer& b, int channelToUse, uint8_t normalisationMode, uint8_t normalisationThreshold=0);
		AudioBufferInt16(int16_t* externalData_, int numSamples);
		AudioBufferInt16(const int16_t* externalData_, int numSamples);
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

		~AudioBufferInt16();
        juce::AudioSampleBuffer getFloatBuffer() const;

		void reverse(int startSample, int numSamples);
		void negate();
		void applyGainRamp(int startOffset, int rampLength, float startGain, float endGain);

		int16_t* getWritePointer(int startSample = 0);
		const int16_t* getReadPointer(int startSample = 0) const;

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
		int16_t* data = nullptr;
		int16_t* externalData = nullptr;

		NormaliseMap map;
	};

	/** Loads a file into a AudioSampleBuffer. */
	static juce::AudioSampleBuffer loadFile(const juce::File& f, double& speed);;

	static float getFLACRatio(const juce::File& f, double& speed);

	/** returns the lowest bit rate needed to store the data without losses. */
	static uint8_t getPossibleBitReductionAmount(const AudioBufferInt16& b);

	/** returns the amount of blocks in the buffer using the global compression block size. */
	static int getBlockAmount (juce::AudioSampleBuffer& b);

	/** A few operations on blocks of signed 16 bit integer data. */
	struct IntVectorOperations
	{
		/** Copies the values from src1 into dst, and calculates the normalized difference. */
		static void sub(int16_t* dst, const int16_t* src1, const int16_t* src2, int numValues);

		/** dst = dst - src inplace. */
		static void sub(int16_t* dst, const int16_t* src, int numValues);

		/** Adds the values from src to dst. */
		static void add(int16_t* dst, const int16_t* src, int numSamples);

		static void mul(int16_t* dst, const int16_t value, int numSamples);

		static void div(int16_t* dst, const int16_t value, int numSamples);

		/** Removes the dc offset and returns the value. */
		static int16_t removeDCOffset(int16_t* data, int numValues);

		/** Returns the absolute max value in the data block. */
		static int16_t max(const int16_t* d, int numValues);

		/** Clears the data (sets it to zero). */
		static void clear(int16_t* d, int numValues);
	};

	/** Gets the possible bit reduction amount for the next cycle with the given cycleLength. 
	*
	*	Don't use this directly, but use getCycleLengthWithLowestBitrate() instead. */
	static uint8_t getBitrateForCycleLength(const AudioBufferInt16& block, int cycleLength, AudioBufferInt16& workBuffer);

	static void normaliseBlock(int16_t* data, int numSamples, int normalisationAmount, int direction, bool applyDither=false);

	/** Get the cycle length the yields the lowest bit rate for the next cycle and store the bitrate in bitRate. */
	static int getCycleLengthWithLowestBitRate(const AudioBufferInt16& block, int& bitRate, AudioBufferInt16& workBuffer);

	/** calculates the max bit reduction when applying the last cycle. */
	static uint8_t getBitReductionWithTemplate(AudioBufferInt16& lastCycle, AudioBufferInt16& nextCycle, bool removeDc);

	/** Return a section b as new AudioBufferInt16 without allocating. */
	static AudioBufferInt16 getPart(AudioBufferInt16& b, int startIndex, int numSamples);

	/** Return a section b as new AudioBufferInt16 without allocating. */
	static AudioBufferInt16 getPart(const AudioBufferInt16& b, int startIndex, int numSamples);

	struct Misc
	{
		static uint64_t NumberOfSetBits(uint64_t i);

		static uint8_t getSampleRateIndex(double sampleRate);

		static uint32_t createChecksum();

		static bool validateChecksum(uint32_t data);
	};

	static int getPaddedSampleSize(int samplesNeeded);

	static uint8_t getBitReductionForDifferential(AudioBufferInt16& b);

	static int getByteAmountForDifferential(AudioBufferInt16& b);

	static void dump(const AudioBufferInt16& b, juce::String fileName = juce::String());

	static void dump(const juce::AudioSampleBuffer& b, juce::String fileName = juce::String());

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
		static void distributeFullSamples(AudioBufferInt16& dst, const uint16_t* fullSamplesPacked, int numSamples);

		static void addErrorSignal(AudioBufferInt16& dst, const uint16_t* errorSignalPacked, int numSamples);

		

	};

	static uint8_t checkBuffersEqual(juce::AudioSampleBuffer& workBuffer, juce::AudioSampleBuffer& referenceBuffer);

	static juce::AudioSampleBuffer getPart(HiseSampleBuffer& b, int startIndex, int numSamples);

	/** Return a section b as new AudioSampleBuffer without allocating. */
	static juce::AudioSampleBuffer getPart(juce::AudioSampleBuffer& b, int startIndex, int numSamples);

	static juce::AudioSampleBuffer getPart(juce::AudioSampleBuffer& b, int channelIndex, int startIndex, int numSamples);
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
		numFlags
	};

	struct CompressData
	{
        juce::Array<juce::File> fileList;
        juce::File optionalHeaderFile;
        juce::File targetFile;
        juce::String metadataJSON;
		int64_t partSize = -1;
		double* progress = nullptr;
		double* totalProgress = nullptr;
	};

	struct DecompressData
	{
		OverwriteOption option;
		bool supportFullDynamics = false;
        juce::File sourceFile;
        juce::File targetDirectory;
		double* progress = nullptr;
		double* partProgress = nullptr;
		double* totalProgress = nullptr;
		bool debugLogMode = false;

	};

	HlacArchiver(juce::Thread* threadToUse) :
		thread(threadToUse)
	{}

	struct Listener
	{
        virtual ~Listener() {};
        
		virtual void logStatusMessage(const juce::String& message) = 0;

		virtual void logVerboseMessage(const juce::String& verboseMessage) = 0;

		virtual void criticalErrorOccured(const juce::String& message) = 0;
	};

	/** Extracts the compressed data from the given file. */
	bool extractSampleData(const DecompressData& data);

	/** Compressed the given data using the supplied Thread. */
	void compressSampleData(const CompressData& data);

	static juce::String getMetadataJSON(const juce::File& sourceFile);

    juce::var readMetadataFromArchive(const juce::File& archiveFile);

	void setListener(Listener* l)
	{
		listener = l;
	}

private:

    juce::FileInputStream* writeTempFile(juce::AudioFormatReader* reader, int bitDepth=16);

	Listener* listener = nullptr;

    juce::String getFlagName(Flag f);

    juce::File getPartFile(const juce::File& originalFile, int partIndex);

	bool writeFlag(juce::FileOutputStream* fos, Flag flag);

	bool readAndCheckFlag(juce::FileInputStream* fis, Flag flag);

	Flag readFlag(juce::FileInputStream* fis);

	Flag currentFlag = Flag::numFlags;

	bool decompressMode = false;

    juce::Thread* thread = nullptr;
	
	
    juce::File tmpFile;

	double deltaPerFile = 0.1;
	double fileProgress = 0.0;

	CompressData cData;
	
	double* progress = nullptr;
	

};


} // namespace hlac

#endif  // COMPRESSIONHELPERS_H_INCLUDED
