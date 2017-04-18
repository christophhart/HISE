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


CompressionHelpers::AudioBufferInt16::AudioBufferInt16(AudioSampleBuffer& b, bool normalizeBeforeStoring)
{
#if JUCE_DEBUG
	const float level = b.getMagnitude(0, b.getNumSamples());
	jassert(level <= 1.0f);
#endif

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

CompressionHelpers::AudioBufferInt16::AudioBufferInt16(int size_)
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
		AudioSampleBuffer b(reader->numChannels, (int)reader->lengthInSamples);

		double start = Time::getMillisecondCounterHiRes();
		reader->read(&b, 0, (int)reader->lengthInSamples, 0, true, true);
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

	const int fileUncompressed = (int)reader->lengthInSamples * reader->numChannels * 2;

	const int flacSize = (int)mos->getDataSize();

	MemoryInputStream* mis = new MemoryInputStream(mos->getMemoryBlock(), true);

	ScopedPointer<AudioFormatReader> flacReader = flac.createReaderFor(mis, true);

	AudioSampleBuffer b(flacReader->numChannels, (int)flacReader->lengthInSamples);

	double start = Time::getMillisecondCounterHiRes();

	flacReader->read(&b, 0, (int)flacReader->lengthInSamples, 0, true, true);

	double stop = Time::getMillisecondCounterHiRes();

	double sampleLength = flacReader->lengthInSamples / flacReader->sampleRate;

	double delta = (stop - start) / 1000.0;

	speed = sampleLength / delta;

	Logger::writeToLog("FLAC Decoding Performance: " + String(speed, 1) + "x realtime");

	return (float)flacSize / (float)fileUncompressed;

}

uint8 CompressionHelpers::getPossibleBitReductionAmount(const AudioBufferInt16& b)
{
	return BitCompressors::getMinBitDepthForData(b.getReadPointer(), b.size);
}

int CompressionHelpers::getBlockAmount(AudioSampleBuffer& b)
{
	return b.getNumSamples() / COMPRESSION_BLOCK_SIZE + 1;
}

uint8 CompressionHelpers::getBitrateForCycleLength(const AudioBufferInt16& block, int cycleLength, AudioBufferInt16& workBuffer)
{
	auto part = getPart(block, 0, cycleLength);

	IntVectorOperations::sub(workBuffer.getWritePointer(), part.getReadPointer(), block.getReadPointer(cycleLength), cycleLength);

	return BitCompressors::getMinBitDepthForData(workBuffer.getReadPointer(), cycleLength);
}

int CompressionHelpers::getCycleLengthWithLowestBitRate(const AudioBufferInt16& block, int& bitRate, AudioBufferInt16& workBuffer)
{
	bitRate = 16;
	int lowestCycleSize = -1;

	for (int i = 100; i < 1024; i++)
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
	return getPart(b, 0, startIndex, numSamples);
}


AudioSampleBuffer CompressionHelpers::getPart(AudioSampleBuffer& b, int channelIndex, int startIndex, int numSamples)
{
	float* d = b.getWritePointer(channelIndex, startIndex);

	return AudioSampleBuffer(&d, 1, numSamples);
}

int CompressionHelpers::getPaddedSampleSize(int samplesNeeded)
{
    if(samplesNeeded % COMPRESSION_BLOCK_SIZE == 0)
    {
        return samplesNeeded;
    }
    else
    {
        int blocks = samplesNeeded / COMPRESSION_BLOCK_SIZE + 1;
        return blocks * COMPRESSION_BLOCK_SIZE;
    }
}

uint8 CompressionHelpers::getBitReductionForDifferential(AudioBufferInt16& b)
{
	AudioBufferInt16 copy(b.size);

	memcpy(copy.getWritePointer(), b.getReadPointer(), 2*b.size);

	Diff::downSampleBuffer(copy);

	IntVectorOperations::sub(copy.getWritePointer(), b.getReadPointer(), b.size);

	auto br2 = getPossibleBitReductionAmount(copy);

	return br2;
}

int CompressionHelpers::getByteAmountForDifferential(AudioBufferInt16& b)
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


void CompressionHelpers::dump(const AudioBufferInt16& b)
{
	AudioSampleBuffer fb = b.getFloatBuffer();

	dump(fb);
}


void CompressionHelpers::dump(const AudioSampleBuffer& b)
{
	WavAudioFormat afm;
    
#if JUCE_WINDOWS
	File dumpFile("D:\\compressionTest\\dump.wav");
#else
    File dumpFile("/Volumes/Shared/compressionTest/dump.wav");
#endif



	FileOutputStream* fis = new FileOutputStream(dumpFile.getNonexistentSibling());
	StringPairArray metadata;
	ScopedPointer<AudioFormatWriter> writer = afm.createWriterFor(fis, 44100, b.getNumChannels(), 16, metadata, 5);

	if (writer != nullptr)
		writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
}

