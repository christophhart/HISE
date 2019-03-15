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


void DictionaryHelpers::checkResult(size_t code)
{
	if (ZSTD_isError(code))
	{
		String errorMessage;
		errorMessage << ZSTD_getErrorName(code);

		throw errorMessage;
	}
}

size_t DictionaryHelpers::getDecompressedSize(const MemoryBlock& mb)
{
	auto s = ZSTD_getFrameContentSize(mb.getData(), mb.getSize());

	if (s == ZSTD_CONTENTSIZE_ERROR || s == ZSTD_CONTENTSIZE_UNKNOWN)
		throw String("Can't resolve content size");
    
    return s;
}

size_t DictionaryHelpers::decompressWithOptionalDictionary(const MemoryBlock& input, size_t numBytesToDecompress, MemoryBlock& output, PointerTypes::DecompressionContext* context, PointerTypes::DecompressionDictionary* dictionary)
{
	auto numBytesUncompressed = DictionaryHelpers::getDecompressedSize(input);
	DictionaryHelpers::checkResult(numBytesUncompressed);

	output.ensureSize(numBytesUncompressed);

	size_t numRead = 0;

	if (dictionary != nullptr)
	{
		numRead = ZSTD_decompress_usingDDict(context, output.getData(), numBytesUncompressed, input.getData(), numBytesToDecompress, dictionary);
	}
	else
	{
		numRead = ZSTD_decompressDCtx(context, output.getData(), numBytesUncompressed, input.getData(), numBytesToDecompress);
	}	

	DictionaryHelpers::checkResult(numRead);

	return numRead;
}

size_t DictionaryHelpers::compressWithOptionalDictionary(PointerTypes::CompressionContext* context, MemoryBlock& output, const MemoryBlock& input, PointerTypes::CompressionDictionary* dictionary, int compressionLevel)
{
	size_t numWritten = 0;

	auto min_size = jmax<size_t>(input.getSize(), 256);

	output.ensureSize(min_size, true);

	if (dictionary != nullptr)
	{
		numWritten = ZSTD_compress_usingCDict(context, output.getData(), output.getSize(), input.getData(), input.getSize(), dictionary);
	}
	else
		numWritten = ZSTD_compressCCtx(context, output.getData(), output.getSize(), input.getData(), input.getSize(), compressionLevel);

	DictionaryHelpers::checkResult(numWritten);

	return numWritten;
}

zstd::DictionaryHelpers::TrainingData DictionaryHelpers::getTrainingData(const Array<String>& stringList)
{
	MemoryOutputStream mos;

	Array<size_t> sampleSizes;

	for (const auto& text : stringList)
	{
		auto before = mos.getPosition();
		mos.writeString(text);
		auto after = mos.getPosition();

		sampleSizes.add(after - before);
	}

	return { mos.getMemoryBlock(), sampleSizes };
}

zstd::DictionaryHelpers::TrainingData DictionaryHelpers::getTrainingData(const Array<File>& fileList)
{
	MemoryOutputStream mos;

	Array<size_t> sampleSizes;

	int numUsed = 0;

	for (const auto& f : fileList)
	{
		MemoryBlock mb;
		f.loadFileAsData(mb);
		mos.write(mb.getData(), mb.getSize());
		sampleSizes.add(mb.getSize());

		numUsed++;

		if (numUsed >= 200 || mos.getPosition() > 4000000)
			break;
	}

	return { mos.getMemoryBlock(), sampleSizes };
}

zstd::DictionaryHelpers::TrainingData DictionaryHelpers::getTrainingData(const Array<ValueTree>& valueTreeList)
{
	MemoryOutputStream mos;

	Array<size_t> sampleSizes;

	int numUsed = 0;

	for (const auto& v : valueTreeList)
	{
		int posBefore = mos.getPosition();
		v.writeToStream(mos);
		int numWritten = mos.getPosition() - posBefore;
		sampleSizes.add(numWritten);

		numUsed++;

		if (numUsed >= 200 || mos.getPosition() > 1000000)
			break;

	}

	return { mos.getMemoryBlock(), sampleSizes };
}

