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


#include "HlacTests.h"

#if HLAC_INCLUDE_TEST_SUITE

using IntBuffer = CompressionHelpers::AudioBufferInt16;

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
	options[(int)Option::WholeBlock] = HlacEncoder::CompressorOptions::getPreset(HlacEncoder::CompressorOptions::Presets::WholeBlock);
	options[(int)Option::Diff] = HlacEncoder::CompressorOptions::getPreset(HlacEncoder::CompressorOptions::Presets::Diff);

	options[0].normalisationMode = 2;
	options[1].normalisationMode = 2;

}

void CodecTest::runTest()
{
	testHiseSampleBufferClearing();

	return;

	testIntegerBuffers();
	testCopyWithNormalisation();
	testHiseSampleBuffer();

	testNormalisation();
	testHiseSampleBufferMinNormalisation();


	SignalType testOnly = SignalType::numSignalTypes;
	Option soloOption = Option::numCompressorOptions;
	bool testOnce = true;

	for (int i = 0; i < (int)SignalType::numSignalTypes; i++)
	{
		if (testOnly != SignalType::numSignalTypes && i != (int)testOnly)
			continue;

		beginTest("Testing codecs for signal " + getNameForSignal((SignalType)i));

		for (int j = 0; j < (int)Option::numCompressorOptions; j++)
		{
			if (soloOption != Option::numCompressorOptions && j != (int)soloOption)
				continue;

			logMessage("Testing " + getNameForOption((Option)j));

			testCodec((SignalType)i, (Option)j, false);

			if (testOnce)
				continue;

			testCodec((SignalType)i, (Option)j, false);
			testCodec((SignalType)i, (Option)j, true);
			testCodec((SignalType)i, (Option)j, false);
			testCodec((SignalType)i, (Option)j, true);
			testCodec((SignalType)i, (Option)j, false);
			testCodec((SignalType)i, (Option)j, true);
		}
	}

	
}

void CodecTest::testIntegerBuffers()
{
	beginTest("Testing integer buffers");

	Random r;

	const int numSamples = CompressionHelpers::getPaddedSampleSize(r.nextInt(Range<int>(1000, 10000)));

	for (int i = 0; i < (int)SignalType::numSignalTypes; i++)
	{
		logMessage("Testing " + getNameForSignal((SignalType)i));
		AudioSampleBuffer src = createTestSignal(numSamples, 1, (SignalType)i, r.nextFloat() * 0.6f + 0.4f);

		CompressionHelpers::AudioBufferInt16 intBuffer(src, 0, 0);

		AudioSampleBuffer dst = intBuffer.getFloatBuffer();

		auto error = CompressionHelpers::checkBuffersEqual(dst, src);

		expectEquals<int>(error, 0, "Int Buffer conversion");
	}	
}


void CodecTest::testNormalisation()
{
	beginTest("Test normalisation");

	Random r;

	float v = 0.326746434f;

	{
		auto gainedValue = (v * 1.0f * (float)INT16_MAX);
		int16 i = (int16)(gainedValue);

		float v2 = (float)i / ((float)INT16_MAX * 1.0f);

		auto diff = v2 - v;
		auto error = Decibels::gainToDecibels(diff);

		expectEquals(error, -100.0f);
	}

	{
		logMessage("testing static normalisation");

		AudioSampleBuffer src = createTestSignal(1024, 1, SignalType::Static, v);
		CompressionHelpers::AudioBufferInt16 dst(1024);

		dst.getMap().setMode(2);
		dst.getMap().normalise(src.getReadPointer(0), dst.getWritePointer(), 1024);

		AudioSampleBuffer exp(1, 1024);

		dst.getMap().normalisedInt16ToFloat(exp.getWritePointer(0), dst.getReadPointer(), 0, 1024);

		float v1 = src.getSample(0, 0);
		float v2 = exp.getSample(0, 0);

		auto error = CompressionHelpers::checkBuffersEqual(src, exp);

		expectEquals<int>(error, 0, "Normalisation");
	}

	{
		logMessage("Testing odd offsets and normalisation in smaller chunks");

		AudioSampleBuffer src = createTestSignal(2500, 1, SignalType::Static, 0.3f);
		CompressionHelpers::AudioBufferInt16 dst(2500);

		dst.getMap().setMode(2);
		dst.getMap().normalise(src.getReadPointer(0), dst.getWritePointer(), 2500);

		AudioSampleBuffer exp(1, 2500);

		dst.getMap().normalisedInt16ToFloat(exp.getWritePointer(0), dst.getReadPointer(), 0, 315);

		dst.getMap().normalisedInt16ToFloat(exp.getWritePointer(0, 315), dst.getReadPointer() + 315, 315, 2500-315);

		float v1 = src.getSample(0, 0);
		float v2 = exp.getSample(0, 0);

		auto error = CompressionHelpers::checkBuffersEqual(src, exp);

		expectEquals<int>(error, 0, "Normalisation");
	}

	const int numSamples = CompressionHelpers::getPaddedSampleSize(r.nextInt(Range<int>(1000, 10000)));

	for (int i = 4; i < (int)SignalType::numSignalTypes; i++)
	{
		logMessage("Testing " + getNameForSignal((SignalType)i));
		AudioSampleBuffer src = createTestSignal(numSamples, 1, (SignalType)i, v); // r.nextFloat() * 0.6f + 0.4f

		auto mag = Decibels::gainToDecibels(src.getMagnitude(0, src.getNumSamples()));

		CompressionHelpers::AudioBufferInt16 intBuffer(src, 0, 2);

		AudioSampleBuffer dst = intBuffer.getFloatBuffer();

		float v1 = src.getSample(0, 0);
		float v2 = dst.getSample(0, 0);

		auto error = CompressionHelpers::checkBuffersEqual(dst, src);

		if (error != 0)
		{
			dst = intBuffer.getFloatBuffer();
		}

		expectEquals<int>(error, 0, "Normalisation");
	}
}


void CodecTest::testHiseSampleBuffer()
{
	beginTest("Testing HiseSampleBuffer");

	Random r;

	const int numSamples = CompressionHelpers::getPaddedSampleSize(r.nextInt(Range<int>(8000, 10000)));

	AudioSampleBuffer src = createTestSignal(numSamples, 1, SignalType::DecayingSineWithHarmonic, 0.6f);

	HeapBlock<uint32> blockOffsets;
	blockOffsets.calloc(10000);

	HlacEncoder encoder;
	encoder.setOptions(options[(int)Option::Diff]);

	MemoryOutputStream mos;

	HiseSampleBuffer s1 = HiseSampleBuffer(src);
	encoder.compress(src, mos, blockOffsets);
	
	HlacDecoder decoder;

	decoder.setupForDecompression();

	const int offset = 948;
	const int numSamplesToDecode = numSamples - offset;

	HiseSampleBuffer dst = HiseSampleBuffer(false, 1, numSamples);

	MemoryInputStream mis(mos.getMemoryBlock(), true);

	decoder.decode(dst, false, mis, offset, numSamplesToDecode);

	auto b1 = CompressionHelpers::getPart(src, offset, numSamplesToDecode);
	auto b2 = CompressionHelpers::getPart(dst, 0, numSamplesToDecode);

	auto error = CompressionHelpers::checkBuffersEqual(b1, b2);

	expectEquals<int>((int)error, 0, "Test HiseSampleBuffer");
}

