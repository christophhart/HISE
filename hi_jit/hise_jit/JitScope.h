/*
  ==============================================================================

    JitScope.h
    Created: 4 Apr 2017 9:19:51pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef JITSCOPE_H_INCLUDED
#define JITSCOPE_H_INCLUDED



class HiseJITScope::Pimpl : public DynamicObject
{
	friend class HiseJITScope;

public:

	Pimpl()
	{
		runtime = new asmjit::JitRuntime();

		addExposedFunctions();
	}

	~Pimpl()
	{
		compiledFunctions.clear();
		exposedFunctions.clear();

		runtime = nullptr;
	}

	void addExposedFunctions()
	{
		NATIVE_JIT_ADD_C_FUNCTION_1(float, sinf, float);
		NATIVE_JIT_ADD_C_FUNCTION_1(double, sin, double);
		NATIVE_JIT_ADD_C_FUNCTION_1(float, cosf, float);
		NATIVE_JIT_ADD_C_FUNCTION_1(double, cos, double);
		NATIVE_JIT_ADD_C_FUNCTION_1(float, tanf, float);
		NATIVE_JIT_ADD_C_FUNCTION_1(double, tan, double);
		NATIVE_JIT_ADD_C_FUNCTION_1(float, atanf, float);
		NATIVE_JIT_ADD_C_FUNCTION_1(double, atan, double);
		NATIVE_JIT_ADD_C_FUNCTION_1(float, atanhf, float);
		NATIVE_JIT_ADD_C_FUNCTION_1(double, atanh, double);
		NATIVE_JIT_ADD_C_FUNCTION_2(float, powf, float, float);
		NATIVE_JIT_ADD_C_FUNCTION_2(double, pow, double, double);
		NATIVE_JIT_ADD_C_FUNCTION_1(double, sqrt, double);
		NATIVE_JIT_ADD_C_FUNCTION_1(float, sqrtf, float);

		NATIVE_JIT_ADD_C_FUNCTION_1(float, tanhf, float);
		NATIVE_JIT_ADD_C_FUNCTION_1(double, tanh, double);

		NATIVE_JIT_ADD_C_FUNCTION_1(double, fabs, double);
		NATIVE_JIT_ADD_C_FUNCTION_1(int, abs, int);

		NATIVE_JIT_ADD_C_FUNCTION_1(float, fabsf, float);
		NATIVE_JIT_ADD_C_FUNCTION_1(double, exp, double);
		NATIVE_JIT_ADD_C_FUNCTION_1(float, expf, float);

	}

	BaseFunction* getExposedFunction(const Identifier& id)
	{
		for (int i = 0; i < exposedFunctions.size(); i++)
		{
			if (exposedFunctions[i]->functionName == id)
			{
				return exposedFunctions[i];
			}
		}

		return nullptr;
	}

#if INCLUDE_GLOBALS
	GlobalBase* getGlobal(const Identifier& id)
	{
		for (int i = 0; i < globals.size(); i++)
		{
			if (globals[i]->id == id) return globals[i];
		}

		return nullptr;
	}


	void setGlobalVariable(int globalIndex, const var& value)
	{
		if (auto g = globals[globalIndex])
		{
			TypeInfo t = g->getType();

			if (value.isInt64() || value.isDouble() || value.isInt())
			{
				if (HiseJITTypeHelpers::matchesType<float>(t)) GlobalBase::store<float>(g, (float)value);
				else if (HiseJITTypeHelpers::matchesType<double>(t)) GlobalBase::store<double>(g, (double)value);
				else if (HiseJITTypeHelpers::matchesType<int>(t)) GlobalBase::store<int>(g, (int)value);
				else if (HiseJITTypeHelpers::matchesType<BooleanType>(t)) GlobalBase::store<BooleanType>(g, (int)value > 0 ? 1 : 0);
				else throw String(g->id.toString() + " - var type mismatch: " + value.toString());
			}
#if INCLUDE_BUFFERS
			else if (value.isBuffer() && HiseJITTypeHelpers::matchesType<Buffer*>(t))
			{
				g->setBuffer(value.getBuffer());
			}
#endif
			else
			{
				throw String(g->id.toString() + " - var type mismatch: " + value.toString());
			}
		}
	}

	void setGlobalVariable(const juce::Identifier& id, const juce::var& value)
	{
		setGlobalVariable(getIndexForGlobal(id), value);
	}

	int getIndexForGlobal(const juce::Identifier& id) const
	{
		for (int i = 0; i < globals.size(); i++)
		{
			if (globals[i]->id == id) return i;
		}

		return -1;
	}
#endif

	BaseFunction* getCompiledBaseFunction(const Identifier& id)
	{
		for (int i = 0; i < compiledFunctions.size(); i++)
		{
			if (compiledFunctions[i]->functionName == id)
			{
				return compiledFunctions[i];
			}
		}

		return nullptr;
	}

	template <typename ExpectedType> void checkTypeMatch(BaseFunction* b, int parameterIndex)
	{
		if (parameterIndex == -1)
		{
			if (!HiseJITTypeHelpers::matchesType<ExpectedType>(b->getReturnType()))
			{
				throw String("Return type of function \"" + b->functionName + "\" does not match. ") + HiseJITTypeHelpers::getTypeMismatchErrorMessage<ExpectedType>(b->getReturnType());
			}
		}
		else
		{
			if (!HiseJITTypeHelpers::matchesType<ExpectedType>(b->getTypeForParameter(parameterIndex)))
			{
				throw String("Parameter " + String(parameterIndex + 1) + " type of function \"" + b->functionName + "\" does not match. ") + HiseJITTypeHelpers::getTypeMismatchErrorMessage<ExpectedType>(b->getReturnType());
			}
		}
	}

	template <typename ReturnType> ReturnType(*getCompiledFunction1(const Identifier& /*id*/))() { return nullptr; }
	template <typename ReturnType, typename ParamType1> ReturnType(*getCompiledFunction2(const Identifier& /*id*/))(ParamType1) { return nullptr; }
	template <typename ReturnType> ReturnType(*getCompiledFunction2(const Identifier& /*id*/))() { return nullptr; }

