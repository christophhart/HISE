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


namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;

GlobalScope::GlobalScope(int numVariables /*= 1024*/) :
	FunctionClass("Globals"),
	BaseScope("Globals", nullptr, numVariables)
{
	bufferHandler = new BufferHandler();

	auto c = new ConsoleFunctions(this);

	registerObjectFunction(c);

	jassert(scopeType == BaseScope::Global);
}


void GlobalScope::registerObjectFunction(FunctionClass* objectClass)
{
	objectClassesWithJitCallableFunctions.add(objectClass);

	if (auto jco = dynamic_cast<JitCallableObject*>(objectClass))
		jco->registerToMemoryPool(this);
	else
		jassertfalse;
}

void GlobalScope::deregisterObject(const Identifier& id)
{
	bool somethingDone = false;

	for (int i = 0; i < objectClassesWithJitCallableFunctions.size(); i++)
	{
		if (objectClassesWithJitCallableFunctions[i]->getClassName() == id)
		{
			functions.remove(i--);
			somethingDone = true;
		}
	}

	if (somethingDone)
	{
		for (auto l : deleteListeners)
		{
			if (l.get() != nullptr)
				l->objectWasDeleted(id);
		}
	}
}

bool GlobalScope::hasFunction(const Identifier& classId, const Identifier& functionId) const
{
	for (auto of : objectClassesWithJitCallableFunctions)
	{
		if (of->hasFunction(classId, functionId))
			return true;
	}

	return false;
}

void GlobalScope::addMatchingFunctions(Array<FunctionData>& matches, const Identifier& classId, const Identifier& functionId) const
{
	FunctionClass::addMatchingFunctions(matches, classId, functionId);

	for (auto of : objectClassesWithJitCallableFunctions)
		of->addMatchingFunctions(matches, classId, functionId);
}

void GlobalScope::addObjectDeleteListener(ObjectDeleteListener* l)
{
	deleteListeners.addIfNotAlreadyThere(l);
}

void GlobalScope::removeObjectDeleteListener(ObjectDeleteListener* l)
{
	deleteListeners.removeAllInstancesOf(l);
}

}
}
