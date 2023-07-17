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

#ifndef ZSTD_COMPRESSOR_H_INCLUDED
#define ZSTD_COMPRESSOR_H_INCLUDED

namespace zstd {
using namespace juce;

// =========================================================================================================


// =========================================================================================================

/* @internal */
class ZCompressorBase
{
public:

	ZCompressorBase(int compressionLevel_) :
		compressionLevel(compressionLevel_)
	{};

	enum Mode
	{
		CompressOnly,
		DecompressOnly,
		Both,
		numModes
	};

	virtual ~ZCompressorBase()
	{

	}

protected:

	int compressionLevel;

};

// =========================================================================================================

/** The main compressor class of this module.
*
*	A ZCompressor object encapsulates all the logic for converting between different types,
*	compression, decompression, etc.
*
*	In order to use it, create a object on the stack (or reuse it for multiple calls), then call
*	the methods with the given source and target types.
*
*	It handles these JUCE classes natively:
*
*	- ValueTree
*	- String
*	- File
*	- MemoryBlock
*
*	You can give it a DictionaryProvider as template, then it will create the dictionary and use this for compression.
*	If you know the data you are about to process, training a dictionary and using this for the compression will speed up
*	the algorithm and yield smaller sizes. For more information about dictionaries, read the man page of zstd.
*
*/
template <class ProviderType=NoDictionaryProvider<void>> class ZCompressor: public ZCompressorBase
{
public:

	// =========================================================================

	/** Creates a new ZCompressor. If you don't intend to compress and decompress with it, use a exclusive mode, as it will save a few bytes. */
	ZCompressor(Mode m=Both, int compressionLevel_=19);
	~ZCompressor();

	// =========================================================================

	/** Takes a uncompressed SourceType object and compresses it into a TargetType object. 
	*
	*	Possible SourceTypes: File, MemoryBlock, String, ValueTree.
	*	Possible TargetTypes: File, MemoryBlock.
	*/
	template <class SourceType, class TargetType> Result compress(const SourceType& uncompressedSource, TargetType& compressedTarget)
	{
		try
		{
			if (std::is_same<TargetType, juce::String>())
				return Result::fail("Can't use String as target for compression");

			if (std::is_same<TargetType, juce::ValueTree>())
				return Result::fail("Can't use Value as target for compression");

			MemoryOutputStream mos;

			if (!DictionaryHelpers::readIntoMemory(uncompressedSource, mos))
				return Result::fail("Writing to memory failed");

			auto mb = compressRaw(mos.getMemoryBlock());

			if (!DictionaryHelpers::createFromMemory(mb, compressedTarget))
				return Result::fail("Creation from memory failed");

			return Result::ok();
		}
		catch (const juce::String& errorMessage)
		{
			return Result::fail(errorMessage);
		}
	}

	/** Takes a compressed SourceType object and expands it to a uncompressed TargetType object. 
	*
	*	Possible SourceTypes: File, MemoryBlock.
	*	Possible TargetTypes: File, MemoryBlock, String, ValueTree.
	*/
	template <class SourceType, class TargetType> Result expand(const SourceType& compressedSource, TargetType& uncompressedTarget)
	{
		try
		{
			MemoryOutputStream mos;
			if (!DictionaryHelpers::readIntoMemory(compressedSource, mos))
				return Result::fail("Writing to memory failed");

			auto mb = expandRaw(mos.getMemoryBlock());

			if (!DictionaryHelpers::createFromMemory(mb, uncompressedTarget))
				return Result::fail("Creation from memory failed");

			return Result::ok();
		}
		catch (const juce::String& errorMessage)
		{
			return Result::fail(errorMessage);
		}
	}

	/** Converts between the types without compression.
	*	Possible SourceTypes: File, MemoryBlock, String, ValueTree.
	*	Possible TargetTypes: File, MemoryBlock, String, ValueTree.
	*/
	template <class SourceType, class TargetType> Result convert(const SourceType& source, TargetType& target)
	{
		if (typeid(SourceType) == typeid(TargetType))
		{
			// Just copy them normally...
			target = TargetType(*reinterpret_cast<const TargetType*>(&source));
			return Result::ok();
		}

		if (typeid(SourceType) == typeid(ValueTree) && typeid(TargetType) == typeid(String))
			return Result::fail("Can't use String as target for Value");

		MemoryOutputStream mos;

		if (!DictionaryHelpers::readIntoMemory(source, mos))
			return Result::fail("Writing to memory failed");

		auto mb = mos.getMemoryBlock();

		if (!DictionaryHelpers::createFromMemory(mb, target))
			return Result::fail("Creation from memory failed");

		return Result::ok();
	}

	/** Compressed the given MemoryBlock in place.
	*
	*	This is not an actual in place operation, instead, it swaps the data with a temporary block.
	*
	*/
	Result compressInplace(MemoryBlock& uncompressedData);

	/** Expands the given MemoryBlock in place. 
	*
	*	This is not an actual in place operation, instead, it swaps the data with a temporary block.
	*/
	Result expandInplace(MemoryBlock& compressedData);

private:

	// =========================================================================

	/** @internal */
	MemoryBlock compressRaw(const MemoryBlock& uncompressedData);

	/** @internal */
	MemoryBlock expandRaw(const MemoryBlock& compressedData);

	MemoryBlock internalBuffer;

	PointerTypes::CompressionContext* c_context = nullptr;
	PointerTypes::DecompressionContext* d_context = nullptr;

	ReferenceCountedObjectPtr<ZDictionary<MemoryBlock>> c_dictionary;
	ReferenceCountedObjectPtr<ZDictionary<MemoryBlock>> d_dictionary;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZCompressor);

	// =========================================================================
};

// =========================================================================================================

using ZDefaultCompressor = ZCompressor<NoDictionaryProvider<void>>;


template <class SourceType> class ZBatchDecompressor
{
public:

	// =========================================================================

	ZBatchDecompressor(ZDictionary<SourceType>* dictionary = nullptr);
	~ZBatchDecompressor();

	// =========================================================================

	bool decompress(Array<SourceType>& destinationList, InputStream& inputStream);

private:

	// =========================================================================

	PointerTypes::DecompressionContext* context;
	ReferenceCountedObjectPtr<ZDictionary<SourceType>> usedDictionary;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZBatchDecompressor);
};

// =========================================================================================================

template <class SourceType> class ZBatchCompressor
{
public:

	// =========================================================================

	ZBatchCompressor(ZDictionary<SourceType>* dictionary = nullptr, int compressionLevel_ = 19);
	~ZBatchCompressor();

	// =========================================================================

	bool compress(const Array<SourceType>& sourceList, OutputStream& stream);

private:

	// =========================================================================

	PointerTypes::CompressionContext* context;
	ReferenceCountedObjectPtr<ZDictionary<SourceType>> usedDictionary;
	int compressionLevel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZBatchCompressor);
};

template <typename SourceType, class ProviderType = NoDictionaryProvider<SourceType>> class ZstdArchive
{
public:

	ZstdArchive(InputStream& input_);

	bool extract(Array<SourceType>& list);

private:

	ReferenceCountedObjectPtr<ZDictionary<SourceType>> dictionary;
	ScopedPointer<ZBatchDecompressor<SourceType>> decompressor;

	InputStream& input;
};


}

#endif