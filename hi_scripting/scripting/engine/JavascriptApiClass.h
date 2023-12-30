/*
  ==============================================================================

    JavascriptApiClass.h
    Created: 3 Jul 2016 7:52:58pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef JAVASCRIPTAPICLASS_H_INCLUDED
#define JAVASCRIPTAPICLASS_H_INCLUDED

namespace hise { using namespace juce;


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

#define DECLARE_ID(id) static const Identifier id(#id);

namespace VarTypeIdentifiers
{
    static const Identifier Undefined("undefined");
    static const Identifier Integer("int");
    static const Identifier Double("double");
    static const Identifier Number("number");
    static const Identifier String("string");
    DECLARE_ID(Colour);
    DECLARE_ID(Array);
    DECLARE_ID(Buffer);
    DECLARE_ID(ObjectWithLength);
    DECLARE_ID(JSON);
    DECLARE_ID(ScriptObject);
    DECLARE_ID(IndexOrArray);
    static const Identifier Object("object");
    DECLARE_ID(AudioData);
    DECLARE_ID(Function);
    DECLARE_ID(ComplexType);
    DECLARE_ID(NotUndefined);
}

#undef DECLARE_ID

/** A helper class that checks the type of var objects. */
struct VarTypeChecker
{
    // This is taken from the inline function
    static constexpr int NumMaxArgs = 5;
    
    enum VarTypes: uint8
    {
        Undefined = 0,
        Integer = 1,
        Double = 2,
        Number = 3,
        String = 4,
        Colour = String | Number,
        Array = 8,
        IndexOrArray = Array | Integer,
        Buffer = 16,
        AudioData = 8 | 16,
        ObjectWithLength = 4 | 8 | 16,
        JSON = 32,
        ScriptObject = 64,
        Object = 32 | 64,
        Function = 128,
        ComplexType = 4 | 8 | 16 | 32 | 64 | 128,
        NotUndefined = ComplexType | Number
    };
    
    using ParameterTypes = std::array<VarTypes, NumMaxArgs>;
    
    static Identifier getTypeName(VarTypes type)
    {
        switch(type)
        {
            case Undefined: return VarTypeIdentifiers::Undefined;
            case Integer: return VarTypeIdentifiers::Integer;
            case Double: return VarTypeIdentifiers::Double;
            case Number: return VarTypeIdentifiers::Number;
            case String: return VarTypeIdentifiers::String;
            case Colour: return VarTypeIdentifiers::Colour;
            case Array: return VarTypeIdentifiers::Array;
            case IndexOrArray: return VarTypeIdentifiers::IndexOrArray;
            case AudioData: return VarTypeIdentifiers::AudioData;
            case Buffer: return VarTypeIdentifiers::Buffer;
            case ObjectWithLength: return VarTypeIdentifiers::ObjectWithLength;
            case JSON: return VarTypeIdentifiers::JSON;
            case ScriptObject: return VarTypeIdentifiers::ScriptObject;
            case Object: return VarTypeIdentifiers::Object;
            case Function: return VarTypeIdentifiers::Function;
            case ComplexType: return VarTypeIdentifiers::ComplexType;
            default: jassertfalse; return "Unknown";
        }
    }
    
    static forcedinline VarTypes getTypeFromString(const juce::Identifier& name)
    {
        if(name == VarTypeIdentifiers::Undefined) return VarTypes::Undefined;
        if(name == VarTypeIdentifiers::Integer) return VarTypes::Integer;
        if(name == VarTypeIdentifiers::Double) return VarTypes::Double;
        if(name == VarTypeIdentifiers::Number) return VarTypes::Number;
        if(name == VarTypeIdentifiers::String) return VarTypes::String;
        if(name == VarTypeIdentifiers::Colour) return VarTypes::Colour;
        if(name == VarTypeIdentifiers::IndexOrArray) return VarTypes::IndexOrArray;
        if(name == VarTypeIdentifiers::Array) return VarTypes::Array;
        if(name == VarTypeIdentifiers::AudioData) return VarTypes::AudioData;
        if(name == VarTypeIdentifiers::Buffer) return VarTypes::Buffer;
        if(name == VarTypeIdentifiers::ObjectWithLength) return VarTypes::ObjectWithLength;
        if(name == VarTypeIdentifiers::JSON) return VarTypes::JSON;
        if(name == VarTypeIdentifiers::ScriptObject) return VarTypes::ScriptObject;
        if(name == VarTypeIdentifiers::Object) return VarTypes::Object;
        if(name == VarTypeIdentifiers::Function) return VarTypes::Function;
        if(name == VarTypeIdentifiers::ComplexType) return VarTypes::ComplexType;
        if(name == VarTypeIdentifiers::NotUndefined) return VarTypes::NotUndefined;
        
        return VarTypes::Undefined;
    }
    
