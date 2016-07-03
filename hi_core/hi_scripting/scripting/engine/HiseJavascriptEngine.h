/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
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

#ifndef HISEJAVASCRIPTENGINE_H_INCLUDED
#define HISEJAVASCRIPTENGINE_H_INCLUDED

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


class ApiObject2: public ReferenceCountedObject
{
public:

	ApiObject2(int numConstants_):
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
		Constant(const Identifier &id_, var value_):
			id(id_),
			value(value_)
		{

		};

		Constant():
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

class MidiM: public ApiObject2
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

	MathFunctions():
		ApiObject2(2)
	{
		ADD_FUNCTION_1(sin);
		addConstant("PI", float_Pi);
	}

	Identifier getName() const override { static const Identifier id("Math2"); return id; }

	var sin(const var &input) const
	{
		return sinf((double)input);
	}
	
	struct Wrapper
	{
		WRAPPER_FUNCTION_1(MathFunctions, sin)
	};

};


/**
A simple javascript interpreter!

It's not fully standards-compliant, and won't be as fast as the fancy JIT-compiled
engines that you get in browsers, but this is an extremely compact, low-overhead javascript
interpreter, which is integrated with the juce var and DynamicObject classes. If you need
a few simple bits of scripting in your app, and want to be able to easily let the JS
work with native objects defined as DynamicObject subclasses, then this might do the job.

To use, simply create an instance of this class and call execute() to run your code.
Variables that the script sets can be retrieved with evaluate(), and if you need to provide
native objects for the script to use, you can add them with registerNativeObject().

One caveat: Because the values and objects that the engine works with are DynamicObject
and var objects, they use reference-counting rather than garbage-collection, so if your
script creates complex connections between objects, you run the risk of creating cyclic
dependencies and hence leaking.
*/
class HiseJavascriptEngine
{
public:
	/** Creates an instance of the engine.
	This creates a root namespace and defines some basic Object, String, Array
	and Math library methods.
	*/
	HiseJavascriptEngine();

	/** Destructor. */
	~HiseJavascriptEngine();

	/** Attempts to parse and run a block of javascript code.
	If there's a parse or execution error, the error description is returned in
	the result.
	You can specify a maximum time for which the program is allowed to run, and
	it'll return with an error message if this time is exceeded.
	*/
	Result execute(const String& javascriptCode);

	/** Attempts to parse and run a javascript expression, and returns the result.
	If there's a syntax error, or the expression can't be evaluated, the return value
	will be var::undefined(). The errorMessage parameter gives you a way to find out
	any parsing errors.
	You can specify a maximum time for which the program is allowed to run, and
	it'll return with an error message if this time is exceeded.
	*/
	var evaluate(const String& javascriptCode,
		Result* errorMessage = nullptr);

	/** Calls a function in the root namespace, and returns the result.
	The function arguments are passed in the same format as used by native
	methods in the var class.
	*/
	var callFunction(const Identifier& function,
		const var::NativeFunctionArgs& args,
		Result* errorMessage = nullptr);

    
	var executeWithoutAllocation(const Identifier &function,
		const var::NativeFunctionArgs& args,
		Result* errorMessage = nullptr, DynamicObject *scopeToUse=nullptr);

	/** Adds a native object to the root namespace.
	The object passed-in is reference-counted, and will be retained by the
	engine until the engine is deleted. The name must be a simple JS identifier,
	without any dots.
	*/
	void registerNativeObject(const Identifier& objectName, DynamicObject* object);



	/** This value indicates how long a call to one of the evaluate methods is permitted
	to run before timing-out and failing.
	The default value is a number of seconds, but you can change this to whatever value
	suits your application.
	*/
	RelativeTime maximumExecutionTime;

	/** Provides access to the set of properties of the root namespace object. */
	const NamedValueSet& getRootObjectProperties() const noexcept;

	DynamicObject *getRootObject();;

	void registerApiClass(ApiObject2 *apiClass);

private:
	
	//==============================================================================
	struct RootObject : public DynamicObject
	{
		RootObject();

		Time timeout;

		typedef const var::NativeFunctionArgs& Args;
		typedef const char* TokenType;

