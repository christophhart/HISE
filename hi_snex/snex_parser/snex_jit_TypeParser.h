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
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;



class TypeParser : public ParserHelpers::TokenIterator
{
public:

	TypeParser(TokenIterator& other_, NamespaceHandler& handler, const TemplateParameter::List& tp);;
	void matchType();
	bool matchIfType();

private:

	TokenIterator& other;

	Array<TemplateParameter> parseTemplateParameters();
	VariableStorage parseConstExpression(bool canBeTemplateParameter);
	bool parseNamespacedIdentifier();
	bool matchIfSimpleType();
	bool matchIfComplexType();
	ComplexType::Ptr parseComplexType(const juce::String& token);
	Types::ID matchTypeId();
	bool matchIfTypeInternal();
	void parseSubType();

	NamespacedIdentifier nId;
	NamespaceHandler& namespaceHandler;
	TemplateParameter::List previouslyParsedArguments;
};



/** Parses an expression to a type for a compiled code. */
struct ExpressionTypeParser : private ParserHelpers::TokenIterator
{
	ExpressionTypeParser(NamespaceHandler& n, const String& statement, int lineNumber_);

	TypeInfo parseType();

private:

	int lineNumber;

	TypeInfo parseDot(TypeInfo parent);
	TypeInfo parseSubscript(TypeInfo parent);
	TypeInfo parseCall(TypeInfo parent);

	NamespacedIdentifier currentId;
	NamespaceHandler& nh;
};


}
}