    static VarTypes getType(const var& value);
    
    
    static ParameterTypes createParameterTypes(VarTypes t1,
                                               VarTypes t2=VarTypes::Undefined,
                                               VarTypes t3=VarTypes::Undefined,
                                               VarTypes t4=VarTypes::Undefined,
                                               VarTypes t5=VarTypes::Undefined)
    {
        return {t1, t2, t3, t4, t5 };
    }
    
    ParameterTypes createParameterTypesFromNames(const StringArray& names)
    {
        ParameterTypes t;
        int index = 0;
        
        for(const auto& n: names)
        {
            if(index == NumMaxArgs)
            {
                jassertfalse;
                break;
            }
            
            t[index++] = getTypeFromString(n);
        }
        
        return t;
    }
    
    /** Creates a parameter type set for the given*/
    static ParameterTypes createParameterTypes(const juce::Array<var>& values)
    {
        ParameterTypes t;
        
        int index = 0;
        
        for(const auto& v: values)
        {
            if(index == NumMaxArgs)
            {
                // You can' use this with more than NumMaxArgs parameters
                jassertfalse;
                break;
            }
                
            t[index++] = getType(v);
        }
        
        return t;
    }
    
#if ENABLE_SCRIPTING_SAFE_CHECKS
    static Result checkType(const var& value, VarTypes expectedType, bool allowUndefined=false)
    {
        auto t = getType(value);
        
        auto matches = (t & expectedType) != 0;
        
        if(!matches && allowUndefined)
            matches = t == VarTypes::Undefined;
        
        if(!matches)
        {
            juce::String errorMessage;
            errorMessage << "Illegal type: " << getTypeName(getType(value));
            errorMessage << ", expected: " << getTypeName(expectedType);
            
            return Result::fail(errorMessage);
        }
        
        return Result::ok();
    }
#endif
};

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

	void addRegister(const Identifier &id, var newValue, VarTypeChecker::VarTypes expectedType=VarTypeChecker::Undefined);
	void setRegister(int registerIndex, var newValue);

	const var &getFromRegister(int registerIndex) const;
	int getRegisterIndex(const Identifier &id) const;
	int getNumUsedRegisters() const;
	Identifier getRegisterId(int index) const;
	const var *getVarPointer(int index) const;
	var *getVarPointer(int index);

    VarTypeChecker::VarTypes getRegisterVarType(int index) const
    {
        ignoreUnused(index);
        
#if ENABLE_SCRIPTING_SAFE_CHECKS
        if(isPositiveAndBelow(index, NUM_VAR_REGISTERS))
        {
            return registerTypes[index];
        }
#endif
        
        return VarTypeChecker::Undefined;
    }
    
	ReadWriteLock& getLock(int index);

private:

	// ================================================================================================================

	var registerStack[NUM_VAR_REGISTERS];
    
#if ENABLE_SCRIPTING_SAFE_CHECKS
    VarTypeChecker::VarTypes registerTypes[NUM_VAR_REGISTERS];
#endif
    
	Identifier registerStackIds[NUM_VAR_REGISTERS];
	ReadWriteLock registerLocks[NUM_VAR_REGISTERS];

	const var empty;

	// ================================================================================================================
};


#define NUM_API_FUNCTION_SLOTS 64


