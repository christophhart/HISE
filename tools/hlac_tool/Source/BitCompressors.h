/*
  ==============================================================================

    BitCompressors.h
    Created: 11 Apr 2017 11:57:40am
    Author:  Christoph

  ==============================================================================
*/

#ifndef BITCOMPRESSORS_H_INCLUDED
#define BITCOMPRESSORS_H_INCLUDED

#include "JuceHeader.h"

#define LOG_RATIO(x) 

#define USE_SSE 0


#if 1 || USE_SSE
#include <ipp.h>
#endif

struct BitCompressors
{
	struct Base
	{
		virtual ~Base() {};

		virtual int getAllowedBitRange() const { return -1; };
		virtual bool compress(uint8* destination, const int16* data, int numValues) { ignoreUnused(destination, data, numValues); return false; }
		virtual bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) { ignoreUnused(destination, data, numValuesToDecompress); return false; }
		virtual int getByteAmount(int numValuesToCompress) { ignoreUnused(numValuesToCompress); return 0; };
	};

	struct Collection
	{
	public:

		Collection()
		{
			compressors.add(new ZeroBit());
			compressors.add(new OneBit());
			compressors.add(new TwoBit());
			compressors.add(new FourBit());
			compressors.add(new FourBit());
			compressors.add(new SixBit());
			compressors.add(new SixBit());
			compressors.add(new EightBit());
			compressors.add(new EightBit());
			compressors.add(new TenBit());
			compressors.add(new TenBit());
			compressors.add(new TwelveBit());
			compressors.add(new TwelveBit());
			compressors.add(new FourteenBit());
			compressors.add(new FourteenBit());
			compressors.add(new SixteenBit());
			compressors.add(new SixteenBit());
		}

		Base* getSuitableCompressorForBitRate(uint8 bitRate);

		Base* getSuitableCompressorForData(const int16* data, int numValues);

		int getNumBytesForBitRate(uint8 bitRate, int elements);

	private:

		bool setDontUseOddCompressors(bool shouldUseOddCompressors)
		{
			useOddCompressors = shouldUseOddCompressors;
		}

		bool useOddCompressors = true;

		OwnedArray<Base> compressors;
	};


	static uint8 getMinBitDepthForData(const int16* data, int numValues, int8 expectedBitDepth = -1);


	struct ZeroBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;;
		int getByteAmount(int numValuesToCompress) override;;
	};

	struct OneBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;;
		int getByteAmount(int numValuesToCompress) override;;
		
	};

	struct TwoBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct FourBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct SixBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct EightBit: public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct TenBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct TwelveBit : public Base
	{
#if USE_SSE
		TwelveBit()
		{
			scratchBuffer = ippsMalloc_16u(16383);
			scratchBuffer2 = ippsMalloc_16u(16383);
		}

		~TwelveBit()
		{
			ippsFree(scratchBuffer);
			ippsFree(scratchBuffer2);
		}
#endif

		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;

#if USE_SSE
		Ipp16u* scratchBuffer;
		Ipp16u* scratchBuffer2;
#endif

	};

	struct FourteenBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct SixteenBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8* destination, const int16* data, int numValues) override;
		bool decompress(int16* destination, const uint8* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct UnitTests : public UnitTest
	{
		UnitTests() :
			UnitTest("Bit reduction unit tests")
		{}

		void runTest() override;
		void fillDataWithAllowedBitRange(int16* data, int size, int bitRange);
		void testCompressor(Base* compressor);

		void testAutomaticCompression(uint8 maxBitSize);
	};
};

static BitCompressors::UnitTests bitTests;



#endif  // BITCOMPRESSORS_H_INCLUDED
