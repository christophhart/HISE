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

void printRuler()
{
#if JUCE_DEBUG
	const char* str = "4   3   2   1   ";
	DBG(str);
#endif
}

#if 0
void printBinary(int val)
{
	int i;
	char str[33];
	for (i = 31; i >= 0; i--)
		str[31 - i] = '0' + ((val >> i) & 0x1);
	str[32] = 0;
	puts(str);
	return;
}
#endif

void printBinary(int16_t val)
{
	int i;
	char str[17];
	for (i = 15; i >= 0; i--)
		str[15 - i] = '0' + ((val >> i) & 0x1);
	str[16] = 0;
	DBG(str);
	return;
}


uint16_t compressInt16(int16_t input, int bitDepth)
{
	const int a = (1 << (bitDepth - 1)) - 1;
	return (uint16_t)((int)input + a);
}

#if USE_SSE
void compressInt16Block(int16* input, int16* output, int bitDepth, int numToCompress)
{
	auto inputIpp = reinterpret_cast<Ipp16u*>(input);
	auto outputIpp = reinterpret_cast<Ipp16u*>(output);

	const uint16 signMask = 0b1000000000000000;

	ippsAbs_16s(input, output, numToCompress);
	ippsAndC_16u_I(signMask, inputIpp, numToCompress);
	ippsRShiftC_16s_I(16 - bitDepth, input, numToCompress);
	ippsOr_16u_I(inputIpp, outputIpp, numToCompress);
}
#endif

constexpr uint16_t getBitMask(int bitDepth) { return (1 << (bitDepth - 1)) - 1; }

int16_t decompressUInt16(uint16_t input, int bitDepth)
{
	const int16_t sub = (1 << (bitDepth - 1)) - 1;
	return (int16_t)input - sub;
}

#if USE_SSE
void decompressInt16Block(int16* input, int16* output, uint8 bitDepth, int numToCompress)
{
	auto ippInput = reinterpret_cast<Ipp16u*>(input);
	auto ippOutput = reinterpret_cast<Ipp16u*>(output);

	const uint16 signMask = 0b0000000000000010;
	const uint16 valueMask = 0b0000011111111111;
	const uint8 shiftAmount = bitDepth - 2;

	ippsAndC_16u(ippInput, valueMask, ippOutput, numToCompress);
	ippsRShiftC_16u_I(shiftAmount, ippInput, numToCompress);
	ippsAndC_16u_I(signMask, ippInput, numToCompress);
	ippsMulC_16s_I(-1, input, numToCompress);
	ippsAddC_16s_I(1, input, numToCompress);
	ippsMul_16s_I(input, output, numToCompress);
}
#endif

void packArrayOfInt16(int16_t* d, int numValues, uint8_t bitDepth)
{
	for (int i = 0; i < numValues; i++)
	{
		d[i] = compressInt16(d[i], bitDepth);
	}
}


void unpackArrayOfInt16(int16_t* d, int /*numValues*/, uint8_t bitDepth)
{
	jassert(reinterpret_cast<uint64_t>(d) % 16 == 0);

#if HI_ENABLE_LEGACY_CPU_SUPPORT || !JUCE_WINDOWS
	for (int i = 0; i < 8; i++)
	{
		d[i] = decompressUInt16(d[i], bitDepth);
	}
#else

	// SSE 2.0 needed -> Pentium 4

	const int16 sub = (1 << (bitDepth - 1)) - 1;

	auto sub_ = _mm_set1_epi16(sub);

	auto d_ = _mm_load_si128((__m128i*)d);
	d_ = _mm_sub_epi16(d_, sub_);
	_mm_store_si128((__m128i*)d, d_);

#endif

}


int BitCompressors::ZeroBit::getAllowedBitRange() const
{
	return 0;
}

bool BitCompressors::ZeroBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
    juce::ignoreUnused(destination, data, numValues);

	return true;
}

bool BitCompressors::ZeroBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
    juce::ignoreUnused(destination, data, numValuesToDecompress);

	return true;
}

int BitCompressors::ZeroBit::getByteAmount(int numValuesToCompress)
{
    juce::ignoreUnused(numValuesToCompress);

	return 0;
}


int BitCompressors::OneBit::getAllowedBitRange() const
{
	return 1;
}

bool BitCompressors::OneBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
	const int16_t mask = 0b0000000000000001;

	while (numValues >= 8)
	{
		uint8_t target = 0;

		target |= (*data++ & mask);
		target |= ((*data++ & mask) << 1);
		target |= ((*data++ & mask) << 2);
		target |= ((*data++ & mask) << 3);
		target |= ((*data++ & mask) << 4);
		target |= ((*data++ & mask) << 5);
		target |= ((*data++ & mask) << 6);
		target |= ((*data++ & mask) << 7);

		numValues -= 8;

		*destination++ = target;
	}

	if (numValues != 0)
	{
		uint8_t lastByte = 0;
		int shiftAmount = 0;

		while (--numValues >= 0)
		{
			lastByte |= (*data++ & mask) << shiftAmount++;
		}

		*destination = lastByte;
	}


	return true;
}