/** A API class is a class with a fixed number of methods and constants that can be called from Javascript.
*
*	It is used to improve the performance of calling C++ functions from Javascript. 
*   This is achieved by resolving the function call at compile time making the call from Javascript almost as fast 
*   as a regular C++ function call (the arguments still need to be evaluated).
*
*	You can also define constants which are resolved into literal values at compile time making them exactly 
*   as fast as typing the literal value.
*
*	A ApiClass needs to be registered on the C++ side before the ScriptingEngine parses and executes the code using
*   HiseJavascriptEngine::registerApiClass().
*
*   To use this class, subclass it, write the methods you want to expose to Javascript as member functions of your
*   subclass and use these two macros to publish them to the Javascript Engine:
*
 *      @code{.cpp}
 *		class MyApiClass: public ApiClass
 *		{
 *		public:
 *		   MyApiClass():
 *		     ApiClass(0) // we don't need constants here...
 *		   {
 *		       ADD_API_METHOD_1(square); // This adds the method wrapper to the function list
 *		   }
 *
 *
 *		   double square(double x) const
 *		   {
 *		       return x*x;
 *		   };
 *
 *		   struct Wrapper // You'll need a subclass called `Wrapper` for it to work...
 *		   {
 *		   		// this defines a wrapper function that is using a static_cast to call the member function.
 *		 		ADD_API_METHOD_WRAPPER_1(MyApiClass, square);
 *		   };
 *		};
        @endcode
 *
 *  The Macros support up to 5 arguments. You'll need another macro for void functions: `API_VOID_METHOD_WRAPPER_X`,
 *  but apart from this, it is pretty straight forward...
 *
 *  For a living example of an API class take a look at eg. the Math class or the ScriptingApi::Engine class.
 */
class ApiClass : public ReferenceCountedObject,
				 public DebugableObjectBase
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

    /** Creates a Api class with the given amount of constants.
    *
    *   If numConstants_ is not zero, you'll need to add the constants in your subclass constructor using addConstant().
    *   You also need to register every function in your class there.
    */
	ApiClass(int numConstants_);;
	virtual ~ApiClass();

	/** You can overwrite this method and return true if you want to allow illegal calls that would otherwise
	*	fire a warning. This is eg. used in the Console class to prevent firing when debugging. */
	virtual bool allowIllegalCallsOnAudioThread(int /*functionIndex*/) const { return false; }

	String getDebugName() const override { return getObjectName().toString(); }

	String getDebugValue() const override { return ""; }

	bool isWatchable() const override { return false; };

	// ================================================================================================================

    /** Adds a constant. You can give it a name (it must be a valid Identifier) and a value and will be resolved at
    *   compile time to accelerate the access to it. */
	void addConstant(String constantName, var value);
    
    /** Returns the constant at the given index. */
	const var getConstantValue(int index) const;
    
    /** Return the index for the given name. */
	int getConstantIndex(const Identifier &id) const;

	/** Returns the name for the constant as it is used in the scripting context. */
	Identifier getConstantName(int index) const;

	// ================================================================================================================

    /** Adds a function with no parameters. 
    *
    *   You don't need to use this directly, but use the macro ADD_API_METHOD_0() for it. */
	void addFunction(const Identifier &id, call0 newFunction);;
	
    /** Adds a function with one parameter.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_1() for it. */
    void addFunction1(const Identifier &id, call1 newFunction);
	
    /** Adds a function with two parameters.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_2() for it. */
    void addFunction2(const Identifier &id, call2 newFunction);
	
    /** Adds a function with three parameters.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_3() for it. */
    void addFunction3(const Identifier &id, call3 newFunction);
	
    /** Adds a function with four parameters.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_4() for it. */
    void addFunction4(const Identifier &id, call4 newFunction);
	
    /** Adds a function with five parameters.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_5() for it. */
    void addFunction5(const Identifier &id, call5 newFunction);

