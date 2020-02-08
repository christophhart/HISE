/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

#include <type_traits>

namespace snex {
namespace jit {
using namespace juce;

/** A Symbol is used to identifiy the data slot. */
struct Symbol
{
	static Symbol createRootSymbol(const Identifier& id);

	Symbol();

	Symbol(const Array<Identifier>& ids, Types::ID t_, bool isConst_);


	bool operator==(const Symbol& other) const;

	bool matchesIdAndType(const Symbol& other) const;

	Symbol getParentSymbol() const;

	Symbol getChildSymbol(const Identifier& id, Types::ID newType=Types::ID::Dynamic, bool isConst_=false) const;

	Symbol withParent(const Symbol& parent) const;

	Symbol withType(const Types::ID type) const;

	bool isExplicit() const { return fullIdList.size() > 1; }

	bool isConst() const { return const_; }

	juce::String toString() const;

	operator bool() const;

	std::string debugName;
	// a list of identifiers...
	Array<Identifier> fullIdList;
	Identifier id;
	bool const_ = false;
	VariableStorage constExprValue = {};

	Types::ID type = Types::ID::Dynamic;
};



/** A wrapper around a function. */
struct FunctionData
{
	template <typename T> void addArgs()
	{
		args.add(Types::Helpers::getTypeFromTypeId<T>());
	}

	template <typename T1, typename T2> void addArgs()
	{
		args.add(Types::Helpers::getTypeFromTypeId<T1>());
		args.add(Types::Helpers::getTypeFromTypeId<T2>());
	}

	template <typename T1, typename T2, typename T3> void addArgs()
	{
		args.add(Types::Helpers::getTypeFromTypeId<T1>());
		args.add(Types::Helpers::getTypeFromTypeId<T2>());
		args.add(Types::Helpers::getTypeFromTypeId<T3>());
	}

	template <typename ReturnType> static FunctionData createWithoutParameters(const Identifier& id, void* ptr = nullptr)
	{
		FunctionData d;

		d.id = id;
		d.returnType = Types::Helpers::getTypeFromTypeId<ReturnType>();
		d.function = reinterpret_cast<void*>(ptr);

		return d;
	}

	template <typename ReturnType, typename...Parameters> static FunctionData create(const Identifier& id, ReturnType(*ptr)(Parameters...) = nullptr)
	{
		FunctionData d = createWithoutParameters<ReturnType>(id, reinterpret_cast<void*>(ptr));
		d.addArgs<Parameters...>();
		return d;
	}

    template <typename T> void setFunction(T* typedFunctionPointer)
    {
        function = reinterpret_cast<void*>(typedFunctionPointer);
    }
    
	juce::String getSignature(const Array<Identifier>& parameterIds = {}) const;

	operator bool() const noexcept { return function != nullptr; };

	bool matchesArgumentTypes(Types::ID returnType, const Array<Types::ID>& argsList);

	bool matchesArgumentTypes(const Array<Types::ID>& typeList);

	bool matchesArgumentTypes(const FunctionData& otherFunctionData, bool checkReturnType = true) const;

	void setDescription(const juce::String& d, const StringArray& parameterNames = StringArray())
	{
		description = d;
		
		for (int i = 0; i < args.size(); i++)
			args.getReference(i).parameterName = parameterNames[i];
	}

	juce::String description;

	/** the function ID. */
	Identifier id;

	/** If this is not null, the function will be a member function for the given object. */
	void* object = nullptr;

	/** the function pointer. Use call<ReturnType, Args...>() for type checking during debugging. */
	void* function = nullptr;

	/** The return type. */
	Types::ID returnType;

	struct Argument
	{
		Argument() {};

		

		Argument(Types::ID type_, bool isAlias_=false) :
			type(type_),
			isAlias(isAlias_)
		{};

		bool operator==(const Types::ID& t) const
		{
			return t == type;
		}

		bool operator==(const Argument& other) const
		{
			return type == other.type && isAlias == other.isAlias;
		}

		Types::ID type = Types::ID::Dynamic;
		bool isAlias = false;
		String parameterName;
	};

	/** The argument list. */
	Array<Argument> args;

	/** A pretty formatted function name for debugging purposes. */
	String functionName;

	template <typename... Parameters> void callVoid(Parameters... ps) const
	{
		if (function != nullptr)
			callVoidUnchecked(ps...);
	}

	template <typename... Parameters> forcedinline void callVoidUnchecked(Parameters... ps) const
	{
		using signature = void(*)(Parameters...);

		auto f_ = (signature)function;
		f_(ps...);
	}

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callUnchecked(Parameters... ps) const
	{
		using signature = ReturnType & (*)(Parameters...);
		auto f_ = (signature)function;
		auto& r = f_(ps...);
		return ReturnType(r);
	}

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callUncheckedWithCopy(Parameters... ps) const
	{
		using signature = ReturnType(*)(Parameters...);
		auto f_ = (signature)function;
		return static_cast<ReturnType>(f_(ps...));
	}

