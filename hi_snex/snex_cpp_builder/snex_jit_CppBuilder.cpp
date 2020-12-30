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


namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;


}

snex::CppBuilder& CppBuilder::operator<<(const String& line)
{
	if (line.contains("\n"))
	{
		auto newLines = StringArray::fromLines(line);

		for (auto& s : newLines)
			s = s.trim();

		newLines.removeEmptyStrings();

		lines.addArray(newLines);
	}
	else
		lines.add(line.trim());
	
	return *this;
}

String CppBuilder::toString() const
{
	switch (t)
	{
	case OutputType::WrapInBlock:  return wrapInBlock();
	default:					   return parseLines();
	}
}

bool CppBuilder::isIntendKeyword(int line) const
{
	if (matchesEnd(line, JitTokens::closeBrace))
		return false;

	const static TokenType ik[] = { JitTokens::for_, JitTokens::if_, JitTokens::while_ };

	for (const auto& s : ik)
	{
		if (matchesStart(line, s))
			return true;
	}

	return false;
}


int CppBuilder::getIntendDelta(int line) const
{
	if (matchesStart(line, JitTokens::namespace_))
		return -1000;

	if (matchesStart(line, JitTokens::public_, JitTokens::private_, JitTokens::protected_))
		return -1;

	if (matchesStart(line, "#"))
		return -1000;
}

bool CppBuilder::matchesEnd(int line, TokenType t, TokenType other1/*=nullptr*/, TokenType other2/*=nullptr*/) const
{
	// Skip comments
	if (matchesStart(line, "//"))
		return false;

	return lines[line].endsWith(t) ||
		(other1 != nullptr && lines[line].endsWith(other1)) ||
		(other2 != nullptr && lines[line].endsWith(other2));
}

bool CppBuilder::matchesStart(int line, TokenType t, TokenType other1 /*= nullptr*/, TokenType other2 /*= nullptr*/) const
{
	return lines[line].startsWith(t) ||
		(other1 != nullptr && lines[line].startsWith(other1)) ||
		(other2 != nullptr && lines[line].startsWith(other2));
}

String CppBuilder::parseLines() const
{
	if (t >= OutputType::AddTabs)
	{
		String s;

		int intendLevel = 0;

		for (int i = 0; i < lines.size(); i++)
		{
			auto thisIntend = jmax(0, getIntendDelta(i));

			for (int j = 0; j < thisIntend; j++)
				s << ' ';

			s << lines[i] << "\n";

			if (matchesStart(i, JitTokens::openBrace) || (isIntendKeyword(i) && !matchesStart(i + 1, JitTokens::openBrace)))
				intendLevel++;

			if (matchesEnd(i, JitTokens::closeBrace))
				intendLevel--;
		}

		return s;
	}
	else
		return lines.joinIntoString("\n");
}

String CppBuilder::wrapInBlock() const
{
	if (matchesStart(0, JitTokens::openBrace))
	{
		return parseLines();
	}
	else
	{
		CppBuilder copy(*this);
		copy.lines.insert(0, "{");
		copy.lines.add("}");
		return copy.parseLines();
	}
}

}