#if ENABLE_SCRIPTING_SAFE_CHECKS
    void addForcedParameterTypes(const Identifier& id, const VarTypeChecker::ParameterTypes& types)
    {
        ForcedParameterInfo info;
        
        info.id = id;
        info.types = types;
        
        getIndexAndNumArgsForFunction(id, info.index, info.numArgs);
        forcedParameterTypes.add(info);
    }
#endif
    
    VarTypeChecker::ParameterTypes getForcedParameterTypes(int index, int numArgs) const
    {
#if ENABLE_SCRIPTING_SAFE_CHECKS
        for(const auto& t: forcedParameterTypes)
        {
            if(t.index == index && t.numArgs == numArgs)
                return t.types;
        }
#endif
        
        return {};

    }
    
    VarTypeChecker::ParameterTypes getForcedParameterTypes(const Identifier& id) const
    {
#if ENABLE_SCRIPTING_SAFE_CHECKS
        for(const auto& t: forcedParameterTypes)
        {
            if(t.id == id)
                return t.types;
        }
#endif
        
        return {};
    }
    
    /** This will fill in the information for the given function. 
    *
    *   The JavascriptEngine uses this to resolve the function call into a function pointer at compile time.
    *   When the script is executed, this information will be used for blazing fast access to the methods.*/
	void getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const;
    
    /** Calls the function with the index and the argument data.
    *
    *   You'll need to call getIndexAndNumArgsForFunction() before calling this. */
	var callFunction(int index, var *args, int numArgs);

    /** This returns all function names alphabetically sorted. This is used by the autocomplete popup. */
	void getAllFunctionNames(Array<Identifier> &ids) const;
    
    /** Returns all constant names as alphabetically sorted array. This is used by the autocomplete popup. */
	void getAllConstants(Array<Identifier> &ids) const;

	ReadWriteLock apiClassLock;

	int getNumChildElements() const override { return numConstants; }
	
	DebugInformationBase* getChildElement(int index) override
	{
		auto name = getConstantName(index);
		auto s = new SettableDebugInfo();
		s->codeToInsert << "%PARENT%." << name;
		s->value = getConstantValue(index);
		s->watchable = false;
		s->autocompleteable = false;
		return s;
	}


	bool isAutocompleteable() const override { return true; }

	void setFunctionIsInlineable(const Identifier& id)
	{
		inlineableFunctions.add(id);
	}

	bool isInlineableFunction(const Identifier& id) const
	{
		return inlineableFunctions.contains(id);
	}

	var getListOfOptimizableFunctions() const
	{
		Array<var> l;

		for (auto o : optimizableFunctions)
		{
			if (o != nullptr)
				l.add(var(dynamic_cast<ReferenceCountedObject*>(o.get())));
		}

		return var(l);
	}

	void addOptimizableFunction(const var& functionObject)
	{
		if (auto obj = dynamic_cast<DebugableObjectBase*>(functionObject.getObject()))
		{
			optimizableFunctions.addIfNotAlreadyThere(obj);
		}
	}

	DebugableObjectBase::Location getCurrentLocationInFunctionCall()
	{
		return currentLocation;
	}

	void setWantsCurrentLocation(bool shouldSaveCurrentLocation)
	{
		wantsLocation = shouldSaveCurrentLocation;
	}

	bool wantsCurrentLocation() const { return wantsLocation; }

	void setCurrentLocation(const String& file, int charNumber)
	{
		currentLocation.fileName = file;
		currentLocation.charNumber = charNumber;
	}

private:

	bool wantsLocation = false;
	DebugableObjectBase::Location currentLocation;

	Array<WeakReference<DebugableObjectBase>> optimizableFunctions;

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

#if ENABLE_SCRIPTING_SAFE_CHECKS
    struct ForcedParameterInfo
    {
        int numArgs;
        int index;
        Identifier id;
        VarTypeChecker::ParameterTypes types;
    };
    
    Array<ForcedParameterInfo> forcedParameterTypes;
#endif
    
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

	Array<Identifier> inlineableFunctions;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApiClass)

	// ================================================================================================================
};

} // namespace hise
#endif  // JAVASCRIPTAPICLASS_H_INCLUDED
