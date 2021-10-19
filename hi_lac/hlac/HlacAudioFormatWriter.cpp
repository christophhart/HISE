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

namespace hlac {

HiseLosslessAudioFormatWriter::HiseLosslessAudioFormatWriter(EncodeMode mode_, juce::OutputStream* output, double sampleRate, int numChannels, uint32_t* blockOffsetBuffer) :
    juce::AudioFormatWriter(output, "HLAC", sampleRate, numChannels, 16),
    tempOutputStream(new juce::MemoryOutputStream()),
    blockOffsets(blockOffsetBuffer),
    mode(mode_)
{
	auto option = HlacEncoder::CompressorOptions::getPreset (HlacEncoder::CompressorOptions::Presets::Diff);

	encoder.setOptions(option);

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

	tempOutputStream->flush();

	deleteTemp();
	return true;
}


void HiseLosslessAudioFormatWriter::setOptions(HlacEncoder::CompressorOptions& newOptions)
{
	options = newOptions;
	encoder.setOptions(newOptions);
}

void HiseLosslessAudioFormatWriter::setEnableFullDynamics(bool shouldEnableFullDynamics)
{
	options.normalisationMode = shouldEnableFullDynamics ? 2 : 0;
	encoder.setOptions(options);
}

bool HiseLosslessAudioFormatWriter::write(const int** samplesToWrite, int numSamples)
{
	tempWasFlushed = false;

	bool isStereo = samplesToWrite[1] != nullptr;

	if (options.useCompression)
	{
		if (isStereo)
		{
			float* const* r = const_cast<float**>(reinterpret_cast<const float**>(samplesToWrite));

            auto b = juce::AudioSampleBuffer(r, 2, numSamples);

			encoder.compress(b, *tempOutputStream, blockOffsets);
		}
		else
		{
			float* r = const_cast<float*>(reinterpret_cast<const float*>(samplesToWrite[0]));

			auto b = juce::AudioSampleBuffer(&r, 1, numSamples);

			encoder.compress(b, *tempOutputStream, blockOffsets);
		}

	}
	else
	{
		float* const* r = const_cast<float**>(reinterpret_cast<const float**>(samplesToWrite));
		numChannels = isStereo ? 2 : 1;

		auto source = juce::AudioSampleBuffer (r, numChannels, numSamples);

        juce::MemoryBlock tempBlock;

		const int bytesToWrite = numSamples * numChannels * sizeof(int16_t);

		tempBlock.setSize(bytesToWrite, false);

		AudioFormatWriter::WriteHelper<juce::AudioData::Int16, juce::AudioData::Float32, juce::AudioData::LittleEndian>::write(
			tempBlock.getData(), numChannels, (const int* const *)source.getArrayOfReadPointers(), numSamples);

		tempOutputStream->write(tempBlock.getData(), bytesToWrite);
	}
	
	numBytesWritten = tempOutputStream->getPosition();

	return true;
}


void HiseLosslessAudioFormatWriter::setTemporaryBufferType(bool shouldUseTemporaryFile)
{
	usesTempFile = shouldUseTemporaryFile;

	deleteTemp();

	if (shouldUseTemporaryFile)
	{
		auto* fosOriginal = dynamic_cast<juce::FileOutputStream*>(output);

		const bool createTempFileInTargetDirectory = fosOriginal != nullptr;

		if (createTempFileInTargetDirectory)
		{
			auto originalFile = fosOriginal->getFile();
			tempFile = std::make_unique<juce::TemporaryFile>(originalFile, juce::TemporaryFile::OptionFlags::putNumbersInBrackets);
			auto tempTarget = tempFile->getFile();
			tempOutputStream = tempFile->getFile().createOutputStream();
		}
		else
		{
			tempFile = std::make_unique<juce::TemporaryFile>(juce::File::getCurrentWorkingDirectory(), juce::TemporaryFile::OptionFlags::putNumbersInBrackets);
			auto tempTarget = tempFile->getFile();
		}
	}
	else
	{
		tempOutputStream = std::make_unique<juce::MemoryOutputStream>();
	}
}


void HiseLosslessAudioFormatWriter::preallocateMemory(int64_t numSamplesToWrite, int numChannelsToAllocate)
{
	if (auto* mos = dynamic_cast<juce::MemoryOutputStream*>(tempOutputStream.get()))
	{
		int64_t b = numSamplesToWrite * numChannelsToAllocate * 2 * 2 / 3;

		// Set the limit to 1.5GB
		int64_t limit = 1024;
		limit *= 1024;
		limit *= 1024;
		limit *= 3;
		limit /= 2;

		if (b > limit)
			setTemporaryBufferType(true);
		else
			mos->preallocate(b);
	}
}

int64_t HiseLosslessAudioFormatWriter::getNumBytesWritten() const
{
	return numBytesWritten;
}

bool HiseLosslessAudioFormatWriter::writeHeader()
{
	if (options.useCompression)
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

		return header.write(output);
	}
	else
	{
		auto monoHeader = HiseLosslessHeader::createMonolithHeader(numChannels, sampleRate);

		return monoHeader.write(output);
	}
}


bool HiseLosslessAudioFormatWriter::writeDataFromTemp()
{
	if (usesTempFile)
	{
		auto* to = dynamic_cast<juce::FileOutputStream*>(tempOutputStream.get());

		jassert(to != nullptr);

        juce::FileInputStream fis(to->getFile());
		return output->writeFromInputStream(fis, fis.getTotalLength()) == fis.getTotalLength();
	}
	else
	{
		auto* to = dynamic_cast<juce::MemoryOutputStream*>(tempOutputStream.get());

		jassert(to != nullptr);

        juce::MemoryInputStream mis(to->getData(), to->getDataSize(), false);
		return output->writeFromInputStream(mis, mis.getTotalLength()) == mis.getTotalLength();
	}
}

void HiseLosslessAudioFormatWriter::deleteTemp()
{
	// If you hit this assertion, it means that you didn't call flush after writing the last data.
	// This means nothing will get written to the actual output stream...
	jassert(tempWasFlushed);

	tempOutputStream = nullptr;
	tempFile = nullptr;
	
}

} // namespace hlac
