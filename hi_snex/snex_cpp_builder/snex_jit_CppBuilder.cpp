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

snex::cppgen::Base& Base::operator<<(juce_wchar specialCharacter)
{
	jassert(specialCharacter == AlignMarker || specialCharacter == IntendMarker);

	auto l = lines[lines.size() - 1];
	l << specialCharacter;
	lines.set(lines.size() - 1, l);
	return *this;
}

void Base::addComment(const String& comment, CommentType commentType)
{
	if (t == OutputType::Uglify)
		return;

	switch (commentType)
	{
	case CommentType::AlignOnSameLine:
	{
        // Remove the intend markers to avoid compile errors since this is expected
        // be a single line
        
        auto cToUse = comment.removeCharacters(StringHelpers::withToken(IntendMarker));
        
		String l;

		auto lastLine = lines[lines.size() - 1];

		if (lastLine.containsChar(IntendMarker))
		{
			auto lv = StringArray::fromTokens(lastLine, StringHelpers::withToken(IntendMarker), "");

			auto cv = lv[1];
			cv << AlignMarker << "// " << cToUse;

			lv.set(1, cv);

			l = lv.joinIntoString(StringHelpers::withToken(IntendMarker));
		}
		else
		{
			l << lastLine << AlignMarker << "// " << cToUse;
		}
		
		lines.set(lines.size() - 1, l);
		break;
	}
	case CommentType::FillTo80:
	{
		String c;

		c << "%FILL40";
		c << comment;
		
		lines.add(c);
		addEmptyLine();
		break;
	}
	case CommentType::FillTo80Light:
	{
		String c = comment;
		c << " %FILL80";
		lines.add("// " + c);
		addEmptyLine();
		break;
	}
	case CommentType::Raw:
		lines.add("// " + comment);
		break;
	case CommentType::RawWithNewLine:
		lines.add("// " + comment);
		addEmptyLine();
		break;
	}
}

String Base::parseUglified()
{
	String s;
	for (auto& l : lines)
	{
		l = l.removeCharacters(StringHelpers::withToken(IntendMarker, StringHelpers::withToken(AlignMarker)));
		s << l.trim();
	}

	return s;
}

void Base::replaceWildcard(const String& wc, const String& expression)
{
	for (auto& l : lines)
		l = l.replace(wc, expression);
}

int Base::getRealLineLength(const String& s)
{
	int l = 0;

	auto start = s.getCharPointer();
	auto end = start + s.length();

	while (start != end)
	{
		if (*start == '\t')
		{
			l += (4 - l % 4);
		}
		else
			l++;

		start++;
	}

	return l;
}

