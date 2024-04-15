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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise {
namespace simple_css
{
using namespace juce;

/** A common database object containing all supported properties. */
struct KeywordDataBase
{
	enum class KeywordType
	{
		Type,
		Property,
		PseudoClass,
		AtRules,
		ReservedKeywords,
		ExpressionKeywords,
		numKeywords,
		Undefined
	};

	static CodeEditorComponent::ColourScheme getColourScheme();

	KeywordDataBase();

	static String getKeywordName(KeywordType type);
	const StringArray& getValuesForProperty(const String& propertyId) const;

	const StringArray getInheritedProperties() const { return inheritedProperties; }

	template <typename EnumType=int> EnumType getAsEnum(const String& propertyId, const String& value, EnumType defaultValue) const
	{
		auto idx = getValuesForProperty(propertyId).indexOf(value);

		if(idx != -1)
			return static_cast<EnumType>(idx);

		return defaultValue;
	}

	/** Prints a list of all supported properties. */
	static void printReport();

	/** Contains all value strings that represent literal values or constants. */
	std::map<String, StringArray> valueNames;

	/** Contains all value strings that will evaluate some expression within a parenthesis. */
	std::map<String, StringArray> functNames;
	
	const StringArray& getKeywords(KeywordType type) const;
	std::array<StringArray, (int)KeywordType::numKeywords> keywords;
	KeywordType getKeywordType(const String& t);

	StringArray inheritedProperties;
};



struct LanguageManager: public mcl::LanguageManager
{
	struct Tokeniser: public CodeTokeniser
	{
		SharedResourcePointer<KeywordDataBase> database;

		enum class Token
		{
			Type,
			Properties,
			PseudoClass,
			AtRule,
			Keyword,
			Expression,
			Class,
			ID,
			SpecialCharacters,
			Value,
			Comment,
			Important,
			StringLiteral,
			numTokens
		};

		Tokeniser() = default;

		static void skipNumberValue(CodeDocument::Iterator& source);
		static void skipToSemicolon(CodeDocument::Iterator& source);
		static void skipComment(CodeDocument::Iterator& source);
		static void skipStringLiteral(CodeDocument::Iterator& source);

		static String skipWord(CodeDocument::Iterator& source);
		static bool isNumber(CodeDocument::Iterator& source);
		static bool isIdentifierStart(CodeDocument::Iterator& source);

		int readNextToken (CodeDocument::Iterator& source) override;

		/** Returns a suggested syntax highlighting colour scheme. */
	    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;
	};

	struct CssTokens: public mcl::TokenCollection::Provider
	{
		void addTokens(mcl::TokenCollection::List& tokens) override;
		KeywordDataBase database;
	};

	LanguageManager(mcl::TextDocument& doc_);
	
	void processBookmarkTitle(juce::String& bookmarkTitle) override {}
	void setupEditor(mcl::TextEditor* editor) override;
    void addTokenProviders(mcl::TokenCollection* t) override;
	Identifier getLanguageId() const override { return "CSS"; }
	CodeTokeniser* createCodeTokeniser() override { return new Tokeniser(); }

	SharedResourcePointer<KeywordDataBase> database;
	mcl::TextDocument& doc;
};

} // namespace simple_css
} // namespace hise
