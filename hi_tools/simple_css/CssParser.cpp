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

namespace hise
{
namespace simple_css
{

std::pair<bool, Colour> ColourParser::getColourFromHardcodedString(const String& colourId)
{
	struct NamedColour
	{
		String name;
		uint32 colour;
	};

	std::initializer_list<NamedColour> colours = {{ "black", 0xFF000000  },
		{ "silver", 0xFFc0c0c0  },
		{ "gray", 0xFF808080  },
		{ "white", 0xFFffffff  },
		{ "maroon", 0xFF800000  },
		{ "red", 0xFFff0000  },
		{ "purple", 0xFF800080  },
		{ "fuchsia", 0xFFff00ff  },
		{ "green", 0xFF008000  },
		{ "lime", 0xFF00ff00  },
		{ "olive", 0xFF808000  },
		{ "yellow", 0xFFffff00  },
		{ "navy", 0xFF000080  },
		{ "blue", 0xFF0000ff  },
		{ "teal", 0xFF008080  },
		{ "aqua", 0xFF00ffff  },
		{ "aliceblue", 0xFFf0f8ff  },
		{ "antiquewhite", 0xFFfaebd7  },
		{ "aqua", 0xFF00ffff  },
		{ "aquamarine", 0xFF7fffd4  },
		{ "azure", 0xFFf0ffff  },
		{ "beige", 0xFFf5f5dc  },
		{ "bisque", 0xFFffe4c4  },
		{ "black", 0xFF000000  },
		{ "blanchedalmond", 0xFFffebcd  },
		{ "blue", 0xFF0000ff  },
		{ "blueviolet", 0xFF8a2be2  },
		{ "brown", 0xFFa52a2a  },
		{ "burlywood", 0xFFdeb887  },
		{ "cadetblue", 0xFF5f9ea0  },
		{ "chartreuse", 0xFF7fff00  },
		{ "chocolate", 0xFFd2691e  },
		{ "coral", 0xFFff7f50  },
		{ "cornflowerblue", 0xFF6495ed  },
		{ "cornsilk", 0xFFfff8dc  },
		{ "crimson", 0xFFdc143c  },
		{ "cyan", 0xFF00ffff  },
		{ "darkblue", 0xFF00008b  },
		{ "darkcyan", 0xFF008b8b  },
		{ "darkgoldenrod", 0xFFb8860b  },
		{ "darkgray", 0xFFa9a9a9  },
		{ "darkgreen", 0xFF006400  },
		{ "darkgrey", 0xFFa9a9a9  },
		{ "darkkhaki", 0xFFbdb76b  },
		{ "darkmagenta", 0xFF8b008b  },
		{ "darkolivegreen", 0xFF556b2f  },
		{ "darkorange", 0xFFff8c00  },
		{ "darkorchid", 0xFF9932cc  },
		{ "darkred", 0xFF8b0000  },
		{ "darksalmon", 0xFFe9967a  },
		{ "darkseagreen", 0xFF8fbc8f  },
		{ "darkslateblue", 0xFF483d8b  },
		{ "darkslategray", 0xFF2f4f4f  },
		{ "darkslategrey", 0xFF2f4f4f  },
		{ "darkturquoise", 0xFF00ced1  },
		{ "darkviolet", 0xFF9400d3  },
		{ "deeppink", 0xFFff1493  },
		{ "deepskyblue", 0xFF00bfff  },
		{ "dimgray", 0xFF696969  },
		{ "dimgrey", 0xFF696969  },
		{ "dodgerblue", 0xFF1e90ff  },
		{ "firebrick", 0xFFb22222  },
		{ "floralwhite", 0xFFfffaf0  },
		{ "forestgreen", 0xFF228b22  },
		{ "fuchsia", 0xFFff00ff  },
		{ "gainsboro", 0xFFdcdcdc  },
		{ "ghostwhite", 0xFFf8f8ff  },
		{ "gold", 0xFFffd700  },
		{ "goldenrod", 0xFFdaa520  },
		{ "gray", 0xFF808080  },
		{ "green", 0xFF008000  },
		{ "greenyellow", 0xFFadff2f  },
		{ "grey", 0xFF808080  },
		{ "honeydew", 0xFFf0fff0  },
		{ "hotpink", 0xFFff69b4  },
		{ "indianred", 0xFFcd5c5c  },
		{ "indigo", 0xFF4b0082  },
		{ "ivory", 0xFFfffff0  },
		{ "khaki", 0xFFf0e68c  },
		{ "lavender", 0xFFe6e6fa  },
		{ "lavenderblush", 0xFFfff0f5  },
		{ "lawngreen", 0xFF7cfc00  },
		{ "lemonchiffon", 0xFFfffacd  },
		{ "lightblue", 0xFFadd8e6  },
		{ "lightcoral", 0xFFf08080  },
		{ "lightcyan", 0xFFe0ffff  },
		{ "lightgoldenrodyellow", 0xFFfafad2  },
		{ "lightgray", 0xFFd3d3d3  },
		{ "lightgreen", 0xFF90ee90  },
		{ "lightgrey", 0xFFd3d3d3  },
		{ "lightpink", 0xFFffb6c1  },
		{ "lightsalmon", 0xFFffa07a  },
		{ "lightseagreen", 0xFF20b2aa  },
		{ "lightskyblue", 0xFF87cefa  },
		{ "lightslategray", 0xFF778899  },
		{ "lightslategrey", 0xFF778899  },
		{ "lightsteelblue", 0xFFb0c4de  },
		{ "lightyellow", 0xFFffffe0  },
		{ "lime", 0xFF00ff00  },
		{ "limegreen", 0xFF32cd32  },
		{ "linen", 0xFFfaf0e6  },
		{ "magenta", 0xFFff00ff  },
		{ "maroon", 0xFF800000  },
		{ "mediumaquamarine", 0xFF66cdaa  },
		{ "mediumblue", 0xFF0000cd  },
		{ "mediumorchid", 0xFFba55d3  },
		{ "mediumpurple", 0xFF9370db  },
		{ "mediumseagreen", 0xFF3cb371  },
		{ "mediumslateblue", 0xFF7b68ee  },
		{ "mediumspringgreen", 0xFF00fa9a  },
		{ "mediumturquoise", 0xFF48d1cc  },
		{ "mediumvioletred", 0xFFc71585  },
		{ "midnightblue", 0xFF191970  },
		{ "mintcream", 0xFFf5fffa  },
		{ "mistyrose", 0xFFffe4e1  },
		{ "moccasin", 0xFFffe4b5  },
		{ "navajowhite", 0xFFffdead  },
		{ "navy", 0xFF000080  },
		{ "oldlace", 0xFFfdf5e6  },
		{ "olive", 0xFF808000  },
		{ "olivedrab", 0xFF6b8e23  },
		{ "orange", 0xFFffa500  },
		{ "orangered", 0xFFff4500  },
		{ "orchid", 0xFFda70d6  },
		{ "palegoldenrod", 0xFFeee8aa  },
		{ "palegreen", 0xFF98fb98  },
		{ "paleturquoise", 0xFFafeeee  },
		{ "palevioletred", 0xFFdb7093  },
		{ "papayawhip", 0xFFffefd5  },
		{ "peachpuff", 0xFFffdab9  },
		{ "peru", 0xFFcd853f  },
		{ "pink", 0xFFffc0cb  },
		{ "plum", 0xFFdda0dd  },
		{ "powderblue", 0xFFb0e0e6  },
		{ "purple", 0xFF800080  },
		{ "rebeccapurple", 0xFF663399  },
		{ "red", 0xFFff0000  },
		{ "rosybrown", 0xFFbc8f8f  },
		{ "royalblue", 0xFF4169e1  },
		{ "saddlebrown", 0xFF8b4513  },
		{ "salmon", 0xFFfa8072  },
		{ "sandybrown", 0xFFf4a460  },
		{ "seagreen", 0xFF2e8b57  },
		{ "seashell", 0xFFfff5ee  },
		{ "sienna", 0xFFa0522d  },
		{ "silver", 0xFFc0c0c0  },
		{ "skyblue", 0xFF87ceeb  },
		{ "slateblue", 0xFF6a5acd  },
		{ "slategray", 0xFF708090  },
		{ "slategrey", 0xFF708090  },
		{ "snow", 0xFFfffafa  },
		{ "springgreen", 0xFF00ff7f  },
		{ "steelblue", 0xFF4682b4  },
		{ "transparent", 0x00000000 },
		{ "tan", 0xFFd2b48c  },
		{ "teal", 0xFF008080  },
		{ "thistle", 0xFFd8bfd8  },
		{ "tomato", 0xFFff6347  },
		{ "turquoise", 0xFF40e0d0  },
		{ "violet", 0xFFee82ee  },
		{ "wheat", 0xFFf5deb3  },
		{ "white", 0xFFffffff  },
		{ "whitesmoke", 0xFFf5f5f5  },
		{ "yellow", 0xFFffff00  },
		{ "yellowgreen", 0xFF9acd32  }
	};

	for(const auto& c: colours)
	{
		if(c.name == colourId)
		{
			return { true, Colour(c.colour) };
		}
	}

	return { false, Colours::transparentBlack }; 
}

ColourParser::ColourParser(const String& value)
{
	if(value[0] == '#')
	{
		String colourValue = "0xFF";

		if(value.length() == 4)
		{
			for(int i = 1; i < 4; i++)
			{
				colourValue << value[i];
				colourValue << value[i];
			}
		}
		else
		{
			colourValue << value.substring(1, 1000);
		}

		c = Colour((uint32)colourValue.getHexValue64());
	}
	else if(value.startsWith("rgb") || value.startsWith("hsl"))
	{
		auto content = value.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);
		auto values = StringArray::fromTokens(content, ",", "\"'");
		values.trim();

		auto r = jlimit(0, 255, values[0].getIntValue());
		auto g = jlimit(0, 255, values[1].getIntValue());
		auto b = jlimit(0, 255, values[2].getIntValue());
		auto a = jlimit(0, 255, values.size() > 3 ? roundToInt(values[3].getFloatValue() * 255) : 255);
		c = value.startsWith("hsl") ? Colour::fromHSL((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f) : Colour::fromRGBA(r, g, b, a);
	}
	else
	{
		c = getColourFromHardcodedString(value).second;
	}
}

String ColourGradientParser::toString(Rectangle<float> area, const ColourGradient& grad)
{
	Line<float> l(grad.point1, grad.point2);

	auto ls = l.getStart();
	auto le = l.getEnd();

	auto tl = area.getTopLeft(); 
	auto tr = area.getTopRight();
	auto bl = area.getBottomLeft();
	auto br = area.getBottomRight();

	String s;

	s << "linear-gradient(" ;

	if(le == tl)
	{
		if(ls == bl)
			s << "to top";
		else if (ls == br)
			s << "to left top";
		else if(ls == tr)
			s << "to left";
		else
		{
			jassertfalse;
			s << "to left";
		}

	}
	else if(le == tr)
	{
		if(ls == tl)
			s << "to right";
		else if(ls == bl)
			s << "to right top";
		else if(ls == br)
			s << "to top";
		else
		{
			jassertfalse;
			s << "to top";
		}
	}
	else if(le == bl)
	{
		if(ls == tl)
			s << "to bottom";
		else if(ls == tr)
			s << "to left bottom";
		else if(ls == br)
			s << "to left";
		else
		{
			jassertfalse;
			s << "to bottom";
		}
	}
	else if(le == br)
	{
		if(ls == tl)
			s << "to right bottom";
		else if(ls == tr)
			s << "to top";
		else if(ls == bl)
			s << "to right";
		else
		{
			jassertfalse;
			s << "to right";
		}
	}
	else
	{
		auto deg = roundToInt(radiansToDegrees(l.getAngle()));
		s << String(deg) << "deg";
	}
	
	for(int i = 0; i < grad.getNumColours(); i++)
	{
		auto idx = grad.getColourPosition(i);
		s << ", #" << grad.getColourAtPosition(idx).toString();

	}

	s << ")";

	return s;
}

ColourGradientParser::ColourGradientParser(Rectangle<float> area, const String& items)
{
	auto tokens = StringArray::fromTokens(items, ",", "()");
	tokens.trim();

	int colourIndex = 0;

	if(tokens[0].startsWith("to "))
	{
		int flag = 0;
		auto t = tokens[0].substring(3);
		colourIndex++;

		enum Direction
		{
			left = 1,
			right = 2,
			top = 4,
			leftTop = 5,
			rightTop = 6,
			bottom = 8,
			leftBottom = 9,
			rightBottom = 10
		};

		if(t.contains("top"))
			flag |= Direction::top;
		if(t.contains("left"))
			flag |= Direction::left;
		if(t.contains("bottom"))
			flag |= Direction::bottom;
		if(t.contains("right"))
			flag |= Direction::right;

		switch((Direction)flag)
		{
		case left:
			gradient.point1 = area.getTopRight();
			gradient.point2 = area.getTopLeft();
			break;
		case right:
			gradient.point1 = area.getTopLeft();
			gradient.point2 = area.getTopRight();
			break;
		case top:
			gradient.point1 = area.getBottomLeft();
			gradient.point2 = area.getTopLeft();
			break;
		case leftTop:
			gradient.point1 = area.getBottomRight();
			gradient.point2 = area.getTopLeft();
			break;
		case rightTop:
			gradient.point1 = area.getBottomLeft();
			gradient.point2 = area.getTopRight();
			break;
		case bottom:
			gradient.point1 = area.getTopLeft();
			gradient.point2 = area.getBottomLeft();
			break;
		case leftBottom:
			gradient.point1 = area.getTopRight();
			gradient.point2 = area.getBottomLeft();
			break;
		case rightBottom:
			gradient.point1 = area.getTopLeft();
			gradient.point2 = area.getBottomRight();
			break;
		}
	}
	else if (tokens[0].endsWith("deg"))
	{
		auto maxEdge = jmax(area.getWidth(), area.getHeight());
		auto square = area.withSizeKeepingCentre(maxEdge, maxEdge);

		gradient.point1 = { area.getCentreX(), square.getY() };
		gradient.point2 = { area.getCentreX(), square.getBottom() };

		auto rad = float_Pi + (float)tokens[0].getIntValue() / 180.0f * float_Pi;

		auto t = AffineTransform::rotation(rad, area.getCentreX(), area.getCentreY());

		gradient.point1.applyTransform(t);
		gradient.point2.applyTransform(t);

		//gradient.point1 = area.getConstrainedPoint(gradient.point1);
		//gradient.point2 = area.getConstrainedPoint(gradient.point2);

		colourIndex++;
	}
	else
	{
		gradient.point1 = area.getTopLeft();
		gradient.point2 = area.getBottomLeft();
	}

    Colour lastColour = Colours::transparentBlack;
    
	for(int i = colourIndex; i < tokens.size(); i++)
	{
		auto colourTokens = StringArray::fromTokens(tokens[i], " ", "()");
			
		if(colourTokens.size() > 1)
		{
			lastColour = ColourParser(colourTokens[0]).getColour();

			if(gradient.getNumColours() == 0)
				gradient.addColour(0.0f, lastColour);

			for(int j = 1; j< colourTokens.size(); j++)
			{
				float proportion = (float)colourTokens[j].getIntValue() / 100.0f;
				gradient.addColour(proportion, lastColour);
			}
		}
		else
		{
			lastColour = ColourParser(tokens[i]).getColour();

			float proportion = (float)(i - colourIndex) / jmax(1.0f, (float)(tokens.size() - colourIndex - 1));
            
            FloatSanitizers::sanitizeFloatNumber(proportion);
            
            
			gradient.addColour(jlimit(0.0f, 1.0f, proportion), lastColour);
		}
	}
    
    while(gradient.getNumColours() <= 1)
        gradient.addColour(1.0f, lastColour);
}

ColourGradient ColourGradientParser::getGradient() const
{ return gradient; }

TransformParser::TransformData::TransformData(TransformTypes t):
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





TransformParser::TransformData TransformParser::TransformData::interpolate(const TransformData& other,
	float alpha) const
{
	TransformData copy(TransformTypes(jmax((int)type, (int)other.type)));

	copy.values[0] = values[0];

	if(numValues == 1)
		copy.values[1] = values[0];

	copy.numValues = jmax(numValues, other.numValues);

	auto interpolate = [](float x1, float x2, float alpha)
	{
		const auto invDelta = 1.0f - alpha;
		return invDelta * x1 + alpha * x2;
	};

	copy.values[0] = interpolate(copy.values[0], other.values[0], alpha);
	copy.values[1] = interpolate(copy.values[1], other.values[1], alpha);

	return copy;
}

AffineTransform TransformParser::TransformData::toTransform(const std::vector<TransformData>& list, Point<float> center)
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

TransformParser::TransformParser(KeywordDataBase* db, const String& stackedTransforms):
	t(stackedTransforms),
    database(db)
{}

std::vector<TransformParser::TransformData> TransformParser::parse(Rectangle<float> totalArea, float defaultSize)
{
	auto ptr = t.begin();
	auto end = t.end();

	std::vector<TransformData> list;

	while(ptr != end)
	{
		char nameBuffer[20];
		int bufferIndex = 0;

		while(ptr != end)
		{
			if(*ptr == '(')
			{
				++ptr;
				break;
			}

			if(isPositiveAndBelow(bufferIndex, sizeof(nameBuffer)))
				nameBuffer[bufferIndex++] = *ptr++;
			else
			{
				break;
			}
		}

		nameBuffer[bufferIndex] = 0;

		String transformId(nameBuffer);
		
		KeywordDataBase database;
		auto typeIndex = database.getAsEnum("transform", transformId, TransformTypes::none);
		TransformData nd(typeIndex);
			
		bufferIndex = 0;

		int valueIndex = 0;

		while(ptr != end)
		{
			if(*ptr == ',' || *ptr == ')')
			{
				nameBuffer[bufferIndex] = 0;

				auto areaToUse = totalArea;

				if(nd.type >= TransformTypes::scale)
					areaToUse = { 1.0f, 1.0f };

				nd.values[valueIndex++] = ExpressionParser::evaluate(String(nameBuffer).trim(), { valueIndex == 0, areaToUse, defaultSize });

				bufferIndex = 0;

				if(*ptr == ')')
				{
					++ptr;
					break;
				}
				else
				{
					++ptr;
					continue;
				}
			}

			if(isPositiveAndBelow(bufferIndex, sizeof(nameBuffer)))
				nameBuffer[bufferIndex++] = *ptr++;
			else
			{
				break;
			}
		}

		nd.numValues = jmin(2, valueIndex);

		list.push_back(std::move(nd));
	}

	return list;
}

ShadowParser::ShadowParser(const std::vector<String>& tokens)
{
	Data currentData;

	auto flush = [&]()
	{
		if(!currentData.somethingSet)
			return;

		if(currentData.positions.size() < 3)
			currentData.positions.add("0px"); // add blur

		if(currentData.positions.size() < 4)
			currentData.positions.add("0px"); // add spread

		data.push_back(currentData);
		currentData = {};
	};

	for(int i = 0; i < tokens.size(); i++)
	{
		auto thisToken = tokens[i];

		auto flushBefore = shouldFlushBefore(thisToken);
		auto flushAfter = shouldFlushAfter(thisToken);

		if(flushBefore)
			flush();

		auto vt = Parser::findValueType(thisToken);

		if(thisToken == "inset")
			currentData.inset = true;
		if(vt == ValueType::Colour)
			currentData.c = ColourParser(thisToken).getColour();
		else if(vt == ValueType::Size || vt == ValueType::Number)
			currentData.positions.add(thisToken);

		currentData.somethingSet = true;

		if(flushAfter)
			flush();
	}

	flush();
}

ShadowParser::ShadowParser(const String& alreadyParsedString, Rectangle<float> totalArea)
{
	if(alreadyParsedString == "none")
		return;

	auto lines = StringArray::fromTokens(alreadyParsedString, "|", "");
	lines.removeEmptyStrings();

	for(const auto& l: lines)
	{
		if(l.startsWith("none"))
			continue;

		auto tokens = StringArray::fromTokens(l, ";", "");
		jassert(tokens.size() == 3);

		Data nd;
			
		nd.inset = tokens[0].contains("inset");
        nd.c = Colour::fromString(tokens[1].trim().substring(2, 1000));
		auto positions = StringArray::fromTokens(tokens[2].substring(2, 1000), " ", "");

		nd.size[0] = parseSize(positions[1], totalArea);
		nd.size[1] = parseSize(positions[2], totalArea);
		nd.size[2] = parseSize(positions[3], totalArea);
		nd.size[3] = parseSize(positions[4], totalArea);

		data.push_back(nd);
	}
}

std::vector<melatonin::ShadowParameters> ShadowParser::getShadowParameters(bool wantsInset) const
{
	std::vector<melatonin::ShadowParameters> list;

	for(int i = 0; i < data.size(); i++)
	{
		if(data[i].inset != wantsInset)
			continue;

		list.push_back(data[i].toShadowParameter());
	}

	return list;
}

std::vector<melatonin::ShadowParameters> ShadowParser::interpolate(const ShadowParser& other, double alpha,
	int wantsInset) const
{
	
	std::vector<melatonin::ShadowParameters> list;

	auto maxNum = jmax(data.size(), other.data.size());

	for(int i = 0; i < maxNum; i++)
	{
		ShadowParser::Data a, b;

		if(isPositiveAndBelow(i, data.size()))
			a = data[i].copyWithoutStrings(1.0f);
		else
			a = other.data[i].copyWithoutStrings(0.0f);

		if(isPositiveAndBelow(i, other.data.size()))
			b = other.data[i].copyWithoutStrings(1.0f);
		else
			b = data[i].copyWithoutStrings(0.0f);

		auto p = a.interpolate(b, alpha).toShadowParameter();

		if(p.inner == (bool)wantsInset)
			list.push_back(p);
	}

	

	return list;
}

String ShadowParser::toParsedString() const
{
	String fullProp;

	for(auto& sd: data)
	{
		fullProp << "t:" << (sd.inset ? "inset;" : "outer;");
		fullProp << "c:" << sd.c.toString() << ";";
		fullProp << "p:[ ";

		for(const auto& v: sd.positions)
			fullProp << v << " ";

		fullProp << "]|";
	}
	
	return fullProp;
}

bool ShadowParser::shouldFlushBefore(String& token)
{
	if(token.startsWithChar(','))
	{
		token = token.substring(1, 1000);
		return true;
	}

	return false;
}

bool ShadowParser::shouldFlushAfter(String& token)
{
	if(token.endsWithChar(','))
	{
		token = token.substring(0, token.length() - 1);
		return true;
	}

	return false;
}

int ShadowParser::parseSize(const String& s, Rectangle<float> totalArea)
{
	return roundToInt(ExpressionParser::evaluate(s, { false, totalArea, 16.0f }));
}

melatonin::ShadowParameters ShadowParser::Data::toShadowParameter() const
{
	melatonin::ShadowParameters p;
	p.color = c;
	p.inner = inset;
	p.radius = size[2];
	p.offset = { size[0], size[1] };
	p.spread = size[3];
	return p;
}

ShadowParser::Data ShadowParser::Data::interpolate(const Data& other, double alpha) const
{
	Data mix;

	mix.inset = inset || other.inset;
	mix.size[0] = roundToInt(Interpolator::interpolateLinear((double)size[0], (double)other.size[0], alpha));
	mix.size[1] = roundToInt(Interpolator::interpolateLinear((double)size[1], (double)other.size[1], alpha));
	mix.size[2] = roundToInt(Interpolator::interpolateLinear((double)size[2], (double)other.size[2], alpha));
	mix.size[3] = roundToInt(Interpolator::interpolateLinear((double)size[3], (double)other.size[3], alpha));
	mix.c = c.interpolatedWith(other.c, (float)alpha);

	return mix;
}

ShadowParser::Data ShadowParser::Data::copyWithoutStrings(float alpha) const
{
	Data copy;
	copy.somethingSet = somethingSet;
	copy.inset = inset;
	memcpy(copy.size, size, sizeof(int)*4);
	copy.c = c.withMultipliedAlpha(alpha);
	return copy;
}

float ExpressionParser::evaluateLiteral(const String& s, const Context<>& context)
{
	float value;
	auto fullSize = context.useWidth ? context.fullArea.getWidth() : context.fullArea.getHeight();
	
    if(s == "auto")
        return fullSize;
	if(s.endsWith("vh"))
	{
		fullSize = context.fullArea.getHeight();
		value = s.getFloatValue() * 0.01 * fullSize;
	}
	else if(s.endsWithChar('x'))
		value =  s.getFloatValue();
	else if(s.endsWithChar('%'))
		value = fullSize * s.getFloatValue() * 0.01f;
	else if(s.endsWith("em"))
		value = s.getFloatValue() * context.defaultFontSize;
	else if(s.endsWith("deg"))
		value = s.getFloatValue() / 180.0f * float_Pi;
	else 
		value = s.getFloatValue();

	FloatSanitizers::sanitizeFloatNumber(value);
	return value;
}

String ExpressionParser::Node::evaluateToCodeGeneratorLiteral(const Context<String>& context) const
{
	jassert(context.isCodeGenContext());

	if(s.endsWithChar('x'))
		return s.upToLastOccurrenceOf("px", false, false);
	if(s.endsWith("em"))
		return String(context.defaultFontSize * s.getFloatValue(), 2);

	auto fullSize = context.fullArea + (context.useWidth ? ".getWidth()" : ".getHeight()");

	String rv;
	float value = 0.0f;
	
	if(s.endsWith("vh"))
	{
		fullSize = context.fullArea + ".getHeight()";
		value =  s.getFloatValue() * 0.01;
	}

	if(s.endsWith("%"))
	{
		value = s.getFloatValue() * 0.01f;
	}

	rv << "( " << fullSize << " * " << String(value) << ")";
	return rv;
}

float ExpressionParser::Node::evaluate(const Context<>& context) const
{
	switch(type)
	{
	case ExpressionType::literal: return evaluateLiteral(s, context);
	case ExpressionType::calc:
		{
			if(children.size() == 2)
			{
				auto l = children[0].evaluate(context);
				auto r = children[1].evaluate(context);

				switch(op)
				{
				case '+': return l + r;
				case '-': return l - r;
				case '/': return r > 0.0f ? l / r : 0.0f;
				case '*': return l * r;
				default: return 0.0f;
				}
			}
		};
	case ExpressionType::min:
		{
			if(children.empty())
				return 0.0f;

			float value = std::numeric_limits<float>::max();

			for(const auto& c: children)
				value = jmin(value, c.evaluate(context));

			return value;
		}
	case ExpressionType::max:
		{
			if(children.empty())
				return 0.0f;

			float value = std::numeric_limits<float>::min();

			for(const auto& c: children)
				value = jmax(value, c.evaluate(context));

			return value;
		}
	case ExpressionType::clamp:
		{
			if(children.size() == 3)
			{
				auto minValue = children[0].evaluate(context);
				auto maxValue = children[2].evaluate(context);
				auto value = children[1].evaluate(context);
				return jlimit(minValue, maxValue, value);
			}

			return 0.0f;
		}
	case ExpressionType::numExpressionTypes:
	case ExpressionType::none:
	default:
		break;
	}

	return 0.0f;
}

void ExpressionParser::match(String::CharPointerType& ptr, const String::CharPointerType end, juce_wchar t)
{
	if(ptr == end && t != 0)
	{
		String errorMessage;
		errorMessage << "expected: " << String(t) << ", got EOF";
		throw Result::fail(errorMessage);
	}

	if(*ptr != t)
	{
		String errorMessage;
		errorMessage << "expected: " << t << ", got: " << *ptr;
		throw Result::fail(errorMessage);
	}

	++ptr;
}

void ExpressionParser::skipWhitespace(String::CharPointerType& ptr, const String::CharPointerType end)
{
	while(ptr != end && CharacterFunctions::isWhitespace(*ptr))
		ptr++;
}

ExpressionParser::Node ExpressionParser::parseNode(String::CharPointerType& ptr, const String::CharPointerType end)
{
	Node node;

	static constexpr int NumTypes = (int)ExpressionType::numExpressionTypes;
	static const std::array<const char*, NumTypes> types({ "none", "value", "calc", "min", "max", "clamp" });

	while(ptr != end)
	{
		auto isExpressionKeyword = *ptr == 'c' || *ptr == 'm';

		if(isExpressionKeyword)
		{
			int bufferIndex = 0;
			char buffer[6];
			memset(buffer, 0, sizeof(buffer));

			while(ptr != end)
			{
				buffer[bufferIndex++] = *ptr++;

				if(*ptr == '(' || CharacterFunctions::isWhitespace(*ptr))
				{
					buffer[bufferIndex] = 0;
					int typeIndex = 0;

					for(const auto& t: types)
					{
						if(memcmp(t, buffer, bufferIndex) == 0)
						{
							node.type = (ExpressionType)typeIndex;
							break;
						}
								
						typeIndex++;
					}

					if(node.type == ExpressionType::none)
						throw Result::fail("Unknown expression type " + String(buffer));

					bufferIndex = 0;

					skipWhitespace(ptr, end);
					match(ptr, end, '(');

					while(ptr != end)
					{
						node.children.push_back(parseNode(ptr, end));

						if(ptr != end && *ptr == ')')
						{
							++ptr;
							return node;
						}
								
						if(ptr != end)
						{
							skipWhitespace(ptr, end);
							node.op = *ptr++;
							skipWhitespace(ptr, end);
						}
							
					}
				}
			}
		}
		else
		{
			node.type = ExpressionType::literal;

			while(ptr != end)
			{
				if(CharacterFunctions::isWhitespace(*ptr) || *ptr == ',' || *ptr == ')')
					break;
					
				node.s << *ptr++;
			}

			skipWhitespace(ptr, end);

			break;
		}
	}

	return node;
}

String ExpressionParser::evaluateToCodeGeneratorLiteral(const String& expression, const Context<String>& context)
{
	jassert(context.isCodeGenContext());
		
	auto ptr = expression.begin();
	auto end = expression.end();

	Node root = parseNode(ptr, end);

	return root.evaluateToCodeGeneratorLiteral(context);
}

float ExpressionParser::evaluate(const String& expression, const Context<>& context)
{
	if(!CharacterFunctions::isLetter(expression[0]))
		return evaluateLiteral(expression, context);

	auto ptr = expression.begin();
	auto end = expression.end();

	try
	{
		Node root = parseNode(ptr, end);

		auto value = root.evaluate(context);
		FloatSanitizers::sanitizeFloatNumber(value);
		return value;
	}
	catch(Result& r)
	{
		DBG(r.getErrorMessage());
		return context.defaultFontSize;
	}
}


struct BezierCurve
{
	BezierCurve(Point<double> p1, Point<double> p2)
	{
		cx = 3.0 * p1.getX();
		bx = 3.0 * (p2.getX() - p1.getX()) - cx;
		ax = 1.0 - cx - bx;

		cy = 3.0 * p1.getY();
		by = 3.0 * (p2.getY() - p1.getY()) - cy;
		ay = 1.0 - cy - by;
	}

