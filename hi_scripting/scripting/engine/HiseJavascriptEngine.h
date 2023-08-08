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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
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

namespace hise { using namespace juce;

class JavascriptProcessor;
class DialogWindowWithBackgroundThread;


class HiseJavascriptPreprocessor: public ReferenceCountedObject
{
public:
    
    using Ptr = ReferenceCountedObjectPtr<HiseJavascriptPreprocessor>;
    
    HiseJavascriptPreprocessor() {};
    
    Result process(String& code, const String& externalFile);
    
#if USE_BACKEND
    void setEnableGlobalPreprocessor(bool shouldBeEnabled);

    void reset();

    SparseSet<int> getDeactivatedLinesForFile(const String& fileId);

    DebugableObjectBase::Location getLocationForPreprocessor(const String& id) const;

    snex::jit::ExternalPreprocessorDefinition::List definitions;

    HashMap<String, SparseSet<int>> deactivatedLines;
    bool globalEnabled = false;
#endif
};

/** The HISE Javascript Engine.
 *
 *	This class is a modified version of the original Javascript engine found in JUCE.
 *	It adds some language features that were missing in the original code:
 *
 *	- `switch` statements
 *	- `const` (immutable) variables
 *	- `for ... in` iterator loop
 *
 *	As well as some HISE specific features, which are not standard Javascript, but a useful
 *	addition for a DSP scripting language. However, they don't break any Javascript features.
 *
 *	**Inline Functions**
 *  no overhead inlining of functions with parameters and return value).
 *
 *      inline function square(x)
 *      {
 *          return x*x;
 *      };
 *
 *	**C++ API Wrapper class**
 *
 *  A class interface for low overhead calling of C++ functions / methods. Instead of using the standard
 *  DynamicObject class, which involves O(n) lookup, allocations and creating a scope for each function call,
 *  calling a ApiClass method comes with almost no overhead (it directly accesses the C++ function via a func pointer).
 *  
 *  @see ApiClass
 *	
 *  **VariantBuffer**
 *
 *  A native object type to efficiently operate on an array of floats. There are also some overloaded operators
 *  for making things faster to type (and execute!):
 *
 *      var buffer = Buffer.create(256); // Creates a buffer with 256 samples
 *      0.5 >> buffer;                   // Fills the buffer with 0.5;
 *      buffer * 1.2;                    // Multiplies every sample with 1.2;
 *      for (s in buffer) s = s * 1.2;   // Does the same thing but for each sample (much slower).
 *      
 *  @see VariantBuffer.
 *
 *  **Register variables**
 *
 *  A special storage location with accelerated access / lookup times. There are 32 slots which can be directly
 *  from Javascript (without a lookup in the NamedValueSet of a scope) to improve the performance for temporary / working 
 *  variables.
 *
 *  @see VarRegister
 *
 *
 *
 */
class HiseJavascriptEngine: public ApiProviderBase
{
public:
	/** Creates an instance of the engine.
	This creates a root namespace and defines some basic Object, String, Array
	and Math library methods.
	*/
	HiseJavascriptEngine(JavascriptProcessor *p, MainController* mc);

	/** Destructor. */
	~HiseJavascriptEngine();

	struct TimeoutExtender
	{
		TimeoutExtender(HiseJavascriptEngine* e);;

		~TimeoutExtender();

		uint32 start;
		WeakReference<HiseJavascriptEngine> engine;
	};

	struct TokenProvider : public mcl::TokenCollection::Provider,
						   public hise::GlobalScriptCompileListener
	{
		struct DebugInformationToken;
		struct ObjectMethodToken;
		struct KeywordToken;
		struct ApiToken;

		TokenProvider(JavascriptProcessor* jp_);;

		~TokenProvider();

		void addTokens(mcl::TokenCollection::List& tokens) override;

        static void precompileCallback(TokenProvider& p, bool unused);

		void scriptWasCompiled(JavascriptProcessor *processor) override;

		WeakReference<JavascriptProcessor> jp;
        JUCE_DECLARE_WEAK_REFERENCEABLE(TokenProvider);
	};

