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


struct ColourParser
{
	static Colour getColourFromHardcodedString(const String& colourId);

	ColourParser(const String& value);

	Colour getColour() const { return c; }

private:

	Colour c;
};

struct ColourGradientParser
{
	ColourGradientParser(Rectangle<float> area, const String& items);

	ColourGradient getGradient() const { return gradient; }

private:

	ColourGradient gradient;
};

struct TransformParser
{
	struct TransformData
	{
		explicit TransformData(TransformTypes t);

		TransformTypes type = TransformTypes::numTransformTypes;
		std::array<float, 2> values = {0.0f, 0.0f};
		int numValues = 0;

		TransformData interpolate(const TransformData& other, float alpha) const;

		static AffineTransform toTransform(const std::vector<TransformData>& list, Point<float> center);
	};

	TransformParser(const String& stackedTransforms);
	std::vector<TransformData> parse(Rectangle<float> totalArea, float defaultSize=16.0);

private:

	String t;
};

struct ShadowParser
{
	ShadowParser(const std::vector<String>& tokens);
	ShadowParser(const String& alreadyParsedString, Rectangle<float> totalArea);
	ShadowParser() = default;

	std::vector<melatonin::ShadowParameters> getShadowParameters(bool wantsInset) const;
	std::vector<melatonin::ShadowParameters> interpolate(const ShadowParser& other, double alpha, int wantsInset) const;

	String toParsedString() const;
	
private:

	static bool shouldFlushBefore(String& token);
	static bool shouldFlushAfter(String& token);
	int getRadius(int index) const noexcept { return data[index].size[2]; }
	int getSpread(int index) const noexcept { return data[index].size[3]; }
	Point<int> getOffset(int index) const noexcept { return { data[index].size[0], data[index].size[1] }; }
	Colour getColour(int index) const noexcept { return data[index].c; }
	static int parseSize(const String& s, Rectangle<float> totalArea);

	struct Data
	{
		operator bool() const { return somethingSet; }

		melatonin::ShadowParameters toShadowParameter() const;

		Data interpolate(const Data& other, double alpha) const;

		Data copyWithoutStrings(float alpha) const
		{
			Data copy;
			copy.somethingSet = somethingSet;
			copy.inset = inset;
			memcpy(copy.size, size, sizeof(int)*4);
			copy.c = c.withMultipliedAlpha(alpha);
			return copy;
		}

		bool somethingSet = false;
		bool inset = false;
		StringArray positions;
		int size[4];
		Colour c;
	};

	std::vector<Data> data;
	
};

struct ExpressionParser
{
	struct Context
	{
		bool useWidth = false;
		Rectangle<float> fullArea = { 1.0f, 1.0f };
		float defaultFontSize = 16.0f;
	};

	static float evaluate(const String& expression, const Context& context);

private:

	struct Node
	{
		using List = std::vector<Node>;
		using Ptr = ReferenceCountedObjectPtr<Node>;

		ExpressionType type = ExpressionType::none;
		juce_wchar op = 0;
		String s;
		List children;

		float evaluate(const Context& context) const;
	};

	static float evaluateLiteral(const String& s, const Context& context);
	static void match(String::CharPointerType& ptr, const String::CharPointerType end, juce_wchar t);
	static void skipWhitespace(String::CharPointerType& ptr, const String::CharPointerType end);
	static Node parseNode(String::CharPointerType& ptr, const String::CharPointerType end);
	
};

struct Parser
{
	Parser(const String& cssCode);;

	Result parse();

	StyleSheet::Collection getCSSValues() const;

#if 0
	static float parseSize(const String& s, std::pair<bool, Rectangle<float>> fullArea, float defaultHeight=16.0)
	{
		ExpressionParser::Context context;
		context.fullArea = fullArea.second;
		context.defaultFontSize = defaultHeight;
		context.useWidth = fullArea.first;

		return ExpressionParser::evaluate(s, context);
		
	}
#endif

private:

	friend class ShadowParser;

	enum TokenType
	{
		EndOfFIle = 0,
		OpenBracket,
		CloseBracket,
		Keyword,
		Dot,
		Hash,
		Colon,
		Comma,
		Semicolon,
		OpenParen,
		Quote,
		CloseParen,
		ValueString,
		numTokenTypes
	};

	struct RawLine
	{
		String property;
		std::vector<String> items;
	};

	struct RawClass
	{
		std::vector<Selector::RawList> selectors;
		std::vector<RawLine> lines;
	};

	String currentToken;
	std::vector<RawClass> rawClasses;

	void skip();
	bool match(TokenType t);
	void throwError(const String& errorMessage);
	bool matchIf(TokenType t);
	String getLocation();

	PseudoState parsePseudoClass();
	RawClass parseSelectors();

	static ValueType findValueType(const String& value);
	static String getTokenName(TokenType t);
	static String processValue(const String& value, ValueType t=ValueType::Undefined);
	static String getTokenSuffix(PropertyType p, const String& keyword, String& token);
	static PropertyType getPropertyType(const String& p);
	static std::function<double(double)> parseTimingFunction(const String& t);

	String code;
	String::CharPointerType ptr, end;
};

	
} // namespace simple_css
} // namespace hise