bool BitCompressors::OneBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
	const uint8_t masks[8] = { 0b00000001, 0b00000010, 0b00000100, 0b00001000,
		0b00010000, 0b00100000, 0b01000000, 0b10000000 };

	while (numValuesToDecompress >= 8)
	{
		const uint8_t byte = *data;

		*destination++ = byte & masks[0];
		*destination++ = (byte & masks[1]) >> 1;
		*destination++ = (byte & masks[2]) >> 2;
		*destination++ = (byte & masks[3]) >> 3;
		*destination++ = (byte & masks[4]) >> 4;
		*destination++ = (byte & masks[5]) >> 5;
		*destination++ = (byte & masks[6]) >> 6;
		*destination++ = (byte & masks[7]) >> 7;

		numValuesToDecompress -= 8;
		++data;
	};

	if (numValuesToDecompress > 0)
	{
		int maskIndex = 0;
		uint8_t lastByte = *data;

		while (--numValuesToDecompress >= 0)
		{
			*destination++ = (lastByte & masks[maskIndex]) >> maskIndex;
            ++maskIndex;
		}
	}

	return true;
}

int BitCompressors::OneBit::getByteAmount(int numValuesToCompress)
{
	return numValuesToCompress / 8 + ((numValuesToCompress % 8 != 0) ? 1 : 0);
}


int BitCompressors::TwoBit::getAllowedBitRange() const
{
	return 2;
}

bool BitCompressors::TwoBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
	const uint16_t signMask =  0b1000000000000000;
	const uint16_t valueMask = 0b0000000000000001;
	const uint16_t valueMovedMask = 0b0000000000000010;

	while (numValues >= 4)
	{
		uint8_t target = 0;

		const int16_t value1 = *data++;
		
		target |= (value1 & valueMask) | ((value1 & signMask) != 0 ? valueMovedMask : 0);

		const int16_t value2 = *data++;
		target |= (((value2 & valueMask) | ((value2 & signMask) != 0 ? valueMovedMask : 0)) << 2);

		const int16_t value3 = *data++;
		target |= (((value3 & valueMask) | ((value3 & signMask) != 0 ? valueMovedMask : 0)) << 4);

		const int16_t value4 = *data++;
		target |= (((value4 & valueMask) | ((value4 & signMask) != 0 ? valueMovedMask : 0)) << 6);

		numValues -= 4;

		*destination++ = target;
	}

	if (numValues != 0)
	{
		uint8_t lastByte = 0;
		int shiftAmount = 0;

		while (--numValues >= 0)
		{
			const int16_t value = *data++;

			lastByte |= (((value & valueMask) | ((value & signMask) != 0 ? valueMovedMask : 0)) << (2*shiftAmount++));
		}

		*destination = lastByte;
	}


	return true;
}

bool BitCompressors::TwoBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
	const uint8_t signMasks[4] =  { 0b00000010, 0b00001000, 0b00100000, 0b10000000 };
	const uint8_t valueMasks[4] = { 0b00000001, 0b00000100, 0b00010000, 0b01000000 };

	while (numValuesToDecompress >= 4)
	{
		const uint8_t byte = *data;

		const int16_t sign1 = ((byte & signMasks[0]) != 0) ? -1 : 1;
		const int16_t value1 = (int16_t)(byte & valueMasks[0]);

		*destination++ = value1 * sign1;

		const int16_t sign2 = ((byte & signMasks[1]) != 0) ? -1 : 1;
		const int16_t value2 = (int16_t)((byte & valueMasks[1]) >> 2);

		*destination++ = value2 * sign2;

		const int16_t sign3 = ((byte & signMasks[2]) != 0) ? -1 : 1;
		const int16_t value3 = (int16_t)((byte & valueMasks[2]) >> 4);

		*destination++ = value3 * sign3;

		const int16_t sign4 = ((byte & signMasks[3]) != 0) ? -1 : 1;
		*destination++ = (int16_t)((byte & valueMasks[3]) >> 6) * sign4;

		numValuesToDecompress -= 4;
		++data;
	};

	if (numValuesToDecompress > 0)
	{
		int maskIndex = 0;
		
		uint8_t lastByte = *data;

		while (--numValuesToDecompress >= 0)
		{
			const int16_t sign = ((lastByte & signMasks[maskIndex]) != 0) ? -1 : 1;
			*destination++ = (int16_t)((lastByte & valueMasks[maskIndex]) >> (maskIndex*2)) * sign;
            
            ++maskIndex;
		}
	}

	return true;
}


int BitCompressors::TwoBit::getByteAmount(int numValuesToCompress)
{
	const int fullBytes = numValuesToCompress / 4;
	const int lastByte = numValuesToCompress % 4 ? 1 : 0;

	return fullBytes + lastByte;
}