	/** Attempts to parse and run a block of javascript code.
	If there's a parse or execution error, the error description is returned in
	the result.
	You can specify a maximum time for which the program is allowed to run, and
	it'll return with an error message if this time is exceeded.
	*/
	Result execute(const String& javascriptCode, bool allowConstDeclarations = true, const Identifier& callbackId = {});

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
                             Result* errorMessage = nullptr, bool allowMessageThread=false);

    
	var callExternalFunctionRaw(var function, const var::NativeFunctionArgs& args);

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

	void abortEverything();

	static RelativeTime getDefaultTimeOut()
	{
		return  RelativeTime(5.0);
	}

	void extendTimeout(int milliSeconds);

	/** Registers a callback to the engine.
	*
	*	If a function with this name is found at parsing, it will be stored as callback for faster execution:
	*	
	*	- no scope (only global variables)
	*	- no arguments
	*	- no overhead if the callback is not found
	*/
	int registerCallbackName(const Identifier &callbackName, int numArgs, double bufferTime);

	var executeInlineFunction(var inlineFunction, var* arguments, Result* result, int numArgs=-1);

	var getInlineFunction(const Identifier& id);

	StringArray getInlineFunctionNames(int numArgs = -1);

	var executeCallback(int callbackIndex, Result *result);

	void setCallbackParameter(int callbackIndex, int parameterIndex, const var& newValue);


	String getHoverString(const String& token);

	DebugInformationBase::Ptr getDebugInformation(int index);

	var getScriptVariableFromRootNamespace(const Identifier & id) const;

	void addShaderFile(const File& f);

	int getNumIncludedFiles() const;
	File getIncludedFile(int fileIndex) const;
	Result getIncludedFileResult(int fileIndex) const;

	int getNumDebugObjects() const override;

	DebugableObjectBase* getDebugObject(const String& token) override;
	

	void clearDebugInformation();
	
	void rebuildDebugInformation();

	/** This value indicates how long a call to one of the evaluate methods is permitted
	to run before timing-out and failing.
	The default value is a number of seconds, but you can change this to whatever value
	suits your application.
	*/
	RelativeTime maximumExecutionTime;

    HiseJavascriptPreprocessor::Ptr preprocessor;
    
	/** Provides access to the set of properties of the root namespace object. */
	const NamedValueSet& getRootObjectProperties() const noexcept;

	DynamicObject *getRootObject();;

	void setCallStackEnabled(bool shouldBeEnabled);

	void registerApiClass(ApiClass *apiClass);

    void setIsInitialising(bool shouldBeInitialising)
    {
        initialising = shouldBeInitialising;
    }
    
    bool isInitialising() const { return initialising; };
    
	static bool isJavascriptFunction(const var& v);
    
	static bool isInlineFunction(const var& v);

	struct ExternalFileData
	{
		enum class Type
		{
			RelativeFile,
			AbsoluteFile,
			EmbeddedScript,
			numTypes
		};

		ExternalFileData(Type t_, const File &f_, const String& name_);;

		ExternalFileData();

		void setErrorMessage(const String &m)
		{
			r = Result::fail(m);
		}

