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


HiseLosslessAudioFormatWriter::HiseLosslessAudioFormatWriter(EncodeMode mode_, OutputStream* output, double sampleRate, int numChannels, uint32* blockOffsetBuffer) :
	AudioFormatWriter(output, "HLAC", sampleRate, numChannels, 16),
	mode(mode_),
	tempOutputStream(new MemoryOutputStream()),
	blockOffsets(blockOffsetBuffer)
{
	usesFloatingPointData = true;
}


HiseLosslessAudioFormatWriter::~HiseLosslessAudioFormatWriter()
{
	flush();
}

bool HiseLosslessAudioFormatWriter::flush()
{
	if (tempWasFlushed)
		return true;

	if (!writeHeader())
		return false;

	if (!writeDataFromTemp())
		return false;

	tempWasFlushed = true;
	deleteTemp();
	return true;
}


void HiseLosslessAudioFormatWriter::setOptions(HlacEncoder::CompressorOptions& newOptions)
{
	encoder.setOptions(newOptions);
}

bool HiseLosslessAudioFormatWriter::write(const int** samplesToWrite, int numSamples)
{
	tempWasFlushed = false;

	bool isStereo = samplesToWrite[1] != nullptr;

	if (isStereo)
	{
		float* const* r = const_cast<float**>(reinterpret_cast<const float**>(samplesToWrite));

		AudioSampleBuffer b = AudioSampleBuffer(r, 2, numSamples);

		encoder.compress(b, *tempOutputStream, blockOffsets);
	}
	else
	{
		float* r = const_cast<float*>(reinterpret_cast<const float*>(samplesToWrite[0]));

		AudioSampleBuffer b = AudioSampleBuffer(&r, 1, numSamples);

		encoder.compress(b, *tempOutputStream, blockOffsets);
	}

	return true;
}


void HiseLosslessAudioFormatWriter::setTemporaryBufferType(bool shouldUseTemporaryFile)
{
	usesTempFile = shouldUseTemporaryFile;

	deleteTemp();

	if (shouldUseTemporaryFile)
	{
		FileOutputStream* fosOriginal = dynamic_cast<FileOutputStream*>(output);

		const bool createTempFileInTargetDirectory = fosOriginal != nullptr;

		if (createTempFileInTargetDirectory)
		{
			File originalFile = fosOriginal->getFile();
			tempFile = new TemporaryFile(originalFile, TemporaryFile::OptionFlags::putNumbersInBrackets);
			File tempTarget = tempFile->getFile();
			tempOutputStream = new FileOutputStream(tempTarget);
		}
		else
		{
			tempFile = new TemporaryFile(File::getCurrentWorkingDirectory(), TemporaryFile::OptionFlags::putNumbersInBrackets);
			File tempTarget = tempFile->getFile();
		}
	}
	else
	{
		tempOutputStream = new MemoryOutputStream();
	}
}

bool HiseLosslessAudioFormatWriter::writeHeader()
{
	auto numBlocks = encoder.getNumBlocksWritten();

	HiseLosslessHeader header(useEncryption, globalBitShiftAmount, sampleRate, numChannels, bitsPerSample, useCompression, numBlocks);

	jassert(header.getVersion() == HLAC_VERSION);
	jassert(header.getBitShiftAmount() == globalBitShiftAmount);
	jassert(header.getNumChannels() == numChannels);
	jassert(header.usesCompression() == useCompression);
	jassert(header.getSampleRate() == sampleRate);
	jassert(header.getBitsPerSample() == bitsPerSample);

	header.storeOffsets(blockOffsets, numBlocks);

	header.write(output);
	return true;
}


bool HiseLosslessAudioFormatWriter::writeDataFromTemp()
{
	if (usesTempFile)
	{
		FileOutputStream* to = dynamic_cast<FileOutputStream*>(tempOutputStream.get());

		jassert(to != nullptr);

		FileInputStream fis(to->getFile());
		return output->writeFromInputStream(fis, fis.getTotalLength()) == fis.getTotalLength();
	}
	else
	{
		MemoryOutputStream* to = dynamic_cast<MemoryOutputStream*>(tempOutputStream.get());

		jassert(to != nullptr);

		MemoryInputStream mis(to->getData(), to->getDataSize(), false);
		return output->writeFromInputStream(mis, mis.getTotalLength()) == mis.getTotalLength();
	}
}

void HiseLosslessAudioFormatWriter::deleteTemp()
{
	// If you hit this assertion, it means that you didn't call flush after writing the last data.
	// This means nothing will get written to the actual output stream...
	jassert(tempWasFlushed);

	tempFile = nullptr;
	tempOutputStream = nullptr;
}