int BitCompressors::FourBit::getAllowedBitRange() const
{
	return 4;
}

bool BitCompressors::FourBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
	const uint16_t valueMovedMask = 0b0000000000001000;


	while (numValues >= 2)
	{
		uint16_t target;

		const int16_t value1 = *data++;
		const int16_t absValue1 = (int16_t)abs(value1);

		const int16_t signMask1 = (value1 != absValue1) ? valueMovedMask : 0;
		target = (absValue1 | signMask1);

		const int16_t value2 = *data++;
		const int16_t absValue2 = (int16_t)abs(value2);
		const int16_t signMask2 = (value2 != absValue2) ? valueMovedMask : 0;

		target |= ((absValue2 | signMask2) << 4);

		*destination++ = (uint8_t)target;

		numValues -= 2;

		
	}

	if (numValues != 0)
	{
		uint16_t lastByte = 0;
		
		while (--numValues >= 0)
		{
			const int16_t value1 = *data++;
			const int16_t absValue1 = (int16_t)abs(value1);

			const int16_t signMask1 = (value1 != absValue1) ? valueMovedMask : 0;
			lastByte = (absValue1 | signMask1);
		}

		*destination = (uint8_t)lastByte;
	}


	return true;
}

bool BitCompressors::FourBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
	

	const uint8_t signMasks[2] =  { 0b00001000, 0b10000000 };
	const uint8_t valueMasks[2] = { 0b00000111, 0b01110000 };

	while (numValuesToDecompress >= 2)
	{
		const uint8_t byte = *data;

		const int16_t sign1 = ((byte & signMasks[0]) != 0) ? -1 : 1;
		const int16_t value1 = (int16_t)(byte & valueMasks[0]);

		*destination++ = value1 * sign1;

		const int16_t sign2 = ((byte & signMasks[1]) != 0) ? -1 : 1;
		const int16_t value2 = (int16_t)((byte & valueMasks[1]) >> 4);

		*destination++ = value2 * sign2;

		numValuesToDecompress -= 2;
		++data;
	};

	if (numValuesToDecompress > 0)
	{
		uint8_t lastByte = *data;

		while (--numValuesToDecompress >= 0)
		{
			const int16_t sign1 = ((lastByte & signMasks[0]) != 0) ? -1 : 1;
			const int16_t value1 = (int16_t)(lastByte & valueMasks[0]);

			*destination++ = value1 * sign1;
		}
	}

	return true;
}

int BitCompressors::FourBit::getByteAmount(int numValuesToCompress)
{
	const int fullBytes = numValuesToCompress / 2;
	const int remainder = numValuesToCompress % 2 ? 1 : 0;

	return fullBytes + remainder;
}


void compress6Bit(uint8_t* sixBytes, const int16_t* eightValues)
{
	uint16_t* threeShorts = reinterpret_cast<uint16_t*>(sixBytes);

	int16_t pData[8];
	memcpy(pData, eightValues, 16);

	packArrayOfInt16(pData, 8, 6);

	threeShorts[0] = pData[0] << 10;
	threeShorts[0] |= pData[1] << 4;
	threeShorts[0] |= pData[2] >> 2;
	threeShorts[1] = pData[2] << 14;
	threeShorts[1] |= pData[3] << 8;
	threeShorts[1] |= pData[4] << 2;
	threeShorts[1] |= pData[5] >> 4;
	threeShorts[2] = pData[5] << 12;
	threeShorts[2] |= pData[6] << 6;
	threeShorts[2] |= pData[7];
}

void decompress6Bit(int16_t* eightValues, const uint8_t* sixBytes)
{
	const uint16_t* dataBlock = reinterpret_cast<const uint16_t*>(sixBytes);

	eightValues[0] = (dataBlock[0] & 0b1111110000000000) >> 10;
	eightValues[1] = (dataBlock[0] & 0b0000001111110000) >> 4;
	eightValues[2] = (dataBlock[0] & 0b0000000000001111) << 2;
	eightValues[2] |= (dataBlock[1] & 0b1100000000000000) >> 14;
	eightValues[3] = (dataBlock[1] & 0b0011111100000000) >> 8;
	eightValues[4] = (dataBlock[1] & 0b0000000011111100) >> 2;
	eightValues[5] = (dataBlock[1] & 0b0000000000000011) << 4;
	eightValues[5] |= (dataBlock[2] & 0b1111000000000000) >> 12;
	eightValues[6] = (dataBlock[2] & 0b0000111111000000) >> 6;
	eightValues[7] = (dataBlock[2] & 0b0000000000111111);;

	unpackArrayOfInt16(eightValues, 8, 6);
}

int BitCompressors::SixBit::getAllowedBitRange() const
{
	return 6;
}


