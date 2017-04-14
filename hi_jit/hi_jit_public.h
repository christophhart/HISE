/*
  ==============================================================================

    hi_native_jit_public.h
    Created: 8 Mar 2017 12:26:32am
    Author:  Christoph

  ==============================================================================
*/

#ifndef HI_NATIVE_JIT_PUBLIC_H_INCLUDED
#define HI_NATIVE_JIT_PUBLIC_H_INCLUDED


#include <typeindex>

typedef std::type_index TypeInfo;
typedef uint8 BooleanType;
typedef uint64_t PointerType;


#if JUCE_64BIT
typedef uint64_t AddressType;
#else
typedef uint32_t AddressType;
#endif



/** The scope object that holds all global values and compiled functions. 
*
*	After compilation, this object will contain all functions and storage for the global variables of one instance.
*
*/
class HiseJITScope : public juce::DynamicObject
{
public:

	/** Creates a new scope. */
	HiseJITScope();

	~HiseJITScope();

	void setName(const juce::Identifier& name_)
	{
		name = name_;
	}

	juce::Identifier getName() { return name; };

	bool isFunction(const juce::Identifier& id) const;

	int getNumArgsForFunction(const juce::Identifier& id) const;

	bool isGlobal(const juce::Identifier& id) const;

	int getIndexForGlobal(const juce::Identifier& id) const;

	bool hasProperty(const juce::Identifier& propertyName) const override;


	const juce::var& getProperty(const juce::Identifier& propertyName) const override;

	void setProperty(const juce::Identifier& propertyName, const juce::var& newValue) override;

	void removeProperty(const juce::Identifier& propertyName) override;


	bool hasMethod(const juce::Identifier& methodName) const override;

	juce::var invokeMethod(juce::Identifier methodName, const juce::var::NativeFunctionArgs& args) override;

	/** Get the number of global variables. */
	int getNumGlobalVariables() const;

	/** Returns the value of the global variable as var. */
	juce::var getGlobalVariableValue(int globalIndex) const;

	/** Returns the type of the global variable. */
	TypeInfo getGlobalVariableType(int globalIndex) const;

	/** Get the name of the variable at the given index. */
	juce::Identifier getGlobalVariableName(int globalIndex) const;


	int isBufferOverflow(int globalIndex) const;


	/** Use this to set a global variable from the outside world. 
	*
	*	It accepts a var, and numbers / Buffers will be automatically converted to the given type.
	*	If you try to pass it a String, a non-Buffer Object or an Array, it will throw an error message.
	*/
	void setGlobalVariable(const juce::Identifier& id, const juce::var& value);

	void setGlobalVariable(int globalIndex, const juce::var& newValue);


	/** Returns a typed pointer to a compiled function with the given name.
	*
	*	The template arguments are the return type and the parameters (supports up to two parameters)
	*
	*	Example usage:
	*	
	*		typedef int(*func)(double, float);
	*		func f = getCompiledFunction<int, double, float>(Identifier("testFunction");
	*
	*/
	template <typename ReturnType, typename... ParameterTypes> ReturnType(*getCompiledFunction(const juce::Identifier& id))(ParameterTypes...);

	typedef juce::ReferenceCountedObjectPtr<HiseJITScope> Ptr;

	class Pimpl;

private:

	juce::Identifier name;

	mutable juce::NamedValueSet cachedValues;

	juce::var dummyVar;

	friend class GlobalParser;


	juce::ScopedPointer<Pimpl> pimpl;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseJITScope)
};