void CodecTest::testCodec(SignalType type, Option option, bool /*testStereo*/)
{
	

	Random r;

	const int numSamples = r.nextInt(Range<int>(16373, 24000));
	const int numChannels = 1;

	MemoryOutputStream mos;

	HeapBlock<uint32> blockOffsets;
	blockOffsets.calloc(24000);

	HlacEncoder encoder;
	HlacDecoder decoder;

	decoder.setupForDecompression();

	auto ts1 = createTestSignal(numSamples, numChannels, type, r.nextFloat()*0.4f + 0.6f);

	expect(ts1.getMagnitude(0, numSamples) <= 1.0f, "  Peak > 1.0f");

	auto ts1_dst = HiseSampleBuffer(true, numChannels, CompressionHelpers::getPaddedSampleSize(numSamples));

	encoder.setOptions(options[(int)option]);
	encoder.compress(ts1, mos, blockOffsets);

	logMessage("  Ratio: " + String(encoder.getCompressionRatio(), 3));

	MemoryInputStream mis(mos.getMemoryBlock(), true);

	decoder.decode(ts1_dst, numChannels > 1, mis);

	auto diffBitRate = CompressionHelpers::checkBuffersEqual(*ts1_dst.getFloatBufferForFileReader(), ts1);

	expectEquals<int>((int)diffBitRate, 0, "Test codec with Signal type " + String((int)type) + " and compression mode " + String((int)option));


}

void CodecTest::testCopyWithNormalisation()
{
	beginTest("Testing copying with normalisation");

	{
		logMessage("Testing appending buffers");

		AudioSampleBuffer b1(1, 5000);

		FloatVectorOperations::fill(b1.getWritePointer(0), 0.1f, 5000);

		AudioSampleBuffer b2(1, 5000);

		FloatVectorOperations::fill(b1.getWritePointer(0), 0.05f, 5000);

		AudioSampleBuffer b3(1, 10000);

		FloatVectorOperations::copy(b3.getWritePointer(0), b1.getReadPointer(0), 5000);
		FloatVectorOperations::copy(b3.getWritePointer(0) + 5000, b2.getReadPointer(0), 5000);

		CompressionHelpers::AudioBufferInt16 ib1(b1, 0, 2);
		CompressionHelpers::AudioBufferInt16 ib2(b2, 0, 2);

		AudioSampleBuffer b4(1, 10000);

		ib1.getMap().normalisedInt16ToFloat(b4.getWritePointer(0), ib1.getReadPointer(), 0, 5000);
		ib2.getMap().normalisedInt16ToFloat(b4.getWritePointer(0) + 5000, ib2.getReadPointer(), 0, 5000);

		auto error = CompressionHelpers::checkBuffersEqual(b3, b4);

		expectEquals<int>(error, 0, "Copy error");
	}
}

AudioSampleBuffer CodecTest::createTestSignal(int numSamples, int numChannels, SignalType type, float maxAmplitude)
{
	Random rd;

	AudioSampleBuffer b(numChannels, numSamples);

	float* l = b.getWritePointer(0);
	float* r = b.getWritePointer(1 % numChannels);

	switch (type)
	{
	case CodecTest::SignalType::Empty:
	{
		b.clear();
		break;
	}
	case CodecTest::SignalType::RampDown:
	{
		for (int i = 0; i < numSamples; i++)
		{
			l[i] = 1.0f - (float)i / (float)numSamples;

			l[i] = l[i] * l[i];

			r[i] = l[i];
		};

		break;
	}
	case CodecTest::SignalType::FullNoise:
	{
		for (int i = 0; i < numSamples; i++)
		{
			l[i] = 2.0f*rd.nextFloat() - 1.0f;
			r[i] = 2.0f*rd.nextFloat() - 1.0f;
		}

		break;
	}
	case CodecTest::SignalType::Static:
	{
		FloatVectorOperations::fill(l, 1.0f, numSamples);
		FloatVectorOperations::fill(r, 1.0f, numSamples);
		break;
	}
	case CodecTest::SignalType::SineOnly:
	{
		double uptime = 0.0;
		double uptimeDelta = 0.01 + rd.nextDouble() * 0.05;

		for (int i = 0; i < numSamples; i++)
		{
			l[i] = (float)sin(uptime);
			uptime += uptimeDelta;
			r[i] = l[i];
		};
		
		break;
	}
	case CodecTest::SignalType::MixedSine:
	{
		double uptime = 0.0;
		double uptimeDelta = 0.01 + rd.nextDouble() * 0.05;

		for (int i = 0; i < numSamples; i++)
		{
			l[i] = (float)sin(uptime) + rd.nextFloat() * 0.05f;
			uptime += uptimeDelta;
			r[i] = l[i] + rd.nextFloat() * 0.05f;
		};

		break;
	}
	case CodecTest::SignalType::DecayingSineWithHarmonic:
	{
		double uptime = 0.0;
		double uptimeDelta = 0.01 + rd.nextDouble() * 0.05;

		const float ra = jlimit<float>(0.7f, 0.8f, rd.nextFloat());
		const float h1 = jlimit<float>(0.2f, 0.5f, rd.nextFloat());
		const float h2 = jlimit<float>(0.1f, 0.3f, rd.nextFloat());
		
		for (int i = 0; i < numSamples; i++)
		{
			l[i] =  ra * (float)sin(uptime);
			l[i] += h1 * (float)sin(2.0*uptime);
			l[i] += h2 * (float)sin(3.0*uptime);
			l[i] += rd.nextFloat() * 0.03f;

			uptime += (uptimeDelta + rd.nextFloat()*0.01f);
			
		};

		FloatVectorOperations::copy(r, l, numSamples);

		b.applyGainRamp(0, numSamples, 1.0f, 0.0f);
		b.applyGainRamp(0, numSamples, 1.0f, 0.0f);
		b.applyGainRamp(0, numSamples, 1.0f, 0.0f);
		b.applyGainRamp(0, numSamples, 1.0f, 0.0f);

		break;
	}
	case CodecTest::SignalType::NastyDiracTrain:
	{
		FloatVectorOperations::clear(l, numSamples);
		FloatVectorOperations::clear(r, numSamples);

		for (int i = 0; i < 256; i+=16)
		{
			l[i] = 0.5f;
			l[i + 1] = 1.0f;
			l[i + 2] = -1.0f;
			r[i] = 0.5f;
			r[i + 1] = 1.0f;
			r[i + 2] = -1.0f;
		}

		break;
	}
	case CodecTest::SignalType::numSignalTypes:
		break;
	default:
		break;
	}

	b.applyGain(maxAmplitude);

	//logMessage("  Amplitude: " + String(Decibels::gainToDecibels(b.getMagnitude(0, numSamples)), 2) + " dB");

	FloatVectorOperations::clip(r, r, -1.0f, 1.0f, numSamples);
	FloatVectorOperations::clip(l, l, -1.0f, 1.0f, numSamples);

	return b;
}

String CodecTest::getNameForOption(Option o) const
{
	switch (o)
	{
	case CodecTest::Option::WholeBlock: return "Block";
		break;
	case CodecTest::Option::Diff: return "Diff";
		break;
		
	case CodecTest::Option::numCompressorOptions:
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
	case CodecTest::SignalType::Empty: return "Empty";
		break;
	case CodecTest::SignalType::Static: return "Static";
	case CodecTest::SignalType::FullNoise: return "Full Noise";
		break;
	case CodecTest::SignalType::SineOnly: return "Sine Wave";
		break;
	case CodecTest::SignalType::MixedSine: return "Sine wave mixes with noise";
		break;
	case CodecTest::SignalType::DecayingSineWithHarmonic: return "Decaying sine wave";
		break;
	case CodecTest::SignalType::NastyDiracTrain:	return "Nasty Dirac Train";
	case CodecTest::SignalType::numSignalTypes:
		break;
	default:
		break;
	}

	return {};
}