bool BitCompressors::SixBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{

	while (numValues >= 8)
	{
		compress6Bit(destination, data);

		data += 8;
		destination += 6;
		numValues -= 8;
	}

	memcpy(destination, data, sizeof(int16_t) * numValues);

	return true;
}

bool BitCompressors::SixBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
#if JUCE_IOS
	while (numValuesToDecompress >= 8)
	{
		decompress6Bit(destination, data);

		destination += 8;
		data += 6;
		numValuesToDecompress -= 8;
	}
#else

	while (numValuesToDecompress >= 64)
	{
		const uint16_t* dataBlock = reinterpret_cast<const uint16_t*>(data);

		destination[0] = (dataBlock[0] & 0b1111110000000000) >> 10;
		destination[1] = (dataBlock[0] & 0b0000001111110000) >> 4;
		destination[2] = (dataBlock[0] & 0b0000000000001111) << 2;
		destination[2] |= (dataBlock[1] & 0b1100000000000000) >> 14;
		destination[3] = (dataBlock[1] & 0b0011111100000000) >> 8;
		destination[4] = (dataBlock[1] & 0b0000000011111100) >> 2;
		destination[5] = (dataBlock[1] & 0b0000000000000011) << 4;
		destination[5] |= (dataBlock[2] & 0b1111000000000000) >> 12;
		destination[6] = (dataBlock[2] & 0b0000111111000000) >> 6;
		destination[7] = (dataBlock[2] & 0b0000000000111111);

		destination[8 + 0] =  (dataBlock[3 + 0] & 0b1111110000000000) >> 10;
		destination[8 + 1] =  (dataBlock[3 + 0] & 0b0000001111110000) >> 4;
		destination[8 + 2] =  (dataBlock[3 + 0] & 0b0000000000001111) << 2;
		destination[8 + 2] |= (dataBlock[3 + 1] & 0b1100000000000000) >> 14;
		destination[8 + 3] =  (dataBlock[3 + 1] & 0b0011111100000000) >> 8;
		destination[8 + 4] =  (dataBlock[3 + 1] & 0b0000000011111100) >> 2;
		destination[8 + 5] =  (dataBlock[3 + 1] & 0b0000000000000011) << 4;
		destination[8 + 5] |= (dataBlock[3 + 2] & 0b1111000000000000) >> 12;
		destination[8 + 6] =  (dataBlock[3 + 2] & 0b0000111111000000) >> 6;
		destination[8 + 7] =  (dataBlock[3 + 2] & 0b0000000000111111);

		destination[16 + 0] =  (dataBlock[6 + 0] & 0b1111110000000000) >> 10;
		destination[16 + 1] =  (dataBlock[6 + 0] & 0b0000001111110000) >> 4;
		destination[16 + 2] =  (dataBlock[6 + 0] & 0b0000000000001111) << 2;
		destination[16 + 2] |= (dataBlock[6 + 1] & 0b1100000000000000) >> 14;
		destination[16 + 3] =  (dataBlock[6 + 1] & 0b0011111100000000) >> 8;
		destination[16 + 4] =  (dataBlock[6 + 1] & 0b0000000011111100) >> 2;
		destination[16 + 5] =  (dataBlock[6 + 1] & 0b0000000000000011) << 4;
		destination[16 + 5] |= (dataBlock[6 + 2] & 0b1111000000000000) >> 12;
		destination[16 + 6] =  (dataBlock[6 + 2] & 0b0000111111000000) >> 6;
		destination[16 + 7] =  (dataBlock[6 + 2] & 0b0000000000111111);

		destination[24 + 0] =  (dataBlock[9 + 0] & 0b1111110000000000) >> 10;
		destination[24 + 1] =  (dataBlock[9 + 0] & 0b0000001111110000) >> 4;
		destination[24 + 2] =  (dataBlock[9 + 0] & 0b0000000000001111) << 2;
		destination[24 + 2] |= (dataBlock[9 + 1] & 0b1100000000000000) >> 14;
		destination[24 + 3] =  (dataBlock[9 + 1] & 0b0011111100000000) >> 8;
		destination[24 + 4] =  (dataBlock[9 + 1] & 0b0000000011111100) >> 2;
		destination[24 + 5] =  (dataBlock[9 + 1] & 0b0000000000000011) << 4;
		destination[24 + 5] |= (dataBlock[9 + 2] & 0b1111000000000000) >> 12;
		destination[24 + 6] =  (dataBlock[9 + 2] & 0b0000111111000000) >> 6;
		destination[24 + 7] =  (dataBlock[9 + 2] & 0b0000000000111111);

		destination[32 + 0] =  (dataBlock[12 + 0] & 0b1111110000000000) >> 10;
		destination[32 + 1] =  (dataBlock[12 + 0] & 0b0000001111110000) >> 4;
		destination[32 + 2] =  (dataBlock[12 + 0] & 0b0000000000001111) << 2;
		destination[32 + 2] |= (dataBlock[12 + 1] & 0b1100000000000000) >> 14;
		destination[32 + 3] =  (dataBlock[12 + 1] & 0b0011111100000000) >> 8;
		destination[32 + 4] =  (dataBlock[12 + 1] & 0b0000000011111100) >> 2;
		destination[32 + 5] =  (dataBlock[12 + 1] & 0b0000000000000011) << 4;
		destination[32 + 5] |= (dataBlock[12 + 2] & 0b1111000000000000) >> 12;
		destination[32 + 6] =  (dataBlock[12 + 2] & 0b0000111111000000) >> 6;
		destination[32 + 7] =  (dataBlock[12 + 2] & 0b0000000000111111);

		destination[40 + 0] =  (dataBlock[15 + 0] & 0b1111110000000000) >> 10;
		destination[40 + 1] =  (dataBlock[15 + 0] & 0b0000001111110000) >> 4;
		destination[40 + 2] =  (dataBlock[15 + 0] & 0b0000000000001111) << 2;
		destination[40 + 2] |= (dataBlock[15 + 1] & 0b1100000000000000) >> 14;
		destination[40 + 3] =  (dataBlock[15 + 1] & 0b0011111100000000) >> 8;
		destination[40 + 4] =  (dataBlock[15 + 1] & 0b0000000011111100) >> 2;
		destination[40 + 5] =  (dataBlock[15 + 1] & 0b0000000000000011) << 4;
		destination[40 + 5] |= (dataBlock[15 + 2] & 0b1111000000000000) >> 12;
		destination[40 + 6] =  (dataBlock[15 + 2] & 0b0000111111000000) >> 6;
		destination[40 + 7] =  (dataBlock[15 + 2] & 0b0000000000111111);

		destination[48 + 0] =  (dataBlock[18 + 0] & 0b1111110000000000) >> 10;
		destination[48 + 1] =  (dataBlock[18 + 0] & 0b0000001111110000) >> 4;
		destination[48 + 2] =  (dataBlock[18 + 0] & 0b0000000000001111) << 2;
		destination[48 + 2] |= (dataBlock[18 + 1] & 0b1100000000000000) >> 14;
		destination[48 + 3] =  (dataBlock[18 + 1] & 0b0011111100000000) >> 8;
		destination[48 + 4] =  (dataBlock[18 + 1] & 0b0000000011111100) >> 2;
		destination[48 + 5] =  (dataBlock[18 + 1] & 0b0000000000000011) << 4;
		destination[48 + 5] |= (dataBlock[18 + 2] & 0b1111000000000000) >> 12;
		destination[48 + 6] =  (dataBlock[18 + 2] & 0b0000111111000000) >> 6;
		destination[48 + 7] =  (dataBlock[18 + 2] & 0b0000000000111111);

		destination[56 + 0] =  (dataBlock[21 + 0] & 0b1111110000000000) >> 10;
		destination[56 + 1] =  (dataBlock[21 + 0] & 0b0000001111110000) >> 4;
		destination[56 + 2] =  (dataBlock[21 + 0] & 0b0000000000001111) << 2;
		destination[56 + 2] |= (dataBlock[21 + 1] & 0b1100000000000000) >> 14;
		destination[56 + 3] =  (dataBlock[21 + 1] & 0b0011111100000000) >> 8;
		destination[56 + 4] =  (dataBlock[21 + 1] & 0b0000000011111100) >> 2;
		destination[56 + 5] =  (dataBlock[21 + 1] & 0b0000000000000011) << 4;
		destination[56 + 5] |= (dataBlock[21 + 2] & 0b1111000000000000) >> 12;
		destination[56 + 6] =  (dataBlock[21 + 2] & 0b0000111111000000) >> 6;
		destination[56 + 7] =  (dataBlock[21 + 2] & 0b0000000000111111);

		unpackArrayOfInt16(destination, 8, 6);
		unpackArrayOfInt16(destination+8, 8, 6);
		unpackArrayOfInt16(destination+16, 8, 6);
		unpackArrayOfInt16(destination+24, 8, 6);
		unpackArrayOfInt16(destination+32, 8, 6);
		unpackArrayOfInt16(destination+40, 8, 6);
		unpackArrayOfInt16(destination+48, 8, 6);
		unpackArrayOfInt16(destination+56, 8, 6);
		
		destination += 64;
		data += 48;
		numValuesToDecompress -= 64;
	}

	while (numValuesToDecompress >= 8)
	{
		decompress6Bit(destination, data);

		destination += 8;
		data += 6;
		numValuesToDecompress -= 8;
	}