zstd::DictionaryHelpers::TrainingData DictionaryHelpers::getTrainingData(const Array<MemoryBlock>& blockList)
{
	MemoryOutputStream mos;

	Array<size_t> sampleSizes;

	int numUsed = 0;

	for (const auto& mb : blockList)
	{
		int posBefore = mos.getPosition();

		mos.write(mb.getData(), mb.getSize());

		int numWritten = mos.getPosition() - posBefore;
		sampleSizes.add(numWritten);

		numUsed++;

		if (numUsed >= 200 || mos.getPosition() > 2000000)
			break;

	}

	return { mos.getMemoryBlock(), sampleSizes };
}

ZSTD_CDict* DictionaryHelpers::create(ZSTD_CDict* t, void* dictBuffer, size_t dictSize, int compressionLevel)
{
	return ZSTD_createCDict(dictBuffer, dictSize, compressionLevel);
}

ZSTD_DDict* DictionaryHelpers::create(ZSTD_DDict* t, void* dictBuffer, size_t dictSize, int /*unused*/)
{
	return ZSTD_createDDict(dictBuffer, dictSize);
}

bool DictionaryHelpers::readIntoMemory(const File& f, MemoryOutputStream& input)
{
	FileInputStream fis(f);
	auto numWritten = input.writeFromInputStream(fis, fis.getTotalLength());

	return numWritten == fis.getTotalLength();
}

bool DictionaryHelpers::readIntoMemory(const ValueTree& v, MemoryOutputStream& input)
{
	v.writeToStream(input);

	return true;
}

bool DictionaryHelpers::readIntoMemory(const String& text, MemoryOutputStream& input)
{
	return input.writeString(text);
}

bool DictionaryHelpers::readIntoMemory(const MemoryBlock& mb, MemoryOutputStream& input)
{
	return input.write(mb.getData(), mb.getSize());
}

bool DictionaryHelpers::createFromMemory(const MemoryBlock& mb, ValueTree& v)
{
	v = ValueTree::readFromData(mb.getData(), mb.getSize());
	return v.isValid();
}

bool DictionaryHelpers::createFromMemory(const MemoryBlock& mb, File& f)
{
	f.create();
	return f.replaceWithData(mb.getData(), mb.getSize());
}

bool DictionaryHelpers::createFromMemory(const MemoryBlock& mb, String& text)
{
	text = mb.toString();

	return text.isNotEmpty();
}

bool DictionaryHelpers::createFromMemory(const MemoryBlock& mb, MemoryBlock& outputBlock)
{
	outputBlock.ensureSize(mb.getSize());
	outputBlock.copyFrom(mb.getData(), 0, mb.getSize());

	return true;
}

size_t DictionaryHelpers::train(void* data, size_t dictSize, TrainingData& tData)
{
	return ZDICT_trainFromBuffer(data, dictSize, tData.flatData.getData(), tData.sampleSizes.getRawDataPointer(), tData.sampleSizes.size());
}

void DictionaryHelpers::freeDictionaries(PointerTypes::CompressionDictionary* c_dictionary, PointerTypes::DecompressionDictionary* d_dictionary)
{
	if (c_dictionary != nullptr)
	{
		ZSTD_freeCDict(c_dictionary);
	}
	if (d_dictionary != nullptr)
	{
		ZSTD_freeDDict(d_dictionary);
	}
}

zstd::PointerTypes::CompressionContext* DictionaryHelpers::createCompressorContext()
{
	return ZSTD_createCCtx();
}

PointerTypes::DecompressionContext* DictionaryHelpers::createDecompressorContext()
{
	return ZSTD_createDCtx();
}

void DictionaryHelpers::freeCompressorContext(PointerTypes::CompressionContext* ctx)
{
	if(ctx != nullptr)
		ZSTD_freeCCtx(ctx);
}

void DictionaryHelpers::freeDecompressorContext(PointerTypes::DecompressionContext* ctx)
{
	if(ctx != nullptr)
		ZSTD_freeDCtx(ctx);
}

juce::String Helpers::createBinaryDataDictionaryFromDirectory(const File& rootDirectory, const String& extension)
{
	Array<File> files;
	
	rootDirectory.findChildFiles(files, File::findFiles, true, extension);

	DBG("Creating Dictionary...");

	ZFileDictionaryPtr dictionary = new ZDictionary<File>(files, true, 19);

	return dictionary->dumpAsBinaryData();
}

}
