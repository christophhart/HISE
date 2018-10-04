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

#ifndef ZSTD_UNIT_TESTS_H_INCLUDED
#define ZSTD_UNIT_TESTS_H_INCLUDED

namespace zstd {
using namespace juce;


class ZStdUnitTests : public UnitTest
{
public:

	ZStdUnitTests() :
		UnitTest("Zstd Unit Tests")
	{

	}

	void runTest() override;

private:

	

	void testCompareWithGzip();

	template <class SourceType, class TargetType> void testConversion()
	{
		beginTest("Testing conversion without compression");

		SourceType s;
		createUncompressedTestData(s);

		TargetType target;
		createEmptyTarget(target);

		ZDefaultCompressor compressor;

		auto result = compressor.convert(s, target);

		expect(result.wasOk(), "Conversion failed");

		if (result.wasOk())
		{
			SourceType v2;

			createEmptyTarget(v2);
			result = compressor.convert(target, v2);
			expect(result.wasOk(), "Deconversion failed");
			expect(compare(s, v2), "Not equal");
		}

		sourceTempFile = nullptr;
		targetTempFiles.clear();
	}


	template <class SourceType, class ProviderType> void testBufferedCompression()
	{
		beginTest("Testing buffered compression");

		SourceType s;
		createUncompressedTestData(s);

		ZCompressor<ProviderType> compressor;

		MemoryBlock mb;

		auto result = compressor.compress(s, mb);

		expect(result.wasOk(), "Compression failed");

		SourceType v2;

		createEmptyTarget(v2);

		compressor.expand(mb, v2);

		expect(compare(s, v2), "Not equal");

		sourceTempFile = nullptr;
		targetTempFiles.clear();
	}

	template <class SourceType, class TargetType, class ProviderType> void testDirectCompression()
	{
		beginTest("Testing direct compression");

		SourceType s;
		createUncompressedTestData(s);

		TargetType target;
		createEmptyTarget(target);

		ZCompressor<ProviderType> compressor;

		auto result = compressor.compress(s, target);

		expect(result.wasOk(), "Compression failed");

		if (result.wasOk())
		{
			SourceType v2;

			createEmptyTarget(v2);

			result = compressor.expand(target, v2);

			expect(result.wasOk(), "Decompression failed");

			expect(compare(s, v2), "Not equal");
		}

		sourceTempFile = nullptr;
		targetTempFiles.clear();
	}

	void initRandomValues();
	String getRandomIdFromPool();;
	double getRandomNumberFromPool();;

	static bool compare(const String& first, const String& second);
	static bool compare(const ValueTree& first, const ValueTree& second);
	static bool compare(const File& first, const File& second);

	void createEmptyTarget(File& f);
	void createEmptyTarget(ValueTree& v);
	void createEmptyTarget(String& text);

	void createUncompressedTestData(File& f, int maxElements = -1);
	void createUncompressedTestData(ValueTree& v, int maxElements = -1);
	void createUncompressedTestData(String& text, int maxElements = -1);

	ScopedPointer<TemporaryFile> sourceTempFile;
	OwnedArray<TemporaryFile> targetTempFiles;

	StringArray ids;
	Array<double> values;

	Random r;
};

static ZStdUnitTests zTest;

}

#endif