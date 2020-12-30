/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {

using namespace juce;

class CppBuilder
{
public:

	using TokenType = const char*;

	enum class OutputType
	{
		NoProcessing,
		AddTabs,
		WrapInBlock,
		numOutputTypes
	};

	CppBuilder(OutputType type):
		t(type)
	{}

	virtual ~CppBuilder()
	{};

	CppBuilder& operator<<(const String& line);

	void addComment(const String& comment)
	{
		lines.add("// " + comment);
	}

	String toString() const;



private:

	bool isIntendKeyword(int line) const;
	int getIntendDelta(int line) const;
	bool matchesEnd(int line, TokenType t, TokenType other1=nullptr, TokenType other2=nullptr) const;
	bool matchesStart(int line, TokenType t, TokenType other1 = nullptr, TokenType other2 = nullptr) const;

	String parseLines() const;
	String wrapInBlock() const;

	const OutputType t;
	StringArray lines;
};


}
