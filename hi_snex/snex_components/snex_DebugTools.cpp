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
using namespace juce;


void debug::MathFunctionProvider::addTokens(mcl::TokenCollection::List& l)
{
	ComplexType::Ptr bt = new DynType(TypeInfo(Types::ID::Float));
	FunctionClass::Ptr fc = new snex::jit::MathFunctions(false, bt);

	Array<NamespacedIdentifier> functions;
	fc->getAllFunctionNames(functions);

	for (auto& f : functions)
	{
		Array<FunctionData> matches;
		fc->addMatchingFunctions(matches, f);

		for (auto m : matches)
		{
			l.add(new MathFunction(m));
		}
	}

	for (int i = 0; i < l.size(); i++)
	{
		if (l[i]->tokenContent.isEmpty())
		{
			l.remove(i--);
			continue;
		}

		for (int j = i + 1; j < l.size(); j++)
		{
			if (*l[j] == *l[i])
			{
				auto s = l[j]->tokenContent;
				l.remove(j--);
			}
		}
	}
}

debug::SymbolProvider::ComplexMemberToken::ComplexMemberToken(SymbolProvider& parent_, ComplexType::Ptr p_, FunctionData& f) :
	Token(f.getSignature()),
	parent(parent_),
	p(p_)
{
	f.getOrResolveReturnType(p);

	c = FourColourScheme::getColour(FourColourScheme::Method);
	tokenContent = f.getSignature({}, false);
	priority = 80;
	codeToInsert = f.getCodeToInsert();
	markdownDescription = f.description;
}

bool debug::SymbolProvider::ComplexMemberToken::matches(const String& input, const String& previousToken, int lineNumber) const
{
	if (auto st = dynamic_cast<SpanType*>(p.get()))
	{
		if (st->getElementSize() == 19)
			jassertfalse;
	}

	if (previousToken.endsWith("."))
	{
		try
		{
			auto typeInfo = parent.nh->parseTypeFromTextInput(previousToken.upToLastOccurrenceOf(".", false, false), lineNumber);

			auto codeToSearch = input.length() == 1 ? getCodeToInsert(input) : tokenContent;

			if (typeInfo.getTypedIfComplexType<ComplexType>() == p.get())
				return matchesInput(input, codeToSearch);
		}
		catch (ParserHelpers::Error& )
		{
			return false;
		}
	}

	return false;
}

void debug::PreprocessorMacroProvider::addTokens(mcl::TokenCollection::List& tokens)
{
	Preprocessor p(doc.getAllContent());

	p.addDefinitionsFromScope(GlobalScope::getDefaultDefinitions());

	try
	{
		p.process();
	}
	catch (ParserHelpers::Error& e)
	{
		DBG(e.toString());
		ignoreUnused(e);
	}

	for (auto ad : p.getAutocompleteData())
	{
		tokens.add(new PreprocessorToken(ad.name, ad.codeToInsert, ad.description, ad.lineNumber));
	}
}

void debug::SymbolProvider::addTokens(mcl::TokenCollection::List& tokens)
{
	ApiDatabase::Instance db;
	c.reset();
	Types::SnexObjectDatabase::registerObjects(c, 2);

	c.compileJitObject(doc.getAllContent());

	auto ct = c.getNamespaceHandler().getComplexTypeList();

	nh = c.getNamespaceHandlerReference();

	for (auto c : ct)
	{
		if (FunctionClass::Ptr fc = c->getFunctionClass())
		{
			TokenCollection::List l;

			for (auto id : fc->getFunctionIds())
			{
				Array<FunctionData> fData;
				fc->addMatchingFunctions(fData, id);

				for (auto& f : fData)
					l.add(new ComplexMemberToken(*this, c, f));
			}

			if (auto st = dynamic_cast<StructType*>(c))
			{
				for (auto ni : l)
					db->addDocumentation(ni, st->id, ni->getCodeToInsert(""));
			}

			tokens.addArray(l);
		}
	}

	auto l2 = c.getNamespaceHandler().getTokenList();

	for (auto ni : l2)
		db->addDocumentation(ni, NamespacedIdentifier(ni->tokenContent), {});

	tokens.addArray(l2);
}

void debug::TemplateProvider::addTokens(mcl::TokenCollection::List& tokens)
{
	GlobalScope s;
	Compiler c(s);
	Types::SnexObjectDatabase::registerObjects(c, 2);

	ApiDatabase::Instance db;

	for (auto id : c.getNamespaceHandler().getTemplateClassTypes())
	{
		auto nt = new TemplateToken(id);
		tokens.add(nt);

		db->addDocumentation(nt, id.id.id, {});	
	}
}

mcl::FoldableLineRange::List debug::SnexLanguageManager::createLineRange(const CodeDocument& doc)
{
	auto lineRanges = LanguageManager::createLineRange(doc);

	snex::Preprocessor p(doc.getAllContent());

	auto s = p.getDeactivatedLines();

	for (int i = 0; i < s.getNumRanges(); i++)
	{
		auto r = s.getRange(i);
		lineRanges.add(new mcl::FoldableLineRange(doc, { r.getStart() - 2, r.getEnd() }));
	}

	return lineRanges;
}

}
