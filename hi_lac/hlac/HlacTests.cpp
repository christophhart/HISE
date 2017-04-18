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

#if HLAC_INCLUDE_TEST_SUITE

//static BitCompressors::UnitTests bitTests;

void BitCompressors::UnitTests::runTest()
{
	ScopedPointer<Base> compressor;

	testCompressor(compressor = new OneBit());
	testCompressor(compressor = new TwoBit());
	testCompressor(compressor = new FourBit());
	testCompressor(compressor = new SixBit());
	testCompressor(compressor = new EightBit());
	testCompressor(compressor = new TenBit());
	testCompressor(compressor = new TwelveBit());
	testCompressor(compressor = new FourteenBit());
	testCompressor(compressor = new SixteenBit());

	testAutomaticCompression(1);
	testAutomaticCompression(2);
	testAutomaticCompression(3);
	testAutomaticCompression(4);
	testAutomaticCompression(5);
	testAutomaticCompression(6);
	testAutomaticCompression(7);
	testAutomaticCompression(8);
	testAutomaticCompression(10);
	testAutomaticCompression(11);
	testAutomaticCompression(12);
	testAutomaticCompression(13);
	testAutomaticCompression(14);
	testAutomaticCompression(15);


}

void BitCompressors::UnitTests::testAutomaticCompression(uint8 maxBitSize)
{
	beginTest("Testing automatic compression with bit rate " + String(maxBitSize));

	Collection compressorCollection;
	Random r;

	const int numToCompress = r.nextInt(Range<int>(4000, 4100));
	int16* uncompressedData = (int16*)malloc(numToCompress * 2);

	fillDataWithAllowedBitRange(uncompressedData, numToCompress, maxBitSize);

	auto compressor = compressorCollection.getSuitableCompressorForData(uncompressedData, numToCompress);

	logMessage("Selected compressor for bitrate " + String(maxBitSize) + ": " + String(compressor->getAllowedBitRange()));

	expectEquals<int>(getMinBitDepthForData(uncompressedData, numToCompress), compressor->getAllowedBitRange(), "Compressor selection");

	const int byteSize = compressor->getByteAmount(numToCompress);
	auto minDepth = getMinBitDepthForData(uncompressedData, numToCompress, (int8)compressor->getAllowedBitRange());

	expect(minDepth == compressor->getAllowedBitRange(), "Bit range for " + String(compressor->getAllowedBitRange()) + " doesn't match");

	uint8* compressedData = (uint8*)malloc(byteSize);
	compressor->compress(compressedData, uncompressedData, numToCompress);
	int16* decompressedData = (int16*)malloc(numToCompress*2);
	const double start = Time::getMillisecondCounterHiRes();

	compressor->decompress(decompressedData, compressedData, numToCompress);

	const double stop = Time::getMillisecondCounterHiRes();
	const double delta = (stop - start);

	logMessage("Time: " + String(delta, 4) + " ms");

	for (int i = 0; i < numToCompress; i++)
	{
		expectEquals<int16>(decompressedData[i], uncompressedData[i], "Sample mismatch at position " + String(i));
	}

	free(uncompressedData);
	free(compressedData);
	free(decompressedData);
}