	double operator()(double x) const
	{
	    return sampleCurveY(solveCurveX(x));
	}

private:

	double ax, ay, bx, by, cx, cy;
	const double epsilon = 1e-5; 

	double sampleCurveX(double t) const { return ((ax * t + bx) * t + cx) * t; }
	double sampleCurveY(double t) const { return ((ay * t + by) * t + cy) * t; }
	double sampleCurveDerivativeX(double t) const { return (3.0 * ax * t + 2.0 * bx) * t + cx; }

	double solveCurveX(double x) const
	{
		double t0; 
		double t1;
		double t2;
		double x2;
		double d2;
		double i;

		for (t2 = x, i = 0; i < 8; i++) {
			x2 = sampleCurveX(t2) - x;
			if (std::abs (x2) < epsilon)
				return t2;
			d2 = sampleCurveDerivativeX(t2);
			if (std::abs(d2) < epsilon)
				break;
			t2 = t2 - x2 / d2;
		}

		t0 = 0.0;
		t1 = 1.0;
		t2 = x;

		if (t2 < t0) return t0;
		if (t2 > t1) return t1;

		while (t0 < t1) {
			x2 = sampleCurveX(t2);
			if (std::abs(x2 - x) < epsilon)
				return t2;
			if (x > x2) t0 = t2;
			else t1 = t2;

			t2 = (t1 - t0) * .5 + t0;
		}

		return t2;
	}
};



Parser::Parser(const String& cssCode):
	code(cssCode),
	ptr(code.begin()),
	end(code.end())
{
		
}

String Parser::getTokenName(TokenType t)
{
	switch(t)
	{
	case EndOfFIle: return "EOF";
	case OpenBracket: return "{";
	case CloseBracket: return "}";
	case Keyword: return "css keyword";
	case Colon: return ":";
	case OpenParen: return "(";
	case CloseParen: return ")";
	case Semicolon: return ";";
	case ValueString: return "value";
	case numTokenTypes: break;
	default: ;
	}

	return {};
}

void Parser::skip()
{
	if(ptr == end)
		return;

	while(CharacterFunctions::isWhitespace(*ptr))
		++ptr;

	if(ptr < (end-1) && *ptr == '/')
	{
		if(*(ptr + 1) == '*')
		{
			/* skip comment */
			while(ptr != end)
			{
				if(*ptr == '*')
				{
					if((ptr + 1) < end && *(ptr + 1) == '/')
					{
						ptr++;
						ptr++;

						skip();
						return;
					}
				}

				ptr++;
			}
		}
	}
}

bool Parser::match(TokenType t)
{
	if(!matchIf(t))
		throwError("Expected token: " + getTokenName(t));

	return true;
}

void Parser::throwError(const String& errorMessage)
{
	String error = getLocation();
	error << errorMessage;

	throw Result::fail(error);
}

bool Parser::matchIf(TokenType t)
{
	skip();

	if(ptr == end)
		return t == TokenType::EndOfFIle;
		
	auto matchChar = [&](juce_wchar c)
	{
		if(*ptr == c)
		{
			ptr++;
			return true;
		}

		return false;
	};

	switch(t)
	{
	case EndOfFIle:    return matchChar(0);
	case OpenBracket:  return matchChar('{');
	case CloseBracket: return matchChar('}');
	case At:		   return matchChar('@');	
	case Dot:		   return matchChar('.');
	case Hash:		   return matchChar('#');
	case OpenParen:	   return matchChar('(');
	case CloseParen:   return matchChar(')');
	case Colon:        return matchChar(':');
	case Semicolon:    return matchChar(';');
	case Comma:		   return matchChar(',');
	case Asterisk:	   return matchChar('*');
	case Quote:		   return matchChar('\'') || matchChar('"');
	case Keyword: 
		{
			currentToken = "";

			while(CharacterFunctions::isLetterOrDigit(*ptr) || *ptr == '-')
				currentToken << *ptr++;

			return currentToken.isNotEmpty();
		}
	case ValueString:
		{
			currentToken = "";

			while(ptr != end && *ptr != ' ' && *ptr != ';')
			{
				if(*ptr == '\'' || *ptr == '"')
				{
					auto quoteChar = *ptr;

					++ptr;

					while(ptr != end)
					{
						if(*ptr == quoteChar)
						{
							ptr++;

							if(*ptr == ';')
								return true;

							break;
						}
							
						currentToken << *ptr++;

						if(ptr != end && *ptr == quoteChar)
						{
							++ptr;
							
							if(ptr != end && *ptr == ';')
								return currentToken.isNotEmpty();
							else
								break;
							
						}
							
					}
				}

                if(matchIf(CloseBracket))
                    throwError("Expected ;");
                
				if(matchIf(OpenParen))
				{
					int numOpen = 1;

					currentToken << '(';

					while(ptr != end)
					{
						if(*ptr == '(')
							++numOpen;

						if(*ptr == ')')
							--numOpen;

						currentToken << *ptr++;

						if(numOpen == 0)
							break;
					}
					
					break;
				}
				else if (ptr != end)
					currentToken << *ptr++;
			}

			currentToken = currentToken.trim();
			return currentToken.isNotEmpty();
		}
	case numTokenTypes: break;
	default: ;
	}

	return false;
}

PseudoState Parser::parsePseudoClass()
{
	KeywordWarning kw(*this);

	int state = 0;
	PseudoElementType element = PseudoElementType::None;

	while(matchIf(TokenType::Colon))
	{
		if(matchIf(TokenType::Colon))
		{
			kw.setLocation(*this);
			match(TokenType::Keyword);

			kw.check(currentToken, KeywordDataBase::KeywordType::PseudoClass);

			if(currentToken == "before")
				element = PseudoElementType::Before;
			if(currentToken == "after")
				element = PseudoElementType::After;
		}
		else
		{
			kw.setLocation(*this);
			match(TokenType::Keyword);

			kw.check(currentToken, KeywordDataBase::KeywordType::PseudoClass);

			if(currentToken == "first-child")
				state |= (int)PseudoClassType::First;
			if(currentToken == "last-child")
				state |= (int)PseudoClassType::Last;
			if(currentToken == "active")
				state |= (int)PseudoClassType::Active;
			if(currentToken == "hidden")
				state |= (int)PseudoClassType::Hidden;
			if(currentToken == "disabled")
				state |= (int)PseudoClassType::Disabled;
			if(currentToken == "hover")
				state |= (int)PseudoClassType::Hover;
			if(currentToken == "focus")
				state |= (int)PseudoClassType::Focus;
			if(currentToken == "root")
				state |= (int)PseudoClassType::Root;
			if(currentToken == "checked")
				state |= (int)PseudoClassType::Checked;
		}

		skip();
	}

	return { (PseudoClassType)state, element };
}

Parser::RawClass Parser::parseSelectors()
{
	RawClass newClass;

	skip();

	Selector::RawList currentList;

	KeywordWarning kw(*this);

	while(ptr != end && *ptr != '{')
	{
		if(matchIf(TokenType::Comma))
		{
			if(!currentList.empty())
			{
				newClass.selectors.push_back(std::move(currentList));
				currentList = {};
			}
		}
		Selector ns;

		if(matchIf(TokenType::Asterisk))
		{
			ns.name = "*";
			ns.type = SelectorType::All;
		}
		else if(matchIf(TokenType::At))
		{
			match(TokenType::Keyword);

			ns.name = currentToken;
			ns.type = SelectorType::AtRule;
		}
		else if(matchIf(TokenType::Colon))
		{
			match(TokenType::Colon);
			match(TokenType::Keyword);
			ns.name = "::" + currentToken;
			ns.type = SelectorType::Class;
		}
		else if(matchIf(TokenType::Dot))
		{
			match(TokenType::Keyword);
			ns.name = currentToken;
			ns.type = SelectorType::Class;
		}
		else if (matchIf(TokenType::Hash))
		{
			match(TokenType::Keyword);
			ns.name = currentToken;
			ns.type = SelectorType::ID;
		}
		else
		{
			kw.setLocation(*this);

			match(TokenType::Keyword);

			if(currentToken == "element")
			{
				match(TokenType::OpenParen);
				match(TokenType::Keyword);
				ns.name = currentToken;
				ns.type = SelectorType::Element;
				match(TokenType::CloseParen);
			}
			else
			{
				kw.check(currentToken, KeywordDataBase::KeywordType::Type);
				ns.name = currentToken;
				ns.type = SelectorType::Type;
			}
		}

		auto isSpace = CharacterFunctions::isWhitespace(*ptr);

		currentList.push_back({ns, parsePseudoClass() });

		if(ns.type == SelectorType::AtRule)
			break;

		if(isSpace)
			currentList.push_back({ Selector(SelectorType::ParentDelimiter, " "), {}});

		skip();
	}
	
	newClass.selectors.push_back(currentList);

	return newClass;
}

Result Parser::parse()
{
	try
	{
		KeywordWarning kw(*this);	

		while(ptr != end)
		{
			auto newClass = parseSelectors();

			if(!matchIf(TokenType::OpenBracket))
			{
				// could be a import statement;

				if(newClass.selectors[0][0].first.toString() == "@import")
				{
					match(TokenType::ValueString);

					auto src = currentToken;
					match(TokenType::Semicolon);

					RawLine l;
					l.property = "src";
					l.items.push_back(src);

					newClass.lines.push_back(std::move(l));
					rawClasses.push_back(newClass);

					skip();
					continue;
				}
				else
				{
					match(TokenType::OpenBracket);
				}
			}

			kw.setLocation(*this);

			while(matchIf(TokenType::Keyword))
			{
				RawLine nl;
				nl.property = currentToken;

				kw.check(currentToken, KeywordDataBase::KeywordType::Property);
				
				match(TokenType::Colon);

				while(ptr != end)
				{
					match(TokenType::ValueString);

					nl.items.push_back(currentToken);

					if(matchIf(TokenType::Semicolon))
						break;
				}
					
				auto currentValue = currentToken;
					
				newClass.lines.push_back(std::move(nl));

				skip();
				kw.setLocation(*this);
			}

			match(TokenType::CloseBracket);

			skip();
			rawClasses.push_back(newClass);
		}
			
		match(TokenType::EndOfFIle);

		return Result::ok();
	}
	catch(Result& r)
	{

		return r;
	}
}

String Parser::getLocation(String::CharPointerType p) const
{
	if(!p)
		p = ptr;

	int line = 0;
	int col = 0;

	auto s = code.begin();

	while(s != p)
	{
		col++;

		if(*s == '\n')
		{
			line++;
			col = 0;
		}

		s++;
	}

	String loc;
	loc << "Line " << String(line+1) + ", column " + String(col+1) << ": ";
	return loc;
}

Parser::KeywordWarning::KeywordWarning(Parser& parent_):
	parent(parent_),
	currentLocation(nullptr)
{}

void Parser::KeywordWarning::setLocation(Parser& p)
{
	currentLocation = p.ptr;
}

void Parser::KeywordWarning::check(const String& s, KeywordDataBase::KeywordType type)
{
	if(!database->getKeywords(type).contains(s))
	{
		String w = parent.getLocation(currentLocation);
		w << "unsupported " + database->getKeywordName(type) << ": ";
		w << s;
		parent.warnings.add(w);
	}
}

ValueType Parser::findValueType(const String& value)
{
	static const StringArray colourPrefixes = { "#", "rgba(", "hsl(", "rgb(" };

	if(value.startsWith("var(--"))
		return ValueType::Variable;

	for(const auto& cp: colourPrefixes)
	{
		if(value.startsWith(cp))
			return ValueType::Colour;
	}

	if(value.endsWith("px") || value.endsWithChar('%') || value.endsWith("em"))
		return ValueType::Size;

	if(ColourParser::getColourFromHardcodedString(value).first)
		return ValueType::Colour;

	if(value.startsWith("linear-gradient"))
		return ValueType::Gradient;

	if(CharacterFunctions::isDigit(value[0]))
		return ValueType::Number;

	return ValueType::Undefined;
}

String Parser::processValue(const String& value, ValueType t)
{
	if(t == ValueType::Undefined)
	{
		t = findValueType(value);
	}

	if(hasVariable(value))
		return value;

	switch(t)
	{
	case ValueType::Colour:
		{
			ColourParser cp(value);
			return "0x" + cp.getColour().toDisplayString(true);
		}
	case ValueType::Gradient:
	{
		return value;
	}
	case ValueType::Time:
	{
		 return String(value.endsWith("ms") ? ((double)value.getIntValue() * 0.001) : value.getDoubleValue());
	}
	case ValueType::numValueTypes: break;
	default: ;
	}

	return value;
}

String Parser::getTokenSuffix(PropertyType p, const String& keyword, String& token)
{
	static StringArray styles({"solid", "dotted", "outset", "dashed"});

	auto appendColour = [](const String& k)
	{
		if(k.endsWith("-color"))
			return "";

		return "-color";
	};

	auto v = findValueType(token);

	if(v == ValueType::Gradient)
	{
		return appendColour(keyword);
	}

	if(token.contains("px") || token.contains("em") || token.contains("%"))
	{
		switch(p)
		{
		case PropertyType::Font: return "";
		case PropertyType::Border: return "-width";
		case PropertyType::Transform: return "";
		case PropertyType::Positioning: return "";
		case PropertyType::Layout: return "";
        case PropertyType::Colour: return "";
		case PropertyType::Variable: return "";		default: jassertfalse; return "";
		}
	}
	if(styles.contains(token))
		return "-style";
	if(v == ValueType::Colour)
	{
		token = processValue(token, ValueType::Colour);

		if(p == PropertyType::Border || keyword == "background")
			return appendColour(keyword);
	}

	return "";
}

PropertyType Parser::getPropertyType(const String& p)
{
	if(p.startsWith("--") || p.startsWith("var(--"))
		return PropertyType::Variable;

	static const StringArray layoutIds({"x", "y", "left", "right", "top", "bottom", "width", "height", "min-width", "min-height", "max-width", "max-height", "opacity", "gap"});

	if(p == "transform")
		return PropertyType::Transform;

	if(p.startsWith("border"))
	{
		if(p.endsWith("radius"))
			return PropertyType::BorderRadius;
		else
			return PropertyType::Border;
	}
	if(p.startsWith("padding"))
		return PropertyType::Positioning;

	if(layoutIds.contains(p))
		return PropertyType::Layout;

	if(p.startsWith("margin"))
		return PropertyType::Positioning;

	if(p.startsWith("layout"))
		return PropertyType::Positioning;

	if(p.startsWith("background"))
		return PropertyType::Colour;

	if(p.startsWith("transition"))
		return PropertyType::Transition;

	if(p.endsWith("-shadow"))
		return PropertyType::Shadow;

	if(p.startsWith("font") || p.startsWith("letter") || p.startsWith("line"))
		return PropertyType::Font;

	return PropertyType::Undefined;
}

std::function<double(double)> Parser::parseTimingFunction(const String& t)
{
	std::map<String, std::function<double(double)>> curves;

	if(t.startsWith("steps"))
	{
		auto values = t.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);

		auto sa = StringArray::fromTokens(values, ",", "");
		sa.trim();

		auto numSteps = (float)sa[0].getIntValue();

		if(numSteps > 0.0)
		{
			enum JumpMode
			{
				Ceil,
				Floor,
				Round
			};

			std::map<String, JumpMode> modes;
			
			modes["jump-start"] = JumpMode::Ceil;
			modes["jump-end"] = JumpMode::Floor;
			modes["jump-both"] = JumpMode::Round;
			modes["jump-none"] = JumpMode::Round;
			modes["start"] = JumpMode::Ceil;
			modes["end"] = JumpMode::Floor;

			JumpMode m = JumpMode::Round;

			if(sa[1].isNotEmpty() && modes.find(sa[1]) != modes.end())
				m = modes[sa[1]];

			switch(m)
			{
			case Ceil: return [numSteps](double input) { return std::ceil(input * numSteps) / numSteps; };
			case Floor: return [numSteps](double input) { return std::floor(input * numSteps) / numSteps; };
			case Round: return [numSteps](double input) { return std::round(input * numSteps) / numSteps; };
			default: ;
			}
		}
	}

