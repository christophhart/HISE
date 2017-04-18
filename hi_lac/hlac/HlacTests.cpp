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

static BitCompressors::UnitTests bitTests;

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