#endif

	memcpy(destination, data, sizeof(int16_t) * numValuesToDecompress);

	return true;
}

int BitCompressors::SixBit::getByteAmount(int numValuesToCompress)
{
	// 8 values => 48 bit = 6 bytes

	auto fullBytes = (6 * numValuesToCompress) / 8;
	auto remainder = 2 * (numValuesToCompress % 8);

	return fullBytes + remainder;
}


int BitCompressors::EightBit::getAllowedBitRange() const
{
	return 8;
}

bool BitCompressors::EightBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
	while (--numValues >= 0)
	{
		*destination++ = (uint8_t)*data++;
	}

	return true;
}

bool BitCompressors::EightBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
    while (--numValuesToDecompress >= 0)
	{
		const int8_t value = *reinterpret_cast<const int8_t*>(data++);
		*destination++ = (int16_t)value;
	}

	return true;
}

int BitCompressors::EightBit::getByteAmount(int numValuesToCompress)
{
	return numValuesToCompress;
}

void compress10Bit(void* tenBytes, const int16_t* eightValues)
{
	uint16_t pData[8];

	pData[0] = compressInt16(eightValues[0], 10);
	pData[1] = compressInt16(eightValues[1], 10);
	pData[2] = compressInt16(eightValues[2], 10);
	pData[3] = compressInt16(eightValues[3], 10);
	pData[4] = compressInt16(eightValues[4], 10);
	pData[5] = compressInt16(eightValues[5], 10);
	pData[6] = compressInt16(eightValues[6], 10);
	pData[7] = compressInt16(eightValues[7], 10);


	uint16_t* fiveShorts = reinterpret_cast<uint16_t*>(tenBytes);

	fiveShorts[0] = pData[0] << 6;
	fiveShorts[0] |= pData[1] >> 4;
	fiveShorts[1] = pData[1] << 12;
	fiveShorts[1] |= pData[2] << 2;
	fiveShorts[1] |= pData[3] >> 8;
	fiveShorts[2] = pData[3] << 8;
	fiveShorts[2] |= pData[4] >> 2;
	fiveShorts[3] = pData[4] << 14;
	fiveShorts[3] |= pData[5] << 4;
	fiveShorts[3] |= pData[6] >> 6;
	fiveShorts[4] = pData[6] << 10;
	fiveShorts[4] |= pData[7];
}

