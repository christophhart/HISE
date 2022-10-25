/*  ===========================================================================

BSD License

For Zstandard software

Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name Facebook nor the names of its contributors may be used to
endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*   ===========================================================================
*/

namespace zstd {
using namespace juce;

struct ZstdInputStream::Pimpl
{
	Pimpl(InputStream* source) :
		compressedData(source)
	{
		zstdStreamObject = ZSTD_createDStream();

		ZSTD_initDStream(zstdStreamObject);

		inSize = ZSTD_DStreamInSize();
		outSize = ZSTD_DStreamOutSize();

		inBufferData.allocate(inSize, true);
		outBufferData.allocate(outSize, true);

		inBuffer.src = inBufferData;
		inBuffer.pos = 0;
		inBuffer.size = inSize;


		outBuffer.dst = outBufferData;
		outBuffer.pos = 0;
		outBuffer.size = outSize;
	}

	~Pimpl()
	{
		ZSTD_freeDStream(zstdStreamObject);
	}

	int read(void* destBuffer, int maxBytesToRead)
	{
		auto compressedSize = compressedData->getTotalLength();

		compressedData->read(inBufferData, (int)compressedSize);

		inBuffer.pos = 0;
		inBuffer.size = compressedSize;
		outBuffer.pos = 0;

		auto result = ZSTD_decompressStream(zstdStreamObject, &outBuffer, &inBuffer);

		DictionaryHelpers::checkResult(result);

		memcpy(destBuffer, outBufferData, maxBytesToRead);

		return (int)inBuffer.pos;
	}

	size_t inSize;
	size_t outSize;

	HeapBlock<uint8> inBufferData;
	HeapBlock<uint8> outBufferData;

	ZSTD_DStream* zstdStreamObject;

	ZSTD_inBuffer inBuffer;
	ZSTD_outBuffer outBuffer;

	bool decompressed = false;
	ScopedPointer<InputStream> compressedData;

};

ZstdInputStream::ZstdInputStream(InputStream* sourceStream) :
	pimpl(new Pimpl(sourceStream))
{
	
}

ZstdInputStream::~ZstdInputStream()
{
	pimpl = nullptr;
}

int ZstdInputStream::read(void* destBuffer, int maxBytesToRead)
{
	return pimpl->read(destBuffer, maxBytesToRead);
}

juce::int64 ZstdInputStream::getTotalLength()
{
	return pimpl->compressedData->getTotalLength();
}

bool ZstdInputStream::isExhausted()
{
	return pimpl->compressedData->isExhausted();
}

juce::int64 ZstdInputStream::getPosition()
{
	return pimpl->compressedData->getPosition();
}

bool ZstdInputStream::setPosition(int64 newPosition)
{
	// no seeking allowed
	jassertfalse;

	return false;
}

}