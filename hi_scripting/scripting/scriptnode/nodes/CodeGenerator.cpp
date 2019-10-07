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





juce::String CppGen::Emitter::addNodeTemplateWrappers(String className, NodeBase* n)
{
	auto v_data = n->getValueTree();

	if (v_data[PropertyIds::DynamicBypass].toString().isNotEmpty())
		className = wrapIntoTemplate(className, "bypass::yes");
	else if (v_data[PropertyIds::Bypassed])
		className = wrapIntoTemplate(className, "skip");

	if (n->hasFixChannelAmount() || dynamic_cast<MultiChannelNode*>(n->getParentNode()) != nullptr)
	{
		auto templateArgs = String(n->getNumChannelsToProcess()) + ", " + className;

		className = wrapIntoTemplate(templateArgs, "fix");
	}
		
	

	return className;
}

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

	s << ")" << f.specifiers;
	
	if (f.body.isEmpty())
		s << " {}";
	else 
		s << "\n{\n" << f.body << "}";

	if(f.addSemicolon)
		s << ";";

	if (f.addNewLine)
	{
		if (f.body.isEmpty())
			s << "\n";
		else
			s << "\n\n";
	}
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

juce::String CppGen::Emitter::createFactoryMacro(bool shouldCreatePoly, bool isSnexClass)
{
	String s;

	if (isSnexClass)
	{
		if (shouldCreatePoly)
			s << "REGISTER_POLY_SNEX;\n";
		else
			s << "REGISTER_MONO_SNEX;\n";
	}
	else
	{
		if (shouldCreatePoly)
			s << "REGISTER_POLY;\n";
		else
			s << "REGISTER_MONO;\n";
	}

	return s;
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


juce::String CppGen::Emitter::createClass(const String& content, const String& templateId, bool createPolyphonicClass)
{
	String s;

	if (createPolyphonicClass)
		s << "template <int NV> ";

	s << "struct instance: public hardcoded<" << templateId << ">\n";
	s << "{\n";

	if (createPolyphonicClass)
		s << "static constexpr int NumVoices = NV;\n\n";

	s << content;

	s << "};\n";

	return s;

}

juce::String CppGen::Emitter::createJitClass(const String& , const String& content)
{
	String s;

	s << "struct instance: public jit_base\n";
	s << "{\n";

	s << content;

	s << "};\n";

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

juce::String CppGen::Emitter::getVarAsCode(const var& value)
{
	if (value.isBool())
		return (bool)value ? "true" : "false";

	if (value.isString())
		return "\"" + value.toString() + "\"";

	if (value.isInt() || value.isInt64())
		return String((int64)value);

	if (value.isDouble())
		return String((double)value);

	return "\"undefined\"";
}

juce::String CppGen::Emitter::createAlias(const String& aliasName, const String& className)
{
	String s;
	s << "using " << aliasName << " = " << className << ";\n";
	return s;
}

juce::String CppGen::Emitter::createTemplateAlias(const String& aliasName, const String& className, const StringArray& templateArguments)
{
	String s;
	s << "using " << aliasName << " = " << className << "<";

	for (int i = 0; i < templateArguments.size(); i++)
	{
		s << templateArguments[i];

		if (i != templateArguments.size() - 1)
			s << ", ";
	}

	s << ">;\n";

	return s;
}

juce::String CppGen::Emitter::wrapIntoTemplate(const String& className, const String& outerTemplate)
{
	String s;

	s << outerTemplate << "<" << className << ">";
	return s;
}

juce::String CppGen::Emitter::prependNamespaces(const String& className, const Array<Identifier>& namespaces)
{
	String s;
	for (auto id : namespaces)
		s << id.toString() << "::";

	s << className;

	return s;
}

juce::String CppGen::Emitter::wrapIntoNamespace(const String& s, const String& namespaceId)
{
	String n;
	n << "\nnamespace " << namespaceId << "\n{\n\n";

	n << s;

	n << "\n}\n\n";

	return n;
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
			tabLevel = jmax(0, tabLevel - 1);

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