		File f;
		String scriptName;
		Result r;
		Type t;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExternalFileData)
	};

	struct Breakpoint;

	struct CyclicReferenceCheckBase
	{
        virtual ~CyclicReferenceCheckBase() {};
        
		struct ThreadData;

		struct Reference
		{
			typedef Array<Reference> List;

			Reference() {};
			Reference(const var& parent_, const var& child_, Identifier parentId_, Identifier childId_);
			Reference(const Reference& other);

			~Reference() {}

			bool equals (const Reference& other) const;
			String toString() const;;
			bool isEmpty() const;
			bool isCyclicReference() const;

			struct ListHelpers
			{
				static bool addAllReferencesWithTarget(const var& sourceVar, const Identifier& sourceId, const var& targetVar, const Identifier& targetId, ThreadData& references);
				static bool checkIfExist(const List& references, const Reference& referenceToCheck);
				static bool checkEqualitySafe(const var& a, const var& b);
				static bool isVarWithReferences(const var &v);
				static Identifier getIdWithParent(const Identifier &parentId, const String& name, bool isObject);
				static bool varHasReferences(const var& v);
			};

			const var parent;
			const var child;
			const Identifier parentId;
			const Identifier childId;

			String description;
		};

		struct ThreadData
		{
			double* progress = nullptr;
			DialogWindowWithBackgroundThread* thread = nullptr;
			Reference::List referenceList;
			String cyclicReferenceString;
			
			int numChecked = 0; 
			int overflowProtection = 0;
			int coallescateOverflowProtection = 0;
			bool overflowHit = false;
		};

		/** Overwrite this method and prepare the object for a cycle reference check. */
		virtual void prepareCycleReferenceCheck() = 0;

		/** Overwrite this method and add all references for child objects to the supplied Array. */
		virtual bool updateCyclicReferenceList(ThreadData& references, const Identifier &id) = 0;

	protected:

		/** This crawls through Javascript Objects and Arrays and adds a list of references. 
		*
		*	If parent and parentId are not defined, then it won't add the reference so call this from your data storages,
		*	and it will recursively scan all objects.
		*/
		static bool updateList(ThreadData& data, const var& varToCheck, const Identifier& parentId);
	};

	bool checkCyclicReferences(CyclicReferenceCheckBase::ThreadData& references, const Identifier &id);

	
	

	//==============================================================================
	struct RootObject : public DynamicObject,
						public CyclicReferenceCheckBase
	{
		

		RootObject();

		Time timeout;

		Array<Breakpoint> breakpoints;

		typedef const var::NativeFunctionArgs& Args;
		typedef const char* TokenType;

		// HISE special storage

		void execute(const String& code, bool allowConstDeclarations);
		var evaluate(const String& code);

		//==============================================================================
		static bool areTypeEqual(const var& a, const var& b);

		static String getTokenName(TokenType t);
		static bool isFunction(const var& v) noexcept;
		static bool isNumeric(const var& v) noexcept;
		static bool isNumericOrUndefined(const var& v) noexcept;
		static int64 getOctalValue(const String& s);
		static Identifier getPrototypeIdentifier();
		static var* getPropertyPointer(DynamicObject* o, const Identifier& i) noexcept;

		bool updateCyclicReferenceList(ThreadData& data, const Identifier &id) override;

		void prepareCycleReferenceCheck() override;

		struct ScopedLocalThisObject
		{
			ScopedLocalThisObject(RootObject& r_, const var& newObject);

			~ScopedLocalThisObject();

			RootObject& r;
			var prevObject;
		};

		var getLocalThisObject() const { return localThreadThisObject.get(); }

		//==============================================================================
		struct CodeLocation;
		struct CallStackEntry;
		struct Scope;

        HiseJavascriptPreprocessor::Ptr preprocessor;

        struct LocalScopeCreator
        {
            using Ptr = WeakReference<LocalScopeCreator>;
            
			struct ScopedSetter
			{
				ScopedSetter(ReferenceCountedObjectPtr<RootObject> r_, LocalScopeCreator::Ptr p);

				~ScopedSetter();;

				RootObject* r;
				LocalScopeCreator::Ptr prevValue;
				bool ok = false;
			};

            virtual ~LocalScopeCreator() {};
            
            virtual DynamicObject::Ptr createScope(RootObject* r) = 0;
            
            JUCE_DECLARE_WEAK_REFERENCEABLE(LocalScopeCreator);
        };
        
		ThreadLocalValue<LocalScopeCreator::Ptr> currentLocalScopeCreator;
        
		struct Statement;
		struct Expression;
		
		struct OptimizationPass
		{
			struct OptimizationResult
			{
				operator bool() const { return passName.isNotEmpty() && numOptimizedStatements > 0; }

				String passName;
				int numOptimizedStatements = 0;
			};

			virtual ~OptimizationPass() {};

			virtual String getPassName() const = 0;

			/** Override this method and return a new optimized statement if this pass can be applied to the statementToOptimize. 
				
				You can use the parentStatement to figure out whether this optimisation pass should apply.
			*/
			virtual Statement* getOptimizedStatement(Statement* parentStatement, Statement* statementToOptimize) = 0;

			static bool callForEach(Statement* root, const std::function<bool(Statement* child)>& f);

			OptimizationResult executePass(Statement* rootStatementToOptimize);
		};

		struct ScriptAudioThreadGuard;

		struct Error
		{
			static Error fromBreakpoint(const Breakpoint &bp);

			static Error fromPreprocessorResult(const Result& r, const String& externalFile);
            
			static Error fromLocation(const CodeLocation& location, const String& errorMessage);

			String getLocationString() const;

			String getEncodedLocation(Processor* p) const;

			String toString(Processor* p) const;

			int charIndex = -1;
			int lineNumber = -1;
			int columnNumber = -1;

			String errorMessage;
			mutable String externalLocation;
		};

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
		struct SelfAssignment;			struct PostAssignment;		struct AnonymousFunctionWithCapture;

		// Function / Objects

		struct FunctionCall;			struct NewOperator;			struct DotOperator;
		struct ObjectDeclaration;		struct ArrayDeclaration;	struct FunctionObject;

		// HISE special

		struct RegisterVarStatement;	struct RegisterName;		struct RegisterAssignment;
		struct ApiConstant;				struct ApiCall;				struct InlineFunction;
		struct ConstVarStatement;		struct ConstReference;		struct ConstObjectApiCall;
		struct GlobalVarStatement;		struct GlobalReference;		struct LocalVarStatement;
		struct LocalReference;			struct LockStatement;	    struct CallbackParameterReference;
		struct CallbackLocalStatement;  struct CallbackLocalReference;  struct ExternalCFunction;
		struct NativeJIT;				struct IsDefinedTest;		

		// Snex stuff

		struct SnexDefinition;			struct SnexConstructor;		struct SnexBinding;
		struct SnexConfiguration;

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
		static var trace(Args a)      { return JSON::toString(get(a, 0)); }
		static var charToInt(Args a)  { return (int)(getString(a, 0)[0]); }
		static var typeof_internal(Args a);
		static var exec(Args a);
		static var eval(Args a);
                            
		void addToCallStack(const Identifier& id, const CodeLocation* location);
		void removeFromCallStack(const Identifier& id);
		String dumpCallStack(const Error& lastError, const Identifier& rootFunctionName);
		void setCallStackEnabled(bool shouldeBeEnabled) { enableCallstack = shouldeBeEnabled; }

		class Callback:  public DynamicObject,
					     public DebugableObject,
                         public LocalScopeCreator
		{
		public:

			Callback(const Identifier &id, int numArgs, double bufferTime_);

			var perform(RootObject *root);

			void setStatements(BlockStatement *s) noexcept;

			bool isDefined() const noexcept;

			Identifier getObjectName() const override;

			const Identifier &getName() const;

			int getNumArgs() const;;

			String getDebugDataType() const;

			String getDebugName() const override;

			DynamicObject::Ptr createScope(RootObject* r) override;

			int getNumChildElements() const override;

			DebugInformation* getChildElement(int index) override;

			String::CharPointerType getProgramPtr() const;

			void setParameterValue(int parameterIndex, const var& newValue);

			var* getVarPointer(const Identifier &id);

			String getDebugValue() const override;

			var createDynamicObjectForBreakpoint();

			void doubleClickCallback(const MouseEvent &/*e*/, Component* /*componentToNotify*/) override;

			void cleanLocalProperties();

			Identifier parameters[4];
			var parameterValues[4];

			NamedValueSet localProperties;

		

			ScopedPointer<BlockStatement> statements;

			private:

			double lastExecutionTime;
			const Identifier callbackName;
			int numArgs;

			const double bufferTime;

			bool isCallbackDefined = false;

			JUCE_DECLARE_WEAK_REFERENCEABLE(Callback);
		};

		struct JavascriptNamespace: public ReferenceCountedObject,
									public DebugableObject,
									public CyclicReferenceCheckBase
		{
			enum class StorageType
			{
				Register=0,
				ConstVariable,
				InlineFunction,
				numStorageTypes
			};

			JavascriptNamespace(const Identifier &id_):
				id(id_)
			{}

			Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Namespace"); }

			Identifier getInstanceName() const override { return id; }

			String getDebugDataType() const override { return "Namespace"; };
			String getDebugName() const override { return id.toString(); };
			String getDebugValue() const override { return id.toString(); };

			void doubleClickCallback(const MouseEvent &, Component* e)
			{
				DebugableObject::Helpers::gotoLocation(e, nullptr, namespaceLocation);
			}

			int getNumChildElements() const override
			{
				return varRegister.getNumUsedRegisters() +
					inlineFunctions.size() +
					constObjects.size();
			}

			DebugInformationBase* getChildElement(int index) override
			{
				return createDebugInformation(index);
			}
			

			int getNumDebugObjects() const
			{
				return 0;
				
			}

			bool updateCyclicReferenceList(ThreadData& data, const Identifier& id) override;

			void prepareCycleReferenceCheck() override;

			virtual OptimizationPass::OptimizationResult runOptimisation(OptimizationPass* p);

			bool optimiseFunction(OptimizationPass::OptimizationResult& r, var function, OptimizationPass* p);

			DebugInformation* createDebugInformation(int index);

			const Identifier id;
			ReferenceCountedArray<DynamicObject> inlineFunctions;
			ReferenceCountedArray<DynamicObject> inlineFunctionSnexBindings;
			NamedValueSet constObjects;
			VarRegister	varRegister;

			NamedValueSet comments;

			Array<DebugableObject::Location> registerLocations;
			Array<DebugableObject::Location> constLocations;

			DebugableObject::Location namespaceLocation;

			JUCE_DECLARE_WEAK_REFERENCEABLE(JavascriptNamespace);
		};

		struct HiseSpecialData: public JavascriptNamespace
		{
			enum class VariableStorageType
			{
				Undeclared,
				LocalScope,
				RootScope,
				Register,
				ConstVariables,
				Globals
			};

			HiseSpecialData(RootObject* root);

			~HiseSpecialData();

			Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("RootNamespace"); }

            void clear();
            
            Callback *getCallback(const Identifier &id);
            
			JavascriptNamespace* getNamespace(const Identifier &id);

			const JavascriptNamespace* getNamespace(const Identifier &id) const;

			DynamicObject* getInlineFunction(const Identifier &id);

			OptimizationPass::OptimizationResult runOptimisation(OptimizationPass* p) override;

			bool updateCyclicReferenceList(CyclicReferenceCheckBase::ThreadData& data, const Identifier& id) override;

			void prepareCycleReferenceCheck() override;

			void setProcessor(JavascriptProcessor *p) noexcept 
			{ 
				processor = p; 
				registerOptimisationPasses();
			}

			void registerOptimisationPasses();

			static bool initHiddenProperties;

			

			ReferenceCountedArray<ApiClass> apiClasses;
			Array<Identifier> apiIds;
			
			ReferenceCountedArray<JavascriptNamespace> namespaces;

			RootObject* root;

			Array<Identifier> callbackIds;
			OwnedArray<RootObject::BlockStatement> callbacks;
			JavascriptProcessor* processor;

			OwnedArray<OptimizationPass> optimizations;

			DynamicObject::Ptr globals;

			DynamicObject::Ptr preparsedconstVariableNames;

			ReferenceCountedArray<Callback> callbackNEW;

			double callbackTimes[32];

			static Array<Identifier> hiddenProperties;

			OwnedArray<ExternalFileData> includedFiles;

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

			DebugInformationBase *getDebugInformation(int debugIndex)
			{
				if (debugIndex < debugInformation.size())
				{
					return debugInformation[debugIndex].get();
				}

				return nullptr;
			}

			void checkIfExistsInOtherStorage(VariableStorageType thisType, const Identifier &name, CodeLocation& l);;

			static DebugInformation *get(ReferenceCountedObject* o) { return dynamic_cast<DebugInformation*>(o); }

		private:

			VariableStorageType getExistingVariableStorage(const Identifier &name);
			void throwExistingDefinition(const Identifier &name, VariableStorageType type, CodeLocation &l);

			CriticalSection debugLock;

			ReferenceCountedArray<DebugInformationBase> debugInformation;

			JUCE_DECLARE_WEAK_REFERENCEABLE(HiseSpecialData);
		};

		void setUseCycleReferenceCheckForNextCompilation()
		{
			shouldUseCycleCheck = true;
		}

		HiseSpecialData hiseSpecialData;

