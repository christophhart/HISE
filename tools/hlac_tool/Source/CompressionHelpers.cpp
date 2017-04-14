/*
  ==============================================================================

    CompressionHelpers.cpp
    Created: 12 Apr 2017 10:16:50pm
    Author:  Christoph

  ==============================================================================
*/



#include "CompressionHelpers.h"

CompressionHelpers::AudioBufferInt16::AudioBufferInt16(AudioSampleBuffer& b, bool normalizeBeforeStoring)
{
	size = b.getNumSamples();
	data.malloc(size);

	if (normalizeBeforeStoring)
	{
		auto r = b.findMinMax(0, 0, size);

		const float peak = jmax<float>(fabsf(r.getStart()), fabsf(r.getEnd()));
		gainFactor = 1.0f / peak;

		b.applyGain(gainFactor); // Slow but don't care...

		AudioDataConverters::convertFloatToInt16LE(b.getReadPointer(0), data, b.getNumSamples());

		b.applyGain(peak);
	}
	else
	{
		AudioDataConverters::convertFloatToInt16LE(b.getReadPointer(0), data, b.getNumSamples());
	}
}

CompressionHelpers::AudioBufferInt16::AudioBufferInt16(int16* externalData_, int numSamples)
{
	size = numSamples;
	externalData = externalData_;
}

CompressionHelpers::AudioBufferInt16::AudioBufferInt16(const int16* externalData_, int numSamples)
{
	size = numSamples;
	externalData = const_cast<int16*>(externalData_);
	isReadOnly = true;
}

CompressionHelpers::AudioBufferInt16::AudioBufferInt16(size_t size_)
{
	size = size_;
	data.calloc(size);
}

AudioSampleBuffer CompressionHelpers::AudioBufferInt16::getFloatBuffer() const
{
	AudioSampleBuffer b(1, size);
	AudioDataConverters::convertInt16LEToFloat(data, b.getWritePointer(0), size);

	b.applyGain(1.0f / gainFactor);

	return b;
}

void CompressionHelpers::AudioBufferInt16::negate()
{
	int16* d = getWritePointer();

	for (int i = 0; i < size; i++)
	{
		d[i] *= -1;
	}
}

int16* CompressionHelpers::AudioBufferInt16::getWritePointer(int startSample /*= 0*/)
{
	jassert(!isReadOnly);

	return externalData != nullptr ? externalData + startSample : data + startSample;
}

const int16* CompressionHelpers::AudioBufferInt16::getReadPointer(int startSample /*= 0*/) const
{
	return externalData != nullptr ? externalData + startSample : data + startSample;
}

AudioSampleBuffer CompressionHelpers::loadFile(const File& f, double& speed)
{
	if (!f.existsAsFile())
	{
		throw String("File " + f.getFullPathName() + " does not exist");
	}
	
	AudioFormatManager afm;
	afm.registerBasicFormats();

	MemoryBlock mb;

	FileInputStream fis(f);

	fis.readIntoMemoryBlock(mb);


	MemoryInputStream* mis = new MemoryInputStream(mb, false);

	ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(mis);

	if (reader != nullptr)
	{
		AudioSampleBuffer b(reader->numChannels, reader->lengthInSamples);

		double start = Time::getMillisecondCounterHiRes();
		reader->read(&b, 0, reader->lengthInSamples, 0, true, true);
		double stop = Time::getMillisecondCounterHiRes();
		
		double sampleLength = reader->lengthInSamples / reader->sampleRate;

		double delta = (stop - start) / 1000.0;

		speed = sampleLength / delta;

		Logger::writeToLog("PCM Decoding Performance: " + String(speed, 1) + "x realtime");

		return b;
	}
	else
	{
		throw String("File " + f.getFileName() + " can not be opened");
	}
}

