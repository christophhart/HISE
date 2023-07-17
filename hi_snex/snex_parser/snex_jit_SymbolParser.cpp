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
USE_ASMJIT_NAMESPACE;

void NamespaceResolver::CanExist::resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l)
{
	auto r = n.resolve(c, true);

	if (!r.wasOk())
		l.throwError(r.getErrorMessage());
}



SymbolParser::SymbolParser(ParserHelpers::TokenIterator& other_, NamespaceHandler& handler_) :
	ParserHelpers::TokenIterator(other_),
	handler(handler_),
	other(other_)
{

}

snex::jit::Symbol SymbolParser::parseExistingSymbol(bool needsStaticTyping)
{
	parseNamespacedIdentifier<NamespaceResolver::MustExist>();

	auto type = handler.getVariableType(currentNamespacedIdentifier);

	location.test(handler.checkVisiblity(currentNamespacedIdentifier));

	auto s = Symbol(currentNamespacedIdentifier, type);

	if (needsStaticTyping && s.typeInfo.isDynamic())
		location.throwError("Can't resolve symbol type");

	return s;
}

snex::jit::Symbol SymbolParser::parseNewDynamicSymbolSymbol(NamespaceHandler::SymbolType t)
{
	jassert(other.currentTypeInfo.isDynamic());
	parseNamespacedIdentifier<NamespaceResolver::MustBeNew>();
	auto s = Symbol(currentNamespacedIdentifier, other.currentTypeInfo);
	return s;
}

snex::jit::Symbol SymbolParser::parseNewSymbol(NamespaceHandler::SymbolType t)
{
	auto type = other.currentTypeInfo;

	parseNamespacedIdentifier<NamespaceResolver::MustBeNew>();

	auto s = Symbol(currentNamespacedIdentifier, type);

	BlockParser::CommentAttacher ca(*this);

	if (t != NamespaceHandler::Unknown)
		handler.addSymbol(s.id, type, t, ca.getInfo());

#if 0
	if (s.typeInfo.isDynamic() && t != NamespaceHandler::UsingAlias)
		location.throwError("Can't resolve symbol type");
#endif

	return s;
}

void NamespaceResolver::MustExist::resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l)
{
	auto r = n.resolve(c);

	if (!r.wasOk())
	{
		l.throwError(r.getErrorMessage());
	}
		
}

void NamespaceResolver::MustBeNew::resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l)
{

}

}
}