void BitCompressors::UnitTests::fillDataWithAllowedBitRange(int16* data, int size, int bitRange)
{
	Random r;

	switch (bitRange)
	{
	case 1:
	{
		for (int i = 0; i < size; i++)
		{
			data[i] = r.nextBool() ? 1 : 0;
		}
		break;
	}
	case 2:
	{
		for (int i = 0; i < size; i++)
		{
			data[i] = (int16)r.nextInt(Range<int>(-1, 2));
		}
		break;
	}
	case 3:
	case 4:
	{
		for (int i = 0; i < size; i++)
		{
			data[i] = (int16)r.nextInt(Range<int>(-7, 8));
		}
		break;
	}
	case 5:
	case 6:
	case 7:
	{
		for (int i = 0; i < size; i++)
		{
			data[i] = (int16)r.nextInt(Range<int>(-31, 32));
		}
		break;
	}
	case 8:
	{
		for (int i = 0; i < size; i++)
		{
			data[i] = (int16)r.nextInt(Range<int>(-127, 128));
		}
		break;
	}
	case 9:
	case 10:
	{
		for (int i = 0; i < size; i++)
		{
			data[i] = (int16)r.nextInt(Range<int>(-511, 512));
		}
		break;
	}
	case 11:
	case 12:
	{
		const int max = 1 << 11;

		for (int i = 0; i < size; i++)
		{
			data[i] = (int16)r.nextInt(Range<int>(-max + 1, max));
		}
		break;
	}
	case 13:
	case 14:
	{
		const int max = 1 << 13;

		for (int i = 0; i < size; i++)
		{
			data[i] = (int16)r.nextInt(Range<int>(-max + 1, max));
		}
		break;
	}
	case 15:
	case 16:
	{
		const int max = 1 << 15;

		for (int i = 0; i < size; i++)
		{
			data[i] = (int16)r.nextInt(Range<int>(-max + 1, max));
		}
		break;
	}

	}
}

void BitCompressors::UnitTests::testCompressor(Base* compressor)
{
	beginTest("Testing compression with bit rate " + String(compressor->getAllowedBitRange()));

	Random r;

	const int numToCompress = r.nextInt(Range<int>(4000, 4100));
	const int byteSize = compressor->getByteAmount(numToCompress);

	int16* uncompressedData = (int16*)malloc(sizeof(int16)*numToCompress);


	fillDataWithAllowedBitRange(uncompressedData, numToCompress, compressor->getAllowedBitRange());

	auto minDepth = getMinBitDepthForData(uncompressedData, numToCompress, (int8)compressor->getAllowedBitRange());

	expect(minDepth == compressor->getAllowedBitRange(), "Bit range for " + String(compressor->getAllowedBitRange()) + " doesn't match");

	uint8* compressedData = (uint8*)malloc(byteSize);

	compressor->compress(compressedData, uncompressedData, numToCompress);

	int16* decompressedData = (int16*)malloc(sizeof(int16)*numToCompress);

	const double start = Time::getMillisecondCounterHiRes();



	compressor->decompress(decompressedData, compressedData, numToCompress);


	const double stop = Time::getMillisecondCounterHiRes();

	const double delta = (stop - start);

	logMessage("Time: " + String(delta, 4) + " ms");



	for (int i = 0; i < numToCompress; i++)
	{
		expectEquals<int16>(decompressedData[i], uncompressedData[i], "Sample mismatch at position " + String(i));
	}

	free(uncompressedData);
	free(compressedData);
	free(decompressedData);
}

#endif

CodecTest::CodecTest() :
	UnitTest("Testing HLAC codec")
{
	HlacEncoder::CompressorOptions wholeBlock;

	wholeBlock.fixedBlockWidth = 512;
	wholeBlock.removeDcOffset = false;
	wholeBlock.useDeltaEncoding = false;
	wholeBlock.useDiffEncodingWithFixedBlocks = false;

	HlacEncoder::CompressorOptions delta;

	delta.fixedBlockWidth = -1;
	delta.removeDcOffset = false;
	delta.useDeltaEncoding = true;
	delta.bitRateForWholeBlock = 4;
	delta.useDiffEncodingWithFixedBlocks = false;
	delta.reuseFirstCycleLengthForBlock = true;
	delta.deltaCycleThreshhold = 0.1f;

	HlacEncoder::CompressorOptions diff;

	diff.fixedBlockWidth = 1024;
	diff.removeDcOffset = false;
	diff.useDeltaEncoding = false;
	diff.bitRateForWholeBlock = 4;
	diff.useDiffEncodingWithFixedBlocks = true;

	options[(int)Option::WholeBlock] = wholeBlock;
	options[(int)Option::Delta] = delta;
	options[(int)Option::Diff] = diff;
}