void CodecTest::testHiseSampleBufferMinNormalisation()
{
	beginTest("Testing HiseSampleBuffer normalisation");
	int s1 = 3000;
	int s2 = 6015;

	auto src1 = createTestSignal(s1, 1, SignalType::RampDown, 0.4f);
	auto src2 = createTestSignal(s2, 1, SignalType::RampDown, 0.1f);

	AudioSampleBuffer expected(1, s1 + s2);

	expected.copyFrom(0, 0, src1, 0, 0, s1);
	expected.copyFrom(0, s1, src2, 0, 0, s2);

	IntBuffer i1(src1, 0, 2);
	IntBuffer i2(src2, 0, 2);

	HiseSampleBuffer h1(std::move(i1));
	HiseSampleBuffer h2(std::move(i2));

	HiseSampleBuffer h3(false, 1, s1 + s2);

	HiseSampleBuffer::copy(h3, h1, 0, 0, s1);

	int p1 = 50;
	int p2 = 601;

	HiseSampleBuffer::copy(h3, h2, s1, 0, p1);
	HiseSampleBuffer::copy(h3, h2, s1 + p1, p1, p2);
	HiseSampleBuffer::copy(h3, h2, s1 + p1 + p2, p1 + p2, s2 - p2 - p1);

	AudioSampleBuffer result(1, h3.getNumSamples());

	h3.convertToFloatWithNormalisation(result.getArrayOfWritePointers(), 1, 0, h3.getNumSamples());

	auto error = CompressionHelpers::checkBuffersEqual(expected, result);

	expectEquals<int>(error, 0);
}


void CodecTest::testHiseSampleBufferClearing()
{
	int numSamples = 44100;
	int offset = 10000;

	beginTest("Testing clearing of normalisation ranges");

	auto bBig = createTestSignal(numSamples, 1, SignalType::RampDown, 1.0f);

	auto bSmall = createTestSignal(numSamples/2, 1, SignalType::RampDown, 1.0f);
	
	auto expected = AudioSampleBuffer(1, numSamples);

	FloatVectorOperations::copy(expected.getWritePointer(0, 0), bBig.getReadPointer(0, 0), numSamples);

	FloatVectorOperations::copy(expected.getWritePointer(0, offset), bSmall.getReadPointer(0, 0), bSmall.getNumSamples());

	CompressionHelpers::dump(expected, "Original.wav");

	IntBuffer bigI(bBig, 0, 2);
	IntBuffer smallI(bSmall, 0, 2);

	HiseSampleBuffer bigHise(std::move(bigI));
	HiseSampleBuffer smallHise(std::move(smallI));
	HiseSampleBuffer::copy(bigHise, smallHise, offset, 0, smallHise.getNumSamples());

	AudioSampleBuffer result(1, numSamples);

	bigHise.convertToFloatWithNormalisation(result.getArrayOfWritePointers(), 1, 0, numSamples);

	CompressionHelpers::dump(result, "Result.wav");

	auto error = CompressionHelpers::checkBuffersEqual(expected, result);

	expectEquals<int>(error, 0, "Clearing doesn't work");

}

static CodecTest codecTest;

class FormatTest : public UnitTest
{
public:

	FormatTest() :
		UnitTest("Testing HLAC Format")
	{

	}

	void runTest() override
	{
		currentOption = HlacEncoder::CompressorOptions::getPreset(HlacEncoder::CompressorOptions::Presets::Diff);
		currentOption.normalisationMode = 2;

		auto s = createTestBuffer(1, 20000);

		CompressionHelpers::dump(s, "Decay.wav");

		IntBuffer i(s, 0, 2);
		HiseSampleBuffer h(std::move(i));

		AudioSampleBuffer c(1, 20000);

		h.convertToFloatWithNormalisation(c.getArrayOfWritePointers(), 1, 0, 20000);

		CompressionHelpers::dump(c, "DecayNormalised.wav");

		testHiseSampleBufferReadWithOffset();

		testHeader();

		testArchiver();

		testFixedSampleBuffer();

		runFormatTestWithOption(HlacEncoder::CompressorOptions::Presets::WholeBlock);
		runFormatTestWithOption(HlacEncoder::CompressorOptions::Presets::Diff);

		testSeeking(1);
		testSeeking(2);

        testReadOperationWithSmallBlockSizes(1, 300000);
        testReadOperationWithSmallBlockSizes(2, 30000);
        
		testStreamingEngineOperation(1, 0, 512);
		testStreamingEngineOperation(2, 0, 512);

		testStreamingEngineOperation(1, 1024, 1024);
		testStreamingEngineOperation(2, 1024, 300);

		testStreamingEngineOperation(1, 4096, 64);
		testStreamingEngineOperation(2, 4096, 201);
		
		testStreamingEngineOperation(1, 701, 785);
		testStreamingEngineOperation(2, 810, 14);

		testStreamingEngineOperation(1, 3103, 800);
		testStreamingEngineOperation(2, 2098, 50);

		testStreamingEngineOperation(1, 24, 256);
		testStreamingEngineOperation(2, 6, 120);

		testStreamingEngineOperation(1, 4095, 1024);
		testStreamingEngineOperation(2, 8190, 16);


#if JUCE_64BIT
		testMemoryMappedFileReaders(1, 3000000);
		testMemoryMappedFileReaders(2, 3000000);

		testMemoryMappedBufferedRead(1, 300000);
		testMemoryMappedBufferedRead(2, 300000);
		
		testMemoryMappedSeeking(1, 240000);
		testMemoryMappedSeeking(2, 240000);

		testMemoryMappedSubsectionReaders(1, 160000);
		testMemoryMappedSubsectionReaders(2, 160000);

		testMemoryMappedReadingWithOffset(1, 32768);
		testMemoryMappedReadingWithOffset(2, 160000);
#endif

		testFlacReadPerformance(1, 12000000);
		testFlacReadPerformance(2, 12000000);

		testUncompressedReadPerformance(1, 12000000);
		testUncompressedReadPerformance(2, 12000000);
		
	}

	void runFormatTestWithOption(HlacEncoder::CompressorOptions::Presets option)
	{
		logMessage(String("Testing compression mode ") + String((int)option));

		

		currentOption = HlacEncoder::CompressorOptions::getPreset(option);
		currentOption.normalisationMode = 2;

		

		testHeader();

		testDualWrite(1);
		testDualWrite(2);

		testPadding(1);
        testPadding(2);
	
		for (int i = 0; i < 5; i++)
		{
			testSeeking(1);
			testSeeking(2);
		}
	}

