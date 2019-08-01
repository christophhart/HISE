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

/** A Object that can be registered to the JIT compiler and can be used to call member functions using a static wrapper.

It also notifies the GlobalScope object when it's deleted which then sends out a notification to all listeners that the
JIT function needs to be revalidated.

*/
struct JitCallableObject : public FunctionClass
{
	JitCallableObject(const Identifier& id);;

	virtual ~JitCallableObject();;

	/** Call this method to automatically register every function to the given memory pool. */
	void registerToMemoryPool(GlobalScope* m);

	Identifier objectId;

protected:

	virtual void registerAllObjectFunctions(GlobalScope* memory) = 0;

	FunctionData* createMemberFunctionForJitCode(const Identifier& functionId);

private:

	WeakReference<GlobalScope> globalScope;

	JUCE_DECLARE_WEAK_REFERENCEABLE(JitCallableObject);
};

}
}