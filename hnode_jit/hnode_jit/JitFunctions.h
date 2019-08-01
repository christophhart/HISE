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

namespace hnode {
namespace jit {
using namespace juce;

/** A wrapper around a function. */
struct FunctionData
{
#if 0
	template<typename T>
	void loadBrush_sub_impl()
	{
		// do some work here
	}

	template<typename... Targs>
	void loadBrush_sub();

	template<typename T, typename... V>
	void loadBrush_sub_helper()
	{
		loadBrush_sub_impl<T>();
		loadBrush_sub<V...>();
	}

	template<typename... Targs>
	void loadBrush_sub()
	{
		loadBrush_sub_helper<Targs...>();
	}

	template<>
	void loadBrush_sub<>()
	{
	}
#endif

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
		d.function = ptr;

		return d;
	}

	template <typename ReturnType, typename...Parameters> static FunctionData create(const Identifier& id, void* ptr = nullptr)
	{
		FunctionData d = createWithoutParameters<ReturnType>(id, ptr);
		d.addArgs<Parameters...>();
		return d;
	}

	String getSignature(const Array<Identifier>& parameterIds = {}) const;

	operator bool() const noexcept { return function != nullptr; };

	bool matchesArgumentTypes(Types::ID returnType, const Array<Types::ID>& argsList);

	bool matchesArgumentTypes(const Array<Types::ID>& typeList);

	bool matchesArgumentTypes(const FunctionData& otherFunctionData, bool checkReturnType = true) const;

	/** the function ID. */
	Identifier id;

	/** If this is not null, the function will be a member function for the given object. */
	void* object = nullptr;

	/** the function pointer. Use call<ReturnType, Args...>() for type checking during debugging. */
	void* function = nullptr;

	/** The return type. */
	Types::ID returnType;

	/** The argument list. */
	Array<Types::ID> args;

	/** A pretty formatted function name for debugging purposes. */
	String functionName;

	template <typename... Parameters> void callVoid(Parameters... ps)
	{
		if (function != nullptr)
			callVoidUnchecked(ps...);
	}

	template <typename... Parameters> forcedinline void callVoidUnchecked(Parameters... ps)
	{
		using signature = void(*)(Parameters...);

		auto f_ = (signature)function;
		f_(ps...);
	}

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callUnchecked(Parameters... ps)
	{
		using signature = ReturnType & (*)(Parameters...);
		auto f_ = (signature)function;
		auto& r = f_(ps...);
		return ReturnType(r);
	}

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callUncheckedWithCopy(Parameters... ps)
	{
		using signature = ReturnType(*)(Parameters...);
		auto f_ = (signature)function;
		return static_cast<ReturnType>(f_(ps...));
	}

	template <typename ReturnType, typename... Parameters> ReturnType call(Parameters... ps)
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

/** A function class is a collection of functions. */
struct FunctionClass
{
	FunctionClass(const Identifier& id) :
		className(id)
	{};

	virtual ~FunctionClass() {};

	virtual bool hasFunction(const Identifier& classId, const Identifier& functionId) const;

	virtual void addMatchingFunctions(Array<FunctionData>& matches, const Identifier& classId, const Identifier& functionId) const;

	void addFunctionClass(FunctionClass* newRegisteredClass);

	void addFunction(FunctionData* newData);

	

	Identifier getClassName() const { return className; }

	bool fillJitFunctionPointer(FunctionData& dataWithoutPointer);

	bool injectFunctionPointer(Identifier& id, void* funcPointer);

protected:

	OwnedArray<FunctionClass> registeredClasses;

	Identifier className;
	OwnedArray<FunctionData> functions;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionClass);
	JUCE_DECLARE_WEAK_REFERENCEABLE(FunctionClass)
};


} // end namespace jit
} // end namespace hnode

