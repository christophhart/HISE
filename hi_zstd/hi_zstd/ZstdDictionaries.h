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

#ifndef ZSTD_DICTIONARIES_H_INCLUDED
#define ZSTD_DICTIONARIES_H_INCLUDED

namespace zstd {
using namespace juce;


// =========================================================================================================


template <class SourceType> class DictionaryProviderBase
{
public:

	// =========================================================================

	DictionaryProviderBase(InputStream* input_) :
		input(input_)
	{}

	virtual ~DictionaryProviderBase() {};

	virtual MemoryBlock createDictionaryData() = 0;

	// =========================================================================

protected:

	InputStream * input;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DictionaryProviderBase)
};


// =========================================================================================================


template <class SourceType> class NoDictionaryProvider : public DictionaryProviderBase<void>
{
public:

	NoDictionaryProvider() :
		DictionaryProviderBase(nullptr)
	{}

	MemoryBlock createDictionaryData() override { return {}; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoDictionaryProvider)
};


template <class SourceType> class HeaderDictionaryProvider : public DictionaryProviderBase<SourceType>
{
public:

	HeaderDictionaryProvider(InputStream* input_) :
		DictionaryProviderBase<SourceType>(input_),
        inputStream(input_)
	{}

	MemoryBlock createDictionaryData() override;

    InputStream* inputStream;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderDictionaryProvider)
};

// =========================================================================================================

template <class SourceType> class ZDictionary : public ReferenceCountedObject
{
public:

	using DecompressorType = PointerTypes::DecompressionDictionary;
	using CompressorType = PointerTypes::CompressionDictionary;

	// =========================================================================

	ZDictionary(const Array<SourceType>& valueTreeList, bool useForCompression, int compressionLevel = 19);;
	ZDictionary(InputStream& existingDictionary, bool useForCompression, int compressionLevel = 19);
	ZDictionary(const MemoryBlock& existingDictionary, bool useForCompression, int compressionLevel = 19);
	~ZDictionary();

	// =========================================================================

	void save(OutputStream& stream);
	juce::String dumpAsBinaryData() const;

	DecompressorType* getRawDictionaryForDecompression() { return d_dictionary; }
	CompressorType* getRawDictionaryForCompression() { return c_dictionary; }

	size_t getDictionarySize() const noexcept { return dictSize; }

	// =========================================================================

private:

	void createDictionary(bool useForCompression, int compressionLevel);

	void train(const Array<SourceType>& fileList, bool useForCompression, int compressionLevel);

	bool useAsCompressorDictionary;
	
	HeapBlock<uint8> data;

	size_t dictSize;
	CompressorType* c_dictionary = nullptr;
	DecompressorType* d_dictionary = nullptr;

	// =========================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZDictionary);
};

// =========================================================================================================

using ZValueTreeDictionaryPtr = ReferenceCountedObjectPtr<ZDictionary<ValueTree>>;
using ZStringDictionaryPtr = ReferenceCountedObjectPtr<ZDictionary<String>>;
using ZFileDictionaryPtr = ReferenceCountedObjectPtr<ZDictionary<File>>;
using ZRawDictionaryPtr = ReferenceCountedObjectPtr<ZDictionary<MemoryBlock>>;

}

#endif