	void testHeader()
	{
		return;

		beginTest("Testing header");

		HiseLosslessAudioFormat hlac;

		Random r;

		for (int i = 0; i < 200; i++)
		{
			bool useEncryption = false;
			uint8 globalShiftAmount = (uint8)r.nextInt(15);

			const double sampleRates[4] = { 44100.0, 48000.0, 88200.0, 96000.0 };
			double sampleRate = sampleRates[r.nextInt(4)];
			const int numChannels = r.nextInt(Range<int>(1, 8));
			const int bitDepth = r.nextBool() ? 16 : 24;
			bool useCompression = r.nextBool();
			const int numBlocks = r.nextInt(Range<int>(5, 900));

			HiseLosslessHeader header = HiseLosslessHeader(useEncryption, globalShiftAmount, sampleRate, numChannels, bitDepth, useCompression, numBlocks);

			expect(useEncryption == header.isEncrypted(), "Encryption");
			expectEquals<int>(header.getBitShiftAmount(), globalShiftAmount, "Shift amount");
			expectEquals<double>(header.getSampleRate(), sampleRate, "Sample rate");
			expectEquals<int>(header.getNumChannels(), numChannels, "Channel amount");
			expectEquals<int>(header.getBitsPerSample(), bitDepth, "Bit depth");
			expect(useCompression == header.usesCompression(), "Use Compression");
			expectEquals<int>(header.getBlockAmount(), numBlocks, "Block amount");
		}

		return;

		beginTest("Testing checksum validator");

		expect(CompressionHelpers::Misc::validateChecksum(0) == false, "Zero must not pass checksum validator");

		for (int i = 0; i < 1000; i++)
		{
			uint32 c = CompressionHelpers::Misc::createChecksum();
			expect(CompressionHelpers::Misc::validateChecksum(c), "Checksum test " + String(i));
			expect(!CompressionHelpers::Misc::validateChecksum(r.nextInt()), "Checksum test negative " + String(i));
		}

	}

	void testFixedSampleBuffer()
	{
		beginTest("Testing fixed sample buffer");

		const int numSamples = 1000;

		hlac::FixedSampleBuffer b1 = FixedSampleBuffer(numSamples);

		hlac::FixedSampleBuffer b2 = FixedSampleBuffer(numSamples);

		auto d = b1.getWritePointer(0);
		auto e = b2.getWritePointer(0);

		for (int i = 0; i < numSamples; i++)
		{
			d[i] = 1000;
			e[i] = 1000;
		};

		b1.applyGainRamp(0, numSamples, 0.0f, 1.0f);
		b2.applyGainRamp(0, numSamples, 1.0f, 0.0f);

		expectEquals<int16>(d[0], 0, "d[0]");
		expectEquals<int16>(d[500], 500, "d[500]");
		expectEquals<int16>(d[999], 999, "d[999]");

		expectEquals<int16>(e[0], 1000, "e[0]");
		expectEquals<int16>(e[500], 499, "e[500]");
		expectEquals<int16>(e[999], 0, "e[999]");

		hlac::FixedSampleBuffer b3 = FixedSampleBuffer(numSamples);

		auto s = b3.getWritePointer();

		CompressionHelpers::IntVectorOperations::add(s, b1.getWritePointer(), numSamples);
		CompressionHelpers::IntVectorOperations::add(s, b2.getWritePointer(), numSamples);

		expectEquals<int16>(s[0], 1000, "e[0]");
		expectEquals<int16>(s[500], 999, "e[500]");
		expectEquals<int16>(s[999], 999, "e[999]");
	}

	AudioSampleBuffer createTestBuffer(int numChannels=1, int size=44100)
	{
		Random r;

		int64 upperLimit = (10 * (int64)size) / 8;
		int64 lowerLimit = (6 * (int64)size) / 8; 

		//size = r.nextInt(Range<int>((int)lowerLimit, (int)upperLimit));

		return CodecTest::createTestSignal(size, numChannels, CodecTest::SignalType::DecayingSineWithHarmonic, 0.9f);
	}

	MemoryBlock writeIntoMemory(Array<AudioSampleBuffer>& buffers)
	{
		Random r;

		HiseLosslessAudioFormat hlac;

		MemoryOutputStream* mos = new MemoryOutputStream();

		StringPairArray empty;

		ScopedPointer<HiseLosslessAudioFormatWriter> writer = dynamic_cast<HiseLosslessAudioFormatWriter*>(hlac.createWriterFor(mos, 44100.0, buffers[0].getNumChannels(), 0, empty, 0));

		currentOption.normalisationMode = 2;

		writer->setOptions(currentOption);
		
		expect(writer != nullptr);

		if (writer != nullptr)
		{
			for (int i = 0; i < buffers.size(); i++)
			{
				writer->writeFromAudioSampleBuffer(buffers[i], 0, buffers[i].getNumSamples());
			}
		}

		writer->flush();

		MemoryBlock mb;

		mb.setSize(mos->getDataSize());
		mb.copyFrom(mos->getData(), 0, mos->getDataSize());

		return mb;
	}

	HiseLosslessAudioFormatReader* createReader(MemoryBlock& mb, bool targetIsFloat)
	{
		HiseLosslessAudioFormat hlac;

		MemoryInputStream* mis = new MemoryInputStream(mb, false);

		auto reader = dynamic_cast<HiseLosslessAudioFormatReader*>(hlac.createReaderFor(mis, true));

        reader->setTargetAudioDataType(AudioDataConverters::DataFormat::float32BE);
        
		expect(reader != nullptr, "Create Reader");

		return reader;
	}

	AudioSampleBuffer readIntoAudioBuffer(MemoryBlock& mb, bool isFloatTarget)
	{
		ScopedPointer<HiseLosslessAudioFormatReader> reader = createReader(mb, isFloatTarget);

		AudioSampleBuffer b2(reader->numChannels, (int)reader->lengthInSamples);

		reader->read(&b2, 0, (int)reader->lengthInSamples, 0, true, true);

		return b2;
	}

	void testArchiver()
	{
		beginTest("Testing Archiver");

		int numChannels = 1;

		Array<AudioSampleBuffer> buffers;

		for (int i = 0; i < 12; i++)
		{
			buffers.add(createTestBuffer(numChannels));
		}
	}
	

	void testPadding(int numChannels)
	{
		beginTest("Testing zero padding with " + String(numChannels) + " channels");

		Array<AudioSampleBuffer> buffers;

		buffers.add(createTestBuffer(numChannels));
		auto mb = writeIntoMemory(buffers);
		auto b2 = readIntoAudioBuffer(mb, true);

		int error = (int)CompressionHelpers::checkBuffersEqual(b2, buffers.getReference(0));

		expect(b2.getNumSamples() % COMPRESSION_BLOCK_SIZE == 0, "zero pad amount matches");
		expectEquals<int>(error, 0, "Error after reading");
	}

	int randomizeChannelAmount()
	{
		Random r;
		return r.nextBool() ? 1 : 2;
	}

	void testDualWrite(int numChannels)
	{
		beginTest("Testing double write with " + String(numChannels) + " channels");

		Array<AudioSampleBuffer> buffers;

		buffers.add(createTestBuffer(numChannels));
		buffers.add(createTestBuffer(numChannels));

		for (auto& b : buffers)
		{
			logMessage("Buffer Size: " + String(b.getNumSamples()));
		}

		auto mb = writeIntoMemory(buffers);

		auto both = readIntoAudioBuffer(mb, false);

		const int paddedFirst = CompressionHelpers::getPaddedSampleSize(buffers[0].getNumSamples());
		const int paddedSecond = CompressionHelpers::getPaddedSampleSize(buffers[1].getNumSamples());

		expectEquals<int>(both.getNumSamples(), paddedFirst + paddedSecond, "Size");

		AudioSampleBuffer both2(numChannels, paddedFirst + paddedSecond);
		both2.clear();

		both2.copyFrom(0, 0, buffers[0], 0, 0, buffers[0].getNumSamples());
		both2.copyFrom(0, paddedFirst, buffers[1], 0, 0, buffers[1].getNumSamples());

		if (numChannels > 1)
		{
			both2.copyFrom(1, 0, buffers[0], 1, 0, buffers[0].getNumSamples());
			both2.copyFrom(1, paddedFirst, buffers[1], 1, 0, buffers[1].getNumSamples());
		}

		int error = (int)CompressionHelpers::checkBuffersEqual(both2, both);

		if (error > 0)
		{
			logMessage("Option:\n" + currentOption.toString());
		}

		

		

		expectEquals<int>(error, 0, "buffers equal");

	}

