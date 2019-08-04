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

namespace hnode {
namespace jit {
using namespace juce;


/** The global scope that is passed to the compiler and contains the global variables
	and all registered objects.

	It has the following additional features:

	 - a listener system that notifies its Listeners when a registered object is deleted.
	   This is useful to invalidate JIT compiled functions that access this object's
	   function
	 - a dynamic list of JIT callable objects that can be registered and called from
	   JIT code. Objects derived from FunctionClass can subscribe and unsubscribe to this
	   scope and will be resolved as member function calls.

	You have to pass a reference to an object of this scope to the Compiler.
*/
class GlobalScope : public FunctionClass,
	public BaseScope
{
public:

	GlobalScope(int numVariables = 1024);

	struct ObjectDeleteListener
	{
		virtual ~ObjectDeleteListener() {};

		virtual void objectWasDeleted(const Identifier& id) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(ObjectDeleteListener);
	};

	void registerObjectFunction(FunctionClass* objectClass);

	void deregisterObject(const Identifier& id);

	bool hasFunction(const Identifier& classId, const Identifier& functionId) const override;

	void addMatchingFunctions(Array<FunctionData>& matches, const Identifier& classId, const Identifier& functionId) const override;

	void addObjectDeleteListener(ObjectDeleteListener* l);
	void removeObjectDeleteListener(ObjectDeleteListener* l);

    VariableStorage& operator[](const Identifier& id)
    {
        return getVariableReference(id);
    }
    
private:

	Array<WeakReference<ObjectDeleteListener>> deleteListeners;

	OwnedArray<FunctionClass> objectClassesWithJitCallableFunctions;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalScope);
	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalScope);
};



}
}
