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


#ifndef BITCOMPRESSORS_H_INCLUDED
#define BITCOMPRESSORS_H_INCLUDED

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

		void setDontUseOddCompressors(bool shouldUseOddCompressors)
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

	struct UnitTests;
};


#endif  // BITCOMPRESSORS_H_INCLUDED
