/*
  ==============================================================================

    hi_native_jit_public.h
    Created: 8 Mar 2017 12:26:32am
    Author:  Christoph

  ==============================================================================
*/

#ifndef HI_NATIVE_JIT_PUBLIC_H_INCLUDED
#define HI_NATIVE_JIT_PUBLIC_H_INCLUDED



/** The scope object that holds all global values and compiled functions. 
*
*	After compilation, this object will contain all functions and storage for the global variables of one instance.
*
*/
class NativeJITScope : public juce::DynamicObject
{
public:

	/** Creates a new scope. */
	NativeJITScope();

	~NativeJITScope();

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

	typedef juce::ReferenceCountedObjectPtr<NativeJITScope> Ptr;

	class Pimpl;

private:

	mutable juce::NamedValueSet cachedValues;

	friend class GlobalParser;

#if JUCE_64BIT
	juce::ScopedPointer<Pimpl> pimpl;
#endif

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeJITScope)
};


/** A JIT compiler for a C language subset based on NativeJIT.
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
*		NativeJITCompiler compiler(code); // code is the string containing the function above
*		ScopedPointer<NativeJITScope> scope1 = compileAndReturnScope();
*		ScopedPointer<NativeJITScope> scope2 = compileAndReturnScope();
*
*		int(*f1)(double, int) = scope1->getCompiledFunction(Identifier("calculateSomething"));
*		int(*f2)(double, int) = scope1->getCompiledFunction(Identifier("calculateSomething"));
*
*		jassert(f1 != f2); // the functions are not the same, they are individually compiled!
*
*		int i1 = f1(2.0, 1);
*		int i2 = f2(2.0, 0);
*/
class NativeJITCompiler : public juce::DynamicObject
{
public:

	/** Creates a compiler for the given code.
	*
	*	It just preprocesses the code on creation. In order to actually compile, call compileAndReturnScope()
	*/
	NativeJITCompiler(const juce::String& codeToCompile);

	/** Destroys the compiler. The lifetime of created scopes are independent. */
	~NativeJITCompiler();

	/** Compiles the code that was passed in during construction and returns a NativeJITScope object that contains the global variables and functions. */
	NativeJITScope* compileAndReturnScope() const;

	/** Checks if the compilation went smooth. */
	bool wasCompiledOK() const;

	/** Get the error message for compilation errors. */
	juce::String getErrorMessage() const;

	/** Returns the code the compiler will be using to create scopes. */
	juce::String getCode(bool getPreprocessedCode) const;

private:

	class Pimpl;

	juce::ScopedPointer<Pimpl> pimpl;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeJITCompiler)
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
class NativeJITDspModule : public juce::DynamicObject
{
public:

	/** Creates a new module. You don't supply the code here, instead you have to supply a pointer to a existing compiler holding the code.
	*	This allows to create multiple independent NativeJITDspModules with the same processing logic.
	*/
	NativeJITDspModule(const NativeJITCompiler* compiler);

	/** Calls the defined init() function if compiled correctly. */
	void init();

	/** Calls the defined prepareToPlay function if compiled correctly. */
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	/** Calls the defined process function and replaces the buffer contents with the processed data. */
	void processBlock(float* data, int numSamples);

	/** Returns the NativeJITScope of this module. You can use it to hook it up to another scripting language. */
	NativeJITScope* getScope();

	/** Returns the NativeJITScope of this module. You can use it to hook it up to another scripting language. */
	const NativeJITScope* getScope() const;

	void enableOverflowCheck(bool shouldCheckForOverflow);

private:

	typedef float(*processFunction)(float);
	typedef int(*initFunction)();
	typedef int(*prepareFunction)(double, int);

	bool allOK() const;

	NativeJITScope::Ptr scope;

	processFunction pf = nullptr;
	prepareFunction pp = nullptr;
	initFunction initf = nullptr;

	bool compiledOk = false;
	bool allFunctionsDefined;
	
	bool overFlowCheckEnabled;
	int overflowIndex = -1;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeJITDspModule)
};


#endif  // HI_NATIVE_JIT_PUBLIC_H_INCLUDED
