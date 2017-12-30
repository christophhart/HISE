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

namespace hlac { using namespace juce; 

CompressionHelpers::AudioBufferInt16::AudioBufferInt16(AudioSampleBuffer& b, int channelToUse, bool normalizeBeforeStoring)
{
#if JUCE_DEBUG
	const float level = b.getMagnitude(0, b.getNumSamples());
	jassert(level <= 1.0f);
#endif

	allocate(b.getNumSamples());
	

	if (normalizeBeforeStoring)
	{
		auto r = b.findMinMax(channelToUse, 0, size);

		const float peak = jmax<float>(fabsf(r.getStart()), fabsf(r.getEnd()));
		gainFactor = 1.0f / peak;

		b.applyGain(gainFactor); // Slow but don't care...

		AudioDataConverters::convertFloatToInt16LE(b.getReadPointer(channelToUse), data, b.getNumSamples());

		b.applyGain(peak);
	}
	else
	{
		AudioDataConverters::convertFloatToInt16LE(b.getReadPointer(channelToUse), data, b.getNumSamples());
	}
}

CompressionHelpers::AudioBufferInt16::AudioBufferInt16(int16* externalData_, int numSamples)
{
	size = numSamples;

	//jassert(reinterpret_cast<uint64>(externalData_) % 32 == 0);

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
	allocate(size_);
}


CompressionHelpers::AudioBufferInt16::~AudioBufferInt16()
{
	deAllocate();
}

AudioSampleBuffer CompressionHelpers::AudioBufferInt16::getFloatBuffer() const
{
	AudioSampleBuffer b(1, size);

	fastInt16ToFloat(data, b.getWritePointer(0), size);

	b.applyGain(1.0f / gainFactor);

	return b;
}


void CompressionHelpers::AudioBufferInt16::reverse(int startSample, int numSamples)
{
	auto s = getWritePointer(startSample);

	auto t = getWritePointer(startSample + numSamples - 1);

	const int numToReverse = numSamples / 2;

	for (int i = 0; i < numToReverse; i++)
	{
		int16 temp = *s;
		*s++ = *t;
		*t-- = temp;
	}

	const int fadeLength = jmin<int>(500, numSamples-1);

	auto s2 = getWritePointer(startSample + numSamples - fadeLength);
	
	float g = 1.0f;

	for (int i = 0; i < fadeLength; i++)
	{
		s2[i] = (int16)((float)s2[i] * g);
		g -= 1.0f / (float)(fadeLength-1);
	}

}

void CompressionHelpers::AudioBufferInt16::negate()
{
	int16* d = getWritePointer();

	for (int i = 0; i < size; i++)
	{
		d[i] *= -1;
	}
}


void CompressionHelpers::AudioBufferInt16::applyGainRamp(int startOffset, int rampLength, float startGain, float endGain)
{
	int16* d = getWritePointer(startOffset);

	const int numToDo = jmin<int>(size - startOffset, rampLength);

	const float delta = (endGain - startGain) / (float)(rampLength-1);

	float level = startGain;

	for (int i = 0; i < numToDo; i++)
	{
		d[i] = (int16)(level * (float)d[i]);
		level += delta;
	}
}

int16* CompressionHelpers::AudioBufferInt16::getWritePointer(int startSample /*= 0*/)
{
	jassert(startSample < size);

    if(startSample >= size)
    {
        return nullptr;
    }
    
	jassert(!isReadOnly);

	return externalData != nullptr ? externalData + startSample : data + startSample;
}

const int16* CompressionHelpers::AudioBufferInt16::getReadPointer(int startSample /*= 0*/) const
{
	jassert(startSample < size);
	
	return externalData != nullptr ? externalData + startSample : data + startSample;
}



void CompressionHelpers::AudioBufferInt16::allocate(int newNumSamples)
{
	size = newNumSamples;

	if (size != 0)
	{
#if  JUCE_WINDOWS
		data = static_cast<int16*>(_aligned_malloc(size * sizeof(int16), 16));
#else
        data = static_cast<int16*>(malloc(size * sizeof(int16)));
#endif

        jassert(data != nullptr);
        
		if (data != nullptr)
			memset(data, 0, size * sizeof(int16));
		else
			size = 0;
	}
}

void CompressionHelpers::AudioBufferInt16::deAllocate()
{
	if (data != nullptr)
	{
#if JUCE_WINDOWS
		_aligned_free(data);
#else
        free(data);
#endif
		data = nullptr;
	}
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


AudioSampleBuffer CompressionHelpers::getPart(HiseSampleBuffer& b, int startIndex, int numSamples)
{
	jassert(startIndex + numSamples <= b.getNumSamples());

	if (b.isFloatingPoint())
	{
		return getPart(*b.getFloatBufferForFileReader(), startIndex, numSamples);
	}
	else
	{
		AudioSampleBuffer buffer(b.getNumChannels(), numSamples);

		for (int i = 0; i < b.getNumChannels(); i++)
		{
			auto src = static_cast<const int16*>(b.getReadPointer(i, startIndex));

			fastInt16ToFloat(src, buffer.getWritePointer(i), numSamples);
		}

		return buffer;
	}
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


void CompressionHelpers::fastInt16ToFloat(const void* source, float* dest, int numSamples)
{
#if HLAC_NO_SSE

	AudioDataConverters::convertInt16LEToFloat(source, dest, numSamples);

#else

	uint64 alignOffsetFloat = reinterpret_cast<uint64>(dest) % 16;
	uint64 alignOffsetInt = reinterpret_cast<uint64>(source) % 16;

	if (alignOffsetFloat == 0 && alignOffsetInt == 0)
	{
		const int16* intData = static_cast<const int16*> (source);
		const float scale = 1.0f / 0x7fff;

		__m128 s = _mm_set1_ps(scale);

		const int numSingle = (numSamples % 4);
		const int numSSE = numSamples - numSingle;

		for (int i = 0; i < numSSE; i += 4)
		{
			__m128i a = _mm_loadl_epi64((__m128i*)(intData + i));
			a = _mm_cvtepi16_epi32(a);
			__m128 a_f = _mm_cvtepi32_ps(a);
			a_f = _mm_mul_ps(a_f, s);
			_mm_store_ps(dest + i, a_f);
		}

		for (int i = 0; i < numSingle; i++)
		{
			dest[i + numSSE] = scale * intData[i + numSSE];
		}
	}
	else
	{
		AudioDataConverters::convertInt16LEToFloat(source, dest, numSamples);
	}

#endif
}

uint8 CompressionHelpers::checkBuffersEqual(AudioSampleBuffer& workBuffer, AudioSampleBuffer& referenceBuffer)
{
    int numToCheck = referenceBuffer.getNumSamples();
    
    ///jassert(workBuffer.getNumSamples() % COMPRESSION_BLOCK_SIZE == 0);
    jassert(workBuffer.getNumSamples() >= numToCheck);

	AudioBufferInt16 wbInt(workBuffer, 0, false);
	AudioBufferInt16 rbInt(referenceBuffer,0, false);

	IntVectorOperations::sub(wbInt.getWritePointer(), rbInt.getReadPointer(), numToCheck);
	
	auto br = getPossibleBitReductionAmount(wbInt);
	
	if (br != 0)
	{
		DBG("Bit rate for error signal: " + String(br));

		//DUMP(wbInt);
		//
		DUMP(referenceBuffer);
		DUMP(workBuffer);

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

		return br;
	}


	if (workBuffer.getNumChannels() > 1)
	{
		AudioBufferInt16 wbIntR(workBuffer, 1, false);
		AudioBufferInt16 rbIntR(referenceBuffer, 1, false);

		IntVectorOperations::sub(wbIntR.getWritePointer(), rbIntR.getReadPointer(), numToCheck);

		auto brR = getPossibleBitReductionAmount(wbIntR);

		if (brR != 0)
		{
			DBG("Second channel error: " + String(br));

			//DUMP(wbInt);
			DUMP(referenceBuffer);
			DUMP(workBuffer);

			return brR;
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


void CompressionHelpers::IntVectorOperations::clear(int16* d, int numValues)
{
	memset(d, 0, sizeof(int16)*numValues);
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

	// Needs 128 bit alignment for the destination buffer.
	jassert(reinterpret_cast<uint64>(dst.getReadPointer()) % 16 == 0);

	jassert(reinterpret_cast<uint64>(fullSamplesPacked) % 16 == 0);

	int16* d = dst.getWritePointer();

	int16* r = const_cast<int16*>(reinterpret_cast<const int16*>(fullSamplesPacked));

	int thisValue = 0;
	int nextValue = 0;

#if HLAC_NO_SSE

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

#else

	for (int i = 0; i < numSamples - 9; i+=4)
	{
		__m128i a = _mm_loadl_epi64((const __m128i*)(r + i));
		__m128i b = _mm_loadl_epi64((const __m128i*)(r + i + 1));
		const __m128i mul3 = _mm_set1_epi32(3);
		
		a = _mm_cvtepi16_epi32(a);
		b = _mm_cvtepi16_epi32(b);

		auto v1 = a;

		auto v2 = _mm_mullo_epi32(a, mul3);
		v2 = _mm_add_epi32(v2, b);
		v2 = _mm_srai_epi32(v2, 2);

		auto v3 = _mm_add_epi32(a, b);
		v3 = _mm_srai_epi32(v3, 1);

		auto v4 = _mm_mullo_epi32(b, mul3);
		v4 = _mm_add_epi32(v4, a);
		v4 = _mm_srai_epi32(v4, 2);

		const uint8 z = 0x80;

		const __m128i mask1a = _mm_set_epi8(z, z, z, z,   z, z, 5, 4,   z, z, z, z,    z, z, 1, 0);
		const __m128i mask1b = _mm_set_epi8(z, z, z, z,   z, z,13,12,   z, z, z, z,    z, z, 9, 8);
		const __m128i mask2a = _mm_set_epi8(z, z, z, z,   5, 4, z, z,   z, z, z, z,    1, 0, z, z);
		const __m128i mask2b = _mm_set_epi8(z, z, z, z,  13,12, z, z,   z, z, z, z,    9, 8, z, z);
		const __m128i mask3a = _mm_set_epi8(z, z, 5, 4,   z, z, z, z,   z, z, 1, 0,    z, z, z, z);
		const __m128i mask3b = _mm_set_epi8(z, z,13,12,   z, z, z, z,   z, z, 9, 8,    z, z, z, z);
		const __m128i mask4a = _mm_set_epi8( 5, 4, z, z,  z, z, z, z,   1, 0, z, z,    z, z, z, z);
		const __m128i mask4b = _mm_set_epi8(13,12, z, z,  z, z, z, z,   9, 8, z, z,    z, z, z, z);

		const __m128i d1a = _mm_shuffle_epi8(v1, mask1a);
		const __m128i d1b = _mm_shuffle_epi8(v2, mask2a);
		const __m128i d1c = _mm_shuffle_epi8(v3, mask3a);
		const __m128i d1d = _mm_shuffle_epi8(v4, mask4a);

		const __m128i d1 = _mm_or_si128(_mm_or_si128(d1a, d1b), _mm_or_si128(d1c, d1d));
		
		const __m128i d2a = _mm_shuffle_epi8(v1, mask1b);
		const __m128i d2b = _mm_shuffle_epi8(v2, mask2b);
		const __m128i d2c = _mm_shuffle_epi8(v3, mask3b);
		const __m128i d2d = _mm_shuffle_epi8(v4, mask4b);

		const __m128i d2 = _mm_or_si128(_mm_or_si128(d2a, d2b), _mm_or_si128(d2c, d2d));

		_mm_store_si128((__m128i*)d, d1);
		_mm_store_si128((__m128i*)(d+8), d2);

		d += 16;
	}

	for (int i = numSamples - 9; i < numSamples - 2; i++)
	{
		thisValue = (int)r[i];
		nextValue = (int)r[i + 1];

		d[0] = (int16)(thisValue);
		d[1] = (int16)((3 * thisValue + nextValue) / 4);
		d[2] = (int16)((thisValue + nextValue) / 2);
		d[3] = (int16)((thisValue + 3 * nextValue) / 4);

		d += 4;
	}


#endif


	thisValue = r[numSamples - 2];
	nextValue = r[numSamples - 1];
	
	d[0] = (int16)(thisValue);
	d[1] = (int16)((2 * thisValue + nextValue) / 3);
	d[2] = (int16)((thisValue + 2 * nextValue) / 3);
	d[3] = (int16)(nextValue);
}

void CompressionHelpers::Diff::addErrorSignal(AudioBufferInt16& dst, const uint16* errorSignalPacked, int numSamples)
{


	jassert(isPowerOfTwo(4*(numSamples+1)/3));

	// Needs 64 bit alignment for the destination buffer.
	jassert(reinterpret_cast<uint64>(dst.getReadPointer()) % 16 == 0);

	int16* d = dst.getWritePointer();

	int16* e = const_cast<int16*>(reinterpret_cast<const int16*>(errorSignalPacked));

	int counter = 0;

#if HLAC_NO_SSE

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

#else

	while (numSamples > 2)
	{
		counter += 16;

		__m128i a1 = _mm_load_si128((__m128i*)d);
		__m128i a2 = _mm_load_si128((__m128i*)(d+8));

		__m128i b1 = _mm_loadl_epi64((__m128i*)e);
		__m128i b2 = _mm_loadl_epi64((__m128i*)(e+3));
		__m128i b3 = _mm_loadl_epi64((__m128i*)(e+6));
		__m128i b4 = _mm_loadl_epi64((__m128i*)(e+9));

		const uint8 z = 0x80;

		const __m128i mask = _mm_set_epi8 (z, z, z, z, z, z, z, z, 5, 4, 3, 2, 1, 0, z, z);
		const __m128i mask2 = _mm_set_epi8(5, 4, 3, 2, 1, 0, z, z, z, z, z, z, z, z, z, z);

		__m128i ba = _mm_shuffle_epi8(b1, mask);
		ba = _mm_or_si128(ba, _mm_shuffle_epi8(b2, mask2));

		__m128i bb = _mm_shuffle_epi8(b3, mask);
		bb = _mm_or_si128(bb, _mm_shuffle_epi8(b4, mask2));

		a1 = _mm_sub_epi16(a1, ba);
		a2 = _mm_sub_epi16(a2, bb);

		_mm_store_si128((__m128i*)d, a1);
		_mm_store_si128((__m128i*)(d+8), a2);

		d += 16;
		e += 12;

		numSamples -= 12;

	}


	d[1] -= e[0];
	d[2] -= e[1];


#endif
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

uint32 CompressionHelpers::Misc::createChecksum()
{
	Random r;
	r.setSeedRandomly();
	r.setSeedRandomly();
	r.setSeedRandomly();

	uint16 randomNumber = (uint16)r.nextInt(Range<int>(2, UINT16_MAX));

	uint8* d = reinterpret_cast<uint8*>(&randomNumber);
	uint16 product = (uint16)(d[0] * d[1]);

	uint32 result;

	uint16* resultPointer = reinterpret_cast<uint16*>(&result);
	resultPointer[0] = randomNumber;
	resultPointer[1] = product;

	return result;
}

bool CompressionHelpers::Misc::validateChecksum(uint32 data)
{
	if (data == 0) return false;

	uint16* numbers = reinterpret_cast<uint16*>(&data);

	uint16 randomNumber = numbers[0];
	uint16 product = numbers[1];

	uint8* bytes = reinterpret_cast<uint8*>(&randomNumber);

	return (uint16)(bytes[0] * bytes[1]) == product;
}

#define CHECK_FLAG(x) readAndCheckFlag(fis, x)

#define VERBOSE_LOG(x) listener->logVerboseMessage(x)
#define STATUS_LOG(x) listener->logStatusMessage(x)

bool HlacArchiver::extractSampleData(const DecompressData& data)
{
	jassert(listener != nullptr);
	jassert(thread != nullptr);

	auto sourceFile = data.sourceFile;
	auto targetDirectory = data.targetDirectory;
	auto option = data.option;
	
	Array<File> parts;

	sourceFile.getParentDirectory().findChildFiles(parts, File::findFiles, false, sourceFile.getFileNameWithoutExtension() + ".*");

	const int numParts = parts.size();



	ScopedPointer<FileInputStream> fis = new FileInputStream(sourceFile);

	FlacAudioFormat flacFormat;
	hlac::HiseLosslessAudioFormat hlacFormat;

	CHECK_FLAG(Flag::BeginMetadata);
	auto metadataString = fis->readString();
	CHECK_FLAG(Flag::EndMetadata);

	VERBOSE_LOG(metadataString);

	StringPairArray metadata;

	int partIndex = 1;

	currentFlag = readFlag(fis);

	while (currentFlag == Flag::BeginName)
	{
		auto name = fis->readString();
		CHECK_FLAG(Flag::EndName);

		VERBOSE_LOG("  Reading Monolith " + name);
		STATUS_LOG("Extracting " + name);
		
		*data.partProgress = (double)fis->getPosition() / (double)fis->getTotalLength();

		const double totalProgress = (double)(partIndex - 1) / (double)numParts;

		const double totalPartProgress = *data.partProgress / (double)numParts;

		*data.totalProgress = totalProgress + totalPartProgress;

		CHECK_FLAG(Flag::BeginTime);
		auto archiveTime = Time::fromISO8601(fis->readString());
		CHECK_FLAG(Flag::EndTime);

		if (thread->threadShouldExit())
			return false;

		
		File targetHlacFile = targetDirectory.getChildFile(name);

		bool overwriteThisFile = true;

		if (targetHlacFile.existsAsFile() && option == OverwriteOption::DontOverwrite)
		{
			overwriteThisFile = false;
		}
		
		if (targetHlacFile.existsAsFile() && option == OverwriteOption::ForceOverwrite)
		{
			targetHlacFile.deleteFile();
		}

		if (targetHlacFile.existsAsFile() && option == OverwriteOption::OverwriteIfNewer)
		{
			Time existingTime = targetHlacFile.getCreationTime();

			if (archiveTime > existingTime)
				targetHlacFile.deleteFile();
		}

		if (overwriteThisFile)
		{
			VERBOSE_LOG("  Overwriting File ");

			File tmpFlacFile = targetHlacFile.getSiblingFile("TmpFlac.flac");

			if (tmpFlacFile.existsAsFile())
				tmpFlacFile.deleteFile();

			ScopedPointer<FileOutputStream> flacTempWriteStream = new FileOutputStream(tmpFlacFile);


			CHECK_FLAG(Flag::BeginMonolithLength);
			auto bytesToRead = fis->readInt64();
			CHECK_FLAG(Flag::EndMonolithLength);

			STATUS_LOG("Creating temp file");

			CHECK_FLAG(Flag::BeginMonolith);
			flacTempWriteStream->writeFromInputStream(*fis, bytesToRead);

			currentFlag = readFlag(fis);

			while (currentFlag == Flag::SplitMonolith)
			{
				partIndex++;

				

				fis = nullptr;

				fis = new FileInputStream(getPartFile(sourceFile, partIndex));

				CHECK_FLAG(Flag::BeginMonolithLength);
				bytesToRead = fis->readInt64();
				CHECK_FLAG(Flag::EndMonolithLength);

				

				CHECK_FLAG(Flag::ResumeMonolith);
				flacTempWriteStream->writeFromInputStream(*fis, bytesToRead);

				currentFlag = readFlag(fis);

			}

			jassert(currentFlag == Flag::EndMonolith);
			flacTempWriteStream->flush();
			flacTempWriteStream = nullptr;

			FileInputStream* flacTempInputStream = new FileInputStream(tmpFlacFile);

			ScopedPointer<AudioFormatReader> flacReader = flacFormat.createReaderFor(flacTempInputStream, true);

			if (flacReader == nullptr)
				return false;

			VERBOSE_LOG("    Samplerate: " + String(flacReader->sampleRate, 1));
			VERBOSE_LOG("    Channels: " + String(flacReader->numChannels));
			VERBOSE_LOG("    Length: " + String(flacReader->lengthInSamples));

			FileOutputStream* monolithOutputStream = new FileOutputStream(targetHlacFile);
			ScopedPointer<AudioFormatWriter> writer = hlacFormat.createWriterFor(monolithOutputStream, flacReader->sampleRate, flacReader->numChannels, 5, metadata, 5);

			STATUS_LOG("Decompressing " + name);

			const int bufferSize = 8192 * 32;

			AudioSampleBuffer tempBuffer(flacReader->numChannels, bufferSize);

			for (int64 readerOffset = 0; readerOffset < flacReader->lengthInSamples; readerOffset += bufferSize)
			{
				if (thread->threadShouldExit())
					return false;

				const int numToRead = jmin<int>(bufferSize, (int)(flacReader->lengthInSamples - readerOffset));

				flacReader->read(&tempBuffer, 0, numToRead, readerOffset, true, true);

				writer->writeFromAudioSampleBuffer(tempBuffer, 0, numToRead);

				*data.progress = (double)readerOffset / (double)flacReader->lengthInSamples;
			}

			
			writer = nullptr;

			flacReader = nullptr;
			tmpFlacFile.deleteFile();
			currentFlag = readFlag(fis);
		}
		else
		{
			VERBOSE_LOG("  Skipping File ");

			CHECK_FLAG(Flag::BeginMonolithLength);
			auto bytesToSkip = fis->readInt64();
			CHECK_FLAG(Flag::EndMonolithLength);

			CHECK_FLAG(Flag::BeginMonolith);
			fis->skipNextBytes(bytesToSkip);
			currentFlag = readFlag(fis);

			while (currentFlag == Flag::SplitMonolith)
			{
				partIndex++;

				

				fis = nullptr;
				fis = new FileInputStream(getPartFile(sourceFile, partIndex));

				CHECK_FLAG(Flag::BeginMonolithLength);
				bytesToSkip = fis->readInt64();
				CHECK_FLAG(Flag::EndMonolithLength);


				CHECK_FLAG(Flag::ResumeMonolith);
				fis->skipNextBytes(bytesToSkip);

				currentFlag = readFlag(fis);
			}

			jassert(currentFlag == Flag::EndMonolith);
			currentFlag = readFlag(fis);
		}
	}

	jassert(currentFlag == Flag::EndOfArchive);

	return true;
}

#undef CHECK_FLAG

#define WRITE_FLAG(x) writeFlag(fos, x)

FileInputStream* HlacArchiver::writeTempFile(AudioFormatReader* reader)
{
	FlacAudioFormat flacFormat;

	StringPairArray metadata;

	tmpFile.deleteFile();
	FileOutputStream* tempOutput = new FileOutputStream(tmpFile);

	const int bufferSize = 8192 * 32;

	AudioSampleBuffer tempBuffer(reader->numChannels, bufferSize);

	ScopedPointer<AudioFormatWriter> writer = flacFormat.createWriterFor(tempOutput, reader->sampleRate, reader->numChannels, 16, metadata, 9);

	bool writeResult = true;

	dynamic_cast<HiseLosslessAudioFormatReader*>(reader)->setTargetAudioDataType(AudioDataConverters::float32BE);

	for (int offsetInReader = 0; offsetInReader < reader->lengthInSamples; offsetInReader += bufferSize)
	{

		if (thread->threadShouldExit())
		{
			tempOutput->flush();
			writer = nullptr;
			tmpFile.deleteFile();
			return nullptr;
		}
			

		double partProgress = (double)offsetInReader / (double)reader->lengthInSamples;

		if (progress != nullptr)
			*progress = partProgress;

		const int numToRead = jmin<int>(bufferSize, (int)(reader->lengthInSamples - offsetInReader));

		reader->read(&tempBuffer, 0, numToRead, offsetInReader, true, true);

		writeResult = writer->writeFromAudioSampleBuffer(tempBuffer, 0, numToRead);

		if (!writeResult)
		{
			VERBOSE_LOG("Error at writing from temp buffer at position " + String(offsetInReader) + ", chunk-length: " + String(numToRead));
			return nullptr;
		}
	}

	tempOutput->flush();
	writer = nullptr;

	return new FileInputStream(tmpFile);
}

void HlacArchiver::compressSampleData(const CompressData& data)
{
	jassert(listener != nullptr);
	jassert(thread != nullptr);

#if USE_BACKEND
	const String& metadataJSON = data.metadataJSON;
	const Array<File>& hlacFiles = data.fileList;
	const File& targetFile = data.targetFile;
	progress = data.progress;

	if (!targetFile.isDirectory())
	{
		int partIndex = 1;

		targetFile.deleteFile();

		FlacAudioFormat flacFormat;

		ScopedPointer<FileOutputStream> fos = new FileOutputStream(targetFile);

		listener->logVerboseMessage("Writing to " + fos->getFile().getFileName());

		WRITE_FLAG(Flag::BeginMetadata);
		fos->writeString(metadataJSON);
		WRITE_FLAG(Flag::EndMetadata);

		hlac::HiseLosslessAudioFormat haf;

		StringPairArray metadata;

		tmpFile = targetFile.getSiblingFile("Temp.dat");

		deltaPerFile = (double)1 / (double)hlacFiles.size();

		for (int i = 0; i < hlacFiles.size(); i++)
		{
			if (thread->threadShouldExit())
				return;

			*data.totalProgress = ((double)i / (double)hlacFiles.size());

			auto sizeLeftInPart = data.partSize - fos->getPosition();

			
			FileInputStream* fis = new FileInputStream(hlacFiles[i]);

			ScopedPointer<AudioFormatReader> reader = haf.createReaderFor(fis, true);

			const String name = hlacFiles[i].getFileName();

			VERBOSE_LOG("  Writing monolith " + name);
			STATUS_LOG("Compressing " + name);

			

			VERBOSE_LOG("    Samplerate: " + String(reader->sampleRate, 1));
			VERBOSE_LOG("    Channels: " + String(reader->numChannels));
			VERBOSE_LOG("    Length: " + String(reader->lengthInSamples));

			WRITE_FLAG(Flag::BeginName);
			fos->writeString(name);
			WRITE_FLAG(Flag::EndName);

			WRITE_FLAG(Flag::BeginTime);
			fos->writeString(hlacFiles[i].getCreationTime().toISO8601(true));
			WRITE_FLAG(Flag::EndTime);

			

			ScopedPointer<FileInputStream> tmpInput = writeTempFile(reader);

			if (tmpInput == nullptr)
				return;

			int64 bytesToWrite = jmin<int64>(tmpInput->getTotalLength(), sizeLeftInPart);

			WRITE_FLAG(Flag::BeginMonolithLength);
			fos->writeInt64(bytesToWrite);
			WRITE_FLAG(Flag::EndMonolithLength);

			WRITE_FLAG(Flag::BeginMonolith);
			fos->writeFromInputStream(*tmpInput, bytesToWrite);

			while(!tmpInput->isExhausted())
			{
				WRITE_FLAG(Flag::SplitMonolith);

				fos->flush();
				fos = nullptr;

				partIndex++;

				fos = new FileOutputStream(getPartFile(targetFile, partIndex));

				bytesToWrite = jmin<int64>(data.partSize, tmpInput->getNumBytesRemaining());

				WRITE_FLAG(Flag::BeginMonolithLength);
				fos->writeInt64(bytesToWrite);
				WRITE_FLAG(Flag::EndMonolithLength);

				WRITE_FLAG(Flag::ResumeMonolith);
				fos->writeFromInputStream(*tmpInput, bytesToWrite);
				fos->flush();
			}
			
			WRITE_FLAG(Flag::EndMonolith);
			

			jassert(tmpInput->isExhausted());

			fos->flush();

			tmpInput = nullptr;
			
		}

		WRITE_FLAG(Flag::EndOfArchive);
		fos->flush();
		fos = nullptr;

		tmpFile.deleteFile();
	}
#else

    ignoreUnused(data);


#endif
}

String HlacArchiver::getMetadataJSON(const File& sourceFile)
{
	ScopedPointer<FileInputStream> fis = new FileInputStream(sourceFile);

	return fis->readString();
}

#define RETURN_FLAG(x) if(f == Flag::x) return #x;

String HlacArchiver::getFlagName(Flag f)
{
	RETURN_FLAG(BeginMetadata);
	RETURN_FLAG(EndMetadata);
	RETURN_FLAG(BeginName);
	RETURN_FLAG(EndName);
	RETURN_FLAG(BeginTime);
	RETURN_FLAG(EndTime);
	RETURN_FLAG(BeginMonolithLength);
	RETURN_FLAG(EndMonolithLength);
	RETURN_FLAG(BeginMonolith);
	RETURN_FLAG(EndMonolith);
	RETURN_FLAG(SplitMonolith);
	RETURN_FLAG(ResumeMonolith);
	RETURN_FLAG(EndOfArchive);

	return "Undefined";
}

#undef RETURN

File HlacArchiver::getPartFile(const File& originalFile, int partIndex)
{
	String newFileName = originalFile.getFileNameWithoutExtension() + ".hr" + String(partIndex);

	VERBOSE_LOG("New Part " + newFileName);

	return originalFile.getSiblingFile(newFileName);
}

bool HlacArchiver::writeFlag(FileOutputStream* fos, Flag flag)
{
	VERBOSE_LOG("    W " + getFlagName(flag));

	if (fos != nullptr)
		return fos->writeInt((int)flag);

	return false;
}

bool HlacArchiver::readAndCheckFlag(FileInputStream* fis, Flag flag)
{
	VERBOSE_LOG("    R " + getFlagName(flag));

	if (fis != nullptr)
	{
		const Flag actualFlag = (Flag)fis->readInt();
		jassert(actualFlag == flag);
		return actualFlag == flag;
	}

	return false;
}

HlacArchiver::Flag HlacArchiver::readFlag(FileInputStream* fis)
{
	const Flag actualFlag = (Flag)fis->readInt();
	VERBOSE_LOG("    R " + getFlagName(actualFlag));

	return actualFlag;
}

#undef VERBOSE_LOG
#undef STATUS_LOG

} // namespace hlac