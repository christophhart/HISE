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



namespace hise {
using namespace juce;

AES::AES(const String& keyToUse)
{
	
	MemoryBlock mb;

	mb.fromBase64Encoding(keyToUse);

	memcpy(key, mb.getData(), 16);

	iv = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };

	AES_init_ctx(&context, key);
}

AES::~AES()
{
	
}

void AES::encrypt(MemoryBlock& mb)
{
	prepare();
	AES_CTR_xcrypt_buffer(&context, (uint8*)mb.getData(), mb.getSize());
}

String AES::encrypt(const String& stringToEncode)
{
	return decrypt(stringToEncode);
}



void AES::decrypt(MemoryBlock& mb)
{
	prepare();
	AES_CTR_xcrypt_buffer(&context, (uint8_t*)mb.getData(), mb.getSize());
}

String AES::decrypt(const String& stringToDecode)
{
	prepare();

	MemoryBlock mb;
	
	String r;

	r.append(stringToDecode, stringToDecode.length());

	auto s = r.toUTF16();

	


	auto length = CharPointer_UTF16::getBytesRequiredFor(s);


	AES_CTR_xcrypt_buffer(&context, (uint8_t*)s.getAddress(), length);

	return String(s);
}

void AES::encrypt(AudioSampleBuffer& b)
{
	decrypt(b);
}

void AES::decrypt(AudioSampleBuffer& b)
{
	prepare();

	for (int i = 0; i < b.getNumChannels(); i++)
	{
		auto d = reinterpret_cast<uint8_t*>(b.getWritePointer(0));

		auto size = b.getNumSamples() * sizeof(float);

		AES_CTR_xcrypt_buffer(&context, d, size);
	}
}

juce::String AES::createKey()
{
	Random r;

	MemoryBlock mb;

	mb.setSize(16, true);

	auto d = (char*)mb.getData();

	for (int i = 0; i < 16; i++)
	{
		d[i] = (char)r.nextInt(256);
	}

	return mb.toBase64Encoding();
}

void AES::prepare()
{
	AES_init_ctx_iv(&context, key, iv.getRawDataPointer());
}

}