	if(t.startsWith("cubic-bezier"))
	{
		auto values = t.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);

		auto sa = StringArray::fromTokens(values, ",", "");
		sa.trim();

		if(sa.size() == 4)
		{
			float values[4];

			values[0] = sa[0].getFloatValue();
			values[1] = sa[1].getFloatValue();
			values[2] = sa[2].getFloatValue();
			values[3] = sa[3].getFloatValue();

			FloatSanitizers::sanitizeArray(values, 4);
			return BezierCurve({(double)values[0], (double)values[1]}, { (double)values[2], (double)values[3]});
		}
	}

	curves["ease"] = BezierCurve({0.25,0.1} ,{0.25,1});
	curves["linear"] = [](double v){ return v; };
	curves["ease-in"] = BezierCurve({0.42,0.0}, {1.0, 1.0} );
	curves["ease-out"] = BezierCurve({0.0,0.0}, {0.58, 1.0} );
	curves["ease-in-out"] = BezierCurve({0.42,0.0}, {0.58, 1.0} );

	if(curves.find(t) != curves.end())
		return curves.at(t);

	return {};
}

StyleSheet::Collection Parser::getCSSValues() const
{
	StyleSheet::List list;

	for(const auto& rc: rawClasses)
	{
		

		Array<PseudoState> thisStates;

		ComplexSelector::List selectorList;

		for(auto& s: rc.selectors)
		{
			selectorList.add(new ComplexSelector(s));

			for(const auto& ts: selectorList.getLast()->thisSelectors.selectors)
			{
				thisStates.addIfNotAlreadyThere(ts.second);
			}
		}

		StyleSheet::Ptr n;

		for(auto existing: list)
		{
			if(existing->matchesComplexSelectorList(selectorList))
			{
				n = existing;
				break;
			}
		}

		if(n == nullptr)
			n = new StyleSheet(selectorList);
		
		auto addOrOverwrite = [&](PropertyType pt, bool isImportant, const String& k, const String& v)
		{
			bool shouldExtend = pt == PropertyType::Shadow || pt == PropertyType::Transform;

			if(pt == PropertyType::Variable)
			{
				n->setPropertyVariable(Identifier(k.substring(2, 1000)), v);
				return;
			}

			for(auto& thisPseudoState: thisStates)
			{
				bool found = false;

				for(auto& elementProperties: n->properties[(int)thisPseudoState.element])
				{
					if(elementProperties.name == k)
					{
						for(auto& propertyValue: elementProperties.values)
						{
							if(propertyValue.first == thisPseudoState.stateFlag)
							{
                                if((int)isImportant >= (int)propertyValue.second.important)
                                {
                                    if(v == "initial" || v == "unset" || v == "inherit")
                                    {
                                        propertyValue.second = PropertyValue(pt, "default", isImportant);
                                    }
                                    else
                                    {
                                        if(shouldExtend)
                                            propertyValue.second.appendToValue(v);
                                        else
                                            propertyValue.second = PropertyValue(pt, v, isImportant);
                                    }
                                }
								found = true;
								break;
							}
						}
					}
				}

				if(!found)
				{
                    auto vToUse = v;
                    
                    if(v == "initial" || v == "unset" || v == "inherit")
                    {
                        vToUse = "default";
                    }
                    
					Property p;
					p.name = k;
					p.values.push_back({thisPseudoState.stateFlag, PropertyValue(pt, vToUse, isImportant)});
					n->properties[(int)thisPseudoState.element].push_back(p);
				}
			}
		};

		std::map<String, Transition> transitions;

		for(const auto& rv: rc.lines)
		{
            bool isImportant = false;
            
			auto p = getPropertyType(rv.property);

            auto tokens = rv.items;
			auto isMultiValue = tokens.size() > 1;
            
            if(tokens[tokens.size()-1] == "!important")
            {
                isImportant = true;
                tokens.pop_back();
                isMultiValue = tokens.size() > 1;
            }
            
			if(p == PropertyType::Positioning)
				isMultiValue = !rv.property.containsChar('-');

			if(p == PropertyType::BorderRadius)
				isMultiValue = rv.property == "border-radius";

			if(isMultiValue)
			{
				switch(p)
				{
				case PropertyType::Positioning:
				{
					if(tokens.size() == 1)
					{
						addOrOverwrite(p, isImportant, rv.property + "-top", tokens[0]);
						addOrOverwrite(p, isImportant, rv.property + "-bottom", tokens[0]);
						addOrOverwrite(p, isImportant, rv.property + "-left", tokens[0]);
						addOrOverwrite(p, isImportant, rv.property + "-right", tokens[0]);
					}
					if(tokens.size() == 2)
					{
						// tokens[0] = t, b
						// tokens[1] = r, l
						addOrOverwrite(p, isImportant, rv.property + "-top", tokens[0]);
						addOrOverwrite(p, isImportant, rv.property + "-bottom", tokens[0]);
						addOrOverwrite(p, isImportant, rv.property + "-left", tokens[1]);
						addOrOverwrite(p, isImportant, rv.property + "-right", tokens[1]);
					}
					if(tokens.size() == 3)
					{
						addOrOverwrite(p, isImportant, rv.property + "-top", tokens[0]);
						addOrOverwrite(p, isImportant, rv.property + "-left", tokens[1]);
						addOrOverwrite(p, isImportant, rv.property + "-right", tokens[1]);
						addOrOverwrite(p, isImportant, rv.property + "-bottom", tokens[2]);
					}
					if(tokens.size() == 4)
					{
						// tokens = [t, r, b, l]
						addOrOverwrite(p, isImportant, rv.property + "-top", tokens[0]);
						addOrOverwrite(p, isImportant, rv.property + "-bottom", tokens[1]);
						addOrOverwrite(p, isImportant, rv.property + "-left", tokens[2]);
						addOrOverwrite(p, isImportant, rv.property + "-right", tokens[3]);
					}

					break;
				}
				case PropertyType::Transition:
				{
					auto transitionTarget = tokens[0];
					
					Transition t;
					t.active = true;
					t.duration = processValue(tokens[1], ValueType::Time).getDoubleValue();

					if(tokens.size() > 2)
						t.f = parseTimingFunction(tokens[2]);
					
					if(tokens.size() > 3)
						t.delay = processValue(tokens[3], ValueType::Time).getDoubleValue();
					
					if(transitionTarget == "all")
					{
						for(auto& l: thisStates)
						{
							if(l.isDefaultState())
								n->setDefaultTransition(l.element, t);
						}
					};
					
					transitions[transitionTarget] = t;
					
					break;
				}
				case PropertyType::BorderRadius:
				{
					if(tokens.size() == 1)
					{
						addOrOverwrite(p, isImportant, "border-top-left-radius", tokens[0]);
						addOrOverwrite(p, isImportant, "border-top-right-radius", tokens[0]);
						addOrOverwrite(p, isImportant, "border-bottom-left-radius", tokens[0]);
						addOrOverwrite(p, isImportant, "border-bottom-right-radius", tokens[0]);
					}
					if(tokens.size() == 2)
					{
						addOrOverwrite(p, isImportant, "border-top-left-radius", tokens[0]);
						addOrOverwrite(p, isImportant, "border-top-right-radius", tokens[1]);
						addOrOverwrite(p, isImportant, "border-bottom-left-radius", tokens[1]);
						addOrOverwrite(p, isImportant, "border-bottom-right-radius", tokens[0]);
					}
					if(tokens.size() == 4)
					{
						addOrOverwrite(p, isImportant, "border-top-left-radius", tokens[0]);
						addOrOverwrite(p, isImportant, "border-top-right-radius", tokens[1]);
						addOrOverwrite(p, isImportant, "border-bottom-left-radius", tokens[2]);
						addOrOverwrite(p, isImportant, "border-bottom-right-radius", tokens[3]);
					}

					break;
				}
				case PropertyType::Shadow:
				{
					bool hasVariables = false;

					for(auto& t: tokens)
					{
						if(getPropertyType(t) == PropertyType::Variable)
						{
							hasVariables = true;
							break;
						}
					}

					if(hasVariables)
					{
						// We need to defer the parsing of the shadow until the variable is resolved
						String s;

						for(auto& t: tokens)
							s << "|" << t;

						addOrOverwrite(PropertyType::Shadow, isImportant, rv.property, s);
					}
					else
					{
						ShadowParser bp(tokens);
						addOrOverwrite(PropertyType::Shadow, isImportant, rv.property, bp.toParsedString());
					}
					
					break;
				}
				case PropertyType::Transform:
				{
					for(const auto& t: tokens)
					{
						addOrOverwrite(PropertyType::Transform, isImportant, rv.property, t);
							
					}

					break;
				}
				case PropertyType::Font:
				{
						jassertfalse; // soon
					break;
				}	
				case PropertyType::Border:
				case PropertyType::Colour:
				{
					for(auto t: tokens)
					{
						auto suffix = getTokenSuffix(p, rv.property, t);
						addOrOverwrite(p, isImportant, rv.property + suffix, processValue(t));
					}

					break;
				}
				default: ;
				}
			}
			else
			{
				auto t = tokens[0];
				auto suffix = getTokenSuffix(p, rv.property, t);

				if(rv.property == "background-image" && t.startsWith("linear-gradient"))
					addOrOverwrite(p, isImportant, "background-color", processValue(t));
				else if (p == PropertyType::Transition)
				{
					Transition tr;
					tr.active = true;
					tr.duration = processValue(t, ValueType::Time).getDoubleValue();

					for(auto& l: thisStates)
					{
						if(l.isDefaultState())
							n->setDefaultTransition(l.element, tr);
					}
					
					transitions["all"] = tr;
				}
				else
					addOrOverwrite(p, isImportant, rv.property + suffix, processValue(t));
			}
		}
		
		for(auto& t: transitions)
		{
			PropertyKey k;
			k.name =t.first;

			for(const auto& l: thisStates)
			{
				k.state = l;

				for(auto& p: n->properties[(int)l.element])
				{
					if(k.looseMatch(p.name))
					{
						for(auto& pv: p.values)
						{
							if(pv.first == k.state.stateFlag)
							{
								pv.second.transition = t.second;

								if(!p.values[0].second)
									p.values[0].second.transition = t.second;

								break;
							}
						}
					}
				}
			}
		}

		list.add(n);
	}

	return StyleSheet::Collection(list);
}

Array<Selector> Parser::getSelectors() const
{
	Array<Selector> s;

	for(const auto& r: rawClasses)
	{
		for(const auto& v: r.selectors)
		{
			for(const auto& v2: v)
			{
				s.addIfNotAlreadyThere(v2.first);
			}
		}
	}

	return s;
}
} // simple_css
} // hise


