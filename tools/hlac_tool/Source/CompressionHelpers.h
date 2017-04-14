/*
  ==============================================================================

    CompressionHelpers.h
    Created: 12 Apr 2017 10:16:50pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef COMPRESSIONHELPERS_H_INCLUDED
#define COMPRESSIONHELPERS_H_INCLUDED


#include "JuceHeader.h"
#include "BitCompressors.h"

#define COMPRESSION_BLOCK_SIZE 4096

struct CompressionHelpers
{

	/** A normalized 16 bit integer buffer
	*
	*	The conversion between AudioSampleBuffers and this type will normalize the float values to retain the full 16bit range.
	*/
	struct AudioBufferInt16
	{
		AudioBufferInt16(AudioSampleBuffer& b, bool normalizeBeforeStoring);
		AudioBufferInt16(int16* externalData_, int numSamples);
		AudioBufferInt16(const int16* externalData_, int numSamples);
		AudioBufferInt16(size_t size_);

		AudioSampleBuffer getFloatBuffer() const;

		void negate();

		int16* getWritePointer(int startSample = 0);
		const int16* getReadPointer(int startSample = 0) const;

		size_t size;
		float gainFactor;

	private:

		bool isReadOnly = false;
		HeapBlock<int16> data;
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
		static void sub(int16* dst, const int16* src1, const int16* src2, size_t numValues);

		/** Adds the values from src to dst. */
		static void add(int16* dst, const int16* src, int numSamples);

		/** Removes the dc offset and returns the value. */
		static int16 removeDCOffset(int16* data, size_t numValues);

		/** Returns the absolute max value in the data block. */
		static int16 max(const int16* d, size_t numValues);
	};

	/** Gets the possible bit reduction amount for the next cycle with the given cycleLength. 
	*
	*	Don't use this directly, but use getCycleLengthWithLowestBitrate() instead. */
	static uint8 getBitrateForCycleLength(const AudioBufferInt16& block, uint16 cycleLength, AudioBufferInt16& workBuffer);

	/** Get the cycle length the yields the lowest bit rate for the next cycle and store the bitrate in bitRate. */
	static uint16 getCycleLengthWithLowestBitRate(const AudioBufferInt16& block, uint8& bitRate, AudioBufferInt16& workBuffer);

	/** calculates the max bit reduction when applying the last cycle. */
	static uint8 getBitReductionWithTemplate(AudioBufferInt16& lastCycle, AudioBufferInt16& nextCycle, bool removeDc);

	/** Return a section b as new AudioBufferInt16 without allocating. */
	static AudioBufferInt16 getPart(AudioBufferInt16& b, int startIndex, int numSamples);

	/** Return a section b as new AudioBufferInt16 without allocating. */
	static AudioBufferInt16 getPart(const AudioBufferInt16& b, int startIndex, int numSamples);

	static uint8 getBitReductionForDifferential(AudioBufferInt16& b);

	static uint16 getByteAmountForDifferential(AudioBufferInt16& b);

	static float getDifference(AudioSampleBuffer& workBuffer, const AudioSampleBuffer& referenceBuffer);

	/** Return a section b as new AudioSampleBuffer without allocating. */
	static AudioSampleBuffer getPart(AudioSampleBuffer& b, int startIndex, int numSamples);
};






#endif  // COMPRESSIONHELPERS_H_INCLUDED
