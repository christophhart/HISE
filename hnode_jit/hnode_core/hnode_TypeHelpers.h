/*
  ==============================================================================

    hnode_TypeHelpers.h
    Created: 2 Sep 2018 6:43:33pm
    Author:  Christoph

  ==============================================================================
*/

#pragma once

namespace hnode
{

namespace Types
{

using namespace juce;


struct Helpers
{
	static void convertFloatToDouble(double* dst, const float* src, int numSamples);

	static void convertDoubleToFloat(float* dst, const double* src, int numSamples);

	static ID getTypeFromTypeName(const String& cppTypeName);

	static ID getTypeFromVariableName(const String& name);
	static String getVariableName(ID id, int index);
	static String getTypeName(ID id);
	static juce_wchar getTypeChar(ID id);
	static String getTypeCharAsString(ID id);
	static Array<ID> getTypeListFromCode(const String& code);
	static Array<ID> getTypeListFromVariables(const StringArray& variableNames);
	static ID getIdFromVar(const var& value);

	static String getPreciseValueString(const VariableStorage& value);

	static String getCppValueString(const var& value, ID type);

	static String getCppValueString(const VariableStorage& value);

	static bool isTypeString(const String& type);

	static bool isFloatingPoint(ID type);

	static String getCppTypeName(ID type);

	static ID getTypeFromStringValue(const String& value);

	static String getTypeIDName(ID Type);


	static bool matchesTypeStrict(ID expected, ID actual);
	static bool matchesType(ID expected, ID actual);
	static bool isFixedType(ID type);
	static ID getMoreRestrictiveType(ID typeA, ID typeB);

	static bool isNumeric(ID id);

	static bool isPinVariable(const String& name);

	static bool binaryOpAllowed(ID left, ID right);

	static FunctionType getFunctionPrototype(const Identifier& id);

	static Colour getColourForType(ID type);

	static String getValidCppVariableName(const String& variableToCheck);

	template <typename T> static String getTypeNameFromTypeId()
	{
		auto type = getTypeFromTypeId<T>();
		return getTypeName(type);
	}

	template <typename T> constexpr static Types::ID getTypeFromTypeId()
	{
		if constexpr (std::is_same<T, float>())
			return Types::ID::Float;
		if constexpr (std::is_same<T, double>())
			return Types::ID::Double;
		if constexpr (std::is_same<T, int>())
			return Types::ID::Integer;
		if constexpr (std::is_same<T, HiseEvent>())
			return Types::ID::Event;
		if constexpr (std::is_same<T, block>())
			return Types::ID::Block;

		return Types::ID::Void;
	};
};


}

}