void CodecTest::runTest()
{
	testIntegerBuffers();

	const bool testDelta = true;

	for (int i = 0; i < (int)SignalType::numSignalTypes; i++)
	{
		if (i != (int)SignalType::MixedSine)
			//continue;

		beginTest("Testing codecs for signal " + getNameForSignal((SignalType)i));

		for (int j = 0; j < (int)Option::numCompressorOptions; j++)
		{
			if (!testDelta && j == (int)Option::Delta)
				continue;

			testCodec((SignalType)i, (Option)j);
			testCodec((SignalType)i, (Option)j);
			testCodec((SignalType)i, (Option)j);
			testCodec((SignalType)i, (Option)j);
		}
	}
}

void CodecTest::testIntegerBuffers()
{
	beginTest("Testing integer buffers");

	Random r;

	const int numSamples = CompressionHelpers::getPaddedSampleSize(r.nextInt(Range<int>(1000, 10000)));

	AudioSampleBuffer src(1, numSamples);

	for (int i = 0; i < (int)SignalType::numSignalTypes; i++)
	{
		logMessage("Testing " + getNameForSignal((SignalType)i));
		AudioSampleBuffer src = createTestSignal(numSamples, 1, (SignalType)i, 1.0f);

		CompressionHelpers::AudioBufferInt16 intBuffer(src, false);

		AudioSampleBuffer dst = intBuffer.getFloatBuffer();

		auto error = CompressionHelpers::checkBuffersEqual(dst, src);

		expectEquals<int>(error, 0, "Int Buffer conversion");
	}

	
}

void CodecTest::testCodec(SignalType type, Option option)
{
	logMessage("Testing " + getNameForOption(option));

	Random r;

	const int numSamples = r.nextInt(Range<int>(16373, 24000));
	const int numChannels = 1;

	MemoryOutputStream mos;

	HeapBlock<uint32> blockOffsets;
	blockOffsets.calloc(24000);

	HlacEncoder encoder;
	HlacDecoder decoder;

	decoder.setupForDecompression();

	auto ts1 = createTestSignal(numSamples, numChannels, type, jlimit<float>(0.5f, 1.0f, 0.99f));

	expect(ts1.getMagnitude(0, numSamples) <= 1.0f, "Peak > 1.0f");

	auto ts1_dst = AudioSampleBuffer(numChannels, CompressionHelpers::getPaddedSampleSize(numSamples));

	encoder.setOptions(options[(int)option]);
	encoder.compress(ts1, mos, blockOffsets);

	logMessage("Ratio: " + String(encoder.getCompressionRatio(), 3));

	MemoryInputStream mis(mos.getMemoryBlock(), true);

	decoder.decode(ts1_dst, mis);

	auto diffBitRate = CompressionHelpers::checkBuffersEqual(ts1_dst, ts1);

	expectEquals<int>((int)diffBitRate, 0, "Test codec with Signal type " + String((int)type) + " and compression mode " + String((int)option));


}

