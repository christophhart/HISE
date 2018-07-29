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

#ifndef ZSTD_DICTIONARIES_IMPL_H_INCLUDED
#define ZSTD_DICTIONARIES_IMPL_H_INCLUDED

namespace zstd {
using namespace juce;


template <class SourceType>
MemoryBlock zstd::HeaderDictionaryProvider<SourceType>::createDictionaryData()
{
    auto dictSize = inputStream->readInt();
	MemoryBlock dictData;
	inputStream->readIntoMemoryBlock(dictData, dictSize);
	return dictData;
}


template <class SourceType>
zstd::ZDictionary<SourceType>::ZDictionary(const Array<SourceType>& valueTreeList, bool useForCompression, int compressionLevel /*= 19*/) :
	dictSize(100 * 1024)
{
	data.allocate(dictSize, true);
	train(valueTreeList, useForCompression, compressionLevel);
}


template <class SourceType>
zstd::ZDictionary<SourceType>::ZDictionary(InputStream& existingDictionary, bool useForCompression, int compressionLevel /*= 19*/) :
	dictSize(existingDictionary.getTotalLength())
{
	data.allocate(dictSize, true);
	existingDictionary.read(data.get(), dictSize);

	createDictionary(useForCompression, compressionLevel);
}


template <class SourceType>
zstd::ZDictionary<SourceType>::ZDictionary(const MemoryBlock& existingDictionary, bool useForCompression, int compressionLevel /*= 19*/) :
	dictSize(existingDictionary.getSize())
{
	data.allocate(dictSize, true);

	memcpy(data, existingDictionary.getData(), dictSize);

	createDictionary(useForCompression, compressionLevel);
}


template <class SourceType>
zstd::ZDictionary<SourceType>::~ZDictionary()
{
	DictionaryHelpers::freeDictionaries(c_dictionary, d_dictionary);
}


template <class SourceType>
void zstd::ZDictionary<SourceType>::save(OutputStream& stream)
{
	stream.write(data, dictSize);
}


template <class SourceType>
String zstd::ZDictionary<SourceType>::dumpAsBinaryData() const
{
	String code = "static const unsigned char dictionary[] = { ";

	for (int i = 0; i < dictSize; i++)
	{
		code << String((short)data[i]);

		if (i != dictSize - 1)
			code << ", ";

		if (i > 0 && (i % 60 == 0))
			code << "\n";
	}

	code << " };\n";

	return code;
}




template <class SourceType>
void zstd::ZDictionary<SourceType>::createDictionary(bool useForCompression, int compressionLevel)
{
	if (useForCompression)
	{
		c_dictionary = DictionaryHelpers::create(c_dictionary, data, dictSize, compressionLevel);
	}
	else
	{
		d_dictionary = DictionaryHelpers::create(d_dictionary, data, dictSize, compressionLevel);
	}
}


template <class SourceType>
void zstd::ZDictionary<SourceType>::train(const Array<SourceType>& fileList, bool useForCompression, int compressionLevel)
{
	auto tData = DictionaryHelpers::getTrainingData(fileList);
	dictSize = DictionaryHelpers::train(data, dictSize, tData);
	DictionaryHelpers::checkResult(dictSize);

	createDictionary(useForCompression, compressionLevel);
}


}

#endif
