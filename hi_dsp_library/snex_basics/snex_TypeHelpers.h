/*
  ==============================================================================

    hnode_TypeHelpers.h
    Created: 2 Sep 2018 6:43:33pm
    Author:  Christoph

  ==============================================================================
*/

#pragma once

namespace snex
{

class VariableStorage;

#define IF_(typeName) if(type == Types::Helpers::getTypeFromTypeId<typeName>())

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

	static size_t getSizeForType(ID type);

	static bool matchesTypeLoose(ID expected, ID actual);
	static bool matchesTypeStrict(ID expected, ID actual);
	static bool matchesType(ID expected, ID actual);
	static bool isFixedType(ID type);
	static ID getMoreRestrictiveType(ID typeA, ID typeB);

	template <typename T> static constexpr bool isRefArrayType()
	{
		return T::ArrayType == Types::ArrayID::DynType ||
			T::ArrayType == Types::ArrayID::HeapType ||
			T::ArrayType == Types::ArrayID::ProcessDataType;
	}

	static bool isNumeric(ID id);

	static bool isPinVariable(const String& name);

	static bool binaryOpAllowed(ID left, ID right);

	static Colour getColourForType(ID type);

	static String getValidCppVariableName(const String& variableToCheck);

	static juce::String getIntendation(int level);

	static void dumpNativeData(juce::String& s, int intendationLevel, const juce::String& symbol, void* dataStart, void* dataPointer, size_t byteSize, Types::ID type);

	static String getStringFromDataPtr(Types::ID type, void* data);;

	template <typename T> static String getTypeNameFromTypeId()
	{
		auto type = getTypeFromTypeId<T>();
		return getTypeName(type);
	}

	template <typename T> constexpr static bool isPointerType()
	{
		auto t = getTypeFromTypeId<T>();
		return t == Types::ID::Pointer || t == Types::ID::Block || t == Types::ID::Void;
	}

	template <typename T> constexpr static Types::ID getTypeFromTypeId()
	{
		return Types::getTypeFromTypeId<T>();
	};
};

}


struct NamespacedIdentifier
{
	explicit NamespacedIdentifier(const Identifier& id)
	{
		add(id);
	}

	static NamespacedIdentifier fromString(const juce::String& s)
	{
		auto sa = StringArray::fromTokens(s, "::", "");

		sa.removeEmptyStrings();

		NamespacedIdentifier c;
		for (auto s : sa)
			c.add(Identifier(s));

		return c;
	}

	static NamespacedIdentifier null()
	{
		static const NamespacedIdentifier n("null");
		return n;
	}

	bool isNull() const
	{
		return null() == *this;
	}

	NamespacedIdentifier()
	{}

	Array<Identifier> getIdList() const
	{
		Array<Identifier> l;
		l.addArray(namespaces);
		l.add(id);
		return l;
	}

#if SNEX_ENABLE_DEBUG_TYPENAMES
	std::string debugName;
#endif

	Array<Identifier> namespaces;
	Identifier id;

	bool isValid() const
	{
		return id.isValid();
	}

	bool isInGlobalNamespace() const
	{
		return namespaces.isEmpty();
	}

	bool isExplicit() const
	{
		return isInGlobalNamespace();
	}

	Identifier getIdentifier() const
	{
		return id;
	}

	bool isParentOf(const NamespacedIdentifier& other) const
	{
		auto otherName = other.toString();
		auto thisName = toString();
		return otherName.startsWith(thisName);
	}

	NamespacedIdentifier removeSameParent(const NamespacedIdentifier& other) const
	{
		auto l1 = getIdList();
		
		for (auto& o : other.getIdList())
			l1.removeAllInstancesOf(o);

		NamespacedIdentifier copy(l1.getFirst());

		for (int i = 1; l1.size(); i++)
			copy = copy.getChildId(l1[i]);

		return copy;
	}

	void relocateSelf(const NamespacedIdentifier& oldParent, const NamespacedIdentifier& newParent)
	{
		jassert(oldParent.isParentOf(*this));

		NamespacedIdentifier s(newParent);

		auto opl = oldParent.getIdList();
		auto ids = getIdList();

		for (int i = 0; i < ids.size(); i++)
		{
			if (opl[i] != ids[i])
			{
				s = s.getChildId(ids[i]);
			}
		}

		jassert(newParent.isParentOf(s));

		SNEX_TYPEDEBUG(debugName = s.debugName);
		namespaces = s.namespaces;
		id = s.id;
	}

	NamespacedIdentifier relocate(const NamespacedIdentifier& oldParent, const NamespacedIdentifier& newParent) const
	{
		NamespacedIdentifier s(*this);

		s.relocateSelf(oldParent, newParent);
		return s;
	}

	NamespacedIdentifier getChildId(const Identifier& id) const
	{
		auto c = *this;
		c.add(id);
		return c;
	}

	NamespacedIdentifier getParent() const
	{
		if (namespaces.isEmpty())
			return {};

		auto c = *this;
		c.pop();
		return c;
	}

	bool operator !=(const NamespacedIdentifier& other) const
	{
		return !(*this == other);
	}

	bool operator==(const NamespacedIdentifier& other) const
	{
		if (id != other.id)
			return false;

		if (namespaces.size() == other.namespaces.size())
		{
			for (int i = 0; i < namespaces.size(); i++)
			{
				if (namespaces[i] != other.namespaces[i])
					return false;
			}

			return true;
		}

		return false;
	}

	NamespacedIdentifier withId(const Identifier& id) const
	{
		auto c = *this;
		c.add(id);
		return c;
	}

	juce::String toString() const
	{
		juce::String s;
		s.preallocateBytes(256);

		for (auto n : namespaces)
			s << n << "::";

		s << id;

		return s;
	}

	void add(const Identifier& newId)
	{
		if (id.isValid())
			namespaces.add(id);

		id = newId;

		SNEX_TYPEDEBUG(debugName = toString().toStdString());
	}

	Result pop()
	{
		if (id.isNull())
			return Result::fail("Can't pop namespace");

		id = namespaces.getLast();
		namespaces.removeLast();

		SNEX_TYPEDEBUG(debugName = toString().toStdString());

		return Result::ok();
	}
};


}