	void testHiseSampleBufferReadWithOffset()
	{
		beginTest("Test decoding into buffer with offset");

		Array<AudioSampleBuffer> buffers;

		auto tb = createTestBuffer();

		buffers.add(tb);

		int numSamples = buffers.getLast().getNumSamples();

		auto mb = writeIntoMemory(buffers);

		ScopedPointer<HiseLosslessAudioFormatReader> reader = createReader(mb, false);

		HlacSubSectionReader sub(reader, 0, numSamples);
		HiseSampleBuffer b(false, 1, numSamples);

		sub.readIntoFixedBuffer(b, 3000, numSamples - 3000, 3000);

		auto expected = CompressionHelpers::getPart(tb, 3000, numSamples - 3000);
		auto result = AudioSampleBuffer(1, numSamples - 3000);
		
		b.convertToFloatWithNormalisation(result.getArrayOfWritePointers(), 1, 3000, numSamples - 3000);

		auto error = CompressionHelpers::checkBuffersEqual(expected, result);
		expectEquals<int>(error, 0, "Buffer read with offset doesn't work");
	}

	void testSeeking(int numChannels)
	{
		beginTest("Test seeking with " + String(numChannels) + " channels");

		AudioSampleBuffer big(numChannels, 3000000);

		auto signal = createTestBuffer(numChannels);

		Random r;

		int signalLength = signal.getNumSamples();
		int offset = r.nextInt(Range<int>(0, big.getNumSamples() - 3 * signalLength)) % COMPRESSION_BLOCK_SIZE;

		offset = 400;

		big.clear();
		big.copyFrom(0, offset, signal, 0, 0, signalLength);
		
		if(numChannels > 1)
			big.copyFrom(1, offset, signal, 1, 0, signalLength);

		Array<AudioSampleBuffer> buffers;
		buffers.add(big);

		auto mb = writeIntoMemory(buffers);

		AudioSampleBuffer signalCopy(numChannels, CompressionHelpers::getPaddedSampleSize(signalLength));

		signalCopy.clear();

		ScopedPointer<HiseLosslessAudioFormatReader> reader = createReader(mb, true);

		FloatVectorOperations::fill(signalCopy.getWritePointer(0), 1.0f, 400);

		reader->read(&signalCopy, 0, signalLength, offset, true, true);

		int error = (int)CompressionHelpers::checkBuffersEqual(signalCopy, signal);

		if (error > 0)
		{
			logMessage("Signal Length: " + String(signal.getNumSamples()));
			logMessage("Offset: " + String(offset));


			logMessage("Option:\n" + currentOption.toString());


		}



		expectEquals<int>(error, 0, "Seeking");
	}

	void testFlacReadPerformance(int numChannels, int length)
	{
		FlacAudioFormat flac;

		auto signal = createTestBuffer(numChannels, length);

		length = signal.getNumSamples();

		int mb = length / 1024 / 1024;

		beginTest("Testing FLAC file reader performance with " + String(numChannels) + " channels and length " + String(mb) + "MB");

		TemporaryFile tempFile;

		File f = tempFile.getFile();

		FileOutputStream* fos = new FileOutputStream(f);

		StringPairArray empty;

		ScopedPointer<AudioFormatWriter> writer = flac.createWriterFor(fos, 44100.0, numChannels, 16, empty, 5);

		int numBytesWritten = 0;

		if (writer != nullptr)
		{
			writer->writeFromAudioSampleBuffer(signal, 0, signal.getNumSamples());
			writer->flush();
			numBytesWritten = fos->getPosition();
			writer = nullptr;
			fos = nullptr;
		}

		FileInputStream* fisN = new FileInputStream(f);
		
		ScopedPointer<AudioFormatReader> normalReader = flac.createReaderFor(fisN, false);

		expectEquals<int>(normalReader->lengthInSamples, length, "FLAC Temp file write OK");


		AudioSampleBuffer signalFLAC = AudioSampleBuffer(numChannels, length);

		const double start1 = Time::getMillisecondCounterHiRes();
		normalReader->read(&signalFLAC, 0, length, 0, true, true);
		const double end1 = Time::getMillisecondCounterHiRes();

		const double delta1 = (end1 - start1) / 1000.0;
		const double sampleLengthInSeconds = (double)length / 44100.0;

		logMessage("Read speed for FLAC reader: " + String(sampleLengthInSeconds / delta1, 1) + "x realtime");

	}


	void testUncompressedReadPerformance(int numChannels, int length)
	{
		WavAudioFormat waf;

		auto signal = createTestBuffer(numChannels, length);

		length = signal.getNumSamples();

		int mb = length / 1024 / 1024;

		beginTest("Testing WAV file reader performance with " + String(numChannels) + " channels and length " + String(mb) + "MB");

		TemporaryFile tempFile;

		File f = tempFile.getFile();

		FileOutputStream* fos = new FileOutputStream(f);

		StringPairArray empty;

		ScopedPointer<AudioFormatWriter> writer = waf.createWriterFor(fos, 44100.0, numChannels, 16, empty, 5);

		int numBytesWritten = 0;

		if (writer != nullptr)
		{
			writer->writeFromAudioSampleBuffer(signal, 0, signal.getNumSamples());
			writer->flush();
			numBytesWritten = fos->getPosition();
			writer = nullptr;
			fos = nullptr;
		}

		FileInputStream* fisN = new FileInputStream(f);

		ScopedPointer<AudioFormatReader> normalReader = waf.createReaderFor(fisN, false);

		expectEquals<int>(normalReader->lengthInSamples, length, "WAV Temp file write OK");

		AudioSampleBuffer signalFLAC = AudioSampleBuffer(numChannels, length);

		const double start1 = Time::getMillisecondCounterHiRes();
		normalReader->read(&signalFLAC, 0, length, 0, true, true);
		const double end1 = Time::getMillisecondCounterHiRes();

		const double delta1 = (end1 - start1) / 1000.0;
		const double sampleLengthInSeconds = (double)length / 44100.0;

		logMessage("Read speed for WAV reader: " + String(sampleLengthInSeconds / delta1, 1) + "x realtime");



		FileInputStream* fisM = new FileInputStream(f);

		ScopedPointer<MemoryMappedAudioFormatReader> memoryReader = waf.createMemoryMappedReader(fisM);

		expectEquals<int>(memoryReader->lengthInSamples, length, "WAV Temp file write OK");

		AudioSampleBuffer signalMemory = AudioSampleBuffer(numChannels, length);

		memoryReader->mapEntireFile();

		const double start2 = Time::getMillisecondCounterHiRes();

		


		memoryReader->read(&signalMemory, 0, length, 0, true, true);
		const double end2 = Time::getMillisecondCounterHiRes();

		const double delta2 = (end2 - start2) / 1000.0;

		logMessage("Read speed for memory mapped WAV reader: " + String(sampleLengthInSeconds / delta2, 1) + "x realtime");

	}