	template <typename ReturnType, typename... Parameters> ReturnType call(Parameters... ps) const
	{
		// You must not call this method if you return an event or a block.
		// Use callWithReturnCopy instead...

		if (Types::Helpers::getTypeFromTypeId<ReturnType>() == Types::ID::Event ||
			Types::Helpers::getTypeFromTypeId<ReturnType>() == Types::ID::Block)
		{
			if (function != nullptr)
				return callUnchecked<ReturnType, Parameters...>(ps...);
			else
				return ReturnType();
		}
		else
		{
			if (function != nullptr)
				return callUncheckedWithCopy<ReturnType, Parameters...>(ps...);
			else
				return ReturnType();
		}
	}
};

class BaseScope;

/** A function class is a collection of functions. */
struct FunctionClass: public DebugableObjectBase
{
	struct Constant
	{
		Identifier id;
		VariableStorage value;
	};

	FunctionClass(const Symbol& id) :
		classSymbol(id)
	{};

	virtual ~FunctionClass()
	{
		registeredClasses.clear();
		functions.clear();
	};

	// =========================================================== DebugableObject overloads

	static ValueTree createApiTree(FunctionClass* r)
	{
		ValueTree p(r->getObjectName());

		for (auto f : r->functions)
		{
			ValueTree m("method");
			m.setProperty("name", f->id.toString(), nullptr);
			m.setProperty("description", f->getSignature(), nullptr);

			juce::String arguments = "(";

			int index = 0;

			for (auto arg : f->args)
			{
				index++;

				if (arg.type == Types::ID::Block && r->getObjectName().toString() == "Block")
					continue;

				if (arg.type == Types::ID::Event && r->getObjectName().toString() == "Message")
					continue;

				arguments << Types::Helpers::getTypeName(arg.type);

				if (arg.parameterName.isNotEmpty())
					arguments << " " << arg.parameterName;

				if (index < (f->args.size()))
					arguments << ", ";
			}

			arguments << ")";

			m.setProperty("arguments", arguments, nullptr);
			m.setProperty("returnType", Types::Helpers::getTypeName(f->returnType), nullptr);
			m.setProperty("description", f->description, nullptr);
			m.setProperty("typenumber", (int)f->returnType, nullptr);

			p.addChild(m, -1, nullptr);
		}

		return p;
	}

	ValueTree getApiValueTree() const
	{
		ValueTree v("Api");

		for (auto r : registeredClasses)
		{
			v.addChild(createApiTree(r), -1, nullptr);
		}

		return v;
	}

	juce::String getCategory() const override { return "API call"; };

	Identifier getObjectName() const override { return classSymbol.id; }

	juce::String getDebugValue() const override { return classSymbol.id.toString(); };

	juce::String getDebugDataType() const override { return "Class"; };

	void getAllFunctionNames(Array<Identifier>& functions) const 
	{
		functions.addArray(getFunctionIds());
	};

	void setDescription(const String& s, const StringArray& names)
	{
		if (auto last = functions.getLast())
			last->setDescription(s, names);
	}

	virtual void getAllConstants(Array<Identifier>& ids) const 
	{
		for (auto c : constants)
			ids.add(c.id);
	};

	DebugInformationBase* createDebugInformationForChild(const Identifier& id) override
	{
		for (auto& c : constants)
		{
			if (c.id == id)
			{
				auto s = new SettableDebugInfo();
				s->codeToInsert << getInstanceName().toString() << "." << id.toString();
				s->dataType = "Constant";
				s->type = Types::Helpers::getTypeName(c.value.getType());
				s->typeValue = ApiHelpers::DebugObjectTypes::Constants;
				s->value = Types::Helpers::getCppValueString(c.value);
				s->name = s->codeToInsert;
				s->category = "Constant";

				return s;
			}
		}

		return nullptr;
	}

	const var getConstantValue(int index) const 
	{ 
		return var(constants[index].value.toDouble());
	};

	// =====================================================================================

	virtual bool hasFunction(const Symbol& s) const;

	bool hasConstant(const Symbol& s) const;

	void addFunctionConstant(const Identifier& constantId, VariableStorage value);

	virtual void addMatchingFunctions(Array<FunctionData>& matches, const Symbol& symbol) const;

	void addFunctionClass(FunctionClass* newRegisteredClass);

	void addFunction(FunctionData* newData);

	Array<Identifier> getFunctionIds() const;

	const Symbol& getClassName() const { return classSymbol; }

	bool fillJitFunctionPointer(FunctionData& dataWithoutPointer);

	bool injectFunctionPointer(FunctionData& dataToInject);

	FunctionClass* getSubFunctionClass(const Symbol& id)
	{
		for (auto f : registeredClasses)
		{
			if (f->getClassName() == id)
				return f;
		}

		return nullptr;
	}

	VariableStorage getConstantValue(const Symbol& constantSymbol) const;


protected:

	OwnedArray<FunctionClass> registeredClasses;

	Symbol classSymbol;
	OwnedArray<FunctionData> functions;

	Array<Constant> constants;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionClass);
	JUCE_DECLARE_WEAK_REFERENCEABLE(FunctionClass)
};


} // end namespace jit
} // end namespace snex