void decompress10Bit(uint16_t* eightValues, void* tenBytes)
{
	const uint16_t* dataBlock = reinterpret_cast<const uint16_t*>(tenBytes);

	eightValues[0] = (dataBlock[0] & 0b1111111111000000) >> 6;
	eightValues[1] = (dataBlock[0] & 0b0000000000111111) << 4;
	eightValues[1] |= (dataBlock[1] & 0b1111000000000000) >> 12;
	eightValues[2] = (dataBlock[1] & 0b0000111111111100) >> 2;
	eightValues[3] = (dataBlock[1] & 0b0000000000000011) << 8;
	eightValues[3] |= (dataBlock[2] & 0b1111111100000000) >> 8;
	eightValues[4] = (dataBlock[2] & 0b0000000011111111) << 2;
	eightValues[4] |= (dataBlock[3] & 0b1100000000000000) >> 14;
	eightValues[5] = (dataBlock[3] & 0b0011111111110000) >> 4;
	eightValues[6] = (dataBlock[3] & 0b0000000000001111) << 6;
	eightValues[6] |= (dataBlock[4] & 0b1111110000000000) >> 10;
	eightValues[7] = (dataBlock[4] & 0b0000001111111111);

	unpackArrayOfInt16(reinterpret_cast<int16_t*>(eightValues), 8, 10);
}

int BitCompressors::TenBit::getAllowedBitRange() const
{
	return 10;
}

bool BitCompressors::TenBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
	while (numValues >= 8)
	{
		compress10Bit((void*)destination, data);

		data += 8;
		destination += 10;
		numValues -= 8;
	}

	memcpy(destination, data, sizeof(int16_t) * numValues);

	return true;
}

bool BitCompressors::TenBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
	while (numValuesToDecompress >= 8)
	{
		decompress10Bit(reinterpret_cast<uint16_t*>(destination), (void*)data);

		destination += 8;
		data += 10;
		numValuesToDecompress -= 8;
	}

	memcpy(destination, data, sizeof(int16_t) * numValuesToDecompress);

	return true;
}

int BitCompressors::TenBit::getByteAmount(int numValuesToCompress)
{
	const int fullBytes = numValuesToCompress * 5 / 4;
	const int remainder = (numValuesToCompress % 8) * 2;

	return fullBytes + remainder;
}

int BitCompressors::TwelveBit::getAllowedBitRange() const
{
	return 12;
}


bool BitCompressors::TwelveBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
	while (numValues >= 4)
	{
		const uint16_t v1 = compressInt16(data[0], 12);
		const uint16_t v2 = compressInt16(data[1], 12);
		const uint16_t v3 = compressInt16(data[2], 12);
		const uint16_t v4 = compressInt16(data[3], 12);

		auto* dataBlock = reinterpret_cast<uint16_t*>(destination);

		dataBlock[0] = v1 << 4;
		dataBlock[0] |= v2 >> 8;
		dataBlock[1] = v2 << 8;
		dataBlock[1] |= v3 >> 4;
		dataBlock[2] = v3 << 12;
		dataBlock[2] |= v4;

		destination += 6;
		data += 4;
		numValues -= 4;
	}

	memcpy(destination, data, sizeof(int16_t) * numValues);

	return true;
}

