/*  HISE Lossless Audio Codec
*	�2017 Christoph Hart
*
*	Redistribution and use in source and binary forms, with or without modification,
*	are permitted provided that the following conditions are met:
*
*	1. Redistributions of source code must retain the above copyright notice,
*	   this list of conditions and the following disclaimer.
*
*	2. Redistributions in binary form must reproduce the above copyright notice,
*	   this list of conditions and the following disclaimer in the documentation
*	   and/or other materials provided with the distribution.
*
*	3. All advertising materials mentioning features or use of this software must
*	   display the following acknowledgement:
*	   This product includes software developed by Hart Instruments
*
*	4. Neither the name of the copyright holder nor the names of its contributors may be used
*	   to endorse or promote products derived from this software without specific prior written permission.
*
*	THIS SOFTWARE IS PROVIDED BY CHRISTOPH HART "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
*	BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*	DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
*	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
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

	/** A normalized 16 bit integer buffer
	*
	*	The conversion between AudioSampleBuffers and this type will normalize the float values to retain the full 16bit range.
	*	The data is 16byte aligned.
	*/
	struct AudioBufferInt16
	{
		AudioBufferInt16(AudioSampleBuffer& b, int channelToUse, bool normalizeBeforeStoring);
		AudioBufferInt16(int16* externalData_, int numSamples);
		AudioBufferInt16(const int16* externalData_, int numSamples);
		AudioBufferInt16(int size_=0);

		

		AudioBufferInt16(AudioBufferInt16&& other)
		{
			deAllocate();

			size = other.size;
			data = other.data;

			other.data = nullptr;

			gainFactor = other.gainFactor;
			isReadOnly = other.isReadOnly;
			externalData = other.externalData;


		}

		AudioBufferInt16& operator= (AudioBufferInt16&& other)
		{
			deAllocate();

			size = other.size;
			data = other.data;
			other.data = nullptr;
			gainFactor = other.gainFactor;
			isReadOnly = other.isReadOnly;
			externalData = other.externalData;

			return *this;
		}

		~AudioBufferInt16();;

		AudioSampleBuffer getFloatBuffer() const;

		void reverse(int startSample, int numSamples);

		void negate();

		int16* getWritePointer(int startSample = 0);
		const int16* getReadPointer(int startSample = 0) const;

		int size = 0;
		float gainFactor = 1.0f;

	private:

		void allocate(int newNumSamples);
		void deAllocate();

		bool isReadOnly = false;
		int16* data = nullptr;

		int16* externalData = nullptr;
	};

	/** Loads a file into a AudioSampleBuffer. */
	static AudioSampleBuffer loadFile(const File& f, double& speed);;

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

	static void dump(const AudioBufferInt16& b);

	static void dump(const AudioSampleBuffer& b);

	static void fastInt16ToFloat(const void* source, float* destination, int numSamples);

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
		numFlags
	};

	struct CompressData
	{
		Array<File> fileList;
		File targetFile;
		String metadataJSON;
		int64 partSize = -1;
		double* progress = nullptr;
		double* totalProgress = nullptr;
	};

	struct DecompressData
	{
		OverwriteOption option;
		File sourceFile;
		File targetDirectory;
		double* progress = nullptr;
		double* partProgress = nullptr;
		double* totalProgress = nullptr;
	};

	HlacArchiver(Thread* threadToUse) :
		thread(threadToUse)
	{}

	struct Listener
	{
        virtual ~Listener() {};
        
		virtual void logStatusMessage(const String& message) = 0;

		virtual void logVerboseMessage(const String& verboseMessage) = 0;
	};

	/** Extracts the compressed data from the given file. */
	bool extractSampleData(const DecompressData& data);

	/** Compressed the given data using the supplied Thread. */
	void compressSampleData(const CompressData& data);

	static String getMetadataJSON(const File& sourceFile);

	void setListener(Listener* l)
	{
		listener = l;
	}

private:

	FileInputStream* writeTempFile(AudioFormatReader* reader);

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
