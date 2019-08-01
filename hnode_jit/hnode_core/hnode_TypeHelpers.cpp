namespace hnode {
using namespace juce;

#ifndef HNODE_ALLOW_DOUBLE_FLOAT_MIX
#define HNODE_ALLOW_DOUBLE_FLOAT_MIX 1
#endif

void Types::Helpers::convertFloatToDouble(double* dst, const float* src, int numSamples)
{
	for (int i = 0; i < numSamples; i++)
	{
		dst[i] = (double)src[i];
	}
}

void Types::Helpers::convertDoubleToFloat(float* dst, const double* src, int numSamples)
{
	for (int i = 0; i < numSamples; i++)
	{
		dst[i] = (float)src[i];
	}
}

hnode::Types::ID Types::Helpers::getTypeFromTypeName(const String& cppTypeName)
{
	if (cppTypeName == "double") return Types::ID::Double;
	if (cppTypeName == "float") return Types::ID::Float;
	if (cppTypeName == "int") return Types::ID::Integer;
	if (cppTypeName == "bool") return Types::ID::Integer;
	
	jassertfalse;
	return Types::ID::Void;
}

hnode::Types::ID Types::Helpers::getTypeFromVariableName(const String& name)
{
	auto typeChar = name.toLowerCase()[0];

	switch (typeChar)
	{
	case 'b':	return Types::ID::Block;
	case 'f':	return Types::ID::Float;
	case 'd':	return Types::ID::Double;
	case 'e':	return Types::ID::Event;
	case 'i':	return Types::ID::Integer;
	case 'n':	return Types::ID::Number;
	case 'a':	return Types::ID::Dynamic;
	default:	jassertfalse; return Types::ID::Void;
	}
}

juce::String Types::Helpers::getVariableName(ID id, int index)
{
	String name;
	name << getTypeChar(id) << String(index + 1);

	return name;
}

juce::String Types::Helpers::getTypeName(ID id)
{
	switch (id)
	{
	case Types::ID::Void:			return "void";
	case Types::ID::Integer:		return "int";
	case Types::ID::Float:			return "float";
	case Types::ID::Double:			return "double";
	case Types::ID::FpNumber:		return "double";
	case Types::ID::Event:			return "event";
	case Types::ID::Block:			return "block";
	case Types::ID::Signal:			return "signal";
	case Types::ID::Number:			return "number";
	case Types::ID::Dynamic:		return "any";
	}

	return "unknown";
}

Colour Types::Helpers::getColourForType(ID type)
{
	switch (type)
	{
	case Types::ID::Void:			return Colours::white;
	case Types::ID::Integer:		return Colour(0xffbe952c);
	case Types::ID::Float:			
	case Types::ID::Double:			
	case Types::ID::FpNumber:		return Colour(0xff3a6666);
	case Types::ID::Block:			return Colour(0xff7559a4);
	case Types::ID::Event:			return Colour(0xFFC65638);
	case Types::ID::Signal:			return Colours::violet;
	case Types::ID::Number:			return Colours::green;
	case Types::ID::Dynamic:		return Colours::white;
	}

	return Colours::transparentBlack;
}


juce::String Types::Helpers::getValidCppVariableName(const String& variableToCheck)
{
	String s = variableToCheck;

	jassert(s.length() > 0);

	if (s.length() > 255)
		s = s.substring(0, 254);

	if (!juce::CharacterFunctions::isLetter(s[0]) && s[0] != '_')
	{
		s = '_' + s;
	}

	s = s.replaceCharacters("*+-/%&|!.",
					        "mpsdmaonp");

	static const char* keywords[63] = { "asm", "else", "new", "this", "auto", "enum", "operator", "throw", "bool", "explicit", "private", "true", "break", "export", "protected", "try", "case", "extern", "public", "typedef", "catch", "false", "register", "typeid", "char", "float", "reinterpret_cast", "typename", "class", "for", "return", "union", "const", "friend", "short", "unsigned", "const_cast", "goto", "signed", "using", "continue", "if", "sizeof", "virtual", "default", "inline", "static", "void", "delete", "int", "static_cast", "volatile", "do", "long", "struct", "wchar_t", "double", "mutable", "switch", "while", "dynamic_cast", "namespace", "template" };

	for (int i = 0; i < 63; i++)
	{
		if (s == keywords[i])
		{
			s = "_" + s;
			break;
		}
	}

	return s;
}

juce::String Types::Helpers::getTypeIDName(ID type)
{
	switch (type)
	{
	case Types::ID::Void:			return "Types::ID::Void";
	case Types::ID::Integer:		return "Types::ID::Integer";
	case Types::ID::Float:			return "Types::ID::Float";
	case Types::ID::Double:			return "Types::ID::Double";
	case Types::ID::FpNumber:		return "Types::ID::FpNumber";
	case Types::ID::Event:			return "Types::ID::Event";
	case Types::ID::Block:			return "Types::ID::Block";
	case Types::ID::Signal:			return "Types::ID::Signal";
	case Types::ID::Number:			return "Types::ID::Number";
	case Types::ID::Dynamic:		return "Types::ID::Dynamic";
	default:						return "Types::ID::numIds";
	}
}


juce::juce_wchar Types::Helpers::getTypeChar(ID id)
{
	return getTypeName(id).toLowerCase()[0];
}

juce::String Types::Helpers::getTypeCharAsString(ID id)
{
	String s;

	s << getTypeChar(id);

	return s;
}

juce::Array<hnode::Types::ID> Types::Helpers::getTypeListFromCode(const String& code)
{
	String variableWildCard = R"(\b(([fbinade][\d]+\b)))";

	auto matches = hise::RegexFunctions::findSubstringsThatMatchWildcard(variableWildCard, code);

	StringArray variableNames;

	for (const auto& m : matches)
	{
		if (m.size() == 3)
		{
			variableNames.addIfNotAlreadyThere(m[0]);
		}
	}

	struct VariableNameComparator
	{
		int compareElements(String a, String b)
		{
			auto a1 = a.substring(1).getIntValue();
			auto a2 = b.substring(1).getIntValue();

			if (a1 > a2)
				return 1;
			if (a1 < a2)
				return -1;

			return 0;
		};
	};

	VariableNameComparator comparator;
	variableNames.strings.sort(comparator, false);

	if (code.contains("event_"))
		variableNames.add("e" + String(variableNames.size()));

	return getTypeListFromVariables(variableNames);
}

juce::Array<hnode::Types::ID> Types::Helpers::getTypeListFromVariables(const StringArray& variableNames)
{
	Array<ID> list;

	for (auto v : variableNames)
	{
		list.add(getTypeFromVariableName(v));
	}

	return list;
}

hnode::Types::ID Types::Helpers::getIdFromVar(const var& value)
{
	if (value.isBool())
		return Types::ID::Integer;
	else if (value.isInt() || value.isInt64())
		return Types::ID::Integer;
	else if (value.isDouble())
		return Types::ID::Double;
	else if (value.isBuffer())
		return Types::ID::Block;

	return Types::Void;
}


String Types::Helpers::getPreciseValueString(const VariableStorage& v)
{
	if (v.getType() == Types::ID::Float)
	{
		std::ostringstream out;
		out.precision(7);
		out << std::fixed << (float)v;
		
		auto str = out.str();

		return String(str.c_str());
	}
	else if (v.getType() == Types::ID::Double)
	{
		std::ostringstream out;
		out.precision(15);
		out << std::fixed << double(v);

		auto str = out.str();

		return String(str.c_str());
	}

	return {};
	
}

juce::String Types::Helpers::getCppValueString(const var& v, ID type)
{
	if (isFloatingPoint(type))
	{
		juce::String value;

		double dValue = (double)v;

		if (fmodf(v, 1.0f) == 0.0f)
			value << String(static_cast<int>(dValue)) << ".0";
		else
		{
			value << dValue;
		}

		value = value.trimCharactersAtEnd("0");

		if (type == Float)
			value << "f";

		return value;
	}
	else
		return String((int)v);
}



juce::String Types::Helpers::getCppValueString(const VariableStorage& v)
{
	auto type = v.getType();

	if (isFloatingPoint(type))
	{
		juce::String value;

		double d = v.toDouble();

		if (fmodf(d, 1.0f) == 0.0f)
			value << String((int)d) << ".0";
		else
		{
			value << d;
		}
		
		value = value.trimCharactersAtEnd("0");

		if (type == Float)
			value << "f";

		return value;
	}
	else
		return String((int)v);
}

bool Types::Helpers::isTypeString(const String& type)
{
	return String("aeidfb").contains(type);
}

bool Types::Helpers::isFloatingPoint(ID type)
{
	return type & Types::ID::FpNumber;
}

juce::String Types::Helpers::getCppTypeName(ID type)
{
	if (isFixedType(type))
		return getTypeName(type);

	return "auto";
}

hnode::Types::ID Types::Helpers::getTypeFromStringValue(const String& value)
{
	if (value.contains("."))
	{
		if (value.contains("f"))
			return Types::ID::Float;
		else
			return Types::Double;
	}
	else
		return Types::ID::Integer;
}


bool Types::Helpers::matchesTypeStrict(ID expected, ID actual)
{
	return (int)expected & (int)actual;
}


bool Types::Helpers::matchesType(ID expected, ID actual)
{
#if HNODE_ALLOW_DOUBLE_FLOAT_MIX
	if (((int)expected | (int)actual) == ((int)ID::Double | (int)ID::Float))
		return true;
#endif

	return matchesTypeStrict(expected, actual);
}

bool Types::Helpers::isFixedType(ID type)
{
	return  type == ID::Block ||
			type == ID::Integer ||
			type == ID::Float ||
			type == ID::Void ||
			type == ID::Double ||
			type == ID::Event;
}

Types::ID Types::Helpers::getMoreRestrictiveType(ID typeA, ID typeB)
{
	if (!matchesType(typeA, typeB))
		return Types::ID::Void;

	if (isFixedType(typeA))
		return typeA;
	else if (isFixedType(typeB))
		return typeB;
	else if (typeA == Dynamic)
		return typeB;
	else
		return typeA;
}

bool Types::Helpers::isNumeric(ID id)
{
	return matchesType(Types::ID::Number, id);
}

bool Types::Helpers::isPinVariable(const String& name)
{
	auto wc = R"(\b[fbinade][1-9]\b)";

	return RegexFunctions::matchesWildcard(wc, name);
}



bool Types::Helpers::binaryOpAllowed(ID left, ID right)
{
	if (left == right)
		return true;

	if (matchesType(left, right))
		return true;

	if (left == Types::Block && isFloatingPoint(right))
		return true;

	return false;
}

#define ADD_TYPE(returnType, name, ...) types.add({ Float, Identifier(#name), {__VA_ARGS__} });

hnode::Types::FunctionType Types::Helpers::getFunctionPrototype(const Identifier& id)
{
	static Array<FunctionType> types;

	if (types.isEmpty())
	{
		ADD_TYPE(Float, sin, Float);
		ADD_TYPE(Float, sign, Float);
		ADD_TYPE(Float, abs, Float);
		ADD_TYPE(Float, round, Float);
		ADD_TYPE(Float, random);
		ADD_TYPE(Integer, randInt, Integer, Integer);
		ADD_TYPE(Number, range, Number, Number, Number);
		ADD_TYPE(Number, min, Number, Number);
		ADD_TYPE(Number, max, Number, Number);

		ADD_TYPE(Number, round, Number);

		ADD_TYPE(Float, sin, Float);
		ADD_TYPE(Float, asin, Float);
		ADD_TYPE(Float, cos, Float);
		ADD_TYPE(Float, acos, Float);
		ADD_TYPE(Float, sinh, Float);
		ADD_TYPE(Float, cosh, Float);
		ADD_TYPE(Float, tan, Float);
		ADD_TYPE(Float, tanh, Float);
		ADD_TYPE(Float, atan, Float);
		ADD_TYPE(Float, log, Float);
		ADD_TYPE(Float, log10, Float);
		ADD_TYPE(Float, exp, Float);
		ADD_TYPE(Float, pow, Float, Float);
		ADD_TYPE(Float, sqr, Float);
		ADD_TYPE(Float, sqrt, Float);
		ADD_TYPE(Float, ceil, Float);
		ADD_TYPE(Float, floor, Float);

		ADD_TYPE(Block, vmul, Block, Block);
		ADD_TYPE(Block, vadd, Block, Block);
		ADD_TYPE(Block, vsub, Block, Block);
		ADD_TYPE(Block, vcopy, Block, Block);
		ADD_TYPE(Block, vset, Block, Float);
		ADD_TYPE(Block, vmuls, Block, Float);
		ADD_TYPE(Block, vadds, Block, Float);
	}

	for (auto pt : types)
	{
		if (pt.functionName == id)
			return pt;
	}

	return {};
}

}
