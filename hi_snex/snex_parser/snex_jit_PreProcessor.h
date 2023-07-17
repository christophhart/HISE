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

using String = juce::String;



#define HNODE_PREPROCESSOR_KEYWORDS(X) \
    

namespace PreprocessorTokens
{
#define DECLARE_HNODE_JIT_TOKEN(name, str)  static const char* const name = str;

DECLARE_HNODE_JIT_TOKEN(if_, "#if")      
DECLARE_HNODE_JIT_TOKEN(endif_, "#endif") 
DECLARE_HNODE_JIT_TOKEN(elif_, "#elif")		
DECLARE_HNODE_JIT_TOKEN(define_, "#define")  
DECLARE_HNODE_JIT_TOKEN(undef_, "#undef") 
DECLARE_HNODE_JIT_TOKEN(include_, "#include") 
DECLARE_HNODE_JIT_TOKEN(else_, "#else")
DECLARE_HNODE_JIT_TOKEN(eof, "$eof")
DECLARE_HNODE_JIT_TOKEN(literal, "$literal")
DECLARE_HNODE_JIT_TOKEN(identifier, "$identifier")
DECLARE_HNODE_JIT_TOKEN(error_, "#error")
DECLARE_HNODE_JIT_TOKEN(code_, "code")
DECLARE_HNODE_JIT_TOKEN(on_, "#on");
}

struct Preprocessor
{
	using Token = const char*;

	Preprocessor(const String& code_);

	juce::String process();
	juce::String getOriginalCode() const { return code; }

	String processWithResult(ExternalPreprocessorDefinition::List& definitions);

	SparseSet<int> getDeactivatedLines();

	struct AutocompleteData
	{
		explicit operator bool() const
		{
			return codeToInsert.isNotEmpty();
		}

		String name;
		String description;
		String codeToInsert;
		int lineNumber;
	};

	Array<AutocompleteData> getAutocompleteData() const;

	void addDefinitionsFromScope(const ExternalPreprocessorDefinition::List& a);

	Result getResult() const { return r; }

	void setCurrentFileName(String newFileName) { currentFileName = newFileName; }

private:

	String currentFileName;

	Result r;
	
	void addNewDefinitions(ExternalPreprocessorDefinition::List& a);

	bool conditionMode = false;

	SparseSet<int> deactivatedLines;

	struct Item : public ReferenceCountedObject
	{
		using List = ReferenceCountedArray<Item>;
		using Ptr = ReferenceCountedObjectPtr<Item>;

		virtual ~Item() {};

		NamespacedIdentifier id;
		String body;
		String description;

		virtual AutocompleteData getAutocompleteData() const
		{
			AutocompleteData ad;
			ad.description = description.isEmpty() ? body : description;
			ad.codeToInsert = "";
			ad.name = id.toString();
			ad.lineNumber = lineNumber;
			return ad;
		}

		int lineNumber;
		int charNumber;
		bool externalDef = false;
	};

	struct Definition : public Item
	{
		bool evaluate(String& b, Result& r)
		{
			jassertfalse;
            return false;
		}

		AutocompleteData getAutocompleteData() const
		{
			auto ad = Item::getAutocompleteData();
			ad.codeToInsert = id.toString();

			ad.description = description;
			ad.description << (!description.isEmpty() ? "  \n" : "");
			ad.description << "Expands to\n> `" << body << "`";
			return ad;
		}
	};

	struct Macro: public Item
	{
		Macro(const Array<Identifier>& arguments_):
			arguments(arguments_)
		{}

		Array<Identifier> arguments;
	
		AutocompleteData getAutocompleteData() const
		{
			auto ad = Item::getAutocompleteData();
			ad.codeToInsert << id.toString() << "(";

			for (auto a : arguments)
			{
				ad.codeToInsert << a.toString();

				if (arguments.getLast() != a)
					ad.codeToInsert << ", ";
			}

			ad.codeToInsert << ")";
			ad.name = ad.codeToInsert;

			ad.description = description;
			ad.description << (!description.isEmpty() ? "  \n" : "");
			ad.description << "Expands to:  \n> `" + body + "`";

			return ad;
		}

		String evaluate(StringArray& parameters, Result& r);
	};

	template <class T> static T* as(Item::Ptr p)
	{
		return dynamic_cast<T*>(p.get());
	}

	struct TextBlock;
	
	using TextBlockList = OwnedArray<TextBlock>;

	static juce::String toString(const TextBlockList& blocks);

	TextBlockList parseTextBlocks();

	void parseDefinition(TextBlock& b);

	bool parseCondition(TextBlock& b);

	bool hasDefinitions() const { return !entries.isEmpty(); }

	static bool isConditionToken(const TextBlock& b);

	bool evaluate(TextBlock& b);

	Item::List entries;

	String code;
};


}
}