/** A JIT compiler for a C language subset based on HiseJIT.
*
*	It is supposed to be used as "scripting" language for the inner loop of a DSP routine. It offers about 70% - 80% performance of
*	native C++ performance (clang -O3)!
*
*	Features:
*
*	- supported types: float, double, int, Buffer (a native float array with safe checks)
*	- strictly typed, parser throws an error for type mismatches (enforces casts)
*	- define functions which can be called from C++ or from other JITted functions
*	- use common inbuilt functions like powf, sin, cos, etc.
*	- use local variables without runtime penalties
*	- call functions with max two parameters
*	- simple branching using the a ? b : c logic
*
*	You can give it a C code snippet as string and it will compile and return instances of scopes that can be used independently.
*
*	Example:
*
*	C code:
*
*		float x = 1.2f;
*		
*		int calculateSomething(double input, int shouldDoubleOutput)
*		{
*			const float x1 = (float)input * x;
*			const float x2 = shouldDoubleOutput > 1 ? x1 * 2.0f : x1;
*			return (int)x2;
*		};
*
*	C++ side:
*
*		HiseJITCompiler compiler(code); // code is the string containing the function above
*		ScopedPointer<HiseJITScope> scope1 = compileAndReturnScope();
*		ScopedPointer<HiseJITScope> scope2 = compileAndReturnScope();
*
*		int(*f1)(double, int) = scope1->getCompiledFunction(Identifier("calculateSomething"));
*		int(*f2)(double, int) = scope1->getCompiledFunction(Identifier("calculateSomething"));
*
*		jassert(f1 != f2); // the functions are not the same, they are individually compiled!
*
*		int i1 = f1(2.0, 1);
*		int i2 = f2(2.0, 0);
*/
class HiseJITCompiler : public juce::DynamicObject
{
public:

	juce::Identifier getModuleName() { return juce::Identifier("Example"); };

	/** Creates a compiler for the given code.
	*
	*	It just preprocesses the code on creation. In order to actually compile, call compileAndReturnScope()
	*/
	HiseJITCompiler(const juce::String& codeToCompile, bool useCppMode=true);

	/** Destroys the compiler. The lifetime of created scopes are independent. */
	~HiseJITCompiler();

	/** Compiles the code that was passed in during construction and returns a HiseJITScope object that contains the global variables and functions. */
	HiseJITScope* compileAndReturnScope() const;

	/** Checks if the compilation went smooth. */
	bool wasCompiledOK() const;

	/** Get the error message for compilation errors. */
	juce::String getErrorMessage() const;

	/** Returns the code the compiler will be using to create scopes. */
	juce::String getCode(bool getPreprocessedCode) const;

private:

	class Pimpl;

	juce::ScopedPointer<Pimpl> pimpl;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseJITCompiler)
};



/** This class wraps a JIT compiler to process a buffer of float arrays. 
*
*	In order to use it, define these functions:
*
*		void init(); // setup your global variables
*		void prepareToPlay(double sampleRate, int blockSize); // initialise the processing
*		float process(float input); // process a sample
*
*	From C++, you can then call processBlock and it will iterate over the float array and call the processing function for each sample.
*/
class HiseJITDspModule : public juce::DynamicObject
{
public:

	/** Creates a new module. You don't supply the code here, instead you have to supply a pointer to a existing compiler holding the code.
	*	This allows to create multiple independent HiseJITDspModules with the same processing logic.
	*/
	HiseJITDspModule(const HiseJITCompiler* compiler);

	/** Calls the defined init() function if compiled correctly. */
	void init();

	/** Calls the defined prepareToPlay function if compiled correctly. */
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	/** Calls the defined process function and replaces the buffer contents with the processed data. */
	void processBlock(float* data, int numSamples);

	/** Returns the HiseJITScope of this module. You can use it to hook it up to another scripting language. */
	HiseJITScope* getScope();

	/** Returns the HiseJITScope of this module. You can use it to hook it up to another scripting language. */
	const HiseJITScope* getScope() const;

	void enableOverflowCheck(bool shouldCheckForOverflow);

	bool allOK() const;

private:

	typedef float(*processFunction)(float);
	typedef void(*initFunction)();
	typedef void(*prepareFunction)(double, int);

	HiseJITScope::Ptr scope;

	processFunction pf = nullptr;
	prepareFunction pp = nullptr;
	initFunction initf = nullptr;

	bool compiledOk = false;
	bool allFunctionsDefined;
	
	bool overFlowCheckEnabled;
	int overflowIndex = -1;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseJITDspModule)
};


struct JITTest
{
	static float test();
};

#endif  // HI_NATIVE_JIT_PUBLIC_H_INCLUDED
