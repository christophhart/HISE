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

/** Parses colour values. Pretty feature complete except for weird color space syntax. */
struct ColourParser
{
	static std::pair<bool, Colour> getColourFromHardcodedString(const String& colourId);

	ColourParser(const String& value);

	Colour getColour() const { return c; }

	String toCodeGeneratorString() const
	{
		String s = "Colour(";
		s << String::toHexString(c.getARGB()) << ")";
		return s;
	}

private:

	Colour c;
};

/** Parses a colour gradient using a list of value items and an area. */
struct ColourGradientParser
{
	ColourGradientParser(Rectangle<float> area, const String& items);

	ColourGradient getGradient() const;
	static String toString(Rectangle<float> area, const ColourGradient& second);

private:

	ColourGradient gradient;
};

/** Parses a affine transform that can be applied to the graphics context. */
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
		static String toCppCode(const std::vector<TransformData>& list, const String& pointName);

		String toString() const
		{
			String m;

			switch(type)
			{
			case TransformTypes::none: return "none";
			case TransformTypes::matrix: return "none";
			case TransformTypes::translate: 
			case TransformTypes::translateX: 
			case TransformTypes::translateY: 
			case TransformTypes::translateZ: m << "translate"; break;
			case TransformTypes::scale: 
			case TransformTypes::scaleX: 
			case TransformTypes::scaleY: 
			case TransformTypes::scaleZ: m << "scale"; break;
			case TransformTypes::rotate: 
			case TransformTypes::rotateX: 
			case TransformTypes::rotateY: 
			case TransformTypes::rotateZ: m << "rotate"; break;
			case TransformTypes::skew: 
			case TransformTypes::skewX: 
			case TransformTypes::skewY: m << "skew"; break;
			case TransformTypes::numTransformTypes: break;
			default: ;
			}

			m << '(';
			m << String(values[0]);

			if(numValues > 1)
			{
				m << ',';
				m << String(values[1]);
			}
			
			m << ')';

			return m; 
		}
	};

	TransformParser(KeywordDataBase* database_, const String& stackedTransforms);
	std::vector<TransformData> parse(Rectangle<float> totalArea, float defaultSize=16.0);

	static String toString(const std::vector<TransformData>& list)
	{
		String s;

		for(const auto& d: list)
		{
			s << d.toString() << "\n";
		}

		return s;
	}

private:

	KeywordDataBase* database;

	String t;
};

/** Parses box-shadow and text-shadow properties to create a melatonin::DropShadow stack. */
class ShadowParser
{
public:

	ShadowParser(const std::vector<String>& tokens);
	ShadowParser(const String& alreadyParsedString, Rectangle<float> totalArea);
	ShadowParser() = default;

	using ShadowList = std::vector<melatonin::ShadowParameters>;

	ShadowList getShadowParameters(bool wantsInset) const;
	ShadowList interpolate(const ShadowParser& other, double alpha, int wantsInset) const;

	static ShadowList interpolateShadowList(const ShadowList& l1, const ShadowList& l2, float alpha)
	{
		ShadowList list;

		if(l1.size() == l2.size())
		{
			for(int i = 0; i < l1.size(); i++)
			{
				auto d1 = Data::fromShadowParameters(l1[i]);
				auto d2 = Data::fromShadowParameters(l2[i]);

				auto mix = d1.interpolate(d2, (double)alpha);
				list.push_back(mix.toShadowParameter());
			}
		}

		return list;
	}

	static String shadowListToString(const ShadowList& l1)
	{
		ShadowParser sd;

		for(auto& l: l1)
		{
			auto nd = Data::fromShadowParameters(l);

			nd.positions.add(String(nd.size[0]));
			nd.positions.add(String(nd.size[1]));
			nd.positions.add(String(nd.size[2]));
			nd.positions.add(String(nd.size[3]));

			sd.data.push_back(nd);
		}

		return sd.toParsedString();
	}

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

