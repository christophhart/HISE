/*
  ==============================================================================

    CssParser.h
    Created: 5 Mar 2024 9:46:58pm
    Author:  chris

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace hise {
namespace simple_css
{
using namespace juce;

/** TODO:
 *
 * - add font stuff
 * - add !important
 * - add box-shadow
 * - add selectors (input, button, select,  :hover, :active, :focus)
 * 
 * - cache stuff (after profiling!)
 * - add JSON converter
 * - add layout stuff (replace layoutdata?): width, height, display (none or inherit/init/default)
 * - write LAF for all stock elements
 * - add code editor language manager
 */

namespace PropertyIds
{
#define DECLARE_ID(x) static const Identifier x(#x);
	DECLARE_ID(margin);
	DECLARE_ID(padding);
#undef DECLARE_ID
}

enum class PropertyType
{
	Positioning,
	Border,
	BorderRadius,
	Colour,
	Transition,
	Font
};

enum class SelectorType
{
	None,
	Type,
	Class,
	ID,
	All
};

enum class ElementType
{
	Body,
	Button,
	TextInput,
	Selector,
	Panel
};

enum class PseudoClassType
{
	None = 0,
	Hover = 1,
	Active = 2,
	Focus = 4
};

enum class ValueType
{
	Undefined,
	Colour,
	Gradient,
	AbsoluteSize,
	RelativeSize,
	numValueTypes
};

struct ColourParser
{
	static Colour getColourFromHardcodedString(const String& colourId);

	ColourParser(const String& value);

	Colour getColour() const
	{
		return c;
	}

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

	double sampleCurveX(double t) const {
	    return ((ax * t + bx) * t + cx) * t;
	}
	double sampleCurveY(double t) const {
	    return ((ay * t + by) * t + cy) * t;
	}
	double sampleCurveDerivativeX(double t) const {
	    return (3.0 * ax * t + 2.0 * bx) * t + cx;
	}

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

struct Animator;

struct StyleSheet: public ReferenceCountedObject
{
	struct Selector
	{
		Selector() = default;

		explicit Selector(ElementType dt)
		{
			type = SelectorType::Type;

			switch(dt)
			{
			case ElementType::Body: name = "body"; break;
			case ElementType::Button: name = "button"; break;
			case ElementType::TextInput: name = "input"; break;
			case ElementType::Selector: name = "select"; break;
			case ElementType::Panel: name = "div"; break;
			default: ;
			}
		}

		using RawList = std::vector<std::pair<Selector, int>>;

		String toString() const
		{
			String s;

			switch(type)
			{
			case SelectorType::None: break;
			case SelectorType::Type: break;
			case SelectorType::Class: s << '.'; break;
			case SelectorType::ID: s << '#'; break;
			case SelectorType::All: s << '*'; break;
			default: ;
			}

			s << name;

			return s;
		}

		std::pair<bool, int> matchesRawList(const RawList& blockSelectors) const
		{
			for(auto& bs: blockSelectors)
			{
				if(bs.first == *this)
					return { true, bs.second };
			}

			return { false, (int)PseudoClassType::None };
		}

		bool operator==(const Selector& other) const
		{
			if(type == SelectorType::All || other.type == SelectorType::All)
				return true;

			if(type == other.type)
			{
				return name == other.name;
			}

			return false;
		}

		SelectorType type;
		String name;
	};

	struct Transition
	{
		String toString() const
		{
			String s;

			if(active)
			{
				s << " tr(";
				s << "dur:" << String(duration, 2) << "s, ";
				s << "del:" << String(duration, 2) << "s";

				if(f)
					s << ", f: true";

				s << ')';
			}

			return s;
		}

		operator bool() const { return active && (duration > 0.0 || delay > 0.0); }

		bool active = false;
		double duration = 0.0;
		double delay = 0.0;
		std::function<double(double)> f;
	};

	struct PropertyKey
	{
		PropertyKey withSuffix(const String& suffix) const
		{
			PropertyKey copy(*this);
			copy.appendSuffixIfNot(suffix);
			return copy;
		}

		bool operator==(const PropertyKey& other) const
		{
			return name == other.name && state == other.state;
		}

