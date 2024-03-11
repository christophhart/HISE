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
		explicit TransformData(TransformTypes t):
		  type(t)
		{
			static constexpr float defaultValues[(int)TransformTypes::numTransformTypes] =
			{
				0.0f, // none
				0.0f, // matrix(n,n,n,n,n,n)
				0.0f, // translate(x,y)
				0.0f, // translateX(x)
				0.0f, // translateY(y)
				0.0f, // translateZ(z)
				1.0f, // scale(x,y)
				1.0f, // scaleX(x)
				1.0f, // scaleY(y)
				1.0f, // scaleZ(z)
				0.0f, // rotate(angle)
				0.0f, // rotateX(angle)
				0.0f, // rotateY(angle)
				0.0f, // rotateZ(angle)
				1.0f, // skew(x-angle,y-angle)
				1.0f, // skewX(angle)
				1.0f, // skewY(angle)
			};

			values[0] = defaultValues[(int)t];
			values[1] = values[0];
		}

		TransformTypes type = TransformTypes::numTransformTypes;
		std::array<float, 2> values = {0.0f, 0.0f};
		int numValues = 0;

		TransformData interpolate(const TransformData& other, float alpha) const;

		static AffineTransform toTransform(const std::vector<TransformData>& list, Point<float> center)
		{
			AffineTransform t;

			if(!list.empty() && !center.isOrigin())
			{
				t = AffineTransform::translation(center * -1.0f);
			}

			for(const auto& d: list)
			{
				auto firstValue = d.values[0];
				auto secondValue = d.values[(int)(d.numValues == 2)];

				switch(d.type)
				{
				case TransformTypes::none: break;
				case TransformTypes::matrix: break;
				case TransformTypes::translate: ;
				case TransformTypes::translateX: ;
				case TransformTypes::translateY: ;
				case TransformTypes::translateZ: t = t.followedBy(AffineTransform::translation(firstValue, secondValue)); break;
				case TransformTypes::scale: ;
				case TransformTypes::scaleX: ;
				case TransformTypes::scaleY: ;
				case TransformTypes::scaleZ: t = t.followedBy(AffineTransform::scale(firstValue, secondValue)); break;
				case TransformTypes::rotate: ;
				case TransformTypes::rotateX: ;
				case TransformTypes::rotateY: ;
				case TransformTypes::rotateZ: t = t.followedBy(AffineTransform::rotation(firstValue)); break;
				case TransformTypes::skew: ;
				case TransformTypes::skewX: ;
				case TransformTypes::skewY: t = t.followedBy(AffineTransform::shear(firstValue, secondValue)); break;
				case TransformTypes::numTransformTypes: break;
				default: ;
				}
			}

			if(!list.empty() && !center.isOrigin())
			{
				t = t.followedBy(AffineTransform::translation(center));
			}

			return t;
		}
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

		bool somethingSet = false;
		bool inset = false;
		StringArray positions;
		int size[4];
		Colour c;
	};

	std::vector<Data> data;
	
};

struct Parser
{
	Parser(const String& cssCode);;

	Result parse();

	StyleSheet::Collection getCSSValues() const;

	static float parseSize(const String& s, float fullSize, float defaultHeight=16.0)
	{
		if(s.endsWithChar('x'))
			return s.getFloatValue();
		if(s.endsWithChar('%'))
			return fullSize * s.getFloatValue() * 0.01f;
		if(s.endsWith("em"))
			return s.getFloatValue() * defaultHeight;
		if(s.endsWith("deg"))
			return s.getFloatValue() / 180.0f * float_Pi;

		return 0;
	}

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
		Selector::RawList selector;
		std::vector<RawLine> lines;
	};

	String currentToken;
	std::vector<RawClass> rawClasses;

	void skip();
	bool match(TokenType t);
	void throwError(const String& errorMessage);
	bool matchIf(TokenType t);
	String getLocation();

	int parsePseudoClass();
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