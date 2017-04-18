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

#ifndef HLACAUDIOFORMATWRITER_H_INCLUDED
#define HLACAUDIOFORMATWRITER_H_INCLUDED



class HiseLosslessAudioFormatWriter : public AudioFormatWriter
{
public:

	enum class EncodeMode
	{
		Block,
		Delta,
		Diff,
		numEncodeModes
	};

	HiseLosslessAudioFormatWriter(EncodeMode mode_, OutputStream* output, double sampleRate, int numChannels, uint32* blockOffsetBuffer);

	~HiseLosslessAudioFormatWriter();

	/** You must call flush after you written all files so that it can create the block offset table and write the header. */
	bool flush() override;

	void setOptions(HlacEncoder::CompressorOptions& newOptions);

	bool write(const int** samplesToWrite, int numSamples) override;

	double getCompressionRatioForLastFile() { return encoder.getCompressionRatio(); }

	/** You can use a temporary file instead of the memory buffer if you encode large files. */
	void setTemporaryBufferType(bool shouldUseTemporaryFile);

private:

	bool writeHeader();
	bool writeDataFromTemp();

	void deleteTemp();

	ScopedPointer<TemporaryFile> tempFile;
	ScopedPointer<OutputStream> tempOutputStream;

	bool tempWasFlushed = true;
	bool usesTempFile = false;

	uint32* blockOffsets;

	HlacEncoder encoder;

	EncodeMode mode;
	HlacEncoder::CompressorOptions options;

	bool useEncryption = false;
	bool useCompression = true;

	uint8 globalBitShiftAmount = 0;
};


#endif  // HLACAUDIOFORMATWRITER_H_INCLUDED
