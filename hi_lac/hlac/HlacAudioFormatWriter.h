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

#ifndef HLACAUDIOFORMATWRITER_H_INCLUDED
#define HLACAUDIOFORMATWRITER_H_INCLUDED

namespace hlac { using namespace juce; 

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

	void setEnableFullDynamics(bool shouldEnableFullDynamics);

	bool write(const int** samplesToWrite, int numSamples) override;

	double getCompressionRatioForLastFile() { return encoder.getCompressionRatio(); }

	/** You can use a temporary file instead of the memory buffer if you encode large files. */
	void setTemporaryBufferType(bool shouldUseTemporaryFile);

	/** Call this to preallocate the amount of memory approximately required for the extraction. */
	void preallocateMemory(int64 numSamplesToWrite, int numChannels);

	/** Returns the number of written bytes for this reader. */
	int64 getNumBytesWritten() const;

private:

	bool writeHeader();
	bool writeDataFromTemp();

	int64 numBytesWritten = 0;

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

} // namespace hlac

#endif  // HLACAUDIOFORMATWRITER_H_INCLUDED
