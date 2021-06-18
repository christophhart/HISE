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


#pragma once

#if !HISE_NO_GUI_TOOLS

namespace hise {
using namespace juce;

class SampleMapDictionaryProvider : public zstd::DictionaryProviderBase<void>
{
public:

	SampleMapDictionaryProvider(InputStream* /*input*/ = nullptr) :
		zstd::DictionaryProviderBase<void>(nullptr)
	{

	};

	MemoryBlock createDictionaryData() override;
};

class JavascriptDictionaryProvider : public zstd::DictionaryProviderBase<void>
{
public:

	JavascriptDictionaryProvider(InputStream* /*input*/ = nullptr) :
		zstd::DictionaryProviderBase<void>(nullptr)
	{}

	MemoryBlock createDictionaryData() override;
};


class PresetDictionaryProvider : public zstd::DictionaryProviderBase<void>
{
public:

	PresetDictionaryProvider(InputStream* /*input*/ = nullptr) :
		zstd::DictionaryProviderBase<void>(nullptr)
	{};

	MemoryBlock createDictionaryData() override;
};

class UserPresetDictionaryProvider : public zstd::DictionaryProviderBase<void>
{
public:

	UserPresetDictionaryProvider(InputStream* /*input*/ = nullptr):
		zstd::DictionaryProviderBase<void>(nullptr)
	{}

	MemoryBlock createDictionaryData() override;
};

}

#endif