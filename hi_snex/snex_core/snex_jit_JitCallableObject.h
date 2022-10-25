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

namespace snex {
namespace jit {
using namespace juce;

/** A Object that can be registered to the JIT compiler and can be used to call member functions using a static wrapper.

It also notifies the GlobalScope object when it's deleted which then sends out a notification to all listeners that the
JIT function needs to be revalidated.

*/
struct JitCallableObject : public FunctionClass
{
	JitCallableObject(const NamespacedIdentifier& id);;

	virtual ~JitCallableObject();;

	/** Call this method to automatically register every function to the given memory pool. */
	void registerToMemoryPool(GlobalScope* m);

	Identifier objectId;

protected:

	virtual void registerAllObjectFunctions(GlobalScope* memory) = 0;

	FunctionData* createMemberFunction(const Types::ID returnType, const Identifier& functionId, const Array<Types::ID>& argTypes)
	{
		auto fId = getClassName().getChildId(functionId);

		ScopedPointer<FunctionData> functionWrapper(new FunctionData());
		functionWrapper->object = this;
		functionWrapper->id = fId;
		functionWrapper->functionName << objectId << "." << functionId << "()";

		for (int i = 0; i < argTypes.size(); i++)
		{
			juce::String s("Param");
			s << juce::String(i);
			functionWrapper->addArgs(Identifier(s), TypeInfo(argTypes[i], (argTypes[i] == Types::ID::Pointer) ? true : false));
		}

		functionWrapper->returnType = TypeInfo(returnType);

		return functionWrapper.release();
	}

private:

	WeakReference<GlobalScope> globalScope;

	JUCE_DECLARE_WEAK_REFERENCEABLE(JitCallableObject);
};

/** This is a example implementation of how to use the JitCallableObjectClass. 

	In order to use it, just create an instance and pass it to the 
	global scope using GlobalScope::registerObjectFunction();
*/
struct JitTestObject : public JitCallableObject
{
public:

	/** Create a class. The id will be used as first element in the dot operator. */
	JitTestObject(Identifier id) :
		JitCallableObject(NamespacedIdentifier(id))
	{};

	/** A member function. */
	int get5(float f1, float f2, float f3, float f4, float f5)
	{
		return (int)(f1*f2*f3*f4*f5 * value);
	}

	/** Another random member function. */
	float getValue() const
	{
		return value;
	}

	/** We need to create static function wrappers that have a pointer to its object as first parameter. */
	struct Wrappers
	{
		JIT_MEMBER_WRAPPER_0(float, JitTestObject, getValue);
		JIT_MEMBER_WRAPPER_5(int, JitTestObject, get5, float, float, float, float, float)
	};

	/** Register all functions that you want to access here. */
	void registerAllObjectFunctions(GlobalScope*)
	{
		using namespace Types;

		{
			// Creates a function data object and fills in the given information
			auto f = createMemberFunction(Float, "getValue", {});

			// Give it the function pointer to the static function wrapper
			f->function = reinterpret_cast<void*>(Wrappers::getValue);

			// Add the function to make it accessible inside SNEX
			addFunction(f);
		}

		{
			
		}
	}

private:

	// Some random member variable.
	float value = 12.0f;
};

}
}
