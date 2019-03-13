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

#ifndef ZSTD_COMPRESSOR_IMPL_H_INCLUDED
#define ZSTD_COMPRESSOR_IMPL_H_INCLUDED

namespace zstd {
using namespace juce;


template <class SourceType>
zstd::ZBatchDecompressor<SourceType>::ZBatchDecompressor(ZDictionary<SourceType>* dictionary /*= nullptr*/) :
	usedDictionary(dictionary)
{
	context = DictionaryHelpers::createDecompressorContext();
}


template <class SourceType>
zstd::ZBatchDecompressor<SourceType>::~ZBatchDecompressor()
{
	DictionaryHelpers::freeDecompressorContext(context);
}

template <class SourceType>
bool zstd::ZBatchDecompressor<SourceType>::decompress(Array<SourceType>& destinationList, InputStream& inputStream)
{
	MemoryBlock output;
	MemoryBlock input;

	while (!inputStream.isExhausted())
	{
		auto bytesForNextEntry = (size_t)inputStream.readInt();
		DictionaryHelpers::checkResult(bytesForNextEntry);

		input.ensureSize(bytesForNextEntry);
		inputStream.read(input.getData(), bytesForNextEntry);

		

		DictionaryHelpers::decompressWithOptionalDictionary(input, bytesForNextEntry, output, context, usedDictionary->getRawDictionaryForDecompression());

		SourceType t;
		DictionaryHelpers::createFromMemory(output, t);
		destinationList.add(t);
	}

	return true;
}


template <class SourceType>
zstd::ZBatchCompressor<SourceType>::ZBatchCompressor(ZDictionary<SourceType>* dictionary /*= nullptr*/, int compressionLevel_ /*= 19*/) :
	usedDictionary(dictionary),
	compressionLevel(compressionLevel_)
{
	context = DictionaryHelpers::createCompressorContext();
}


template <class SourceType>
zstd::ZBatchCompressor<SourceType>::~ZBatchCompressor()
{
	DictionaryHelpers::freeCompressorContext(context);
}



template <class SourceType>
bool zstd::ZBatchCompressor<SourceType>::compress(const Array<SourceType>& sourceList, OutputStream& stream)
{
	MemoryBlock output;

	PointerTypes::CompressionDictionary* dictionary = usedDictionary != nullptr ? usedDictionary->getRawDictionaryForCompression() : nullptr;

	for (const auto& source : sourceList)
	{
		MemoryBlock inputBlock;

		MemoryOutputStream input(inputBlock, true);
		DictionaryHelpers::readIntoMemory(source, input);

		auto numWritten = DictionaryHelpers::compressWithOptionalDictionary(context, output, inputBlock, dictionary, compressionLevel);
		
		stream.writeInt(numWritten);

		if (!stream.write(output.getData(), numWritten))
			return false;
	}

	stream.flush();
	return true;
}

template <typename SourceType, class ProviderType /*= HeaderDictionaryProvider<SourceType>*/>
zstd::ZstdArchive<SourceType, ProviderType>::ZstdArchive(InputStream& input_) :
	input(input_)
{
	ProviderType provider(&input);
	auto dictData = provider.createDictionaryData();
	dictionary = new ZDictionary<SourceType>(dictData, false);
	decompressor = new ZBatchDecompressor<SourceType>(dictionary);
}

template <typename SourceType, class ProviderType /*= HeaderDictionaryProvider<SourceType>*/>
bool zstd::ZstdArchive<SourceType, ProviderType>::extract(Array<SourceType>& list)
{
	return decompressor->decompress(list, input);
}


template <class ProviderType/*=NoDictionaryProvider<void>*/>
MemoryBlock zstd::ZCompressor<ProviderType>::expandRaw(const MemoryBlock& compressedData)
{
	MemoryBlock& uncompressedData = internalBuffer;

	auto numBytesUncompressed = DictionaryHelpers::getDecompressedSize(compressedData);
	uncompressedData.ensureSize(numBytesUncompressed);

	auto dictionary = d_dictionary != nullptr ? d_dictionary->getRawDictionaryForDecompression() : nullptr;
	DictionaryHelpers::decompressWithOptionalDictionary(compressedData, compressedData.getSize(), uncompressedData, d_context, dictionary);

	return uncompressedData;
}

template <class ProviderType/*=NoDictionaryProvider<void>*/>
MemoryBlock zstd::ZCompressor<ProviderType>::compressRaw(const MemoryBlock& uncompressedData)
{
	MemoryBlock& compressedData = internalBuffer;
	compressedData.ensureSize(uncompressedData.getSize());

	auto dictionary = c_dictionary != nullptr ? c_dictionary->getRawDictionaryForCompression() : nullptr;
	auto numBytesCompressed = DictionaryHelpers::compressWithOptionalDictionary(c_context, compressedData, uncompressedData, dictionary, compressionLevel);

	compressedData.setSize(numBytesCompressed);

	return compressedData;
}

template <class ProviderType/*=NoDictionaryProvider<void>*/>
Result zstd::ZCompressor<ProviderType>::expandInplace(MemoryBlock& compressedData)
{
	try
	{
        auto d = expandRaw(compressedData);
		compressedData.swapWith(d);
		return Result::ok();
	}
	catch (const String& errorMessage)
	{
		return Result::fail(errorMessage);
	}
}

template <class ProviderType/*=NoDictionaryProvider<void>*/>
Result zstd::ZCompressor<ProviderType>::compressInplace(MemoryBlock& uncompressedData)
{
	try
	{
        auto d = compressRaw(uncompressedData);
		uncompressedData.swapWith(d);
		return Result::ok();
	}
	catch (const String& errorMessage)
	{
		return Result::fail(errorMessage);
	}
}

template <class ProviderType/*=NoDictionaryProvider<void>*/>
zstd::ZCompressor<ProviderType>::~ZCompressor()
{
	DictionaryHelpers::freeCompressorContext(c_context);
	DictionaryHelpers::freeDecompressorContext(d_context);
	c_dictionary = nullptr;
	d_dictionary = nullptr;
}

template <class ProviderType/*=NoDictionaryProvider<void>*/>
zstd::ZCompressor<ProviderType>::ZCompressor(Mode m/*=Both*/, int compressionLevel_/*=19*/) :
	ZCompressorBase(compressionLevel_)
{
	ProviderType provider;

	auto dictData = provider.createDictionaryData();
	bool useDictionary = dictData.getSize() != 0;

	if (m == CompressOnly || m == Both)
	{
		c_context = DictionaryHelpers::createCompressorContext();

		if (useDictionary)
			c_dictionary = new ZDictionary<MemoryBlock>(dictData, true);
	}

	if (m == DecompressOnly || m == Both)
	{
		d_context = DictionaryHelpers::createDecompressorContext();

		if (useDictionary)
			d_dictionary = new ZDictionary<MemoryBlock>(dictData, false);
	}
}


}


#endif