		static Data fromShadowParameters(const melatonin::ShadowParameters& p)
		{
			Data nd;

			nd.inset = p.inner;
			nd.c = p.color;
			nd.size[2] = p.radius;
			nd.size[0] = p.offset.getX();
			nd.size[1] = p.offset.getY();
			nd.size[3] = p.spread;
			
			return nd;
		}

		Data interpolate(const Data& other, double alpha) const;

		Data copyWithoutStrings(float alpha) const;

		bool somethingSet = false;
		bool inset = false;
		StringArray positions;
		int size[4];
		Colour c;
	};

	std::vector<Data> data;
	
};

/** Parses a CSS expression that performs some very simple math formula calculations. */
struct ExpressionParser
{
	template <typename SourceType=Rectangle<float>> struct Context
	{
		static constexpr bool isCodeGenContext() { return std::is_same<SourceType, juce::String>(); }

		static constexpr SourceType getDefaultSource()
		{
			if constexpr (isCodeGenContext())
				return String();
			else
				return Rectangle<float>(1.0f, 1.0f);
		}
		
		bool useWidth = false;
		SourceType fullArea = Context<SourceType>::getDefaultSource();
		float defaultFontSize = 16.0f;
	};

	static String evaluateToCodeGeneratorLiteral(const String& expression, const Context<String>& context);

	static float evaluate(const String& expression, const Context<>& context);

private:

	struct Node
	{
		using List = std::vector<Node>;
		using Ptr = ReferenceCountedObjectPtr<Node>;

		ExpressionType type = ExpressionType::none;
		juce_wchar op = 0;
		String s;
		List children;

		String evaluateToCodeGeneratorLiteral(const Context<String>& context) const;

		float evaluate(const Context<>& context) const;
	};

	static float evaluateLiteral(const String& s, const Context<>& context);
	static void match(String::CharPointerType& ptr, const String::CharPointerType end, juce_wchar t);
	static void skipWhitespace(String::CharPointerType& ptr, const String::CharPointerType end);
	static Node parseNode(String::CharPointerType& ptr, const String::CharPointerType end);
	
};

/** Parses a string containing CSS code. */
class Parser
{
public:

	Parser(const String& cssCode);;

	/** This will parse the string and return a Result indicating the errors. */
	Result parse();

	/** After you've parsed the string you can query this function to get all the warnings that the CSS code
	 *  produced. It contains missing properties and invalid selectors. */
	StringArray getWarnings() const { return warnings; }

	/** returns a list of StyleSheet objects that can be used to style the appearance of your JUCE components. */
	StyleSheet::Collection getCSSValues() const;

	Array<Selector> getSelectors() const;

private:

	String getLocation(String::CharPointerType p=String::CharPointerType(nullptr)) const;

	struct KeywordWarning
	{
		KeywordWarning(Parser& parent_);

		void setLocation(Parser& p);
		void check(const String& s, KeywordDataBase::KeywordType type);

		SharedResourcePointer<KeywordDataBase> database;
		String::CharPointerType currentLocation;
		Parser& parent;
	};

	friend class ShadowParser;

	enum TokenType
	{
		EndOfFIle = 0,
		OpenBracket,
		CloseBracket,
		At,
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
		Asterisk,
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

	void performAtRule(StyleSheet::List& list, StyleSheet::Ptr rule);

	String currentToken;
	std::vector<RawClass> rawClasses;

	void skip();
	bool match(TokenType t);
	void throwError(const String& errorMessage);
	bool matchIf(TokenType t);
	
	PseudoState parsePseudoClass();
	RawClass parseSelectors();
	
	static ValueType findValueType(const String& value);
	static String getTokenName(TokenType t);
	static String processValue(const String& value, ValueType t=ValueType::Undefined);
	static String getTokenSuffix(PropertyType p, const String& keyword, String& token);
	static bool hasVariable(const String& value) { return value.indexOf("var(--") != -1; }
	static PropertyType getPropertyType(const String& p);
	static std::function<double(double)> parseTimingFunction(const String& t);

	String code;
	String::CharPointerType ptr, end;

	StringArray warnings;
};

	
} // namespace simple_css
} // namespace hise