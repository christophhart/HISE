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

/** Use this macro to define the type that should be returned by calls to getObject(). Normally you pass in the wrapped object (for non-wrapped classes you should use GET_SELF_AS_OBJECT(). */
#define GET_SELF_OBJECT(x) constexpr auto& getObject() { return x; } \
constexpr const auto& getObject() const { return x; }

/** Use this macro to define the expression that should be used in order to get the most nested type. (usually you pass in obj.getWrappedObject(). */
#define GET_WRAPPED_OBJECT(x) constexpr auto& getWrappedObject() { return x; } \
constexpr const auto& getWrappedObject() const { return x; }

/** Use this macro in order to create the getObject() / getWrappedObject() methods that return the object itself. */
#define GET_SELF_AS_OBJECT() GET_SELF_OBJECT(*this); GET_WRAPPED_OBJECT(*this);

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

	std::string debugName;
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

		debugName = s.debugName;
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

		debugName = toString().toStdString();
	}

	Result pop()
	{
		if (id.isNull())
			return Result::fail("Can't pop namespace");

		id = namespaces.getLast();
		namespaces.removeLast();

		debugName = toString().toStdString();

		return Result::ok();
	}
};

struct InitialiserList : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<InitialiserList>;

	struct ChildBase : public ReferenceCountedObject
	{
		using List = ReferenceCountedArray<ChildBase>;

		virtual bool getValue(VariableStorage& v) const = 0;

		virtual InitialiserList::Ptr createChildList() const = 0;

		virtual ~ChildBase() {};

		virtual bool forEach(const std::function<bool(ChildBase*)>& func)
		{
			return func(this);
		}

		virtual juce::String toString() const = 0;
	};

	juce::String toString() const
	{
		juce::String s;
		s << "{ ";

		for (auto l : root)
			s << l->toString();

		s << " }";
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
		addChild(new ListChild(other->root));
	}

	void addImmediateValue(const VariableStorage& v)
	{
		addChild(new ImmediateChild(v));
	}

	void addChild(ChildBase* b)
	{
		root.add(b);
	}

	ReferenceCountedObject* getExpression(int index);

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

	Array<VariableStorage> toFlatConstructorList() const
	{
		Array<VariableStorage> list;

		for (auto c : root)
		{
			VariableStorage v;
			
			if (c->getValue(v))
			{
				list.add(v);
			}
			else
			{
				jassertfalse;
			}
		}

		return list;
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

	bool forEach(const std::function<bool(ChildBase*)>& func)
	{
		for (auto l : root)
		{
			if (l->forEach(func))
				return true;
		}
			
		return false;
	}

	struct ExpressionChild;

	struct MemberPointer : public ChildBase
	{
		MemberPointer(const Identifier& id) :
			variableId(id)
		{};

		juce::String toString() const override
		{
			return variableId.toString();
		}

		bool getValue(VariableStorage& v) const override
		{
			v = value;
			return !v.isVoid();
		}

		InitialiserList::Ptr createChildList() const override
		{
			InitialiserList::Ptr n = new InitialiserList();
			n->addChild(new MemberPointer(variableId));
			return n;
		}

		Identifier variableId;
		VariableStorage value;
	};

private:

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

		InitialiserList::Ptr createChildList() const override
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

		bool forEach(const std::function<bool(ChildBase *)>& func) override
		{
			for (auto l : list)
			{
				if (l->forEach(func))
					return true;
			}

			return false;
		}

		juce::String toString() const override
		{
			juce::String s;
			s << "{ ";

			for (auto l : list)
				s << l->toString();

			s << " }";
			return s;
		}

		ChildBase::List list;
	};

	ChildBase::List root;
};

}
