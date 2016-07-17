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


class JavascriptProcessor;

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
	HiseJavascriptEngine(JavascriptProcessor *p);

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
    
    var callExternalFunction(var function,
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

	void registerGlobalStorge(DynamicObject *globaObject);

	/** Registers a callback to the engine.
	*
	*	If a function with this name is found at parsing, it will be stored as callback for faster execution:
	*	
	*	- no scope (only global variables)
	*	- no arguments
	*	- no overhead if the callback is not found
	*/
	int registerCallbackName(const Identifier &callbackName, int numArgs, double bufferTime);

	void executeCallback(int callbackIndex, Result *result);

	inline void setCallbackParameter(int callbackIndex, int parameterIndex, var newValue);

	DebugInformation*getDebugInformation(int index);

	const DynamicObject *getScriptObject(const Identifier &id) const;

	const Array<File> &getIncludedFiles() const;

	int getNumDebugObjects() const;

	void clearDebugInformation();
	
	void rebuildDebugInformation();

	/** This value indicates how long a call to one of the evaluate methods is permitted
	to run before timing-out and failing.
	The default value is a number of seconds, but you can change this to whatever value
	suits your application.
	*/
	RelativeTime maximumExecutionTime;

	/** Provides access to the set of properties of the root namespace object. */
	const NamedValueSet& getRootObjectProperties() const noexcept;

	DynamicObject *getRootObject();;

	

	void registerApiClass(ApiClass *apiClass);

	//==============================================================================
	struct RootObject : public DynamicObject
	{
		RootObject();

		Time timeout;

		typedef const var::NativeFunctionArgs& Args;
		typedef const char* TokenType;

		// HISE special storage

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
		struct ConstVarStatement;		struct ConstReference;		
		struct GlobalVarStatement;		struct GlobalReference;		struct CallbackParameterReference;

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

		class Callback:  public DynamicObject,
					     public DebugableObject
		{
		public:

			Callback(const Identifier &id, int numArgs, double bufferTime_);

			void perform(RootObject *root);

			void setStatements(BlockStatement *s) noexcept;

			bool isDefined() const noexcept{ return isCallbackDefined; }

			const Identifier &getName() const { return callbackName; }

			int getNumArgs() const { return numArgs; };

			String getDebugDataType() const { return "Callback"; }

			String getDebugName() const override { return callbackName.toString() + "()"; }

			void setParameterValue(int parameterIndex, var newValue)
			{
				parameterValues[parameterIndex] = newValue;
			}

			var* getVarPointer(const Identifier &id)
			{
				for (int i = 0; i < 4; i++)
				{
					if (id == parameters[i]) return &parameterValues[i];
				}

				return nullptr;
			}

			String getDebugValue() const override 
			{
				const double percentage = lastExecutionTime / bufferTime * 100.0;
				return String(percentage, 2) + "%";
			}

			void doubleClickCallback(Component* /*componentToNotify*/) override
			{
				DBG("JUMP");
			}

			Identifier parameters[4];
			var parameterValues[4];

		private:

			ScopedPointer<BlockStatement> statements;
			double lastExecutionTime;
			const Identifier callbackName;
			int numArgs;

			const double bufferTime;

			bool isCallbackDefined = false;
		};

		struct HiseSpecialData
		{
			HiseSpecialData();

			~HiseSpecialData();

            void clear();
            
            Callback *getCallback(const Identifier &id);
            
			void setProcessor(JavascriptProcessor *p) noexcept { processor = p; }

			static bool initHiddenProperties;

			VarRegister varRegister;
			ReferenceCountedArray<ApiClass> apiClasses;
			Array<Identifier> apiIds;
			ReferenceCountedArray<DynamicObject> inlineFunctions;
			NamedValueSet constObjects;

			Array<Identifier> callbackIds;
			OwnedArray<RootObject::BlockStatement> callbacks;
			JavascriptProcessor* processor;

			DynamicObject::Ptr globals;

			ReferenceCountedArray<Callback> callbackNEW;

			double callbackTimes[32];

			static Array<Identifier> hiddenProperties;

			Array<File> includedFiles;

			/** Call this after compiling and a dictionary of all values will be created. */
			void createDebugInformation(DynamicObject *root);

			/** Call this before compiling to clear the debug information. */
			void clearDebugInformation()
			{
				ScopedLock sl(debugLock);

				for (int i = 0; i < debugInformation.size(); i++)
				{
					// There is something clinging to this debug information;
					//jassert(debugInformation[i]->getReferenceCount() == 2);
				}

				debugInformation.clear();
			}

			int getNumDebugObjects() { return debugInformation.size(); }

			DebugInformation *getDebugInformation(int debugIndex)
			{
				if (debugIndex < debugInformation.size())
				{
					return debugInformation[debugIndex];
				}

				return nullptr;
			}

			static DebugInformation *get(ReferenceCountedObject* o) { return dynamic_cast<DebugInformation*>(o); }

			/** Lock this before you obtain debug objects to make sure it will not be rebuilt during access. */
			CriticalSection &getDebugLock()
			{
				return debugLock;
			}

		private:

			CriticalSection debugLock;

			OwnedArray<DebugInformation> debugInformation;
		};

		HiseSpecialData hiseSpecialData;
	};


private:

	ReferenceCountedObjectPtr<RootObject> root;
	void prepareTimeout() const noexcept;

	DynamicObject::Ptr unneededScope;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseJavascriptEngine)
};




#endif  // HISEJAVASCRIPTENGINE_H_INCLUDED
