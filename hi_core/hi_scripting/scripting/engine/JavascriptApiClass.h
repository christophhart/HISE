/*
  ==============================================================================

    JavascriptApiClass.h
    Created: 3 Jul 2016 7:52:58pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef JAVASCRIPTAPICLASS_H_INCLUDED
#define JAVASCRIPTAPICLASS_H_INCLUDED


#define NUM_VAR_REGISTERS 8




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

	const var getFromRegister(int registerIndex)
	{
		if (registerIndex < NUM_VAR_REGISTERS)
		{
			return *(registerStack + registerIndex);
		}

		return var::undefined();
	}

private:

	var registerStack[NUM_VAR_REGISTERS];
	Identifier registerStackIds[NUM_VAR_REGISTERS];
};


#define NUM_API_FUNCTION_SLOTS 32


class ApiObject2 : public ReferenceCountedObject
{
public:

	ApiObject2(int numConstants_) :
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


	typedef std::function<var(ApiObject2*)> call0;
	typedef std::function<var(ApiObject2*, var)> call1;
	typedef std::function<var(ApiObject2*, var, var)> call2;
	typedef std::function<var(ApiObject2*, var, var, var)> call3;

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

		Constant& operator=(const Constant&& other)
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



#define ADD_FUNCTION_0(name) addFunction(Identifier(#name), &Wrapper::name)
#define ADD_FUNCTION_1(name) addFunction1(Identifier(#name), &Wrapper::name)
#define ADD_FUNCTION_2(name) addFunction2(Identifier(#name), &Wrapper::name)
#define ADD_FUNCTION_3(name) addFunction3(Identifier(#name), &Wrapper::name)

#define WRAPPER_FUNCTION_0(className, name)	inline static var name(ApiObject2 *m) { return static_cast<className*>(m)->name(); };
#define WRAPPER_FUNCTION_1(className, name)	inline static var name(ApiObject2 *m, var value1) { return static_cast<className*>(m)->name(value1); };
#define WRAPPER_FUNCTION_2(className, name)	inline static var name(ApiObject2 *m, var value1, var value2) { return static_cast<className*>(m)->name(value1, value2); };
#define WRAPPER_FUNCTION_3(className, name)	inline static var name(ApiObject2 *m, var value1, var value2, var value3) { return static_cast<className*>(m)->name(value1, value2, value3); };
#define WRAPPER_FUNCTION_4(className, name) inline static var name(ApiObject2 *m, var value1, var value2, var value3, var value4) { return static_cast<className*>(m)->name(value1, value2, value3, value4); };

class MidiM : public ApiObject2
{
public:

	MidiM() :
		ApiObject2(8),
		n(0)
	{
		ADD_FUNCTION_0(getNoteNumber);
		ADD_FUNCTION_1(setNoteNumber);
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
		WRAPPER_FUNCTION_1(MidiM, setNoteNumber)
			WRAPPER_FUNCTION_0(MidiM, getNoteNumber);
	};

	int n;
};

class MathFunctions : public ApiObject2
{
public:

	MathFunctions() :
		ApiObject2(2)
	{
		ADD_FUNCTION_1(sin);
		ADD_FUNCTION_1(abs);
		addConstant("PI", float_Pi);
	}

	Identifier getName() const override { static const Identifier id("Math2"); return id; }

	var sin(const var &input) const
	{
		return sinf((double)input);
	}

	var abs(const var &input) const
	{
		return fabs((double)input);
	}

	struct Wrapper
	{
		WRAPPER_FUNCTION_1(MathFunctions, sin)
		WRAPPER_FUNCTION_1(MathFunctions, abs);
	};

};



#endif  // JAVASCRIPTAPICLASS_H_INCLUDED
