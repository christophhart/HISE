/*
  ==============================================================================

    JitScope.cpp
    Created: 4 Apr 2017 9:19:51pm
    Author:  Christoph

  ==============================================================================
*/

#include "JitScope.h"



bool HiseJITScope::isFunction(const Identifier& id) const
{
	return pimpl->getCompiledBaseFunction(id) != nullptr;
}

#if INCLUDE_GLOBALS
bool HiseJITScope::isGlobal(const Identifier& id) const
{
	return pimpl->getGlobal(id) != nullptr;
}


int HiseJITScope::getNumArgsForFunction(const Identifier& id) const
{
	auto b = pimpl->getCompiledBaseFunction(id);

	return b->getNumParameters();
}


int HiseJITScope::getIndexForGlobal(const juce::Identifier& id) const
{
	return pimpl->getIndexForGlobal(id);
}


int HiseJITScope::getNumGlobalVariables() const
{
	return pimpl != nullptr ? pimpl->globals.size() : 0;
}

var HiseJITScope::getGlobalVariableValue(int globalIndex) const
{
	if (globalIndex < getNumGlobalVariables())
	{
		auto* g = pimpl->globals[globalIndex];

		if (HiseJITTypeHelpers::matchesType<float>(g->getType())) return var(GlobalBase::get<float>(g));
		if (HiseJITTypeHelpers::matchesType<double>(g->getType())) return var(GlobalBase::get<double>(g));
		if (HiseJITTypeHelpers::matchesType<int>(g->getType())) return var(GlobalBase::get<int>(g));
		if (HiseJITTypeHelpers::matchesType<BooleanType>(g->getType())) return GlobalBase::get<BooleanType>(g) > 0 ? var(true) : var(false);
#if INCLUDE_BUFFERS
		if (HiseJITTypeHelpers::matchesType<Buffer*>(g->getType())) return var(GlobalBase::getBuffer(g)->b.get());
#endif
	}

	return var();
}

TypeInfo HiseJITScope::getGlobalVariableType(int globalIndex) const
{
	if (globalIndex < getNumGlobalVariables())
	{
		return pimpl->globals[globalIndex]->getType();
	}

	return typeid(void);
}

Identifier HiseJITScope::getGlobalVariableName(int globalIndex) const
{
	if (globalIndex < getNumGlobalVariables())
	{
		return pimpl->globals[globalIndex]->id;
	}

	return Identifier();
}


void HiseJITScope::setGlobalVariable(const juce::Identifier& id, const juce::var& value)
{
	pimpl->setGlobalVariable(id, value);
}


void HiseJITScope::setGlobalVariable(int globalIndex, const juce::var& newValue)
{
	pimpl->setGlobalVariable(globalIndex, newValue);
}
#endif


template <typename ReturnType, typename...ParameterTypes> ReturnType(*HiseJITScope::getCompiledFunction(const Identifier& id))(ParameterTypes...)
{
	int parameterAmount = sizeof...(ParameterTypes);

	try
	{
		if (parameterAmount == 0)
		{
			return pimpl->getCompiledFunction0<ReturnType, ParameterTypes...>(id);
		}
		else if (parameterAmount == 1)
		{
			return pimpl->getCompiledFunction1<ReturnType, ParameterTypes...>(id);
		}
		else if (parameterAmount == 2)
		{
			return pimpl->getCompiledFunction2<ReturnType, ParameterTypes...>(id);
		}
	}
	catch (String e)
	{
		DBG(e);
		return nullptr;
	}


	return nullptr;
}


int HiseJITScope::isBufferOverflow(int globalIndex) const
{
	return pimpl->globals[globalIndex]->hasOverflowError();
}


bool HiseJITScope::hasProperty(const Identifier& propertyName) const
{
	for (int i = 0; i < pimpl->globals.size(); i++)
	{
		if (pimpl->globals[i]->id == propertyName)
			return true;
	}

	return false;
}



const var& HiseJITScope::getProperty(const Identifier& propertyName) const
{
	for (int i = 0; i < getNumGlobalVariables(); i++)
	{
		if (getGlobalVariableName(i) == propertyName)
		{
			cachedValues.set(propertyName, getGlobalVariableValue(i));
			return cachedValues[propertyName];
		}

	}

	return dummyVar;
}

void HiseJITScope::setProperty(const Identifier& propertyName, const var& newValue)
{
	setGlobalVariable(propertyName, newValue);
}

void HiseJITScope::removeProperty(const Identifier& /*propertyName*/)
{
	// Can't remove properties from a Native JIT scope
	jassertfalse;
}


bool HiseJITScope::hasMethod(const Identifier& methodName) const
{
	return pimpl->getCompiledBaseFunction(methodName) != nullptr;
}


var HiseJITScope::invokeMethod(Identifier methodName, const var::NativeFunctionArgs& args)
{
	BaseFunction* b = pimpl->getCompiledBaseFunction(methodName);

	const int numArgs = args.numArguments;

	TypeInfo returnType = b->getReturnType();

	TypeInfo parameter1Type = b->getTypeForParameter(0);

	TypeInfo parameter2Type = b->getTypeForParameter(1);

	const var& arg1 = args.numArguments > 0 ? args.arguments[0] : var();
	const var& arg2 = args.numArguments > 1 ? args.arguments[1] : var();

	switch (numArgs)
	{
	case 0:
	{
#define MATCH_TYPE_AND_RETURN(x) if (TYPE_MATCH(x, returnType)) { auto f = pimpl->getCompiledFunction0<x>(methodName); return var(f()); }

		MATCH_TYPE_AND_RETURN(int);
		MATCH_TYPE_AND_RETURN(double);
		MATCH_TYPE_AND_RETURN(float);
		MATCH_TYPE_AND_RETURN(Buffer*);
		MATCH_TYPE_AND_RETURN(BooleanType);

#undef MATCH_TYPE_AND_RETURN
	}
	case 1:
	{

#define MATCH_TYPE_AND_RETURN(rt, p1) if (TYPE_MATCH(rt, returnType) && TYPE_MATCH(p1, parameter1Type)) { auto f = pimpl->getCompiledFunction1<rt, p1>(methodName); return f((p1)arg1); }

		MATCH_TYPE_AND_RETURN(int, int);
		MATCH_TYPE_AND_RETURN(int, double);
		MATCH_TYPE_AND_RETURN(int, float);

		MATCH_TYPE_AND_RETURN(double, int);
		MATCH_TYPE_AND_RETURN(double, double);
		MATCH_TYPE_AND_RETURN(double, float);

		MATCH_TYPE_AND_RETURN(float, int);
		MATCH_TYPE_AND_RETURN(float, double);
		MATCH_TYPE_AND_RETURN(float, float);

#undef MATCH_TYPE_AND_RETURN
	}
	

	}

	return var();
}



