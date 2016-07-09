/*
  ==============================================================================

    JavascriptApiClass.h
    Created: 3 Jul 2016 7:52:58pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef JAVASCRIPTAPICLASS_H_INCLUDED
#define JAVASCRIPTAPICLASS_H_INCLUDED





struct VariantComparator
{
	int compareElements(const var &a, const var &b) const
	{
		if (isNumericOrUndefined(a) && isNumericOrUndefined(b))
			return (a.isDouble() || b.isDouble()) ? returnCompareResult<double>(a, b) : returnCompareResult<int>(a, b);

		if ((a.isUndefined() || a.isVoid()) && (b.isUndefined() || b.isVoid()))
			return 0;

		if (a.isArray() || a.isObject())
			throw String("Can't compare arrays or objects");

        return 0;
	};

private:

	template <typename PrimitiveType> int returnCompareResult(const var &first, const var&second) const noexcept
	{
		PrimitiveType f = static_cast<PrimitiveType>(first);
		PrimitiveType s = static_cast<PrimitiveType>(second);
		return (f == s) ? 0 : (f > s ? 1 : -1);
	}

	bool isNumericOrUndefined(const var &v) const
	{
		return v.isDouble() || v.isInt() || v.isInt64() || v.isUndefined() || v.isBool();
	}
};



#define NUM_VAR_REGISTERS 32

class VarRegister
{
public:

	VarRegister()
	{
		for (int i = 0; i < NUM_VAR_REGISTERS; i++)
		{
			registerStack[i] = var::undefined();
			registerStackIds[i] = Identifier::null;
		}
	}

	void addRegister(const Identifier &id, var newValue)
	{
		for (int i = 0; i < NUM_VAR_REGISTERS; i++)
		{
			if (registerStackIds[i] == id)
			{
				registerStack[i] = newValue;
				break;
			}

			if (registerStackIds[i].isNull())
			{
				registerStackIds[i] = Identifier(id);
				registerStack[i] = newValue;
				break;
			}
		}
	}

	void setRegister(int registerIndex, var newValue)
	{
		if (registerIndex < NUM_VAR_REGISTERS)
		{
			registerStack[registerIndex] = newValue;
		}
	}

	const var &getFromRegister(int registerIndex) const
	{
		if (registerIndex < NUM_VAR_REGISTERS)
		{
			return *(registerStack + registerIndex);
		}

		return var::undefined();
	}

	int getRegisterIndex(const Identifier &id) const
	{
		for (int i = 0; i < NUM_VAR_REGISTERS; i++)
		{
			if (registerStackIds[i] == id) return i;
		}

		return -1;
	}

	int getNumUsedRegisters() const
	{
		for (int i = 0; i < NUM_VAR_REGISTERS; i++)
		{
			if (registerStackIds[i] == Identifier::null) return i;
		}

		return NUM_VAR_REGISTERS;
	}

	Identifier getRegisterId(int index) const
	{
		if (index < NUM_VAR_REGISTERS)
			return registerStackIds[index];

		return Identifier::null;
	}

	const var *getVarPointer(int index) const { return registerStack + index; }

private:

	var registerStack[NUM_VAR_REGISTERS];
	Identifier registerStackIds[NUM_VAR_REGISTERS];
};


#define NUM_API_FUNCTION_SLOTS 32


/** A API class is a class with a fixed number of methods and constants that can be called from Javascript.
*
*	It is used to improve the performance of calling C++ functions from Javascript. This is achieved by resolving the function call at compile time
*	making the call from Javascript almost as fast as a regular C++ function call (the arguments still need to be evaluated).
*
*	You can also define constants which are resolved into literal values at compile time making them exactly as fast as typing the literal value.
*
*	A ApiClass needs to be registered on the C++ side before the ScriptingEngine parses and executes the code.
*/
class ApiClass : public ReferenceCountedObject
{
public:

	ApiClass(int numConstants_) :
		numConstants(numConstants_)
	{
		for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
		{
			functions0[i] = nullptr;
			functions1[i] = nullptr;
			functions2[i] = nullptr;
			functions3[i] = nullptr;
		}

		constants.ensureStorageAllocated(numConstants);

		for (int i = 0; i < numConstants_; i++)
		{
			constants.add(Constant());
		}
	};

	typedef std::function<var(ApiClass*)> call0;
	typedef std::function<var(ApiClass*, var)> call1;
	typedef std::function<var(ApiClass*, var, var)> call2;
	typedef std::function<var(ApiClass*, var, var, var)> call3;

	virtual Identifier getName() const = 0;

