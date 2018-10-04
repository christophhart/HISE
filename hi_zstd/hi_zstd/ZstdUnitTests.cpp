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

#include "ZstdUnitTests.h"


namespace zstd {
using namespace juce;


void ZStdUnitTests::runTest()
{
	initRandomValues();

	testBufferedCompression<ValueTree, NoDictionaryProvider<void>>();
	testBufferedCompression<String, NoDictionaryProvider<void>>();
	testBufferedCompression<File, NoDictionaryProvider<void>>();

	testDirectCompression<ValueTree, File, NoDictionaryProvider<void>>();
	testDirectCompression<String, File, NoDictionaryProvider<void>>();
	testDirectCompression<File, File, NoDictionaryProvider<void>>();

	testCompareWithGzip();

	testConversion<ValueTree, File>();
	testConversion<String, File>();
	testConversion<File, File>();
	testConversion<File, String>();
	testConversion<File, ValueTree>();
	testConversion<String, String>();
	testConversion<ValueTree, ValueTree>();
}

void ZStdUnitTests::testCompareWithGzip()
{
	ValueTree v;
	createUncompressedTestData(v);

	MemoryOutputStream mos;
	MemoryOutputStream mos2;
	GZIPCompressorOutputStream gos(&mos);

	v.writeToStream(mos2);
	v.writeToStream(gos);

	auto mb = mos.getMemoryBlock();

	auto gzipSize = mb.getSize();

	logMessage("Uncompressed size: " + String(gzipSize) + " bytes");
	logMessage("Gzip size: " + String(gzipSize) + " bytes");
}

void ZStdUnitTests::initRandomValues()
{
	for (int i = 0; i < 16; i++)
	{
		String newId;
		createUncompressedTestData(newId, r.nextInt({ 4, 18 }));
		ids.add(newId);

		values.add(r.nextDouble());
	}
}

juce::String ZStdUnitTests::getRandomIdFromPool()
{
	const int index = r.nextInt(ids.size());
	return ids[index];
}

double ZStdUnitTests::getRandomNumberFromPool()
{
	const int index = r.nextInt(values.size());
	return values[index];
}

bool ZStdUnitTests::compare(const String& first, const String& second)
{
	auto firstLength = first.length();
	auto secondLength = second.length();

	if (firstLength != secondLength)
		return false;

	return first.compare(second) == 0;
}

bool ZStdUnitTests::compare(const ValueTree& first, const ValueTree& second)
{
	if (first.getType() != second.getType())
		return false;

	if (first.getNumChildren() != second.getNumChildren())
		return false;

	for (int i = 0; i < first.getNumProperties(); i++)
	{
		auto id = first.getPropertyName(i);

		if (first[id] != second[id])
			return false;
	}

	for (int i = 0; i < first.getNumChildren(); i++)
	{
		if (!compare(first.getChild(i), second.getChild(i)))
			return false;
	}

	return true;
}

bool ZStdUnitTests::compare(const File& first, const File& second)
{
	auto c1 = first.loadFileAsString();
	auto c2 = second.loadFileAsString();

	return compare(c1, c2);
}

void ZStdUnitTests::createEmptyTarget(File& f)
{
	auto targetTempFile = new TemporaryFile();
	f = targetTempFile->getFile();

	targetTempFiles.add(targetTempFile);
}

void ZStdUnitTests::createEmptyTarget(ValueTree& v)
{

}

void ZStdUnitTests::createEmptyTarget(String& text)
{

}

void ZStdUnitTests::createUncompressedTestData(File& f, int maxElements /*= -1*/)
{
	expect(sourceTempFile == nullptr);

	sourceTempFile = new TemporaryFile();

	f = sourceTempFile->getFile();

	String content;

	createUncompressedTestData(content);

	f.replaceWithText(content);
}

void ZStdUnitTests::createUncompressedTestData(ValueTree& v, int maxElements /*= -1*/)
{
	Random rGenerator;

	if (maxElements == -1)
		maxElements = 8;

	v = ValueTree(getRandomIdFromPool());

	int numProperties = rGenerator.nextInt(16);

	for (int i = 0; i < numProperties; i++)
	{
		if (rGenerator.nextBool())
		{
			v.setProperty(getRandomIdFromPool(), getRandomIdFromPool(), nullptr);
		}
		else
		{
			v.setProperty("value", getRandomNumberFromPool(), nullptr);
		}
	}

	int numChildren = rGenerator.nextInt(maxElements);

	for (int i = 0; i < numChildren; i++)
	{
		ValueTree c;
		createUncompressedTestData(c, maxElements - 1);
		v.addChild(c, -1, nullptr);
	}
}

void ZStdUnitTests::createUncompressedTestData(String& text, int maxElements /*= -1*/)
{
	if (maxElements == -1)
		maxElements = 2048;

	while (--maxElements >= 0)
	{
		text << (char)r.nextInt({ 97, 122 });
	}
}

}

