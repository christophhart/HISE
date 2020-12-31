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
namespace cppgen {
using namespace juce;

Base& Base::operator<<(const String& line)
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

Base& Base::operator<<(const jit::FunctionData& f)
{
	lines.add(f.getSignature({}, false));

	return *this;
}

String Base::toString() const
{
	switch (t)
	{
	case OutputType::WrapInBlock:  return wrapInBlock();
	default:					   return parseLines();
	}
}

bool Base::isIntendKeyword(int line) const
{
	if (line < 0)
		return false;

	if (matchesEnd(line, JitTokens::closeBrace))
		return false;

	const static TokenType ik[] = { JitTokens::for_, JitTokens::if_, JitTokens::while_, JitTokens::else_ };

	for (const auto& s : ik)
	{
		if (matchesStart(line, s))
			return true;
	}

	return false;
}


int Base::getIntendDelta(int line) const
{
	if (matchesStart(line, JitTokens::namespace_))
		return -1000;

	if (matchesStart(line, JitTokens::public_, JitTokens::private_, JitTokens::protected_))
		return -1;

	if (matchesStart(line, "#"))
		return -1000;

	if (isIntendKeyword(line - 1) && !matchesStart(line, JitTokens::openBrace))
		return 1;

	return 0;
}

bool Base::containsButNot(int line, TokenType toContain, TokenType toNotContain) const
{
	const auto& s = lines[line];
	return s.contains(toContain) && !s.contains(toNotContain);
}

bool Base::matchesEnd(int line, TokenType t, TokenType other1/*=nullptr*/, TokenType other2/*=nullptr*/) const
{
	// Skip comments
	if (matchesStart(line, "//"))
		return false;

	return lines[line].endsWith(t) ||
		(other1 != nullptr && lines[line].endsWith(other1)) ||
		(other2 != nullptr && lines[line].endsWith(other2));
}

bool Base::matchesStart(int line, TokenType t, TokenType other1 /*= nullptr*/, TokenType other2 /*= nullptr*/) const
{
	return lines[line].startsWith(t) ||
		(other1 != nullptr && lines[line].startsWith(other1)) ||
		(other2 != nullptr && lines[line].startsWith(other2));
}

String Base::parseLines() const
{
	if (t >= OutputType::AddTabs)
	{
		String s;

		int intendLevel = 0;

		for (int i = 0; i < lines.size(); i++)
		{
			if (containsButNot(i, JitTokens::closeBrace, JitTokens::openBrace))
				intendLevel--;

			auto thisIntend = intendLevel + jmax(0, getIntendDelta(i));

			for (int j = 0; j < thisIntend; j++)
				s << "\t";

			s << lines[i] << "\n";

			if (containsButNot(i, JitTokens::openBrace, JitTokens::closeBrace))
				intendLevel++;


		}

		return s;
	}
	else
		return lines.joinIntoString("\n");
}

String Base::wrapInBlock() const
{
	if (matchesStart(0, JitTokens::openBrace))
	{
		return parseLines();
	}
	else
	{
		Base copy(*this);
		copy.lines.insert(0, "{");
		copy.lines.add("}");
		return copy.parseLines();
	}
}

Struct::Struct(Base& parent, const Identifier& id, const jit::TemplateParameter::List& tp) :
	Op(parent)
{
	String def;

	if (!tp.isEmpty())
	{
		def << JitTokens::template_ << " " << TemplateParameter::ListOps::toString(tp, true);
	}

	def << JitTokens::struct_ << " " << id;

	parent << def;
	parent << "{";
	parent.pushScope(id);
}

Namespace::Namespace(Base& parent, const Identifier& id) :
	Op(parent)
{
	String def;
	def << JitTokens::namespace_ << " " << id;
	parent << def;
	parent << "{";
	parent.pushScope(id);
}

Function::Function(Base& parent, const jit::FunctionData& f) :
	Op(parent)
{
	jit::FunctionData copy(f);
	copy.id = NamespacedIdentifier(f.id.getIdentifier());
	parent << copy;
	parent.pushScope(copy.id.getIdentifier());
}

String UsingTemplate::toString() const
{
	String s;
	s << JitTokens::using_ << " ";
	s << scopedId.getIdentifier() << " ";
	s << JitTokens::assign_ << " ";

	s << getType();
	s << JitTokens::semicolon;

	return s;
}

String UsingTemplate::getType() const
{
	String s;

	s << tId.toString();

	if (!args.isEmpty())
	{
		s << JitTokens::lessThan;

		for (auto& a : args)
			s << a << ", ";

		s = s.upToLastOccurrenceOf(", ", false, false);

		s << JitTokens::greaterThan;
	}

	return s;
}

StackVariable::StackVariable(Base& parent, const Identifier& id, const jit::TypeInfo& t) :
	DefinitionBase(parent, id),
	Op(parent),
	type(t)
{

}

void StackVariable::flush()
{
	String s;
	s << type.toString();
	s = s.replace("any", "auto");
	s << Space;
	s << scopedId.getIdentifier();
	s << Space << JitTokens::assign_ << Space;
	s << expression;
	s << ";";

	parent << s;
	Op::flush();
}

String StackVariable::toString() const
{
	if (isFlushed())
	{
		if (scopedId.getParent() == parent.getCurrentScope())
			return scopedId.getIdentifier().toString();
		else
			return scopedId.toString();
	}
	else
		return expression;
}

}
}