		bool looseMatch(const String& other) const
		{
			if(name == "all")
				return true;

			if(other == name)
				return true;

			if(other.startsWith(name) || name.startsWith(other))
				return true;
		}

		void appendSuffixIfNot(const String& suffix)
		{
			if(!name.endsWith(suffix))
				name << '-' << suffix;
		}

		String name;
		int state;
	};

	struct TransitionValue
	{
		operator bool() const { return active; }

		bool active = false;
		String startValue;
		String endValue;
		double progress = 0.0;
	};

	struct PropertyValue
	{
		PropertyValue() = default;

		PropertyValue(PropertyType pt, const String& v):
		  type(pt),
		  valueAsString(v)
		{};

		operator bool() const { return valueAsString.isNotEmpty() && valueAsString != "default"; }

		String toString() const
		{
			String s;
			s << valueAsString;
			s << transition.toString();
			return s;
		}

		PropertyType type;
		Transition transition;
		String valueAsString;
	};

	struct Property
	{
		String toString() const
		{
			String s;

			s << "  " << name;

			String intend;

			for(int i = 0; i < s.length(); i++)
				intend << " ";

			bool first = true;

			for(const auto& v: values)
			{
				if(!first)
					s << intend;

				s << "[" << String((int)v.first) << "]: " << v.second.toString() << "\n";
				first = false;
			}

			return s;
		}

		PropertyValue getProperty(int stateFlag) const
		{
			for(const auto& v: values)
			{
				if(v.first == stateFlag)
					return v.second;
			}

			int highestMatch = 0;
			int bestMatch = 0;

			// check one overlap
			for(const auto& v: values)
			{
				auto thisMatch = v.first & stateFlag;

				if(thisMatch > highestMatch)
				{
					highestMatch = thisMatch;
					bestMatch = v.first;
				}
			}

			return values.at(bestMatch);
		}

		String name;
		std::map<int, PropertyValue> values;
	};

	TransitionValue getTransitionValue(const PropertyKey& key) const;

	PropertyValue getPropertyValue(const PropertyKey& key) const 
	{
		for(const auto& p: propertiesNEW)
		{
			if(p.name == key.name)
				return p.getProperty(key.state);
		}

		return {};
	}

	using Ptr = ReferenceCountedObjectPtr<StyleSheet>;
	using List = ReferenceCountedArray<StyleSheet>;

	struct Collection
	{
		Collection() = default;

		Collection(List l):
		 list(l)
		{
			// TODO: sort so that it takes the best match first
		}

		operator bool() const { return !list.isEmpty(); }

		void setAnimator(Animator* a)
		{
			for(auto l: list)
				l->animator = a;
		}

		Ptr operator[](const Selector& s) const
		{
			for(auto l: list)
			{
				if(l->selectorNEW == s)
					return l;
			}

			return nullptr;
		}

		String toString() const
		{
			String listContent;

			for(auto cs : list)
			{
				listContent << cs->selectorNEW.toString() << " {\n";

				for(const auto& p: cs->propertiesNEW)
					listContent << p.toString();

				listContent << "}\n";
			}

			return listContent;
		}

	private:

		List list;
	};

	Animator* animator;

	Path getBorderPath(Rectangle<float> totalArea, int stateFlag) const;

	float getPixelValue(Rectangle<float> totalArea, const PropertyKey& key) const;

	Rectangle<float> getArea(Rectangle<float> totalArea, const PropertyKey& key) const;
	
	std::pair<Colour, ColourGradient> getColourOrGradient(Rectangle<float> area, PropertyKey key, Colour defaultColour=Colours::transparentBlack)
	{
		key.appendSuffixIfNot("color");

		auto getValueFromString = [&](const String& v)
		{
			if(v.startsWith("linear-gradient"))
			{
				ColourGradient grad;
				ColourGradientParser p(area, v.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false));
				return std::pair(Colours::transparentBlack, p.getGradient());
			}
			else
			{
				auto c = Colour((uint32)v.getHexValue64());
				return std::pair(c, ColourGradient());
			}
		};