uint8 CompressionHelpers::checkBuffersEqual(AudioSampleBuffer& workBuffer, AudioSampleBuffer& referenceBuffer)
{
    int numToCheck = referenceBuffer.getNumSamples();
    
    jassert(workBuffer.getNumSamples() % COMPRESSION_BLOCK_SIZE == 0);
    jassert(workBuffer.getNumSamples() >= numToCheck);

	AudioBufferInt16 wbInt(workBuffer, false);
	AudioBufferInt16 rbInt(referenceBuffer, false);

	IntVectorOperations::sub(wbInt.getWritePointer(), rbInt.getReadPointer(), numToCheck);
    
	auto br = getPossibleBitReductionAmount(wbInt);

	if (br != 0)
	{
		DBG("Bit rate for error signal: " + String(br));

		//DUMP(wbInt);
		//DUMP(referenceBuffer);
		//DUMP(workBuffer);

		float* w = workBuffer.getWritePointer(0);
		const float* r = referenceBuffer.getReadPointer(0);

		FloatVectorOperations::subtract(w, r, numToCheck);

		float x = workBuffer.getMagnitude(0, numToCheck);

		float db = Decibels::gainToDecibels(x);

		if (db > -96.0f)
		{
			DBG("Buffer mismatch detected!");

			int maxIndex = -1;
			int minIndex = -1;

			float minValue = 1000.0f;
			float maxValue = -1000.0f;

			for (int i = 0; i < numToCheck; i++)
			{
				auto thisValue = workBuffer.getSample(0, i);

				if (thisValue > maxValue)
				{
					maxValue = thisValue;
					maxIndex = i;
				}

				if (thisValue < minValue)
				{
					minValue = thisValue;
					minIndex = i;
				}
			}

			DBG("Min index: " + String(minIndex) + ", value: " + String(minValue));
			DBG("Max index: " + String(maxIndex) + ", value: " + String(maxValue));
		}
	}

	return br;
}

void CompressionHelpers::IntVectorOperations::sub(int16* dst, const int16* src1, const int16* src2, int numValues)
{
	for (int s = 0; s < numValues; s++)
	{
		dst[s] = src1[s] - src2[s];
	}
}


void CompressionHelpers::IntVectorOperations::sub(int16* dst, const int16* src, int numValues)
{
	for (int i = 0; i < numValues; i++)
	{
		dst[i] -= src[i];
	}
}

void CompressionHelpers::IntVectorOperations::add(int16* dst, const int16* src, int numSamples)
{
	for (int i = 0; i < numSamples; i++)
	{
		dst[i] += src[i];
	}
}


void CompressionHelpers::IntVectorOperations::mul(int16*dst, const int16 value, int numSamples)
{
	for (int i = 0; i < numSamples; i++)
	{
		dst[i] *= value;
	}
}


void CompressionHelpers::IntVectorOperations::div(int16* dst, const int16 value, int numSamples)
{
	for (int i = 0; i < numSamples; i++)
	{
		dst[i] /= value;
	}
}

int16 CompressionHelpers::IntVectorOperations::removeDCOffset(int16* data, int numValues)
{
	int64 sum = 0;

	for (int i = 0; i < numValues; i++)
	{
		sum += data[i];
	}

	int16 dcOffset = (int16)(sum /= numValues);

	for (int i = 0; i < numValues; i++)
	{
		data[i] -= dcOffset;
	}

	return dcOffset;
}

int16 CompressionHelpers::IntVectorOperations::max(const int16* d, int numValues)
{
	int16 m = 0;

	for (int i = 0; i < numValues; i++)
	{
		m = jmax<int16>(m, (int16)abs(d[i]));
	}

	return m;
}


int CompressionHelpers::Diff::getNumFullValues(int bufferSize)
{
	jassert(isPowerOfTwo(bufferSize));

	return bufferSize / 4 + 1;
}


int CompressionHelpers::Diff::getNumErrorValues(int bufferSize)
{
	jassert(isPowerOfTwo(bufferSize));

	return bufferSize - getNumFullValues(bufferSize);
}

CompressionHelpers::AudioBufferInt16 CompressionHelpers::Diff::createBufferWithFullValues(const AudioBufferInt16& b)
{
	auto numFullValues = getNumFullValues(b.size);

	AudioBufferInt16 packedBuffer(numFullValues);

	auto r = b.getReadPointer();
	auto w = packedBuffer.getWritePointer();

	for (int i = 0; i < b.size - 4; i+= 4)
	{
		*w = r[i];
		++w;
	}

	auto si = b.size - 4;

	*w = r[si];
	++w;
	*w = r[si+3];
	++w;

	jassert(w == (packedBuffer.getWritePointer() + packedBuffer.size));

	return packedBuffer;
}


CompressionHelpers::AudioBufferInt16 CompressionHelpers::Diff::createBufferWithErrorValues(const AudioBufferInt16& b, const AudioBufferInt16& packedFullValues)
{
	AudioBufferInt16 workBuffer(b.size);

	distributeFullSamples(workBuffer, reinterpret_cast<const uint16*>(packedFullValues.getReadPointer()), packedFullValues.size);

	IntVectorOperations::sub(workBuffer.getWritePointer(), b.getReadPointer(), b.size);

	AudioBufferInt16 packedBuffer(getNumErrorValues(b.size));

	int16* w = packedBuffer.getWritePointer();
	const int16* r = workBuffer.getReadPointer();

	for (int i = 0; i < b.size - 4; i += 4)
	{
		w[0] = r[i + 1];
		w[1] = r[i + 2];
		w[2] = r[i + 3];

		w += 3;
	}

	auto si = b.size - 4;

	w[0] = r[si + 1];
	w[1] = r[si + 2];

	w += 2;

	jassert(w == packedBuffer.getWritePointer() + packedBuffer.size);

	return packedBuffer;
}

