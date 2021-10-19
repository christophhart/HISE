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


#ifndef BITCOMPRESSORS_H_INCLUDED
#define BITCOMPRESSORS_H_INCLUDED

namespace hlac {

#define LOG_RATIO(x) 

#define USE_SSE 0


#if USE_SSE
#include <ipp.h>
#endif

struct BitCompressors
{
	struct Base
	{
		virtual ~Base() {};

		virtual int getAllowedBitRange() const { return -1; };
		virtual bool compress(uint8_t* destination, const int16_t* data, int numValues) { juce::ignoreUnused(destination, data, numValues); return false; }
		virtual bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) { juce::ignoreUnused(destination, data, numValuesToDecompress); return false; }
		virtual int getByteAmount(int numValuesToCompress) { juce::ignoreUnused(numValuesToCompress); return 0; };
	};

	struct Collection
	{
	public:

		Collection()
		{
			compressors.ensureStorageAllocated(17);
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

		Base* getSuitableCompressorForBitRate(uint8_t bitRate);

		Base* getSuitableCompressorForData(const int16_t* data, int numValues);

		int getNumBytesForBitRate(uint8_t bitRate, int elements);

	private:

		void setDontUseOddCompressors(bool shouldUseOddCompressors)
		{
			useOddCompressors = shouldUseOddCompressors;
		}

		bool useOddCompressors = true;

        juce::OwnedArray<Base> compressors;
	};


	static uint8_t getMinBitDepthForData(const int16_t* data, int numValues, int8_t expectedBitDepth = -1);


	struct ZeroBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;;
		int getByteAmount(int numValuesToCompress) override;;
	};

	struct OneBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;;
		int getByteAmount(int numValuesToCompress) override;;
		
	};

	struct TwoBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct FourBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct SixBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct EightBit: public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct TenBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;
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
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;

#if USE_SSE
		Ipp16u* scratchBuffer;
		Ipp16u* scratchBuffer2;
#endif

	};

	struct FourteenBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct SixteenBit : public Base
	{
		int getAllowedBitRange() const override;
		bool compress(uint8_t* destination, const int16_t* data, int numValues) override;
		bool decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress) override;
		int getByteAmount(int numValuesToCompress) override;
	};

	struct UnitTests;
};

} // namespace hlac

#endif  // BITCOMPRESSORS_H_INCLUDED
