namespace snex {
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

snex::Types::ID Types::Helpers::getTypeFromTypeName(const juce::String& cppTypeName)
{
	if (cppTypeName == "double") return Types::ID::Double;
	if (cppTypeName == "float") return Types::ID::Float;
	if (cppTypeName == "int") return Types::ID::Integer;
	if (cppTypeName == "bool") return Types::ID::Integer;
	if (cppTypeName == "block") return Types::ID::Block;
	if (cppTypeName == "void") return Types::ID::Void;
    if (cppTypeName == "void*") return Types::ID::Pointer;
    if (cppTypeName == "pointer") return Types::ID::Pointer;
	
	jassertfalse;
	return Types::ID::Void;
}

snex::Types::ID Types::Helpers::getTypeFromVariableName(const juce::String& name)
{
	auto typeChar = name.toLowerCase()[0];

	switch (typeChar)
	{
	case 'b':	return Types::ID::Block;
	case 'f':	return Types::ID::Float;
	case 'd':	return Types::ID::Double;
	case 'i':	return Types::ID::Integer;
	case 'a':	return Types::ID::Dynamic;
	default:	jassertfalse; return Types::ID::Void;
	}
}

juce::String Types::Helpers::getVariableName(ID id, int index)
{
	juce::String name;
	name << getTypeChar(id) << juce::String(index + 1);

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
	case Types::ID::Block:			return "block";
	case Types::ID::Pointer:		return "pointer";
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
	case Types::ID::Block:			return Colour(0xff7559a4);
	case Types::ID::Pointer:		return Colours::aqua;
	case Types::ID::Dynamic:		return Colours::white;
	}

	return Colours::transparentBlack;
}