	void testMemoryMappedFileReaders(int numChannels, int length)
	{
		HiseLosslessAudioFormat hlac;

		auto signal = createTestBuffer(numChannels, length);

		length = signal.getNumSamples();

		int mb = length / 1024 / 1024;

		beginTest("Testing memory mapped file readers with " + String(numChannels) + " channels and length " + String(mb) + "MB");

		TemporaryFile tempFile;

		File f = tempFile.getFile();

		FileOutputStream* fos = new FileOutputStream(f);

		StringPairArray empty;

		ScopedPointer<AudioFormatWriter> writer = hlac.createWriterFor(fos, 44100.0, numChannels, 0, empty, 0);

		int numBytesWritten = 0;

		if (writer != nullptr)
		{
			writer->writeFromAudioSampleBuffer(signal, 0, signal.getNumSamples());
			writer->flush();
			numBytesWritten = fos->getPosition();
			writer = nullptr;
			fos = nullptr;
		}

		length = CompressionHelpers::getPaddedSampleSize(length);

		FileInputStream* fisN = new FileInputStream(f);
		FileInputStream* fisM = new FileInputStream(f);

		expectEquals<int>(fisN->getTotalLength(), numBytesWritten, "Bytes written");

		ScopedPointer<AudioFormatReader> normalReader = hlac.createReaderFor(fisN, false);
		ScopedPointer<MemoryMappedAudioFormatReader> memoryReader = hlac.createMemoryMappedReader(fisM);

#if JUCE_64BIT
		expect(normalReader != nullptr, "Normal Reader OK");
		expect(memoryReader != nullptr, "Normal Reader OK");

		if (memoryReader == nullptr || normalReader == nullptr)
			return;

		expectEquals<int>(normalReader->lengthInSamples, length, "Temp file write OK");
		expectEquals<int>(normalReader->lengthInSamples, memoryReader->lengthInSamples, "Total length");

		memoryReader->mapEntireFile();

		expectEquals<int>(memoryReader->getMappedSection().getLength(), normalReader->lengthInSamples, "Mapped section");

		AudioSampleBuffer signalNormal = AudioSampleBuffer(numChannels, length);
		AudioSampleBuffer signalMemory = AudioSampleBuffer(numChannels, length);

		const double start1 = Time::getMillisecondCounterHiRes();
		normalReader->read(&signalNormal, 0, length, 0, true, true);
		const double end1 = Time::getMillisecondCounterHiRes();

		const double delta1 = (end1 - start1) / 1000.0;
		const double sampleLengthInSeconds = (double)length / 44100.0;

		logMessage("Read speed for normal reader: " + String(sampleLengthInSeconds / delta1, 1) + "x realtime");



		const double start2 = Time::getMillisecondCounterHiRes();

		memoryReader->read(&signalMemory, 0, length, 0, true, true);
		

		
		const double end2 = Time::getMillisecondCounterHiRes();
		const double delta2 = (end2 - start2) / 1000.0;

		logMessage("Read speed for memory mapped reader: " + String(sampleLengthInSeconds / delta2, 1) + "x realtime");

		auto normalError = (int)CompressionHelpers::checkBuffersEqual(signalNormal, signal);
		auto memoryError = (int)CompressionHelpers::checkBuffersEqual(signalMemory, signal);

		expectEquals<int>(normalError, 0, "Normal Read operation OK");
		expectEquals<int>(memoryError, 0, "MemoryMapped Read operation OK");



#else
		expect(memoryReader == nullptr, "Unsupported on 32bit");
#endif
	}

	void testMemoryMappedBufferedRead(int numChannels, int length)
	{
		HiseLosslessAudioFormat hlac;

		auto signal = createTestBuffer(numChannels, length);

		length = signal.getNumSamples();

		int mb = length / 1024 / 1024;

		beginTest("Testing buffered memory mapped file reading with " + String(numChannels) + " channels and length " + String(mb) + "MB");

		TemporaryFile tempFile;

		File f = tempFile.getFile();

		FileOutputStream* fos = new FileOutputStream(f);

		StringPairArray empty;

		ScopedPointer<AudioFormatWriter> writer = hlac.createWriterFor(fos, 44100.0, numChannels, 0, empty, 0);

		int numBytesWritten = 0;

		if (writer != nullptr)
		{
			writer->writeFromAudioSampleBuffer(signal, 0, signal.getNumSamples());
			writer->flush();
			numBytesWritten = fos->getPosition();
			writer = nullptr;
			fos = nullptr;
		}

		length = CompressionHelpers::getPaddedSampleSize(length);

		FileInputStream* fisN = new FileInputStream(f);
		FileInputStream* fisM = new FileInputStream(f);

		expectEquals<int>(fisN->getTotalLength(), numBytesWritten, "Bytes written");

		ScopedPointer<AudioFormatReader> normalReader = hlac.createReaderFor(fisN, false);
		ScopedPointer<MemoryMappedAudioFormatReader> memoryReader = hlac.createMemoryMappedReader(fisM);

#if JUCE_64BIT
		expect(normalReader != nullptr, "Normal Reader OK");
		expect(memoryReader != nullptr, "Normal Reader OK");

		if (memoryReader == nullptr || normalReader == nullptr)
			return;

		expectEquals<int>(normalReader->lengthInSamples, length, "Temp file write OK");
		expectEquals<int>(normalReader->lengthInSamples, memoryReader->lengthInSamples, "Total length");

		memoryReader->mapEntireFile();

		expectEquals<int>(memoryReader->getMappedSection().getLength(), normalReader->lengthInSamples, "Mapped section");

		AudioSampleBuffer signalNormal = AudioSampleBuffer(numChannels, length);
		AudioSampleBuffer signalMemory = AudioSampleBuffer(numChannels, length);

		AudioSampleBuffer buffer(numChannels, COMPRESSION_BLOCK_SIZE * 2);

		const double start1 = Time::getMillisecondCounterHiRes();

		for (int i = 0; i < length; i += (COMPRESSION_BLOCK_SIZE*2))
		{
			const int numThisTime = jmin<int>(COMPRESSION_BLOCK_SIZE * 2, length - i);

			normalReader->read(&buffer, 0, numThisTime, i, true, true);
			
			for (int j = 0; j < numChannels; j++)
			{
				FloatVectorOperations::copy(signalNormal.getWritePointer(j, i), buffer.getReadPointer(0), numThisTime);
			}
		}

		const double end1 = Time::getMillisecondCounterHiRes();

		const double delta1 = (end1 - start1) / 1000.0;
		const double sampleLengthInSeconds = (double)length / 44100.0;

		logMessage("Read speed for normal reader: " + String(sampleLengthInSeconds / delta1, 1) + "x realtime");

		const double start2 = Time::getMillisecondCounterHiRes();

		for (int i = 0; i < length; i += (COMPRESSION_BLOCK_SIZE * 2))
		{
			const int numThisTime = jmin<int>(COMPRESSION_BLOCK_SIZE * 2, length - i);

			memoryReader->read(&buffer, 0, numThisTime, i, true, true);

			for (int j = 0; j < numChannels; j++)
			{
				FloatVectorOperations::copy(signalMemory.getWritePointer(j, i), buffer.getReadPointer(0), numThisTime);
			}
		}

		const double end2 = Time::getMillisecondCounterHiRes();
		const double delta2 = (end2 - start2) / 1000.0;

		logMessage("Read speed for memory mapped reader: " + String(sampleLengthInSeconds / delta2, 1) + "x realtime");

		auto normalError = (int)CompressionHelpers::checkBuffersEqual(signalNormal, signal);
		auto memoryError = (int)CompressionHelpers::checkBuffersEqual(signalMemory, signal);

		expectEquals<int>(normalError, 0, "Normal Read operation OK");
		expectEquals<int>(memoryError, 0, "MemoryMapped Read operation OK");



#else
		expect(memoryReader == nullptr, "Unsupported on 32bit");
#endif
	}

