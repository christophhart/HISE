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

	static bool isNumeric(ID id);

	static bool isPinVariable(const String& name);

	static bool binaryOpAllowed(ID left, ID right);

	static Colour getColourForType(ID type);

	static String getValidCppVariableName(const String& variableToCheck);

	static juce::String getIntendation(int level);

	static void dumpNativeData(juce::String& s, int intendationLevel, const juce::String& symbol, void* dataStart, void* dataPointer, size_t byteSize, Types::ID type);



	template <typename T> static String getTypeNameFromTypeId()
	{
		auto type = getTypeFromTypeId<T>();
		return getTypeName(type);
	}

	template <typename T> constexpr static Types::ID getTypeFromTypeId()
	{
		if (std::is_same<T, float>())
			return Types::ID::Float;
		if (std::is_same<T, double>())
			return Types::ID::Double;
		if (std::is_same<T, int>())
			return Types::ID::Integer;
		if (std::is_same<T, block>())
			return Types::ID::Block;
		if (std::is_same<T, void*>())
			return Types::ID::Pointer;

		return Types::ID::Void;
	};
};

}

struct InitialiserList : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<InitialiserList>;

	juce::String toString() const
	{
		juce::String s;
		s << "{ ";

		for (auto l : root)
			s << l->toString();

		s << "}";
		return s;
	}

	static Ptr makeSingleList(const VariableStorage& v)
	{
		InitialiserList::Ptr singleList = new InitialiserList();
		singleList->addImmediateValue(v);
		return singleList;
	}

	void addChildList(const InitialiserList* other)
	{
		root.add(new ListChild(other->root));
	}

	void addImmediateValue(const VariableStorage& v)
	{
		root.add(new ImmediateChild(v));
	}

	Result getValue(int index, VariableStorage& v)
	{
		if (auto child = root[index])
		{
			if (child->getValue(v))
				return Result::ok();
			else
				return Result::fail("Can't resolve value at index " + juce::String(index));
		}
		else
			return Result::fail("Can't find item at index " + juce::String(index));
	}

	Ptr createChildList(int index)
	{
		if (auto child = root[index])
		{
			return child->createChildList();
		}

		return nullptr;
	}

	Ptr getChild(int index)
	{
		if (auto cb = dynamic_cast<ListChild*>(root[index].get()))
		{
			Ptr p = new InitialiserList();
			p->root.addArray(cb->list);
			return p;
		}

		return nullptr;
	}

	int size() const
	{
		return root.size();
	}

private:

	struct ChildBase : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<ChildBase>;
		using List = ReferenceCountedArray<ChildBase>;

		virtual bool getValue(VariableStorage& v) const = 0;

		virtual InitialiserList::Ptr createChildList() const = 0;

		virtual ~ChildBase() {};

		virtual juce::String toString() const = 0;
	};

	struct ImmediateChild : public ChildBase
	{
		ImmediateChild(const VariableStorage& v_) :
			v(v_)
		{};

		bool getValue(VariableStorage& target) const override
		{
			target = v;
			return true;
		}

		virtual InitialiserList::Ptr createChildList() const override
		{
			InitialiserList::Ptr n = new InitialiserList();
			n->addImmediateValue(v);
			return n;
		}

		juce::String toString() const override
		{
			return Types::Helpers::getCppValueString(v);
		}

		VariableStorage v;
	};

	struct ListChild : public ChildBase
	{
		ListChild(const ChildBase::List& l) :
			list(l)
		{};

		virtual InitialiserList::Ptr createChildList() const override
		{
			InitialiserList::Ptr n = new InitialiserList();
			n->root.addArray(list);
			return n;
		}

		bool getValue(VariableStorage& target) const override
		{
			if (list.size() == 1)
				return list[0]->getValue(target);
			
			return false;
		}

		juce::String toString() const override
		{
			juce::String s;
			s << "{ ";

			for (auto l : list)
				s << l->toString();

			s << "}";
			return s;
		}

		ChildBase::List list;
	};

	ChildBase::List root;
};

}
