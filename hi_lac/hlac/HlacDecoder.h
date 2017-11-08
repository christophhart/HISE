/*  HISE Lossless Audio Codec
*	©2017 Christoph Hart
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


#ifndef HLACDECODER_H_INCLUDED
#define HLACDECODER_H_INCLUDED

namespace hlac { using namespace juce; 

class HlacDecoder
{
public:


	HlacDecoder():
	currentCycle(0),
	workBuffer(0)
	{};

	void decode(HiseSampleBuffer& destination, bool decodeStereo, InputStream& input, int offsetInSource=0, int numSamples=-1);

	void setupForDecompression();

	double getDecompressionPerformance() const;

	uint32 getCurrentReadPosition() const
	{
		return readOffset;
	}

	void seekToPosition(InputStream& input, uint32 samplePosition, uint32 byteOffset);

private:

	struct CycleHeader
	{
		CycleHeader(uint8 headerInfo_, uint16 numSamples_) :
			headerInfo(headerInfo_),
			numSamples(numSamples_)
		{}

		bool isTemplate() const;
		uint8 getBitRate(bool getFullBitRate = true) const;
		bool isDiff() const;

		uint16 getNumSamples() const;

	private:

		uint8 headerInfo;
		uint16 numSamples;
	};

	void reset();
	
	bool decodeBlock(HiseSampleBuffer& destination, bool decodeStereo, InputStream& input, int channelIndex);

	void decodeDiff(const CycleHeader& header, bool decodeStereo, HiseSampleBuffer& destination, InputStream& input, int channelIndex);

	void decodeCycle(const CycleHeader& header, bool decodeStereo, HiseSampleBuffer& destination, InputStream& input, int channelIndex);

	enum class FloatWriteMode
	{
		Copy,
		Add,
		Clear,
		numWriteModes
	};

	void writeToFloatArray(bool shouldCopy, bool useTempBuffer, HiseSampleBuffer& destination, int channelIndex, int numSamples);

	CycleHeader readCycleHeader(InputStream& input);

	BitCompressors::Collection collection;

	CompressionHelpers::AudioBufferInt16 currentCycle;

	CompressionHelpers::AudioBufferInt16 workBuffer;

	uint16 indexInBlock = 0;
	int leftFloatIndex = 0;
	int rightFloatIndex = 0;
	
	int leftNumToSkip = 0;
	int rightNumToSkip = 0;
	
	uint32 blockOffset = 0;

	uint8 bitRateForCurrentCycle;

	int16 firstCycleLength = -1;

	MemoryBlock readBuffer;

	float ratio = 0.0f;

	int readOffset = 0;

	int readIndex = 0;

	Array<double> decompressionSpeeds;

	double decompressionSpeed = 0.0;
};

} // namespace hlac

#endif  // HLACDECODER_H_INCLUDED
