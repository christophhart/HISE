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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;





void CppGen::Emitter::emitDefinition(String& s, const String& definition, const String& value, bool useQuotes)
{
	s << definition << " (";

	if (useQuotes)
		s << "\"";

	s << value;

	if (useQuotes)
		s << "\"";

	s << ");\n";
}


void CppGen::Emitter::emitConstexprVariable(String& s, const String& variableName, int value)
{
	s << "constexpr static int " << variableName << " = " << String(value) << ";\n";
}

void CppGen::Emitter::emitArgumentList(String& s, const StringArray& args)
{
	for (int i = 0; i < args.size(); i++)
	{
		s << args[i];
		if (i != args.size() - 1)
			s << ", ";
	}
}

void CppGen::Emitter::emitFunctionDefinition(String& s, const MethodInfo& f)
{
	s << f.returnType << " " << f.name << "(";

	emitArgumentList(s, f.arguments);

	s << ")" << f.specifiers << "\n{\n";

	s << f.body;

	s << "}";
	
	if (f.addSemicolon)
		s << ";";

	s << "\n\n";
}

void CppGen::Emitter::emitCommentLine(String& code, int tabLevel, const String& comment)
{
	int numEquals = 80 - comment.length() - tabLevel * 4;

	while (--tabLevel >= 0)
		code << "    ";

	code << "// " << comment << " ";

	while (--numEquals >= 0)
		code << "=";

	code << "\n";
}



void CppGen::Emitter::emitClassEnd(String& s)
{
	NewLine nl;
	s << "};" << nl;
}



juce::String CppGen::Emitter::createLine(String& content)
{
	return content + ";\n";
}

juce::String CppGen::Emitter::createDefinition(String& name, String& assignment)
{
	String s;
	s << "auto " << name << " = " << assignment;
	return createLine(s);
}

juce::String CppGen::Emitter::createRangeString(NormalisableRange<double> range)
{
	String s;
	s << "{ " << createPrettyNumber(range.start, false);
	s << ", " << createPrettyNumber(range.end, false);
	s << ", " << createPrettyNumber(range.interval, false);
	s << ", " << createPrettyNumber(range.skew, false);
	s << " }";
	return s;
}

juce::String CppGen::Emitter::surroundWithBrackets(const String& s)
{
	String b;
	b << "{\n" << s << "}\n";
	return b;
}

juce::String CppGen::Emitter::createClass(const String& content, const String& classId, bool outerClass)
{
	String s;

	s << "\nstruct " << classId;

	if (outerClass)
		s << ": public HiseDspBase, public HardcodedNode\n";
	else
		s << "_\n";

	s << "{\n";
	s << content;
	s << "}";

	if (outerClass)
		s << ";\n";
	else
		s << " " << classId << ";\n\n";

	if (outerClass)
		return Helpers::createIntendation(s);
	else
		return s;
}

juce::String CppGen::Emitter::createPrettyNumber(double value, bool createFloat)
{
	if (fmod(value, 1.0) == 0.0)
	{
		return String((int)value) + ".0" + (createFloat ? "f" : "");
	}
	else
	{
		return String(value) + (createFloat ? "f" : "");
	}
}

juce::String CppGen::Helpers::createIntendation(const String& code)
{
	StringArray lines = StringArray::fromLines(code);

	NewLine nl;
	int tabLevel = 0;

	String newCode;

	auto addTab = [](String& c, int level)
	{
		while (--level >= 0)
			c << "    ";
	};

	bool dontIncrementNextLine = false;

	for (auto line : lines)
	{
		if (line.trim().startsWith("}"))
			tabLevel--;

		if (!line.startsWith("private:") && !line.startsWith("public:"))
			addTab(newCode, tabLevel);

		newCode << line << nl;

		if (line.trim().startsWith("{") &&
			!line.contains("}") &&
			!dontIncrementNextLine)
			tabLevel++;

		dontIncrementNextLine = line.startsWith("namespace");
	}

	return newCode;
}


}