bool BitCompressors::TwelveBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
#if USE_SSE

	const int numInBlockProcessing = numValuesToDecompress - (numValuesToDecompress % 4);

	int16* alignedDestination = (int16*)ippAlignPtr(destination, 32);
	jassert(alignedDestination == destination);
	uint16* dst = scratchBuffer;


	while (numValuesToDecompress >= 4)
	{
		const uint16* dataBlock = reinterpret_cast<const uint16*>(data);

		dst[0] = (dataBlock[0] & 0b1111111111110000) >> 4;
		dst[1] = (dataBlock[0] & 0b0000000000001111) << 8;
		dst[1] |= (dataBlock[1] & 0b1111111100000000) >> 8;
		dst[2] = (dataBlock[1] & 0b0000000011111111) << 4;
		dst[2] |= (dataBlock[2] & 0b1111000000000000) >> 12;
		dst[3] = (dataBlock[2] & 0b0000111111111111);

		//dst[0] = decompressUInt16(dst[0], 12);
		//dst[1] = decompressUInt16(dst[1], 12);
		//dst[2] = decompressUInt16(dst[2], 12);
		//dst[3] = decompressUInt16(dst[3], 12);

		numValuesToDecompress -= 4;
		dst += 4;
		data += 6;

	}

	decompressInt16Block((int16*)scratchBuffer, destination, 12, numInBlockProcessing);
	destination += numInBlockProcessing;

#else

	auto* dst = destination;

	while (numValuesToDecompress >= 4)
	{
		const auto* dataBlock = reinterpret_cast<const uint16_t*>(data);

		dst[0] = (dataBlock[0] & 0b1111111111110000) >> 4;
		dst[1] = (dataBlock[0] & 0b0000000000001111) << 8;
		dst[1] |= (dataBlock[1] & 0b1111111100000000) >> 8;
		dst[2] = (dataBlock[1] & 0b0000000011111111) << 4;
		dst[2] |= (dataBlock[2] & 0b1111000000000000) >> 12;
		dst[3] = (dataBlock[2] & 0b0000111111111111);

		dst[0] = decompressUInt16(dst[0], 12);
		dst[1] = decompressUInt16(dst[1], 12);
		dst[2] = decompressUInt16(dst[2], 12);
		dst[3] = decompressUInt16(dst[3], 12);

		numValuesToDecompress -= 4;
		dst += 4;
		data += 6;

	}
	
	destination = dst;

	memcpy(destination, data, sizeof(int16_t) * numValuesToDecompress);

#endif

	

	return true;
}



int BitCompressors::TwelveBit::getByteAmount(int numValuesToCompress)
{
	const int fullBytes = 6 * numValuesToCompress / 4;
	const int remainder = numValuesToCompress % 4;

	return fullBytes + remainder;
}





void compress14Bit(uint8_t* fourteenBytes, const int16_t* eightValues)
{
	auto* sevenShorts = reinterpret_cast<uint16_t*>(fourteenBytes);

	int16_t pData[8];
	memcpy(pData, eightValues, 16);

	packArrayOfInt16(pData, 8, 14);

	sevenShorts[0] = pData[0] << 2;
	sevenShorts[0] |= pData[1] >> 12;
	sevenShorts[1] = pData[1] << 4;
	sevenShorts[1] |= pData[2] >> 10;
	sevenShorts[2] = pData[2] << 6;
	sevenShorts[2] |= pData[3] >> 8;
	sevenShorts[3] = pData[3] << 8;
	sevenShorts[3] |= pData[4] >> 6;
	sevenShorts[4] = pData[4] << 10;
	sevenShorts[4] |= pData[5] >> 4;
	sevenShorts[5] = pData[5] << 12;
	sevenShorts[5] |= pData[6] >> 2;
	sevenShorts[6] = pData[6] << 14;
	sevenShorts[6] |= pData[7];
}