		if(auto tv = getTransitionValue(key))
		{
			auto v1 = getValueFromString(tv.startValue);
			auto v2 = getValueFromString(tv.endValue);

			if(v1.second.getNumColours() > 0 || v2.second.getNumColours() > 0)
			{
				// Can't blend gradients (for now...)
				jassertfalse;
				return { defaultColour, ColourGradient() };
			}

			return { v1.first.interpolatedWith(v2.first, tv.progress), ColourGradient() };
		}
		if(auto v = getPropertyValue(key))
		{
			return getValueFromString(v.valueAsString);
		}
		
		return { defaultColour, ColourGradient() };
	}

	Selector selectorNEW;
	std::vector<Property> propertiesNEW;
};



struct Parser
{
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
		CloseParen,
		ValueString,
		numTokenTypes
	};

	static String getTokenName(TokenType t)
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
	}

	Parser(const String& cssCode):
	  code(cssCode),
	  ptr(code.begin()),
	  end(code.end())
	{
		
	};

	void skip()
	{
		if(ptr == end)
			return;

		while(CharacterFunctions::isWhitespace(*ptr))
			++ptr;
	}

	bool match(TokenType t)
	{
		if(!matchIf(t))
			throwError("Expected token: " + getTokenName(t));

		return true;
	}

	void throwError(const String& errorMessage)
	{
		String error = getLocation();
		error << errorMessage;

		throw Result::fail(error);
	}
	
	bool matchIf(TokenType t)
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
		case Dot:		   return matchChar('.');
		case Hash:		   return matchChar('#');
		case OpenParen:	   return matchChar('(');
		case CloseParen:   return matchChar(')');
		case Colon:        return matchChar(':');
		case Semicolon:    return matchChar(';');
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
				if(matchIf(OpenParen))
				{
					currentToken << '(';

					while(ptr != end && *ptr != ')')
						currentToken << *ptr++;

					match(CloseParen);

					currentToken << ')';

					break;
				}
				else
					currentToken << *ptr++;
			}

			currentToken = currentToken.trim();
			return currentToken.isNotEmpty();
		}
		case numTokenTypes: break;
		default: ;
		}
	}

	struct RawLine
	{
		String property;
		std::vector<String> items;
	};

	struct RawClass
	{
		StyleSheet::Selector::RawList selector;
		std::vector<RawLine> lines;
	};

	String currentToken;
	
	std::vector<RawClass> rawClasses;

	int parsePseudoClass()
	{
		int state = 0;

		while(matchIf(TokenType::Colon))
		{
			match(TokenType::Keyword);
			
			if(currentToken == "active")
			{
				state |= (int)PseudoClassType::Active;
				state |= (int)PseudoClassType::Hover;
			}
			
			if(currentToken == "hover")
				state |= (int)PseudoClassType::Hover;

			if(currentToken == "focus")
				state |= (int)PseudoClassType::Focus;

			skip();
		}

		return state;
	}

	RawClass parseSelectors()
	{
		RawClass newClass;

		skip();

		while(ptr != end && *ptr != '{')
		{
			StyleSheet::Selector ns;

			if(matchIf(TokenType::Dot))
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
				match(TokenType::Keyword);
				ns.name = currentToken;
				ns.type = SelectorType::Type;
			}

			newClass.selector.push_back({ns, parsePseudoClass() });

			skip();
		}

		return newClass;
	}

	Result parse()
	{
		try
		{
			while(ptr != end)
			{
				auto newClass = parseSelectors();

				match(TokenType::OpenBracket);

				while(matchIf(TokenType::Keyword))
				{
					RawLine newLine;
					newLine.property = currentToken;
					
					match(TokenType::Colon);

					while(ptr != end)
					{
						match(TokenType::ValueString);

						newLine.items.push_back(currentToken);

						if(matchIf(TokenType::Semicolon))
							break;
					}
					
					auto currentValue = currentToken;
					
					newClass.lines.push_back(std::move(newLine));
				}

				match(TokenType::CloseBracket);

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

	String getLocation()
	{
		int line = 0;
		int col = 0;

		auto s = code.begin();

		while(s != ptr)
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
	
	static ValueType findValueType(const String& value)
	{
		static const StringArray colourPrefixes = { "#", "rgba(", "hsl(", "rgb(" };

		for(const auto& cp: colourPrefixes)
		{
			if(value.startsWith(cp))
				return ValueType::Colour;
		}

		if(!ColourParser::getColourFromHardcodedString(value).isTransparent())
			return ValueType::Colour;

		if(value.startsWith("linear-gradient"))
			return ValueType::Gradient;
		
		return ValueType::Undefined;
	}

	static String processValue(const String& value, ValueType t=ValueType::Undefined)
	{
		if(t == ValueType::Undefined)
		{
			t = findValueType(value);
		}

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
		case ValueType::AbsoluteSize: break;
		case ValueType::RelativeSize: break;
		case ValueType::numValueTypes: break;
		default: ;
		}

		return value;
	}
	
	static String getTokenSuffix(PropertyType p, const String& keyword, String& token)
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
			case PropertyType::Font: return "-height";
			case PropertyType::Border: return "-width";
			case PropertyType::Positioning: return "";
			default: jassertfalse; return "-size";
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

	static PropertyType getPropertyType(const String& p)
	{
		if(p.startsWith("border"))
		{
			if(p.endsWith("radius"))
				return PropertyType::BorderRadius;
			else
				return PropertyType::Border;
		}
		if(p.startsWith("padding"))
			return PropertyType::Positioning;

		if(p.startsWith("margin"))
			return PropertyType::Positioning;

		if(p.startsWith("layout"))
			return PropertyType::Positioning;

		if(p.startsWith("background"))
			return PropertyType::Colour;

		if(p.startsWith("transition"))
			return PropertyType::Transition;

		if(p.startsWith("font"))
			return PropertyType::Font;
	}

	static std::function<double(double)> parseTimingFunction(const String& t)
	{
		std::map<String, std::function<double(double)>> curves;

		curves["ease"] = BezierCurve({0.25,0.1} ,{0.25,1});
		curves["linear"] = [](double v){ return v; };
		curves["ease-in"] = BezierCurve({0.42,0.0}, {1.0, 1.0} );
		curves["ease-out"] = BezierCurve({0.0,0.0}, {0.58, 1.0} );
		curves["ease-in-out"] = BezierCurve({0.42,0.0}, {0.58, 1.0} );

		if(curves.find(t) != curves.end())
			return curves.at(t);

		return {};
	}

	StyleSheet::Collection getCSSValues() const
	{
		StyleSheet::List list;

		for(const auto& rc: rawClasses)
		{
			StyleSheet::List thisList;

			int currentPseudoClass = (int)PseudoClassType::None;

			for(auto l: list)
			{
				auto fit = l->selectorNEW.matchesRawList(rc.selector);

				if(fit.first)
				{
					thisList.add(l);
					currentPseudoClass = fit.second;
					break;
				}
			}

			if(thisList.isEmpty())
			{
				for(auto s: rc.selector)
				{
					auto p = new StyleSheet();
					p->selectorNEW = s.first;
					currentPseudoClass = s.second;
					thisList.add(p);
				}
			}

			auto addOrOverwrite = [&](PropertyType pt, const String& k, const String& v)
			{
				for(auto l: thisList)
				{
					bool found = false;

					for(auto& p: l->propertiesNEW)
					{
						if(p.name == k)
						{
							p.values[(int)currentPseudoClass] = { pt, v };
							found = true;
							break;
						}
					}

					if(!found)
					{
						StyleSheet::Property np;
						np.name = k;
						np.values[(int)currentPseudoClass] = { pt, v };

						if(currentPseudoClass != 0)
							np.values[0] = { pt, "" }; 

						auto containsDefault = np.values.find(0) == np.values.end();
						
						l->propertiesNEW.push_back(np);
					}
				}
			};
			
			std::map<String, StyleSheet::Transition> transitions;

			for(const auto& rv: rc.lines)
			{
				auto p = getPropertyType(rv.property);
				
				auto isMultiValue = rv.items.size() > 1;
				
				if(p == PropertyType::Positioning)
					isMultiValue = !rv.property.containsChar('-');

				if(p == PropertyType::BorderRadius)
					isMultiValue = rv.property == "border-radius";

				if(isMultiValue)
				{
					auto& tokens = rv.items;

					switch(p)
					{
					case PropertyType::Positioning:
					{
						if(tokens.size() == 1)
						{
							addOrOverwrite(p, rv.property + "-top", tokens[0]);
							addOrOverwrite(p, rv.property + "-bottom", tokens[0]);
							addOrOverwrite(p, rv.property + "-left", tokens[0]);
							addOrOverwrite(p, rv.property + "-right", tokens[0]);
						}
						if(tokens.size() == 2)
						{
							// tokens[0] = t, b
							// tokens[1] = r, l
							addOrOverwrite(p, rv.property + "-top", tokens[0]);
							addOrOverwrite(p, rv.property + "-bottom", tokens[0]);
							addOrOverwrite(p, rv.property + "-left", tokens[1]);
							addOrOverwrite(p, rv.property + "-right", tokens[1]);
						}
						if(tokens.size() == 4)
						{
							// tokens = [t, r, b, l]
							addOrOverwrite(p, rv.property + "-top", tokens[0]);
							addOrOverwrite(p, rv.property + "-bottom", tokens[1]);
							addOrOverwrite(p, rv.property + "-left", tokens[2]);
							addOrOverwrite(p, rv.property + "-right", tokens[3]);
						}

						break;
					}
					case PropertyType::Transition:
					{
						auto transitionTarget = tokens[0];
						
						StyleSheet::Transition t;
						t.active = true;
						t.duration = tokens[1].getDoubleValue();

						if(tokens.size() > 2)
							t.delay = tokens[2].getDoubleValue();

						if(tokens.size() > 3)
							t.f = parseTimingFunction(tokens[3]);


						transitions[transitionTarget] = t;
						
						break;
					}
					case PropertyType::BorderRadius:
					{
						if(tokens.size() == 1)
						{
							addOrOverwrite(p, "border-top-left-radius", tokens[0]);
							addOrOverwrite(p, "border-top-right-radius", tokens[0]);
							addOrOverwrite(p, "border-bottom-left-radius", tokens[0]);
							addOrOverwrite(p, "border-bottom-right-radius", tokens[0]);
						}
						if(tokens.size() == 2)
						{
							addOrOverwrite(p, "border-top-left-radius", tokens[0]);
							addOrOverwrite(p, "border-top-right-radius", tokens[1]);
							addOrOverwrite(p, "border-bottom-left-radius", tokens[1]);
							addOrOverwrite(p, "border-bottom-right-radius", tokens[0]);
						}
						if(tokens.size() == 4)
						{
							addOrOverwrite(p, "border-top-left-radius", tokens[0]);
							addOrOverwrite(p, "border-top-right-radius", tokens[1]);
							addOrOverwrite(p, "border-bottom-left-radius", tokens[2]);
							addOrOverwrite(p, "border-bottom-right-radius", tokens[3]);
						}

						break;
					}
					case PropertyType::Border:
					case PropertyType::Colour:
					case PropertyType::Font:
					{
						for(auto t: tokens)
						{
							auto suffix = getTokenSuffix(p, rv.property, t);
							addOrOverwrite(p, rv.property + suffix, processValue(t));
						}

						break;
					}
					default: ;
					}
				}
				else
				{
					auto t = rv.items[0];
					auto suffix = getTokenSuffix(p, rv.property, t);
					addOrOverwrite(p, rv.property + suffix, processValue(t));
				}
			}

			for(auto& t: transitions)
			{
				StyleSheet::PropertyKey k;
				k.name =t.first;
				k.state = currentPseudoClass;

				for(auto l: thisList)
				{
					for(auto& p: l->propertiesNEW)
					{
						if(k.looseMatch(p.name))
						{
							if(p.values.find(k.state) != p.values.end())
								p.values[k.state].transition = t.second;
						}
					}
				}
			}

			for(auto l: thisList)
				list.addIfNotAlreadyThere(l);
		}

		return StyleSheet::Collection(list);
	}

	String code;
	String::CharPointerType ptr, end;
};


struct Animator: public Timer
{
	struct ScopedComponentSetter
	{
		ScopedComponentSetter(Animator& parent_, Component* c):
		  parent(parent_),
		  prev(parent.currentlyRenderedComponent)
		{
			parent.currentlyRenderedComponent = c;
		}

		~ScopedComponentSetter()
		{
			parent.currentlyRenderedComponent = prev;
		}

		Animator& parent;
		Component::SafePointer<Component> prev;
	};

	Component::SafePointer<Component> currentlyRenderedComponent;

	struct Item
	{
		Item() = default;

		Item(Animator& parent, StyleSheet::Ptr css_, StyleSheet::Transition tr_):
		  css(css_),
		  transitionData(tr_),
		  target(parent.currentlyRenderedComponent)
		{
			DBG("START");
		};

		~Item()
		{
			DBG("STOP");
		}

		bool timerCallback()
		{
			auto d = 1.0 * 0.03;

			if(transitionData.duration > 0.0)
				d /= transitionData.duration;

			if(reverse)
				d *= -1.0;

			currentProgress += d;

			if(currentProgress > 1.0 || currentProgress < 0.0)
			{
				currentProgress = jlimit(0.0, 1.0, currentProgress);
				return false;
			}

			DBG(currentProgress);

			target->repaint();
			return true;
		}

		Component::SafePointer<Component> target;

		StyleSheet::Ptr css;
		StyleSheet::Transition transitionData;

		StyleSheet::PropertyKey startValue;
		StyleSheet::PropertyKey endValue;
		
		double currentProgress = 0.0;
		bool reverse = false;
		int waitCounter = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Item);
	};

	Animator()
	{
		startTimer(15);
	}

	void timerCallback() override
	{
		for(int i = 0; i < items.size(); i++)
		{
			if(!items[i]->timerCallback())
				items.remove(i--);
		}
	}

	OwnedArray<Item> items;
};

struct StateWatcher
{
	struct Item
	{
		std::pair<bool, int> changed(int stateFlag)
		{
			if(stateFlag != currentState)
			{
				auto prevState = currentState;
				currentState = stateFlag;

				return { true, prevState };
			}

			return { false, currentState };
		}

		Component::SafePointer<Component> c;
		int currentState = 0;
		
	};

	std::pair<bool, int> changed(Component* c, int stateFlag)
	{
		for(auto& i: items)
		{
			if(i.c == c)
				return i.changed(stateFlag);
		}

		items.add({ c, stateFlag });
		return { false, stateFlag };
	}

	Array<Item> items;
};

struct Renderer
{
	Renderer(StyleSheet::Collection styleSheet, Animator* animator_):
	  list(styleSheet),
	  animator(animator_)
	{
		list.setAnimator(animator);
	};

	operator bool() const
	{
		return list;
	}

	StyleSheet::Collection list;

	Animator* animator = nullptr;

	
	void setCurrentBrush(Graphics& g, StyleSheet::Ptr ss, Rectangle<float> area, const StyleSheet::PropertyKey& key)
	{
		if (ss != nullptr)
		{
			auto c = ss->getColourOrGradient(area, key, Colours::transparentBlack);

			if(c.second.getNumColours() > 0)
				g.setGradientFill(c.second);
			else
				g.setColour(c.first);
		}
	}

	static int getPseudoClassFromComponent(Component* c)
	{
		int state = 0;

		auto isHover = c->isMouseOverOrDragging(false);
		auto isDown = c->isMouseButtonDown(false);
		auto isFocus = c->hasKeyboardFocus(false);
		
		if(isHover)
			state |= (int)PseudoClassType::Hover;

		if(isDown)
			state |= (int)PseudoClassType::Active;

		if(isFocus)
			state |= (int)PseudoClassType::Focus;

		return state;
	}

	void drawBackground(Graphics& g, Rectangle<float> area, const StyleSheet::Selector& s, Component* c)
	{
		if(auto ss = list[s])
		{
			auto stateFlag = getPseudoClassFromComponent(c);

			auto ma = ss->getArea(area, {"margin", stateFlag});
			auto p = ss->getBorderPath(ma, stateFlag);

			setCurrentBrush(g, ss, ma, {"background", stateFlag});
			
			g.fillPath(p);

			auto strokeSize = ss->getPixelValue(ma, {"border-width", stateFlag});

			if(strokeSize > 0.0f)
			{
				setCurrentBrush(g, ss, ma, {"border", stateFlag});
				g.strokePath(p, PathStrokeType(strokeSize));
			}
		}
	}
	
};

struct Editor: public Component
{
	Editor():
	  doc(jdoc),
	  editor(doc)
	{
		setRepaintsOnMouseActivity(true);
		setSize(1600, 800);

		addAndMakeVisible(editor);
		addAndMakeVisible(list);

		list.setLookAndFeel(&laf);
		laf.setTextEditorColours(list);

		list.setMultiLine(true);
		list.setReadOnly(true);
		list.setFont(GLOBAL_MONOSPACE_FONT());

		context.attachTo(*this);
	}

	~Editor()
	{
		context.detach();
	}

	hise::GlobalHiseLookAndFeel laf;

	bool keyPressed(const KeyPress& key) override
	{
		if(key == KeyPress::F5Key)
		{
			compile();
			return true;
		}

		return false;
	}

	

	void compile()
	{
		Parser p(jdoc.getAllContent());

		auto ok = p.parse();

		editor.setError(ok.getErrorMessage());

		css = p.getCSSValues();

		auto listContent = css.toString();

		list.setText(listContent, dontSendNotification);

		repaint();
	}

	Rectangle<float> previewArea;

	void resized() override
	{
		auto b = getLocalBounds();

		previewArea = b.removeFromRight(b.getWidth() / 3).toFloat();

		previewArea.removeFromRight(10);

		list.setBounds(b.removeFromRight(b.getWidth() / 3));

		editor.setBounds(b);
	}

	Animator animator;
	StateWatcher stateWatcher;
	
	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black);

		Animator::ScopedComponentSetter scs(animator, this);

		Renderer r(css, &animator);
		
		if(r)
		{
			StyleSheet::Selector selector(ElementType::Body);

			auto currentState = Renderer::getPseudoClassFromComponent(this);
			auto changed = stateWatcher.changed(this, currentState);

			if(changed.first)
			{
				if(auto ss = css[selector])
				{
					for(const auto& p: ss->propertiesNEW)
					{
						if(p.values.find(changed.second) != p.values.end() &&
						   p.values.find(currentState) != p.values.end())
						{
							auto prev = p.values.at(changed.second);
							auto current = p.values.at(currentState);

							if(prev.transition || current.transition)
							{
								auto thisTransition = current.transition ? current.transition : prev.transition;

								StyleSheet::PropertyKey thisStartValue = { p.name, changed.second };
								StyleSheet::PropertyKey thisEndValue = { p.name, currentState };

								bool found = false;

								for(auto i: animator.items)
								{
									if(i->css == ss &&
									   i->target == animator.currentlyRenderedComponent &&
									   i->startValue.name == p.name)
									{
										if(currentState == i->startValue.state)
										{
											i->reverse = !i->reverse;
											found = true;
											break;
										}
										else
										{
											i->endValue.state = currentState;
											i->transitionData = thisTransition;
											found = true;
											break;
										}
									}
								}

								if(found)
									continue;

								auto ad = new Animator::Item(animator, ss, thisTransition);
								
								ad->startValue = thisStartValue;
								ad->endValue = thisEndValue;
								
								animator.items.add(ad);
							}
						}
					}
				}
			}

			//r.checkAnimation(ElementType::Body, state, this);
			r.drawBackground(g, previewArea, selector, this);
		}

		auto b = getLocalBounds().removeFromRight(10).toFloat();

		if(auto i = animator.items.getFirst())
		{
			g.setColour(Colours::white.withAlpha(0.4f));
			g.fillRect(b);
			b = b.removeFromBottom(i->currentProgress * b.getHeight());
			g.fillRect(b);
		}
		

	}

	StyleSheet::Collection css;

	juce::CodeDocument jdoc;
	mcl::TextDocument doc;
	mcl::TextEditor editor;
	juce::TextEditor list;
	OpenGLContext context;
};

	
} // namespace simple_css
} // namespace hise