#if HISE_INCLUDE_SNEX
		snex::jit::GlobalScope snexGlobalScope;
#endif

		private:

		Array<CallStackEntry, SpinLock, 1024> callStack;

		bool enableCallstack = false;

		bool shouldUseCycleCheck = false;

		ThreadLocalValue<var> localThreadThisObject;
	};

	

	struct Breakpoint
	{
	public:

		struct Reference
		{
			Identifier localScopeId;
			int index = -1;
		};

		class Listener
		{
		public:
			virtual void breakpointWasHit(int breakpointIndex) = 0;

			virtual ~Listener();

		private:

			friend class WeakReference<Listener>;

			WeakReference<Listener>::Master masterReference;
		};

		Breakpoint();

		Breakpoint(const Identifier& snippetId_, const String& externalLocation_, int lineNumber_, int charNumber_, int charIndex_, int index_);
		;

		~Breakpoint();

		bool operator ==(const Breakpoint& other) const;

		void copyLocalScopeToRoot(RootObject& r);

		const Identifier snippetId;
		const int lineNumber;
		const int colNumber;
		const int charIndex;
		
		const int index;
		const String externalLocation;

		bool found = false;
		bool hit = false;

		DynamicObject::Ptr localScope;
		
	};

	void getColourAndLetterForType(int type, Colour& colour, char& letter) override
	{
		return ValueTreeApiHelpers::getColourAndCharForType(type, letter, colour);
	}

	void setBreakpoints(Array<Breakpoint> &breakpoints);

	void addBreakpointListener(Breakpoint::Listener* listener)
	{
		breakpointListeners.add(listener);
	}

	void removeBreakpointListener(Breakpoint::Listener* listener)
	{
		breakpointListeners.removeAllInstancesOf(listener);
	}

	void sendBreakpointMessage(int breakpointIndex);

	void setUseCycleReferenceCheckForNextCompilation()
	{
		root->setUseCycleReferenceCheckForNextCompilation();
	}

	static void checkValidParameter(int index, const var& valueToTest, const RootObject::CodeLocation& location);

    LambdaBroadcaster<bool> preCompileListeners;
    
private:

	
    
    bool initialising = false;
	bool externalFunctionPending = false;

	ReferenceCountedObjectPtr<RootObject> root;
	void prepareTimeout() const noexcept;
	
	Array<WeakReference<Breakpoint::Listener>> breakpointListeners;

	

	DynamicObject::Ptr unneededScope;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseJavascriptEngine);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseJavascriptEngine)
};

using ScriptGuard = HiseJavascriptEngine::RootObject::ScriptAudioThreadGuard;

} // namespace hise
#endif  // HISEJAVASCRIPTENGINE_H_INCLUDED