	void testMemoryMappedSeeking(int numChannels, int length)
	{
		HiseLosslessAudioFormat hlac;

		auto signal = createTestBuffer(numChannels, length);

		length = signal.getNumSamples();

		int mb = length / 1024;

		beginTest("Testing memory mapped file seeking with " + String(numChannels) + " channels and length " + String(mb) + "KB");

		TemporaryFile tempFile;

		File f = tempFile.getFile();

		FileOutputStream* fos = new FileOutputStream(f);

		StringPairArray empty;

		ScopedPointer<AudioFormatWriter> writer = hlac.createWriterFor(fos, 44100.0, numChannels, 0, empty, 0);

		int numBytesWritten = 0;

		if (writer != nullptr)
		{
			writer->writeFromAudioSampleBuffer(signal, 0, signal.getNumSamples());
			writer->flush();
			numBytesWritten = fos->getPosition();
			writer = nullptr;
			fos = nullptr;
		}

		length = CompressionHelpers::getPaddedSampleSize(length);

		Random r;

		

		FileInputStream* fisN = new FileInputStream(f);
		FileInputStream* fisM = new FileInputStream(f);

		expectEquals<int>(fisN->getTotalLength(), numBytesWritten, "Bytes written");

		ScopedPointer<AudioFormatReader> normalReader = hlac.createReaderFor(fisN, false);
		ScopedPointer<MemoryMappedAudioFormatReader> memoryReader = hlac.createMemoryMappedReader(fisM);

#if JUCE_64BIT
		expect(normalReader != nullptr, "Normal Reader OK");
		expect(memoryReader != nullptr, "Normal Reader OK");

		if (memoryReader == nullptr || normalReader == nullptr)
			return;

		expectEquals<int>(normalReader->lengthInSamples, length, "Temp file write OK");
		expectEquals<int>(normalReader->lengthInSamples, memoryReader->lengthInSamples, "Total length");

		memoryReader->mapEntireFile();

		expectEquals<int>(memoryReader->getMappedSection().getLength(), normalReader->lengthInSamples, "Mapped section");

		AudioSampleBuffer signalNormal = AudioSampleBuffer(numChannels, length);
		AudioSampleBuffer signalMemory = AudioSampleBuffer(numChannels, length);

		signalNormal.clear();
		signalMemory.clear();

		int offset = length - r.nextInt(Range<int>(40, length));

		const double start1 = Time::getMillisecondCounterHiRes();
		normalReader->read(&signalNormal, offset, length-offset, offset, true, true);
		const double end1 = Time::getMillisecondCounterHiRes();

		const double delta1 = (end1 - start1) / 1000.0;
		const double sampleLengthInSeconds = (double)(length - offset) / 44100.0;

		logMessage("Read speed for normal reader: " + String(sampleLengthInSeconds / delta1, 1) + "x realtime");

		const double start2 = Time::getMillisecondCounterHiRes();

		memoryReader->read(&signalMemory, offset, length-offset, offset, true, true);

		const double end2 = Time::getMillisecondCounterHiRes();
		const double delta2 = (end2 - start2) / 1000.0;

		logMessage("Read speed for memory mapped reader: " + String(sampleLengthInSeconds / delta2, 1) + "x realtime");

		for(int i = 0; i < numChannels; i++)
		{
			FloatVectorOperations::clear(signal.getWritePointer(i), offset);
		}

		auto normalError = (int)CompressionHelpers::checkBuffersEqual(signalNormal, signal);
		auto memoryError = (int)CompressionHelpers::checkBuffersEqual(signalMemory, signal);

		expectEquals<int>(normalError, 0, "Normal Read operation OK");
		expectEquals<int>(memoryError, 0, "MemoryMapped Read operation OK");

#else
		expect(memoryReader == nullptr, "Unsupported on 32bit");
#endif
	}

	void testMemoryMappedSubsectionReaders(int numChannels, int length)
	{
		HiseLosslessAudioFormat hlac;

		auto signal1 = createTestBuffer(numChannels, length);
		auto signal2 = createTestBuffer(numChannels, length);

		length = signal1.getNumSamples() + signal2.getNumSamples();

		int mb = length / 1024;

		beginTest("Testing memory mapped file sub section reader " + String(numChannels) + " channels and length " + String(mb) + "KB");

		TemporaryFile tempFile;

		File f = tempFile.getFile();

		FileOutputStream* fos = new FileOutputStream(f);

		StringPairArray empty;

		ScopedPointer<AudioFormatWriter> writer = hlac.createWriterFor(fos, 44100.0, numChannels, 0, empty, 0);

		int numBytesWritten = 0;

		if (writer != nullptr)
		{
			writer->writeFromAudioSampleBuffer(signal1, 0, signal1.getNumSamples());
			writer->writeFromAudioSampleBuffer(signal2, 0, signal2.getNumSamples());

			writer->flush();
			numBytesWritten = fos->getPosition();
			writer = nullptr;
			fos = nullptr;
		}

		int lengthOfFirst = CompressionHelpers::getPaddedSampleSize(signal1.getNumSamples());
		int lengthOfSecond = CompressionHelpers::getPaddedSampleSize(signal2.getNumSamples());

		length = lengthOfFirst + lengthOfSecond;

		AudioSampleBuffer signal1Normal = AudioSampleBuffer(numChannels, lengthOfFirst);
		AudioSampleBuffer signal2Normal = AudioSampleBuffer(numChannels, lengthOfSecond);

		AudioSampleBuffer signal1Memory = AudioSampleBuffer(numChannels, lengthOfFirst);
		AudioSampleBuffer signal2Memory = AudioSampleBuffer(numChannels, lengthOfSecond);


		FileInputStream* fisN = new FileInputStream(f);
		FileInputStream* fisM = new FileInputStream(f);

		expectEquals<int>(fisN->getTotalLength(), numBytesWritten, "Bytes written");

		ScopedPointer<AudioFormatReader> normalReader = hlac.createReaderFor(fisN, false);
		ScopedPointer<MemoryMappedAudioFormatReader> memoryReader = hlac.createMemoryMappedReader(fisM);
		ScopedPointer<AudioSubsectionReader> sub1Reader = new AudioSubsectionReader(memoryReader, 0, lengthOfFirst, false);
		ScopedPointer<AudioSubsectionReader> sub2Reader = new AudioSubsectionReader(memoryReader, lengthOfFirst, lengthOfSecond, false);

		memoryReader->mapEntireFile();

		normalReader->read(&signal2Normal, 0, lengthOfSecond, lengthOfFirst, true, true);
		normalReader->read(&signal1Normal, 0, lengthOfFirst, 0, true, true);
		
		auto normal1Error = (int)CompressionHelpers::checkBuffersEqual(signal1Normal, signal1);
		auto normal2Error = (int)CompressionHelpers::checkBuffersEqual(signal2Normal, signal2);

		expectEquals<int>(normal1Error, 0, "First buffer normal");
		expectEquals<int>(normal2Error, 0, "Second buffer normal");

		
		sub2Reader->read(&signal2Memory, 0, lengthOfSecond, 0, true, true);
		sub1Reader->read(&signal1Memory, 0, lengthOfFirst, 0, true, true);
		

		auto memory1Error = (int)CompressionHelpers::checkBuffersEqual(signal1Memory, signal1);
		auto memory2Error = (int)CompressionHelpers::checkBuffersEqual(signal2Memory, signal2);

		expectEquals<int>(memory1Error, 0, "First buffer memory");
		expectEquals<int>(memory2Error, 0, "Second buffer memory");

	}

