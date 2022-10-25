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


namespace NamespaceResolver
{
	struct CanExist
	{
		static void resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l);
	};

	struct MustExist
	{
		static void resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l);
	};

	struct MustBeNew
	{
		static void resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l);
	};
}


class SymbolParser : public ParserHelpers::TokenIterator
{
public:

	SymbolParser(ParserHelpers::TokenIterator& other_, NamespaceHandler& handler_);;

	Symbol parseExistingSymbol(bool needsStaticTyping);
	Symbol parseNewDynamicSymbolSymbol(NamespaceHandler::SymbolType t);
	Symbol parseNewSymbol(NamespaceHandler::SymbolType t);

	template <class T> void parseNamespacedIdentifier()
	{
		auto c = handler.getCurrentNamespaceIdentifier();
		auto symbolLoc = location;
		auto id = parseIdentifier();
		auto isExplicit = currentType == JitTokens::double_colon;

		if (isExplicit)
		{
			// reset the namespace or it will stack up...
			c = NamespacedIdentifier();
		}

		c = c.getChildId(id);

		while (matchIf(JitTokens::double_colon))
			c = c.getChildId(parseIdentifier());

		T::resolve(handler, c, symbolLoc);
		currentNamespacedIdentifier = c;

		other.seek(*this);
	}

	NamespacedIdentifier currentNamespacedIdentifier;

private:

	TokenIterator& other;
	NamespaceHandler& handler;
};

}
}