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

#ifndef HLACAUDIOFORMATREADER_H_INCLUDED
#define HLACAUDIOFORMATREADER_H_INCLUDED



struct HiseLosslessHeader
{
	HiseLosslessHeader(InputStream* input);
	HiseLosslessHeader(bool useEncryption, uint8 globalBitShiftAmount, double sampleRate, int numChannels, int bitsPerSample, bool useCompression, uint32 numBlocks);

	int getVersion() const;
	bool isEncrypted() const;
	int getBitShiftAmount() const;
	uint32 getNumChannels() const;
	uint32 getBitsPerSample() const;
	bool usesCompression() const;
	double getSampleRate() const;
	uint32 getBlockAmount() const;

	uint32 getOffsetForReadPosition(int64 samplePosition);

	bool write(OutputStream* output);

	void storeOffsets(uint32* offsets, int numOffsets);

private:

	uint8 headerByte = 0;
	uint8 sampleDataByte = 0;
	uint32 blockAmount = 0;
	HeapBlock<uint32> blockOffsets;
	bool headerValid = false;
	bool isOldMonolith = false;
	uint32 headerSize;
};

class HiseLosslessAudioFormatReader : public AudioFormatReader
{
public:
	HiseLosslessAudioFormatReader(InputStream* input_);

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples) override;

	double getDecompressionPerformanceForLastFile() { return decoder.getDecompressionPerformance(); }

	/** You can choose what the target data type should be. If you read into integer AudioSampleBuffers, you might want to call this method
	*	in order to save unnecessary conversions between float and integer numbers. */
	void setTargetAudioDataType(AudioDataConverters::DataFormat dataType);

private:

	HlacDecoder decoder;

	HiseLosslessHeader header;

};


#endif  // HLACAUDIOFORMATREADER_H_INCLUDED