	void addConstant(String constantName, var value)
	{
		for (int i = 0; i < numConstants; i++)
		{
			if (constants[i].id.isNull())
			{
				constants.set(i, Constant(constantName, value));
			}
		}
	}

	const var getConstantValue(int index) const
	{
		return constants.getUnchecked(index).value;
	}

	int getConstantIndex(const Identifier &id) const
	{
		for (int i = 0; i < numConstants; i++)
		{
			if (constants[i].id == id) return i;
		}

		return -1;
	}

	void addFunction(const Identifier &id, call0 newFunction)
	{
		for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
		{
			if (functions0[i] == nullptr)
			{
				functions0[i] = newFunction;
				id0[i] = id;
				return;
			}
		}
	};

	void addFunction1(const Identifier &id, call1 newFunction)
	{
		for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
		{
			if (functions1[i] == nullptr)
			{
				functions1[i] = newFunction;
				id1[i] = id;
				return;
			}
		}
	}

	void addFunction2(const Identifier &id, call2 newFunction)
	{
		for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
		{
			if (functions2[i] == nullptr)
			{
				functions2[i] = newFunction;
				id2[i] = id;
				return;
			}
		}
	}

	void addFunction3(const Identifier &id, call3 newFunction)
	{
		for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
		{
			if (functions0[i] == nullptr)
			{
				functions3[i] = newFunction;
				id3[i] = id;
				return;
			}
		}
	}

	void getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const
	{
		for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
		{
			if (id0[i] == id)
			{
				index = i;
				numArgs = 0;
				return;
			}
			if (id1[i] == id)
			{
				index = i;
				numArgs = 1;
				return;
			}
		}
		index = -1;
		numArgs = -1;
	}

	var callFunction(int index, var *args, int numArgs)
	{
		if (index > NUM_API_FUNCTION_SLOTS)
		{
			return var::undefined();
		}

		switch (numArgs)
		{
		case 0: { auto f = functions0[index]; return f(this); }
		case 1: { auto f = functions1[index]; return f(this, args[0]); }
		case 2: { auto f = functions2[index]; return f(this, args[0], args[1]); }
		case 3: { auto f = functions3[index]; return f(this, args[0], args[1], args[2]); }
		}

		return var::undefined();
	}

private:

	Identifier id0[NUM_API_FUNCTION_SLOTS];
	Identifier id1[NUM_API_FUNCTION_SLOTS];
	Identifier id2[NUM_API_FUNCTION_SLOTS];
	Identifier id3[NUM_API_FUNCTION_SLOTS];

	call0 functions0[NUM_API_FUNCTION_SLOTS];
	call1 functions1[NUM_API_FUNCTION_SLOTS];
	call2 functions2[NUM_API_FUNCTION_SLOTS];
	call3 functions3[NUM_API_FUNCTION_SLOTS];

	struct Constant
	{
		Constant(const Identifier &id_, var value_) :
			id(id_),
			value(value_)
		{

		};

		Constant() :
			id(Identifier()),
			value(var())
		{};

		Constant& operator=(const Constant& other)
		{
			id = other.id;
			value = other.value;
			return *this;
		}

		

		static Constant null;
		Identifier id;
		var value;
	};

	Array<Constant> constants;
	const int numConstants;
};




class MidiM : public ApiClass
{
public:

	MidiM() :
		ApiClass(8),
		n(0)
	{
		ADD_API_METHOD_0(getNoteNumber);
		ADD_API_METHOD_1(setNoteNumber);
	};

	Identifier getName() const override { static const Identifier id("Message2"); return id; }

	var getNoteNumber() const
	{
		return n;
	}

	var setNoteNumber(int number)
	{
		n = number;

		return var::undefined();
	}

	struct Wrapper
	{
		API_METHOD_WRAPPER_1(MidiM, setNoteNumber)
		API_METHOD_WRAPPER_0(MidiM, getNoteNumber);
	};

	int n;
};

class MathFunctions : public ApiClass
{
public:

	MathFunctions() :
		ApiClass(1)
	{
		ADD_API_METHOD_1(sin);
		ADD_API_METHOD_1(abs);

		addConstant("PI", float_Pi);
	}

	Identifier getName() const override { static const Identifier id("Math2"); return id; }

	var sin(const var &input) const { return std::sin((double)input); }

	var abs(const var &input) const { return std::abs((double)input); }

	struct Wrapper
	{
		API_METHOD_WRAPPER_1(MathFunctions, sin)
		API_METHOD_WRAPPER_1(MathFunctions, abs);
	};

};




#endif  // JAVASCRIPTAPICLASS_H_INCLUDED
