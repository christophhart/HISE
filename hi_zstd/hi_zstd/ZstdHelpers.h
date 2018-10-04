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

#ifndef ZSTD_HELPERS_H_INCLUDED
#define ZSTD_HELPERS_H_INCLUDED

// These pointers are used in the header files, so we forward declare them here:

struct ZSTD_DDict_s;
struct ZSTD_CDict_s;
struct ZSTD_DCtx_s;
struct ZSTD_CCtx_s;

namespace zstd {

namespace PointerTypes
{
	using DecompressionDictionary = ZSTD_DDict_s;
	using CompressionDictionary = ZSTD_CDict_s;
	using DecompressionContext = ZSTD_DCtx_s;
	using CompressionContext = ZSTD_CCtx_s;
}

using namespace juce;




struct DictionaryHelpers
{
	

	static void checkResult(size_t code);

	struct TrainingData
	{
		MemoryBlock flatData;
		Array<size_t> sampleSizes;
	};

	static size_t getDecompressedSize(const MemoryBlock& mb);


	static size_t decompressWithOptionalDictionary(const MemoryBlock& input, size_t numBytesToDecompress, MemoryBlock& output, PointerTypes::DecompressionContext* context, PointerTypes::DecompressionDictionary* dictionary);

	static size_t compressWithOptionalDictionary(PointerTypes::CompressionContext* context, MemoryBlock& output, const MemoryBlock& input, PointerTypes::CompressionDictionary* dictionary, int compressionLevel);

	static TrainingData getTrainingData(const Array<String>& stringList);
	static TrainingData getTrainingData(const Array<File>& fileList);
	static TrainingData getTrainingData(const Array<ValueTree>& valueTreeList);
	static TrainingData getTrainingData(const Array<MemoryBlock>& valueTreeList);

	static PointerTypes::CompressionDictionary* create(PointerTypes::CompressionDictionary* t, void* dictBuffer, size_t dictSize, int compressionLevel);
	static PointerTypes::DecompressionDictionary* create(PointerTypes::DecompressionDictionary* t, void* dictBuffer, size_t dictSize, int /*unused*/);

	static bool readIntoMemory(const File& f, MemoryOutputStream& input);
	static bool readIntoMemory(const ValueTree& v, MemoryOutputStream& input);
	static bool readIntoMemory(const String& text, MemoryOutputStream& input);
	static bool readIntoMemory(const MemoryBlock& mb, MemoryOutputStream& input);
		   
	static bool createFromMemory(const MemoryBlock& mb, ValueTree& v);
	static bool createFromMemory(const MemoryBlock& mb, File& f);
	static bool createFromMemory(const MemoryBlock& mb, String& text);
	static bool createFromMemory(const MemoryBlock& mb, MemoryBlock& outputBlock);

	static size_t train(void* data, size_t dictSize, TrainingData& tData);

	static void freeDictionaries(PointerTypes::CompressionDictionary* c_dictionary, PointerTypes::DecompressionDictionary* d_dictionary);

	static PointerTypes::CompressionContext* createCompressorContext();
	static PointerTypes::DecompressionContext* createDecompressorContext();

	static void freeCompressorContext(PointerTypes::CompressionContext* ctx);
	static void freeDecompressorContext(PointerTypes::DecompressionContext* ctx);
};

struct Helpers
{
	static String createBinaryDataDictionaryFromDirectory(const File& rootDirectory, const String& extension);
};


}

#endif