#pragma warning(push)
#pragma warning (disable: 4127)

	template <typename ReturnType, typename... Other> ReturnType(*getCompiledFunction0(const Identifier& id))(Other...)
	{
		if (sizeof...(Other) != 0) return nullptr;

		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);

			return (ReturnType(*)(Other...))b->func;
		}

		return nullptr;
	}

	template <typename ReturnType, typename ParamType1, typename... Other> ReturnType(*getCompiledFunction1(const Identifier& id))(ParamType1, Other...)
	{
		if (sizeof...(Other) != 0) return nullptr;

		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);
			checkTypeMatch<ParamType1>(b, 0);

			return (ReturnType(*)(ParamType1, Other...))b->func;
		}

		return nullptr;
	}

#pragma warning (pop)

	template <typename ReturnType, typename ParamType1, typename ParamType2> ReturnType(*getCompiledFunction2(const Identifier& id))(ParamType1, ParamType2)
	{
		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);
			checkTypeMatch<ParamType1>(b, 0);
			checkTypeMatch<ParamType2>(b, 1);

			return (ReturnType(*)(ParamType1, ParamType2))b->func;
		}

		return nullptr;
	}


#if INCLUDE_GLOBALS
	OwnedArray<GlobalBase> globals;
#endif

	OwnedArray<BaseFunction> compiledFunctions;

	OwnedArray<BaseFunction> exposedFunctions;

	ScopedPointer<asmjit::JitRuntime> runtime;

	typedef ReferenceCountedObjectPtr<HiseJITScope> Ptr;
};




#endif  // JITSCOPE_H_INCLUDED