		VarRegister varRegister;

		ReferenceCountedArray<ApiObject2> apiClasses;
		Array<Identifier> apiIds;

		void execute(const String& code);

		var evaluate(const String& code);

		//==============================================================================
		static bool areTypeEqual(const var& a, const var& b)
		{
			return a.hasSameTypeAs(b) && isFunction(a) == isFunction(b)
				&& (((a.isUndefined() || a.isVoid()) && (b.isUndefined() || b.isVoid())) || a == b);
		}

		static String getTokenName(TokenType t);
		static bool isFunction(const var& v) noexcept;
		static bool isNumeric(const var& v) noexcept;
		static bool isNumericOrUndefined(const var& v) noexcept;
		static int64 getOctalValue(const String& s);
		static Identifier getPrototypeIdentifier();
		static var* getPropertyPointer(DynamicObject* o, const Identifier& i) noexcept;

		//==============================================================================
		struct CodeLocation;
		struct Scope;
		struct Statement;
		struct Expression;

		typedef ScopedPointer<Expression> ExpPtr;

		struct BinaryOperatorBase;	struct BinaryOperator;

		//==============================================================================

		// Logical Operators

		struct LogicalAndOp;			struct LogicalOrOp;
		struct TypeEqualsOp;			struct TypeNotEqualsOp;
		struct ConditionalOp;

		struct EqualsOp;				struct NotEqualsOp;			struct LessThanOp;
		struct LessThanOrEqualOp;		struct GreaterThanOp;		struct GreaterThanOrEqualOp;

		// Arithmetic

		struct AdditionOp;				struct SubtractionOp;
		struct MultiplyOp;				struct DivideOp;			struct ModuloOp;
		struct BitwiseAndOp;			struct BitwiseOrOp;			struct BitwiseXorOp;
		struct LeftShiftOp;				struct RightShiftOp;		struct RightShiftUnsignedOp;

		// Branching

		struct BlockStatement; 			struct IfStatement;			struct ContinueStatement;
		struct CaseStatement; 			struct SwitchStatement;
		struct ReturnStatement; 		struct BreakStatement;
		struct NextIteratorStatement; 	struct LoopStatement;

		// Variables

		struct VarStatement;			struct LiteralValue; 		struct UnqualifiedName;
		struct ArraySubscript;			struct Assignment;
		struct SelfAssignment;			struct PostAssignment;

		// Function / Objects

		struct FunctionCall;			struct NewOperator;			struct DotOperator;
		struct ObjectDeclaration;		struct ArrayDeclaration;	struct FunctionObject;

		// HISE special

		struct RegisterVarStatement;	struct RegisterName;		struct RegisterAssignment;
		struct ApiConstant;				struct ApiCall;				struct InlineFunction;

		// Parser classes

		struct TokenIterator;
		struct ExpressionTreeBuilder;

		//==============================================================================
		static var get(Args a, int index) noexcept{ return index < a.numArguments ? a.arguments[index] : var(); }
		static bool isInt(Args a, int index) noexcept{ return get(a, index).isInt() || get(a, index).isInt64(); }
		static int getInt(Args a, int index) noexcept{ return get(a, index); }
		static double getDouble(Args a, int index) noexcept{ return get(a, index); }
		static String getString(Args a, int index) noexcept{ return get(a, index).toString(); }

		// Object classes

		struct MathClass;
		struct IntegerClass;
		struct ObjectClass;
		struct ArrayClass;
		struct StringClass;
		struct JSONClass;

		//==============================================================================
		static var trace(Args a)      { Logger::outputDebugString(JSON::toString(a.thisObject)); return var::undefined(); }
		static var charToInt(Args a)  { return (int)(getString(a, 0)[0]); }

		static var typeof_internal(Args a);

		static var exec(Args a);

		static var eval(Args a);
	};

	const ReferenceCountedObjectPtr<RootObject> root;
	void prepareTimeout() const noexcept;

	DynamicObject::Ptr unneededScope;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseJavascriptEngine)
};




#endif  // HISEJAVASCRIPTENGINE_H_INCLUDED