void CompressionHelpers::Diff::downSampleBuffer(AudioBufferInt16& b)
{
	jassert(b.size % 4 == 0);
	
	int16* d = b.getWritePointer();

	for (int i = 0; i < b.size - 4; i += 4)
	{
		int16 firstValue = d[0];
		int16 lastValue = d[4];

		d[1] = 3 * firstValue / 4 + lastValue / 4;
		d[2] = firstValue / 2 + lastValue / 2;
		d[3] = firstValue / 4 + 3 * lastValue / 4;

		d += 4;
	}

	d = b.getWritePointer();

	const int si = b.size - 4;

	d[si + 1] = 2 * d[si] / 3 + d[si + 3] / 3;
	d[si + 2] = d[si] / 3 + 2 * d[si + 3] / 3;
}

#define OPTIMIZED 0

void CompressionHelpers::Diff::distributeFullSamples(AudioBufferInt16& dst, const uint16* fullSamplesPacked, int numSamples)
{
	// Use this only on power two block sizes (512 samples = 128 four packs + 1 last value)
	jassert(isPowerOfTwo(numSamples - 1));

	// Needs 64 bit alignment for the destination buffer.
	jassert(reinterpret_cast<uint64>(dst.getReadPointer()) % 8 == 0);

	int16* d = dst.getWritePointer();

	const int16* r = reinterpret_cast<const int16*>(fullSamplesPacked);

	int thisValue = 0;
	int nextValue = 0;

	for (int i = 0; i < numSamples - 2; i++)
	{
		thisValue = (int)r[i];
		nextValue = (int)r[i + 1];

		d[0] = (int16)(thisValue);
		d[1] = (int16)((3 * thisValue + nextValue) / 4);
		d[2] = (int16)((thisValue + nextValue) / 2);
		d[3] = (int16)((thisValue + 3 * nextValue) / 4);

		d += 4;
	}

	thisValue = r[numSamples - 2];
	nextValue = r[numSamples - 1];
	
	d[0] = (int16)(thisValue);
	d[1] = (int16)((2 * thisValue + nextValue) / 3);
	d[2] = (int16)((thisValue + 2 * nextValue) / 3);
	d[3] = (int16)(nextValue);

#if 0
	int readIndex = 0;

	int thisValue = (int)(r[readIndex]);
	++readIndex;
	--numSamples;

	while (numSamples > 2)
	{
		int nextValue = (int)(r[readIndex]);
		++readIndex;
		numSamples -= 1;

		d[0] = (int16)(thisValue);
		d[1] = (int16)((3*thisValue + nextValue) / 4);
		d[2] = (int16)((thisValue + nextValue) / 2);
		d[3] = (int16)((thisValue + 3 * nextValue) / 4);

		d += 4;
		thisValue = nextValue;
		
	}


	++readIndex;
	int nextValue = r[readIndex];

	d += 4;

	d[0] = (int16)(thisValue);
	d[1] = (int16)((2 * thisValue + nextValue) / 3);
	d[2] = (int16)((thisValue + 2 * nextValue) / 3);
	d[3] = (int16)(nextValue);

#endif


}

void CompressionHelpers::Diff::addErrorSignal(AudioBufferInt16& dst, const uint16* errorSignalPacked, int numSamples)
{


	jassert(isPowerOfTwo(4*(numSamples+1)/3));

	// Needs 64 bit alignment for the destination buffer.
	jassert(reinterpret_cast<uint64>(dst.getReadPointer()) % 8 == 0);

	int16* d = dst.getWritePointer();

	const int16* e = reinterpret_cast<const int16*>(errorSignalPacked);

	int counter = 0;

	while (numSamples > 2)
	{
		counter += 4;

		d[1] -= e[0];
		d[2] -= e[1];
		d[3] -= e[2];

		d += 4;
		e += 3;

		numSamples -= 3;
	}

	d[1] -= e[0];
	d[2] -= e[1];
}

uint64 CompressionHelpers::Misc::NumberOfSetBits(uint64 i)
{
#if JUCE_MSVC && JUCE_64BIT
	return __popcnt64(i);
#else
    
    BigInteger b((int64)i);
    
    return b.countNumberOfSetBits();
    
#endif
}

uint8 CompressionHelpers::Misc::getSampleRateIndex(double sampleRate)
{
	if (sampleRate == 44100.0)
		return 0;
	if (sampleRate == 48000.0)
		return 1;
	if (sampleRate == 88200.0)
		return 2;
	if (sampleRate == 96000.0)
		return 3;

	jassertfalse;

	return 0;
}