float CompressionHelpers::getFLACRatio(const File& f, double& speed)
{
	FlacAudioFormat flac;

	AudioFormatManager afm;

	afm.registerBasicFormats();

	ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(f);

	MemoryOutputStream* mos = new MemoryOutputStream();

	ScopedPointer<AudioFormatWriter> writer = flac.createWriterFor(mos, reader->sampleRate, reader->numChannels, 16, reader->metadataValues, 5);

	writer->writeFromAudioReader(*reader, 0, reader->lengthInSamples);

	const int fileUncompressed = reader->lengthInSamples * reader->numChannels * 2;

	const int flacSize = mos->getDataSize();

	MemoryInputStream* mis = new MemoryInputStream(mos->getMemoryBlock(), true);

	ScopedPointer<AudioFormatReader> flacReader = flac.createReaderFor(mis, true);

	AudioSampleBuffer b(flacReader->numChannels, flacReader->lengthInSamples);

	double start = Time::getMillisecondCounterHiRes();

	flacReader->read(&b, 0, flacReader->lengthInSamples, 0, true, true);

	double stop = Time::getMillisecondCounterHiRes();

	double sampleLength = flacReader->lengthInSamples / flacReader->sampleRate;

	double delta = (stop - start) / 1000.0;

	speed = sampleLength / delta;

	Logger::writeToLog("FLAC Decoding Performance: " + String(speed, 1) + "x realtime");

	return (float)flacSize / (float)fileUncompressed;

	writer = nullptr;
	reader = nullptr;

}

uint8 CompressionHelpers::getPossibleBitReductionAmount(const AudioBufferInt16& b)
{
	return BitCompressors::getMinBitDepthForData(b.getReadPointer(), b.size);
}

int CompressionHelpers::getBlockAmount(AudioSampleBuffer& b)
{
	return b.getNumSamples() / COMPRESSION_BLOCK_SIZE + 1;
}

uint8 CompressionHelpers::getBitrateForCycleLength(const AudioBufferInt16& block, uint16 cycleLength, AudioBufferInt16& workBuffer)
{
	auto part = getPart(block, 0, cycleLength);

	IntVectorOperations::sub(workBuffer.getWritePointer(), part.getReadPointer(), block.getReadPointer(cycleLength), cycleLength);

	return BitCompressors::getMinBitDepthForData(workBuffer.getReadPointer(), cycleLength);
}

uint16 CompressionHelpers::getCycleLengthWithLowestBitRate(const AudioBufferInt16& block, uint8& bitRate, AudioBufferInt16& workBuffer)
{
	bitRate = 16;
	uint16 lowestCycleSize = -1;

	for (uint16 i = 100; i < 1024; i++)
	{
		auto thisRate = getBitrateForCycleLength(block, i, workBuffer);

		if (thisRate < bitRate)
		{
			bitRate = thisRate;
			lowestCycleSize = i;
		}
	}

	return lowestCycleSize;
}

uint8 CompressionHelpers::getBitReductionWithTemplate(AudioBufferInt16& lastCycle, AudioBufferInt16& nextCycle, bool removeDc)
{
	jassert(lastCycle.size == nextCycle.size);

	uint8 templateBitRate = getPossibleBitReductionAmount(lastCycle);

	AudioBufferInt16 workBuffer(lastCycle.size);

	IntVectorOperations::sub(workBuffer.getWritePointer(), lastCycle.getReadPointer(), nextCycle.getReadPointer(), lastCycle.size);

	if(removeDc)
		IntVectorOperations::removeDCOffset(workBuffer.getWritePointer(), lastCycle.size);

	int8 deltaBitRate = getPossibleBitReductionAmount(workBuffer);

	if (deltaBitRate < 0 || deltaBitRate > templateBitRate)
		return 0;
	

	return templateBitRate - deltaBitRate;
}

CompressionHelpers::AudioBufferInt16 CompressionHelpers::getPart(AudioBufferInt16& b, int startIndex, int numSamples)
{
	int16* d = b.getWritePointer(startIndex);

	return AudioBufferInt16(d, numSamples);
}