juce::String Types::Helpers::getValidCppVariableName(const juce::String& variableToCheck)
{
	juce::String s = variableToCheck;

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

juce::String Types::Helpers::getIntendation(int level)
{
	juce::String intent;

	for (int i = 0; i < level; i++)
		intent << "  ";
	return intent;

}

void Types::Helpers::dumpNativeData(juce::String& s, int intendationLevel, const juce::String& symbol, void* dataStart, void* dataPointer, size_t byteSize, Types::ID type)
{
	juce::String nl("\n");

	s << "|";

	auto intent = getIntendation(intendationLevel);

	size_t byteOffset = (uint8*)dataPointer - (uint8*)dataStart;
    ignoreUnused(byteOffset);
    
	s << intent << Types::Helpers::getCppTypeName((Types::ID)type) << " " << symbol;
	s << "\t{ " << getStringFromDataPtr(type, dataPointer);

#if SNEX_INCLUDE_MEMORY_ADDRESS_IN_DUMP
	s << ", address: 0x" << String::toHexString((uint64_t)dataPointer).toUpperCase() << " }";

	if (byteOffset % byteSize != 0)
		s << " (Unaligned!)";
#endif
}

String Types::Helpers::getStringFromDataPtr(Types::ID type, void* data)
{
	var v;

	switch (type)
	{
	case Types::ID::Integer: v = var(*reinterpret_cast<int*>(data)); break;
	case Types::ID::Double: v = var(*reinterpret_cast<double*>(data)); break;
	case Types::ID::Float: v = var(*reinterpret_cast<float*>(data)); break;
	case Types::ID::Pointer: v = var(*reinterpret_cast<int64*>(data)); break;
	default: jassertfalse;
	}

	return Types::Helpers::getCppValueString(VariableStorage(type, v));
}

juce::String Types::Helpers::getTypeIDName(ID type)
{
	switch (type)
	{
	case Types::ID::Void:			return "Types::ID::Void";
	case Types::ID::Integer:		return "Types::ID::Integer";
	case Types::ID::Float:			return "Types::ID::Float";
	case Types::ID::Double:			return "Types::ID::Double";
	case Types::ID::Block:			return "Types::ID::Block";
	case Types::ID::Dynamic:		return "Types::ID::Dynamic";
	case Types::ID::Pointer:		return "Types::ID::Pointer";
	default:						return "Types::ID::numIds";
	}
}


size_t Types::Helpers::getSizeForType(ID type)
{
	switch (type)
	{
	case Types::ID::Integer: return sizeof(int);
	case Types::ID::Float: return sizeof(float);
	case Types::ID::Double: return sizeof(double);
	case Types::ID::Block: return sizeof(block);
	case Types::ID::Pointer: return sizeof(int*);
    default: return 0;
	}
}

bool Types::Helpers::matchesTypeLoose(ID expected, ID actual)
{
	return expected == actual || (isNumeric(expected) && isNumeric(actual));
}

juce::juce_wchar Types::Helpers::getTypeChar(ID id)
{
	return getTypeName(id).toLowerCase()[0];
}

juce::String Types::Helpers::getTypeCharAsString(ID id)
{
	juce::String s;

	s << getTypeChar(id);

	return s;
}

juce::Array<snex::Types::ID> Types::Helpers::getTypeListFromCode(const juce::String& code)
{
	juce::String variableWildCard = R"(\b(([fbinade][\d]+\b)))";

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
		int compareElements(juce::String a, juce::String b)
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
		variableNames.add("e" + juce::String(variableNames.size()));

	return getTypeListFromVariables(variableNames);
}

juce::Array<snex::Types::ID> Types::Helpers::getTypeListFromVariables(const StringArray& variableNames)
{
	Array<ID> list;

	for (auto v : variableNames)
	{
		list.add(getTypeFromVariableName(v));
	}

	return list;
}

snex::Types::ID Types::Helpers::getIdFromVar(const var& value)
{
	if (value.isBool())
		return Types::ID::Integer;
	else if (value.isInt() || value.isInt64())
		return Types::ID::Integer;
	else if (value.isDouble())
		return Types::ID::Double;
	else if (value.isBuffer())
		return Types::ID::Block;
	else
		jassertfalse;

	return Types::Void;
}


juce::String Types::Helpers::getPreciseValueString(const VariableStorage& v)
{
	if (v.getType() == Types::ID::Float)
	{
		std::ostringstream out;
		out.precision(7);
		out << std::fixed << (float)v;
		
		auto str = out.str();

		return juce::String(str.c_str());
	}
	else if (v.getType() == Types::ID::Double)
	{
		std::ostringstream out;
		out.precision(15);
		out << std::fixed << double(v);

		auto str = out.str();

		return juce::String(str.c_str());
	}

	return {};
	
}

juce::String Types::Helpers::getCppValueString(const var& v, ID type)
{
	if (isFloatingPoint(type))
	{
		juce::String value;

		double dValue = (double)v;

		auto fracPart = fmod(dValue, 1.0);

		if (fracPart == 0.0f || ((hmath::abs(dValue) > 10.0) && fracPart < 0.001))
			value << juce::String(static_cast<int>(dValue)) << ".0";
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
		return juce::String((int)v);
}



juce::String Types::Helpers::getCppValueString(const VariableStorage& v)
{
	auto type = v.getType();

	if (isFloatingPoint(type))
	{
		juce::String value;

		double d = v.toDouble();

		if (fmod(d, 1.0) == 0.0)
			value << juce::String((int)d) << ".0";
		else
		{
			value << d;
		}
		
		value = value.trimCharactersAtEnd("0");

		if (type == Float)
			value << "f";

		return value;
	}
	else if (type == Types::ID::Pointer)
	{
		return "p0x" + juce::String::toHexString(reinterpret_cast<uint64_t>(v.getDataPointer())).toUpperCase() + "";
	}
	else if (type == Types::ID::Block)
	{
		return "block()";
	}
	else
		return juce::String((int)v);
}

bool Types::Helpers::isTypeString(const juce::String& type)
{
	return juce::String("aeidfb").contains(type);
}

bool Types::Helpers::isFloatingPoint(ID type)
{
	return type == Types::ID::Float || type == Types::ID::Double;
}

juce::String Types::Helpers::getCppTypeName(ID type)
{
	if (isFixedType(type))
		return getTypeName(type);

	return "auto";
}

snex::Types::ID Types::Helpers::getTypeFromStringValue(const juce::String& value)
{
	if (value.contains("p"))
		return Types::ID::Pointer;
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
	return expected == Dynamic || expected == actual;
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
			type == ID::Pointer;
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
	return id == Integer || id == Float || id == Double;
}

bool Types::Helpers::isPinVariable(const juce::String& name)
{
	auto wc = R"(\b[fbinade][1-9]\b)";

	return RegexFunctions::matchesWildcard(wc, name);
}



bool Types::Helpers::binaryOpAllowed(ID left, ID right)
{
	if (left == Types::Pointer || right == Types::Pointer)
		return false;

	if (left == right)
		return true;

	if (matchesType(left, right))
		return true;

	if (left == Types::Block && isFloatingPoint(right))
		return true;

	return false;
}

#define ADD_TYPE(returnType, name, ...) types.add({ Float, Identifier(#name), {__VA_ARGS__} });

}
