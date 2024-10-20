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

Colour ColourParser::getColourFromHardcodedString(const String& colourId)
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
			return Colour(c.colour);
		}
	}

	return Colours::transparentBlack;
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

		auto r = values[0].getIntValue();
		auto g = values[1].getIntValue();
		auto b = values[2].getIntValue();
		auto a = values.size() > 3 ? roundToInt(values[3].getFloatValue() * 255) : 255;
		c = value.startsWith("hsl") ? Colour::fromHSL((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f) : Colour::fromRGBA(r, g, b, a);
	}
	else
	{
		c = getColourFromHardcodedString(value);
	}
}

ColourGradientParser::ColourGradientParser(Rectangle<float> area, const String& items)
{
	auto tokens = StringArray::fromTokens(items, ",", "()");
	tokens.trim();

	int colourIndex = 0;

	if(tokens[0].startsWith("to "))
	{
		colourIndex++;

		if(tokens[0].endsWith("left"))
		{
			gradient.point1 = area.getTopRight();
			gradient.point2 = area.getTopLeft();
		}
		if(tokens[0].endsWith("right"))
		{
			gradient.point1 = area.getTopLeft();
			gradient.point2 = area.getTopRight();
		}
		if(tokens[0].endsWith("top"))
		{
			gradient.point1 = area.getBottomLeft();
			gradient.point2 = area.getTopLeft();
		}
		if(tokens[0].endsWith("bottom"))
		{
			gradient.point1 = area.getTopLeft();
			gradient.point2 = area.getBottomLeft();
		}
	}
	else if (tokens[0].endsWith("deg"))
	{
		gradient.point1 = area.getTopLeft();
		gradient.point2 = area.getBottomLeft();

		auto rad = (float)tokens[0].getIntValue() / 180.0f * float_Pi;

		auto t = AffineTransform::rotation(rad, area.getCentreX(), area.getCentreY());

		gradient.point1.applyTransform(t);
		gradient.point2.applyTransform(t);

		gradient.point1 = area.getConstrainedPoint(gradient.point1);
		gradient.point2 = area.getConstrainedPoint(gradient.point2);

		colourIndex++;
	}
	else
	{
		gradient.point1 = area.getTopLeft();
		gradient.point2 = area.getBottomLeft();
	}

	for(int i = colourIndex; i < tokens.size(); i++)
	{
		auto colourTokens = StringArray::fromTokens(tokens[i], " ", "()");
			
		if(colourTokens.size() > 1)
		{
			auto c = ColourParser(colourTokens[0]).getColour();

			if(gradient.getNumColours() == 0)
				gradient.addColour(0.0f, c);

			for(int j = 1; j< colourTokens.size(); j++)
			{
				float proportion = (float)colourTokens[j].getIntValue() / 100.0f;
				gradient.addColour(proportion, c);
			}
		}
		else
		{
			auto c = ColourParser(tokens[i]).getColour();

			float proportion = (float)(i - colourIndex) / (float)(tokens.size() - colourIndex - 1);
			gradient.addColour(proportion, c);
		}
	}
}

StyleSheet::TransitionValue StyleSheet::getTransitionValue(const PropertyKey& key) const
{
	TransitionValue tv;

	if(animator == nullptr)
		return tv;

	for(const auto i: animator->items)
	{
		if(i->target != animator->currentlyRenderedComponent)
			continue;

		if(i->css != this)
			continue;

		if(i->startValue.name == key.name)
		{
			tv.startValue = getPropertyValue(i->startValue).valueAsString;
			tv.endValue = getPropertyValue(i->endValue).valueAsString;
			tv.progress = i->currentProgress;

			if(i->transitionData.f)
				tv.progress = i->transitionData.f(tv.progress);

			tv.active = true;
			break;
		}
	}

	return tv;
}

Path StyleSheet::getBorderPath(Rectangle<float> totalArea, int stateFlag) const
{
	float corners[4];
		
	corners[0] = getPixelValue(totalArea, {"border-top-left-radius", stateFlag});
	corners[1] = getPixelValue(totalArea, {"border-top-right-radius", stateFlag});
	corners[2] = getPixelValue(totalArea, {"border-bottom-left-radius", stateFlag});
	corners[3] = getPixelValue(totalArea, {"border-bottom-right-radius", stateFlag});

	float empty[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	Path p;

	if(memcmp(corners, empty, sizeof(float) * 4) == 0)
	{
		p.addRectangle(totalArea);
	}
	else
	{
		float csx, csy, x, y, cex, cey;

		{
			x = totalArea.getX();
			y = totalArea.getY();
			csx = x;
			csy = totalArea.getY() + corners[0];
			cex = totalArea.getX() + corners[0];
			cey = totalArea.getY();

			p.startNewSubPath(csx, csy);
			p.quadraticTo(x, y, cex, cey);
		}

		{
			x = totalArea.getRight();
			y = totalArea.getY();

			csx = x - corners[1];
			csy = y;
			cex = x;
			cey = y + corners[1];

			p.lineTo(csx, csy);
			p.quadraticTo(x, y, cex, cey);
		}

		{
			x = totalArea.getRight();
			y = totalArea.getBottom();
			csx = x;
			csy = y - corners[3];
			cex = x - corners[3];
			cey = y;

			p.lineTo(csx, csy);
			p.quadraticTo(x, y, cex, cey);
		}

		{
			x = totalArea.getX();
			y = totalArea.getBottom();
			csx = x + corners[2];
			csy = y;
			cex = x;
			cey = y - corners[2];

			p.lineTo(csx, csy);
			p.quadraticTo(x, y, cex, cey);
		}

		p.closeSubPath();
	}

	return p;
}

float StyleSheet::getPixelValue(Rectangle<float> totalArea, const PropertyKey& key) const
{
	auto getValueFromString = [&](const String& v)
	{
		if(v.endsWithChar('x'))
			return (float)v.getIntValue();

		auto useHeight = key.name.contains("top") || key.name.contains("bottom");

		if(v == "auto")
			return useHeight ? totalArea.getHeight() : totalArea.getWidth();
		if(v.endsWithChar('%'))
			return totalArea.getHeight() * (float)v.getIntValue() * 0.01f;
	};

	if(auto tv = getTransitionValue(key))
	{
		auto v1 = getValueFromString(tv.startValue);
		auto v2 = getValueFromString(tv.endValue);
		return Interpolator::interpolateLinear(v1, v2, (float)tv.progress);
	}
	if(auto v = getPropertyValue(key))
	{
		return getValueFromString(v.valueAsString);
	}
	
	return 0.0f;
}

Rectangle<float> StyleSheet::getArea(Rectangle<float> totalArea, const PropertyKey& key) const
{
	auto oa = totalArea;
	totalArea.removeFromLeft(getPixelValue(oa, key.withSuffix("left")));
	totalArea.removeFromTop(getPixelValue(oa, key.withSuffix("top")));
	totalArea.removeFromBottom(getPixelValue(oa, key.withSuffix("bottom")));
	totalArea.removeFromRight(getPixelValue(oa, key.withSuffix("right")));

	return totalArea;
}
}

}