	void testMemoryMappedReadingWithOffset(int numChannels, int length)
	{
		HiseLosslessAudioFormat hlac;

		auto signal = createTestBuffer(numChannels, length);

		length = signal.getNumSamples();

		int mb = length / 1024 / 1024;

		beginTest("Testing buffered memory mapped file reading with " + String(numChannels) + " channels and length " + String(mb) + "MB");

		TemporaryFile tempFile;

		File f = tempFile.getFile();

		FileOutputStream* fos = new FileOutputStream(f);

		StringPairArray empty;

		ScopedPointer<AudioFormatWriter> writer = hlac.createWriterFor(fos, 44100.0, numChannels, 0, empty, 0);

		int numBytesWritten = 0;

		if (writer != nullptr)
		{
			writer->writeFromAudioSampleBuffer(signal, 0, signal.getNumSamples());
			writer->flush();
			numBytesWritten = fos->getPosition();
			writer = nullptr;
			fos = nullptr;
		}

		length = CompressionHelpers::getPaddedSampleSize(length);

		FileInputStream* fisM = new FileInputStream(f);

		ScopedPointer<MemoryMappedAudioFormatReader> memoryReader = hlac.createMemoryMappedReader(fisM);

#if JUCE_64BIT
		expect(memoryReader != nullptr, "Normal Reader OK");

		if (memoryReader == nullptr)
			return;

		memoryReader->mapEntireFile();

		AudioSampleBuffer signalMemory = AudioSampleBuffer(numChannels, length);
		signalMemory.clear();

		AudioSampleBuffer buffer(numChannels, COMPRESSION_BLOCK_SIZE * 2);

		for (int i = 10000; i < length; i += (COMPRESSION_BLOCK_SIZE * 2))
		{
			const int numThisTime = jmin<int>(COMPRESSION_BLOCK_SIZE * 2, length - i);

			memoryReader->read(&buffer, 0, numThisTime, i, true, true);

			for (int j = 0; j < numChannels; j++)
			{
				FloatVectorOperations::copy(signalMemory.getWritePointer(j, i), buffer.getReadPointer(0), numThisTime);
			}
		}

		
		FloatVectorOperations::clear(signal.getWritePointer(0, 0), 10000);

		if(numChannels > 1)
			FloatVectorOperations::clear(signal.getWritePointer(1, 0), 10000);

		auto memoryError = (int)CompressionHelpers::checkBuffersEqual(signalMemory, signal);

		
		expectEquals<int>(memoryError, 0, "MemoryMapped Read operation OK");



#else
		expect(memoryReader == nullptr, "Unsupported on 32bit");
#endif
	}

	void testReadOperationWithSmallBlockSizes(int numChannels, int length)
	{
		beginTest("Test small sample amount read operation");

		Array<AudioSampleBuffer> buffers;

		buffers.add(createTestBuffer(numChannels, length));

		length = buffers.getLast().getNumSamples();

		auto mb = writeIntoMemory(buffers);

		ScopedPointer<HiseLosslessAudioFormatReader> reader = createReader(mb, true);

		AudioSampleBuffer signal2 = AudioSampleBuffer(numChannels, reader->lengthInSamples);

		const int smallStep = 16;

		signal2.clear();

		const int bufferSize = (int)jmin<int>(length, (int64)4096);
		AudioSampleBuffer tempSampleBuffer((int)numChannels, bufferSize);

		float* const* const floatBuffer = tempSampleBuffer.getArrayOfWritePointers();
		int* const* intBuffer = reinterpret_cast<int* const*> (floatBuffer);
		

		for (int i = 0; i < length; i += smallStep)
		{
			if (!reader->read(intBuffer, numChannels, i, smallStep, false))
				break;

			signal2.copyFrom(0, i, tempSampleBuffer, 0, 0, smallStep);

			if (numChannels > 1)
			{
				signal2.copyFrom(1, i, tempSampleBuffer, 1, 0, smallStep);
			}
		}

		Range<float> results;

		reader->readMaxLevels(0, length, &results, numChannels);

        auto b12 = buffers.getLast();
        
		int error = (int)CompressionHelpers::checkBuffersEqual(signal2, b12);

		expectEquals<int>(error, 0, "Small read size");
	}

	void testStreamingEngineOperation(int numChannels, int offset, int chunkSize)
	{
		beginTest("Testing HISE streaming-like access for HLAC");

		Random r;

		int firstReadOffset = offset;
		int totalLength = 22050;
		int bufferSize = 8192;
		
		auto original = createTestBuffer(numChannels, totalLength);

		IntBuffer originalIntBuffer(original, 0, 2);

		HiseSampleBuffer originalHiseBuffer(std::move(originalIntBuffer));

		AudioSampleBuffer original2(numChannels, original.getNumSamples());

		originalHiseBuffer.convertToFloatWithNormalisation(original2.getArrayOfWritePointers(), numChannels, 0, original2.getNumSamples());

		auto error = CompressionHelpers::checkBuffersEqual(original2, original);

		expectEquals<int>(error, 0, "Normal conversion doesn't work");

		totalLength = original.getNumSamples();

		Array<AudioSampleBuffer> buffers;

		buffers.add(original);

		auto mb = writeIntoMemory(buffers);

		HiseSampleBuffer b1(false, numChannels, bufferSize);
		HiseSampleBuffer b2(false, numChannels, bufferSize);

		HiseSampleBuffer vb(false, numChannels, bufferSize);
		
		HiseSampleBuffer result(false, numChannels, totalLength);
		result.clear();

		ScopedPointer<HiseLosslessAudioFormatReader> reader = createReader(mb, true);

		if (firstReadOffset > 0)
		{
			HlacSubSectionReader firstReader(reader, 0, firstReadOffset);

			firstReader.readIntoFixedBuffer(result, 0, firstReadOffset, 0);

		}

		int index = firstReadOffset;
		int numToDo = totalLength - index;

		result.allocateNormalisationTables(0);

		while (numToDo > 0)
		{
			int numForInnerLoop = jmin<int>(numToDo, bufferSize);

			HlacSubSectionReader subReader(reader, index, numForInnerLoop);

			b1.clearNormalisation({});

			subReader.readIntoFixedBuffer(b1, 0, numForInnerLoop, 0);

			int indexInSource = 0;

			while (numForInnerLoop > 0)
			{
				int numToDoInner = jmin<int>(chunkSize, numForInnerLoop);

				HiseSampleBuffer::copy(result, b1, index + indexInSource, indexInSource, numToDoInner);

				numForInnerLoop -= numToDoInner;
				indexInSource += numToDoInner;
			}

			numToDo -= bufferSize;
			index += bufferSize;
		}

		result.minimizeNormalisationInfo();

		auto rb = AudioSampleBuffer(result.getNumChannels(), result.getNumSamples());
		result.convertToFloatWithNormalisation(rb.getArrayOfWritePointers(), result.getNumChannels(), 0, result.getNumSamples());

		error = CompressionHelpers::checkBuffersEqual(rb, original);

		expectEquals<int>(error, 0, "Sequenced copy doesn't work");
	}

	HlacEncoder::CompressorOptions currentOption;
private:

	


};

static FormatTest formatTest; 