CompressionHelpers::AudioBufferInt16 CompressionHelpers::getPart(const AudioBufferInt16& b, int startIndex, int numSamples)
{
	const int16* d = b.getReadPointer(startIndex);

	return AudioBufferInt16(d, numSamples);
}

AudioSampleBuffer CompressionHelpers::getPart(AudioSampleBuffer& b, int startIndex, int numSamples)
{
	float* d = b.getWritePointer(0, startIndex);

	return AudioSampleBuffer(&d, 1, numSamples);
}

uint8 CompressionHelpers::getBitReductionForDifferential(AudioBufferInt16& b)
{
	AudioBufferInt16 copy(b.size);

	memcpy(copy.getWritePointer(), b.getReadPointer(), 2*b.size);

	int16* d = copy.getWritePointer();

	for (int i = 0; i < b.size-4; i+= 4)
	{
		int16 firstValue = d[0];
		int16 lastValue = d[4];

		d[1] = 3 * firstValue / 4 + lastValue / 4;
		d[2] = firstValue / 2 + lastValue / 2;
		d[3] = firstValue / 4 + 3 * lastValue / 4;

		d += 4;
	}


	d = copy.getWritePointer();

	int16* w = b.getWritePointer();

	const int si = b.size - 4;

	d[si+1] = 2 * d[si] / 3 + d[si+3] / 3;
	d[si+2] = d[si] / 3 + 2* d[si+3] / 3;

	for (int i = 0; i < b.size; i++)
	{
		d[i] = w[i] - d[i];
	}

	auto br = getPossibleBitReductionAmount(copy);

	return br;
}

uint16 CompressionHelpers::getByteAmountForDifferential(AudioBufferInt16& b)
{
	BitCompressors::Collection c;

	auto br = getPossibleBitReductionAmount(b);
	auto brDiff = getBitReductionForDifferential(b);

	int numFullValues = (b.size-4) / 4 + 2; // the last 4 values are two full values
	int numReducedValues = 3 * (b.size - 4) / 4 + 2; // the last 4 values are two values

	jassert(numFullValues + numReducedValues == b.size);

	auto fullSize = c.getNumBytesForBitRate(br, numFullValues);
	auto reducedSize = c.getNumBytesForBitRate(brDiff, numReducedValues);

	return fullSize + reducedSize;
}

float CompressionHelpers::getDifference(AudioSampleBuffer& workBuffer, const AudioSampleBuffer& referenceBuffer)
{
	jassert(workBuffer.getNumSamples() == referenceBuffer.getNumSamples());

	float* w = workBuffer.getWritePointer(0);
	const float* r = referenceBuffer.getReadPointer(0);

	for (int i = 0; i < workBuffer.getNumSamples(); i++)
	{
		w[i] = w[i] - r[i];
	}

	float x = workBuffer.getMagnitude(0, workBuffer.getNumSamples());

	return Decibels::gainToDecibels(x);
}

void CompressionHelpers::IntVectorOperations::sub(int16* dst, const int16* src1, const int16* src2, size_t numValues)
{
	for (int s = 0; s < numValues; s++)
	{
		dst[s] = src1[s] - src2[s];
	}
}

void CompressionHelpers::IntVectorOperations::add(int16* dst, const int16* src, int numSamples)
{
	for (int i = 0; i < numSamples; i++)
	{
		dst[i] += src[i];
	}
}

int16 CompressionHelpers::IntVectorOperations::removeDCOffset(int16* data, size_t numValues)
{
	int64 sum = 0;

	for (int i = 0; i < numValues; i++)
	{
		sum += data[i];
	}

	int16 dcOffset = sum /= numValues;

	for (int i = 0; i < numValues; i++)
	{
		data[i] -= dcOffset;
	}

	return dcOffset;
}

int16 CompressionHelpers::IntVectorOperations::max(const int16* d, size_t numValues)
{
	int16 m = 0;

	for (int i = 0; i < numValues; i++)
	{
		m = jmax<int>(m, abs(d[i]));
	}

	return m;
}