AudioSampleBuffer CodecTest::createTestSignal(int numSamples, int numChannels, SignalType type, float maxAmplitude)
{
	Random rd;

	AudioSampleBuffer b(numChannels, numSamples);

	float* l = b.getWritePointer(0);
	float* r = b.getWritePointer(1 % numChannels);

	switch (type)
	{
	case hlac::CodecTest::SignalType::Empty:
	{
		b.clear();
		break;
	}
	case hlac::CodecTest::SignalType::FullNoise:
	{
		for (int i = 0; i < numSamples; i++)
		{
			l[i] = 2.0f*rd.nextFloat() - 1.0f;
			r[i] = 2.0f*rd.nextFloat() - 1.0f;
		}

		break;
	}
	case hlac::CodecTest::SignalType::Static:
	{
		FloatVectorOperations::fill(l, 1.0f, numSamples);
		FloatVectorOperations::fill(r, 1.0f, numSamples);
	}
	case hlac::CodecTest::SignalType::SineOnly:
	{
		double uptime = 0.0;
		double uptimeDelta = 0.01 + rd.nextDouble() * 0.05;

		for (int i = 0; i < numSamples; i++)
		{
			l[i] = sin(uptime);
			uptime += uptimeDelta;
			r[i] = l[i];
		};
		
		break;
	}
	case hlac::CodecTest::SignalType::MixedSine:
	{
		double uptime = 0.0;
		double uptimeDelta = 0.01 + rd.nextDouble() * 0.05;

		for (int i = 0; i < numSamples; i++)
		{
			l[i] = sin(uptime) + rd.nextFloat() * 0.05f;
			uptime += uptimeDelta;
			r[i] = l[i] + rd.nextFloat() * 0.05f;
		};

		break;
	}
	case hlac::CodecTest::SignalType::DecayingSineWithHarmonic:
	{
		double uptime = 0.0;
		double uptimeDelta = 0.01 + rd.nextDouble() * 0.05;

		const float ra = jlimit<float>(0.7f, 0.8f, rd.nextFloat());
		const float h1 = jlimit<float>(0.2f, 0.5f, rd.nextFloat());
		const float h2 = jlimit<float>(0.1f, 0.3f, rd.nextFloat());
		
		for (int i = 0; i < numSamples; i++)
		{
			l[i] =  ra * sin(uptime);
			l[i] += h1 * sin(2.0f*uptime);
			l[i] += h2 * sin(3.0f*uptime);
			l[i] += rd.nextFloat() * 0.03f;

			uptime += (uptimeDelta + rd.nextFloat()*0.001f);
			
		};

		FloatVectorOperations::copy(r, l, numSamples);

		b.applyGainRamp(0, numSamples, 1.0f, 0.0f);
		b.applyGainRamp(0, numSamples, 1.0f, 0.0f);
		b.applyGainRamp(0, numSamples, 1.0f, 0.0f);
		b.applyGainRamp(0, numSamples, 1.0f, 0.0f);

		break;
	}
		
	case hlac::CodecTest::SignalType::numSignalTypes:
		break;
	default:
		break;
	}

	

	FloatVectorOperations::multiply(l, maxAmplitude, numSamples);
	FloatVectorOperations::multiply(r, maxAmplitude, numSamples);

	logMessage("Amplitude: " + String(Decibels::gainToDecibels(b.getMagnitude(0, numSamples)), 2) + " dB");

	FloatVectorOperations::clip(r, r, -1.0f, 1.0f, numSamples);
	FloatVectorOperations::clip(l, l, -1.0f, 1.0f, numSamples);

	return b;
}

String CodecTest::getNameForOption(Option o) const
{
	switch (o)
	{
	case hlac::CodecTest::Option::WholeBlock: return "Block";
		break;
	case hlac::CodecTest::Option::Delta: return "Delta";
		break;
	case hlac::CodecTest::Option::Diff: return "Diff";
		break;
	case hlac::CodecTest::Option::numCompressorOptions:
		break;
	default:
		break;
	}

	return "";
}

String CodecTest::getNameForSignal(SignalType s) const
{
	switch (s)
	{
	case hlac::CodecTest::SignalType::Empty: return "Empty";
		break;
	case hlac::CodecTest::SignalType::Static: return "Static";
	case hlac::CodecTest::SignalType::FullNoise: return "Full Noise";
		break;
	case hlac::CodecTest::SignalType::SineOnly: return "Sine Wave";
		break;
	case hlac::CodecTest::SignalType::MixedSine: return "Sine wave mixes with noise";
		break;
	case hlac::CodecTest::SignalType::DecayingSineWithHarmonic: return "Decaying sine wave";
		break;
	case hlac::CodecTest::SignalType::numSignalTypes:
		break;
	default:
		break;
	}

	return {};
}

static CodecTest codecTest;