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

class IdentifierComparator
{
public:

	int compareElements(const Identifier &a, const Identifier &b) const
	{
		if (a.toString() > b.toString()) return 1;
		if (a.toString() < b.toString()) return -1;

		return 0;
	};
};


#define NUM_VAR_REGISTERS 32

/** A VarRegister is a container with a fixed amount of var slots that can be used for faster variable access in the Javascript engine. */
class VarRegister
{
public:

	// ================================================================================================================

	VarRegister();
	VarRegister(VarRegister &other);
	~VarRegister();

	VarRegister& operator=(const VarRegister &) { return *this; }

	// ================================================================================================================

	void addRegister(const Identifier &id, var newValue);
	void setRegister(int registerIndex, var newValue);

	const var &getFromRegister(int registerIndex) const;
	int getRegisterIndex(const Identifier &id) const;
	int getNumUsedRegisters() const;
	Identifier getRegisterId(int index) const;
	const var *getVarPointer(int index) const;

private:

	// ================================================================================================================

	var registerStack[NUM_VAR_REGISTERS];
	Identifier registerStackIds[NUM_VAR_REGISTERS];

	const var empty;

	// ================================================================================================================
};


#define NUM_API_FUNCTION_SLOTS 32


/** A API class is a class with a fixed number of methods and constants that can be called from Javascript.
*
*	It is used to improve the performance of calling C++ functions from Javascript. This is achieved by resolving the function call at compile time
*	making the call from Javascript almost as fast as a regular C++ function call (the arguments still need to be evaluated).
*
*	You can also define constants which are resolved into literal values at compile time making them exactly as fast as typing the literal value.
*
*	A ApiClass needs to be registered on the C++ side before the ScriptingEngine parses and executes the code using
*/
class ApiClass : public ReferenceCountedObject
{
public:

	// ================================================================================================================

	typedef var(*call0)(ApiClass*);
	typedef var(*call1)(ApiClass*, var);
	typedef var(*call2)(ApiClass*, var, var);
	typedef var(*call3)(ApiClass*, var, var, var);
	typedef var(*call4)(ApiClass*, var, var, var, var);
	typedef var(*call5)(ApiClass*, var, var, var, var, var);

	// ================================================================================================================

	ApiClass(int numConstants_);;
	virtual ~ApiClass();

	virtual Identifier getName() const = 0;

	// ================================================================================================================

	void addConstant(String constantName, var value);
	const var getConstantValue(int index) const;
	int getConstantIndex(const Identifier &id) const;

	// ================================================================================================================

	void addFunction(const Identifier &id, call0 newFunction);;
	void addFunction1(const Identifier &id, call1 newFunction);
	void addFunction2(const Identifier &id, call2 newFunction);
	void addFunction3(const Identifier &id, call3 newFunction);
	void addFunction4(const Identifier &id, call4 newFunction);
	void addFunction5(const Identifier &id, call5 newFunction);

	void getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const;
	var callFunction(int index, var *args, int numArgs);

	void getAllFunctionNames(Array<Identifier> &ids) const;
	void getAllConstants(Array<Identifier> &ids) const;

private:

	// ================================================================================================================

	Identifier id0[NUM_API_FUNCTION_SLOTS];
	Identifier id1[NUM_API_FUNCTION_SLOTS];
	Identifier id2[NUM_API_FUNCTION_SLOTS];
	Identifier id3[NUM_API_FUNCTION_SLOTS];
	Identifier id4[NUM_API_FUNCTION_SLOTS];
	Identifier id5[NUM_API_FUNCTION_SLOTS];

	call0 functions0[NUM_API_FUNCTION_SLOTS];
	call1 functions1[NUM_API_FUNCTION_SLOTS];
	call2 functions2[NUM_API_FUNCTION_SLOTS];
	call3 functions3[NUM_API_FUNCTION_SLOTS];
	call4 functions4[NUM_API_FUNCTION_SLOTS];
	call5 functions5[NUM_API_FUNCTION_SLOTS];

	// ================================================================================================================

	struct Constant
	{
		Constant(const Identifier &id_, var value_);
		Constant();
		Constant& operator=(const Constant& other);

		static Constant null;
		Identifier id;
		var value;
	};

	Constant constants[8];
	const int numConstants;

	Constant* constantsToUse;

	Array<Constant> constantBigStorage;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApiClass)

	// ================================================================================================================
};


#endif  // JAVASCRIPTAPICLASS_H_INCLUDED
