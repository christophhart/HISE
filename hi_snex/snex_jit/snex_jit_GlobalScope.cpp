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
	FunctionClass({}),
	BaseScope({}, nullptr)
{
	bufferHandler = new BufferHandler();

	objectClassesWithJitCallableFunctions.add(new ConsoleFunctions(this));
	addFunctionClass(new MathFunctions());

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

void GlobalScope::deregisterObject(const NamespacedIdentifier& id)
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

void GlobalScope::registerFunctionsToNamespaceHandler(NamespaceHandler& handler)
{
	NamespaceHandler::ScopedNamespaceSetter sns(handler, NamespacedIdentifier());

	for (auto of : objectClassesWithJitCallableFunctions)
	{
		handler.addSymbol(of->getClassName(), TypeInfo(Types::ID::Pointer, true), NamespaceHandler::StaticFunctionClass);
	}

	for (auto rc : registeredClasses)
	{
		handler.addSymbol(rc->getClassName(), TypeInfo(Types::ID::Pointer, true), NamespaceHandler::StaticFunctionClass);
	}
}

bool GlobalScope::hasFunction(const NamespacedIdentifier& symbol) const
{
	for (auto of : objectClassesWithJitCallableFunctions)
	{
		if (of->hasFunction(symbol))
			return true;
	}

	for (auto rc : registeredClasses)
	{
		if (rc->hasFunction(symbol))
			return true;
	}

	return false;
}

void GlobalScope::addMatchingFunctions(Array<FunctionData>& matches, const NamespacedIdentifier& symbol) const
{
	FunctionClass::addMatchingFunctions(matches, symbol);

	for (auto of : objectClassesWithJitCallableFunctions)
		of->addMatchingFunctions(matches, symbol);
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