String Base::toString() const
{
    
    
	switch (t)
	{
	case OutputType::Uglify:       return const_cast<Base*>(this)->parseUglified();
	case OutputType::WrapInBlock:  return wrapInBlock();
	case OutputType::StatementListWithoutSemicolon: return parseRawAndAddSemicolon();
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

	if (matchesStart(line, "public:", "private:", "protected:"))
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

String Base::parseRawAndAddSemicolon() const
{
	String s;

	for (const auto& l : lines)
	{
		if (l.isEmpty())
			continue;

		s << l << ";" << "\n";
	}

	return s;
}

String Base::parseLines() const
{
	String s;

	if (header)
	{
		auto hContent = StringArray::fromLines(header());

		ParseState state(hContent);

		

		for (int i = 0; i < hContent.size(); i++)
		{
			s << parseLine(state, i);
		}
	}

	if (t >= OutputType::AddTabs)
	{
		ParseState state(lines);

		for (int i = 0; i < lines.size(); i++)
		{
			auto l = lines[i];

			auto thisIsEmpty = !l.containsNonWhitespaceChars();

			if (state.lastLineWasEmpty && thisIsEmpty)
				continue;

			state.lastLineWasEmpty = thisIsEmpty;

			s << parseLine(state, i);
		}

		if (s.contains("%FILL"))
		{
			String ns;

			auto again = StringArray::fromLines(s);

			int maxLineLength = 0;

			for (auto& a : again)
				maxLineLength = jmax(maxLineLength, getRealLineLength(a));

            maxLineLength = jmin(100, maxLineLength);
            
			for (auto& l : again)
			{
				if (l.contains("%FILL80"))
				{
					auto lc = l.upToFirstOccurrenceOf("%FILL80", false, false);
					auto required = jmax(0, maxLineLength - getRealLineLength(lc));

					ns << lc;

					for (int i = 0; i < required; i++)
						ns << '-';

					ns << "\n";
				}
				else if (l.contains("%FILL40"))
				{
					auto lc = l.fromFirstOccurrenceOf("%FILL40", false, false);

					auto required = jmax(0, (maxLineLength - getRealLineLength(lc) - 6) / 2);

					String h;

					for (int i = 0; i < required; i++)
						h << "=";

					ns << "// " << h << JitTokens::bitwiseOr << Space << lc << Space << JitTokens::bitwiseOr << h << "\n";

				}
				else
				{
					ns << l << "\n";
				}
			}

			return ns;
		}

		
	}
	else
		s << lines.joinIntoString("\n");

	return s;
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


String Base::parseLine(ParseState& state, int i) const
{
	state.currentLine = i;
	auto lineContent = state.linesToUse[i];
	return parseIntendation(state, lineContent);
}


String Base::parseIntendation(ParseState& state, const String& lineContent) const
{
	String s;

	auto i = state.currentLine;

	if (matchesStart(i, JitTokens::namespace_))
		state.intendLevel = -1;

	if (containsButNot(i, JitTokens::closeBrace, JitTokens::openBrace))
		state.intendLevel = jmax(0, state.intendLevel - 1);

	auto prevIntend = state.intendLevel;

	state.intendLevel = jmax(0, state.intendLevel + getIntendDelta(i));

	for (int j = 0; j < state.intendLevel; j++)
		s << "\t";

	s << parseLineWithIntendedLineBreak(state, lineContent);

	state.intendLevel = prevIntend;

	if (containsButNot(i, JitTokens::openBrace, JitTokens::closeBrace))
		state.intendLevel++;

	return s;
}

String Base::parseLineWithIntendedLineBreak(ParseState& state, const String& lineContent) const
{
	if (lineContent.containsChar(IntendMarker))
	{
		String m;
		m << IntendMarker;
		auto sublines = StringArray::fromTokens(lineContent, m, "");

		auto before = sublines[0].length();

		String spaces;

		for (int j = 0; j < state.intendLevel; j++)
			spaces << "    ";

		for (int i = 0; i < before; i++)
			spaces << Space;

		String s;

		auto firstLine = sublines[0] + sublines[1];

		s << parseLineWithAlignedComment(state, firstLine);

		for (int i = 2; i < sublines.size(); i++)
		{
			s << spaces;

			if (sublines[i].containsChar(AlignMarker))
				jassertfalse;

			s << parseLineWithAlignedComment(state, sublines[i]);
		}

		return s;
	}

	return parseLineWithAlignedComment(state, lineContent);
}

String Base::parseLineWithAlignedComment(ParseState& state, const String& lineContent) const
{
	if (lineContent.containsChar(AlignMarker))
	{
		String s;

		if (state.currentAlignLength == -1)
		{
			for (int j = state.currentLine; j < state.linesToUse.size(); j++)
			{
				if (state.linesToUse[j].containsChar(AlignMarker))
				{
					state.currentAlignLength = jmax(state.currentAlignLength, state.linesToUse[j].indexOfChar(AlignMarker) + 1);
				}
					
				else
					break;
			}
		}

		auto thisLength = lineContent.indexOfChar(AlignMarker);
		int spacesRequired = state.currentAlignLength - thisLength;
		jassert(spacesRequired > 0);

		String spaces;
		for (int i = 0; i < spacesRequired; i++)
			spaces << Space;

		String a;
		a << AlignMarker;

		s << lineContent.replace(a, spaces) << "\n";

		return s;
	}
	else
	{
		state.currentAlignLength = -1;

		String s;

		s << lineContent << "\n";

		return s;
	}
}




Struct::Struct(Base& parent, const Identifier& id, const Array<DefinitionBase*>& baseClasses, const jit::TemplateParameter::List& tp) :
	Op(parent),
	DefinitionBase(parent, id)
{
	templateArguments.addArray(tp);
	parent.addIfNotEmptyLine();

	String def;

	if (!tp.isEmpty())
	{
		def << JitTokens::template_ << Space << TemplateParameter::ListOps::toString(tp, true) << Space;
	}

	def << JitTokens::struct_ << Space << id;

	if (!baseClasses.isEmpty())
	{
		def << JitTokens::colon;
		
		auto useIntend = baseClasses.size() > 1;

		for (auto bc : baseClasses)
		{
			def << Space;
			
			if (useIntend)
				def << AlignMarker;
			
			def << JitTokens::public_ << Space << bc->toExpression() << ", \n";
		}

		def = def.upToLastOccurrenceOf(", \n", false, false);
	}

	parent << def;
	parent << "{";
	parent.pushScope(id);
}

Struct::Struct(Base& parent, const Identifier& id, const Array<NamespacedIdentifier>& baseClasses, const jit::TemplateParameter::List& tp, bool /*useIds*/) :
	Op(parent),
	DefinitionBase(parent, id)
{
	templateArguments.addArray(tp);
	parent.addIfNotEmptyLine();

	String def;

	if (!tp.isEmpty())
	{
		def << JitTokens::template_ << Space << TemplateParameter::ListOps::toString(tp, true) << Space;
	}

	def << JitTokens::struct_ << Space << id;

	if (!baseClasses.isEmpty())
	{
		def << JitTokens::colon;

		auto useIntend = baseClasses.size() > 1;

		for (auto bc : baseClasses)
		{
			def << Space;

			if (useIntend)
				def << AlignMarker;

			def << JitTokens::public_ << Space << bc.toString() << ", \n";
		}

		def = def.upToLastOccurrenceOf(", \n", false, false);
	}

	parent << def;
	parent << "{";
	parent.pushScope(id);
}

Namespace::Namespace(Base& parent, const Identifier& id, bool isEmpty_) :
	Op(parent),
	isEmpty(isEmpty_)
{
	if (!isEmpty)
	{
		String def;
		def << JitTokens::namespace_ << " " << id;
		parent << def;
		parent << "{";
		parent.pushScope(id);
	}
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

	if (!templateArguments.isEmpty())
	{
		s << "template ";
		s << snex::jit::TemplateParameter::ListOps::toString(templateArguments, true);
		s << "\n";
	}

	s << JitTokens::using_ << " ";
	s << scopedId.getIdentifier() << " ";
	s << JitTokens::assign_ << " ";

    auto e = toExpression();
    
    auto existingId = parent.pushDefinition(scopedId, e);
    
    if(existingId.isValid())
    {
        if(existingId.getParent() == scopedId.getParent())
        {
            s << existingId.getIdentifier().toString();
        }
        else
            s << existingId.toString();
        
        appendTemplateParameters(s);
        
        if(s.length() < 80)
            s = s.replaceCharacter('\n', ' ');
        
    }
    else
    {
        s << e;
    }
    
    
	s << JitTokens::semicolon;

	return s;
}

String UsingTemplate::toExpression() const
{
	if (isFlushed())
	{
		auto s = DefinitionBase::toExpression();

        appendTemplateParameters(s);
        
		
		
		return s;
	}
		

	return getUsingExpression();
}

String UsingTemplate::getUsingExpression() const
{
	String s;

	s << tId.toString();

	auto useIntendMarkers = args.size() > 2;

	for (auto& a : args)
	{
		if (a.length() > 22)
			useIntendMarkers = true;
	}

	if (!args.isEmpty())
	{
		s << JitTokens::lessThan;

		for (auto& a : args)
		{
			if (useIntendMarkers)
				s << IntendMarker;

			String i;
			i << IntendMarker;

			auto t = a.removeCharacters(i);

			s << t << ", ";
		}
			
		s = s.upToLastOccurrenceOf(", ", false, false);
		s << JitTokens::greaterThan;
	}

	return s;
}

void UsingTemplate::flush()
{
    auto e = toExpression();

    if (!e.isEmpty())
    {
        if(scopedId != parent.getCurrentScope())
            parent << toString();
    }

    Op::flush();
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

String StringHelpers::makeValidCppName(const String& input)
{
	String s;

	if (CharacterFunctions::isDigit(input[0]))
		s << "_";

	s << input;
	s = s.replace("-", "_");
	s = s.removeCharacters("\"/\\ \t\n\r!§$%&/()=[]{}");
	return s;
}

}
}