void decompress14Bit(int16_t* eightValues, const uint8_t* fourteenBytes)
{
	const auto* dataBlock = reinterpret_cast<const uint16_t*>(fourteenBytes);

	eightValues[0] = (dataBlock[0] & 0b1111111111111100) >> 2;
	eightValues[1] = (dataBlock[0] & 0b0000000000000011) << 12;
	eightValues[1] |= (dataBlock[1] & 0b1111111111110000) >> 4;
	eightValues[2] = (dataBlock[1] & 0b0000000000001111) << 10;
	eightValues[2] |= (dataBlock[2] & 0b1111111111000000) >> 6;
	eightValues[3] = (dataBlock[2] & 0b0000000000111111) << 8;
	eightValues[3] |= (dataBlock[3] & 0b1111111100000000) >> 8;
	eightValues[4] = (dataBlock[3] & 0b0000000011111111) << 6;
	eightValues[4] |= (dataBlock[4] & 0b1111110000000000) >> 10;
	eightValues[5] = (dataBlock[4] & 0b0000001111111111) << 4;
	eightValues[5] |= (dataBlock[5] & 0b1111000000000000) >> 12;
	eightValues[6] = (dataBlock[5] & 0b0000111111111111) << 2;
	eightValues[6] |= (dataBlock[6] & 0b1100000000000000) >> 14;
	eightValues[7] = (dataBlock[6] & 0b0011111111111111);

	unpackArrayOfInt16(eightValues, 8, 14);
}

int BitCompressors::FourteenBit::getAllowedBitRange() const
{
	return 14;
}

bool BitCompressors::FourteenBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
	while (numValues >= 8)
	{
		compress14Bit(destination, data);
		data += 8;
		destination += 14;
		numValues -= 8;
	}

	memcpy(destination, data, sizeof(int16_t) * numValues);

	return true;
}

bool BitCompressors::FourteenBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
	while (numValuesToDecompress >= 8)
	{
		decompress14Bit(destination, data);

		destination += 8;
		data += 14;
		numValuesToDecompress -= 8;
	}

	memcpy(destination, data, sizeof(int16_t) * numValuesToDecompress);

	return true;
}

int BitCompressors::FourteenBit::getByteAmount(int numValuesToCompress)
{
	// 8 values => 112 bit = 7 bytes;

	auto fullBytes = (14 * numValuesToCompress) / 8;
	auto remainder = 2 * (numValuesToCompress % 8);

	return fullBytes + remainder;
}


int BitCompressors::SixteenBit::getAllowedBitRange() const
{
	return 16;
}

bool BitCompressors::SixteenBit::compress(uint8_t* destination, const int16_t* data, int numValues)
{
	memcpy(destination, data, sizeof(int16_t) * numValues);
	return true;
}

bool BitCompressors::SixteenBit::decompress(int16_t* destination, const uint8_t* data, int numValuesToDecompress)
{
	memcpy(destination, data, sizeof(int16_t) * numValuesToDecompress);
	return true;
}

int BitCompressors::SixteenBit::getByteAmount(int numValuesToCompress)
{
	return numValuesToCompress * sizeof(int16_t);
}





uint8_t BitCompressors::getMinBitDepthForData(const int16_t* data, int numValues, int8_t expectedBitDepth)
{
    juce::ignoreUnused(expectedBitDepth);

	bool isZero = true;

	for (int i = 0; i < numValues; i++)
	{
		if (data[i] != 0)
		{
			isZero = false;
			break;
		}
	}

	if (isZero)
	{
		return 0;
	}

	bool isOneBitCompressable = true;

	for (int i = 0; i < numValues; i++)
	{
		if (data[i] != 0 && data[i] != 1)
		{
			isOneBitCompressable = false;
			break;
		}
	}

	if (isOneBitCompressable)
	{
		jassert(expectedBitDepth < 0 || expectedBitDepth == 1);
		return 1;
	}

	for (uint8_t bitDepth = 2; bitDepth < 16; bitDepth++)
	{
		bool canBeCompressed = true;

		const uint16_t mask = getBitMask(bitDepth);

		for (int i = 0; i < numValues; i++)
		{
			if (abs(data[i]) > mask)
			{
				canBeCompressed = false;
				break;
			}
		}

		if (!canBeCompressed)
		{
			continue;
		}
		else
		{
			jassert(expectedBitDepth < 0 || expectedBitDepth == bitDepth);

			return bitDepth;
		}
	}

	jassert(expectedBitDepth < 0 || expectedBitDepth == 16);

	return 16;
}

BitCompressors::Base* BitCompressors::Collection::getSuitableCompressorForBitRate(uint8_t bitRate)
{
	return compressors[bitRate];
}

BitCompressors::Base* BitCompressors::Collection::getSuitableCompressorForData(const int16_t* data, int numValues)
{
	auto bitDepth = getMinBitDepthForData(data, numValues);

	if (bitDepth < 17)
	{
		if (useOddCompressors)
		{
			return compressors[bitDepth];
		}
		else
		{
			if (bitDepth > 9)
			{
				return compressors[16];
			}
			else
			{
				return compressors[8];
			}
		}

		
	}

	return nullptr;
}

int BitCompressors::Collection::getNumBytesForBitRate(uint8_t bitRate, int elements)
{
	if (useOddCompressors)
	{
		return compressors[bitRate]->getByteAmount(elements);
	}
	else
	{
		if (bitRate > 9)
		{
			return compressors[16]->getByteAmount(elements);
		}
		else
		{
			return compressors[8]->getByteAmount(elements);
		}
	}
	
}


} // namespace hlac
