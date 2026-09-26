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

#ifndef SCRIPTINGBASEOBJECTS_H_INCLUDED
#define SCRIPTINGBASEOBJECTS_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSynthGroup;
class ProcessorWithScriptingContent;
class JavascriptMidiProcessor;

class ScriptParameterHandler
{
public:

	virtual ~ScriptParameterHandler() {}

	virtual Identifier getParameterId(int index) const = 0;
	virtual int getParameterIndexForIdentifier(const Identifier& id) const = 0;
	virtual int getNumParameters() const = 0;
	virtual void setParameter(int index, float newValue) = 0;
	virtual float getParameter(int index) const = 0;
};

#if USE_BACKEND
struct CommandLineException: public std::exception
{
	CommandLineException(const String& r) :
		r(Result::fail(r))
	{};

	Result r;
};
#endif

struct LightweightDiagnostics: public DiagnosticBase
{
    LightweightDiagnostics(ProcessorWithScriptingContent* p_):
      p(p_)
    {}
    
    struct Factory
    {
        Factory();
        
        using CreatorFunction = std::function<DiagnosticBase*(ProcessorWithScriptingContent*)>;
        
        template <typename T> void registerType()
        {
            items[T::getClassName()] = [](ProcessorWithScriptingContent* p)
            {
                return new typename T::Diagnostics(p);
            };
        }
        
        DiagnosticBase* createLightweightPrototype(ProcessorWithScriptingContent* p, const Identifier& id) const
        {
            for(const auto& i: items)
            {
                if(i.first == id)
                    return i.second(p);
            }
            
            return nullptr;
        }
        
        private:
        
        std::map<Identifier, CreatorFunction> items;
    };
    
    struct FunctionInfo
    {
        Identifier methodName;
        int numArgs;
        // TODO: add argument type check data
    };
    
    void addFunction(const Identifier &id, call0 unused) override { functions.add({id, 0}); }
    void addFunction1(const Identifier &id, call1 unused) override { functions.add({id, 1}); }
    void addFunction2(const Identifier &id, call2 unused) override { functions.add({id, 2}); }
    void addFunction3(const Identifier &id, call3 unused) override { functions.add({id, 3}); }
    void addFunction4(const Identifier &id, call4 unused) override { functions.add({id, 4}); }
    void addFunction5(const Identifier &id, call5 unused) override { functions.add({id, 5}); }
    
    bool getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const override
    {
        int idx = 0;
        
        for(const auto& f: functions)
        {
            if(f.methodName == id)
            {
                numArgs = f.numArgs;
                index = idx;
                return true;
            }
            
            idx++;
        }
        
        numArgs = -1;
        index = -1;
        return false;
    }
    
    Array<FunctionInfo> functions;
    ProcessorWithScriptingContent* p;
};

/** The base class for all scripting API classes.
*	@ingroup scripting
*
*	It contains some basic methods for error handling.
*/
class ScriptingObject
{
public:

	virtual ~ScriptingObject() {};

	/** \internal sanity check if the function call contains the correct amount of arguments. */
	bool checkArguments(const String &callName, int numArguments, int expectedArgumentAmount);
	
	/** \internal sanity check if all arguments are valid. 
	*
	*	It returns -1 if everything is ok or the index of the faulty argument. 
	*/
	int checkValidArguments(const var::NativeFunctionArgs &args);

	/** \internal Checks if the callback is made on the audio thread (check this with every API call that changes the MidiMessage. */
	bool checkIfSynchronous(const Identifier &callbackName) const;


	ProcessorWithScriptingContent *getScriptProcessor();
	const ProcessorWithScriptingContent *getScriptProcessor() const;

	Processor* getProcessor() { return thisAsProcessor; };
	const Processor* getProcessor() const { return thisAsProcessor; }

protected:

	ScriptingObject(ProcessorWithScriptingContent *p);


	/** \internal Prints a error in the script to the console. */
	void reportScriptError(const String &errorMessage) const;

	/** \internal Prints a specially formatted message if the function is not allowed to be called in this callback. */
	void reportIllegalCall(const String &callName, const String &allowedCallback) const;
	
    void logErrorAndContinue(const String& errorMessage) const;
    
private:

	WeakReference<ProcessorWithScriptingContent> processor;	
	Processor* thisAsProcessor;
};

class EffectProcessor;


class ConstScriptingObject : public ScriptingObject,
							 public ApiClass
{
public:

	ConstScriptingObject(ProcessorWithScriptingContent* p, int numConstants);

	virtual Identifier getObjectName() const = 0;
	Identifier getInstanceName() const override;

	/** \internal Overwrite this method and check if the object got deleted. Best thing is to use a WeakReference and check if it's nullptr. */
	virtual bool objectDeleted() const;

	/** \internal Overwrite this method and check if the object exists. Best thing is to initialize the pointer to nullptr and check that. */
	virtual bool objectExists() const;


	/** Return all attached functions and callbacks of this object so that the compiler can run its optimisations. */
	//virtual var getOptimizableFunctions() const { return {}; }

	/** The parser will call this function with every function call and its location so you can use it to track down calls. 
		Note: this will only work with objects that are defined as const variables. */
	virtual bool addLocationForFunctionCall(const Identifier& id, const DebugableObjectBase::Location& location);;

	/** \internal This method combines the calls to objectDeleted() and objectExists() and creates a nice error message. */
	bool checkValidObject() const;

	void setName(const Identifier &name_) noexcept;;

	void gotoLocationWithDatabaseLookup();

	

private:

	Identifier name;

	using CreateFunction = std::function<DiagnosticBase*(ProcessorWithScriptingContent*)>;
	
	static std::map<Identifier, CreateFunction>& getDiagnosticPrototypeFactory()
	{
		static std::map<Identifier, CreateFunction> map;
		return map;
	}
	
public:

	static ReferenceCountedObjectPtr<DiagnosticBase> createDiagnosticPrototype(const Identifier& className,
		ProcessorWithScriptingContent* pwsc)
	{
		auto& map = getDiagnosticPrototypeFactory();
		auto it = map.find(className);

		if (it != map.end())
			return it->second(pwsc);

		return nullptr;
	}

protected:

	template <typename T, typename...As> void registerDiagnosticPrototype(T& obj, As... arguments)
	{
		auto id = obj.getObjectName();

		getDiagnosticPrototypeFactory()[id] = [arguments...](ProcessorWithScriptingContent* pwsc)
		{
			return new T(pwsc, arguments...);
		};
	}

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(ConstScriptingObject);
};

/** The base class for all objects that can be created by a script. 
*	@ingroup scripting
*
*	This class should be used whenever a complex data object should be created and used from within the script.
*	You need to overwrite the objectDeleted() and objectExists() methods.
*	From then on, you can use the checkValidObject() function within methods for sanity checking.
*/
class DynamicScriptingObject: public ScriptingObject,
							 public DynamicObject
{
public:

	DynamicScriptingObject(ProcessorWithScriptingContent *p);;

	virtual ~DynamicScriptingObject();;

	/** \internal Overwrite this method and return the class name of the object which will be used in the script context. */
	virtual Identifier getObjectName() const = 0;

	String getInstanceName() const;

protected:

	/** \internal Overwrite this method and check if the object got deleted. Best thing is to use a WeakReference and check if it's nullptr. */
	virtual bool objectDeleted() const = 0;

	/** \internal Overwrite this method and check if the object exists. Best thing is to initialize the pointer to nullptr and check that. */
	virtual bool objectExists() const = 0;

	/** \internal This method combines the calls to objectDeleted() and objectExists() and creates a nice error message. */
	bool checkValidObject() const;

	void setName(const String &name_) noexcept;;

private:

	String name;

	struct Wrappers
	{
		static var checkExists(const var::NativeFunctionArgs& args);
	};

};

class HiseJavascriptEngine;


/** This object can hold a function that can be called asynchronously on the scripting thread. 

	If you pass in a callback to an object / function that should be executed on certain C++ events,
	use this wrapper around the function and it will:
	
	- automatically call it on the scripting thread
	- make sure that the function will not be executed when the script was recompiled
	- avoid cyclic references that might lead to memory leaks

	You can delete the object right after calling `call`...
	*/
struct WeakCallbackHolder : private ScriptingObject
{
	struct CallableObject
	{
		CallableObject() :
			lastResult(Result::ok())
		{};

		virtual ~CallableObject() {};
		virtual Result call(HiseJavascriptEngine* engine, const var::NativeFunctionArgs& args, var* returnValue);

        virtual bool isRealtimeSafe() const = 0;


		enum class CallScope : uint8
		{
			Unknown = 0,   // no enrichment data
			Init,          // onInit only
			Unsafe,        // runtime OK, not audio thread
			Warning,       // audio thread OK with caveats
			Safe           // anywhere, anytime
		};


		enum class StrictnessLevel { Unset, Unsafe, Warn, Strict };

#if USE_BACKEND
		/** Returned by getRealtimeSafetyReport(). The call site (which has access
		 *  to the global StrictnessLevel setting) uses worstScope to decide whether
		 *  to block registration or just log, and message for console output.
		 */
		struct SafetyReport
		{
			CallScope worstScope = CallScope::Safe;
			String message;
		};

		/** Returns a safety report for this callable at the given strictness.
		 *  Unset/Unsafe: always returns empty report.
		 *  Warn/Strict: returns worstScope + formatted message if issues were found.
		 *  Default: returns empty (no analysis data). Override in InlineFunction::Object (Layer 3).
		 */
		virtual SafetyReport getRealtimeSafetyReport(StrictnessLevel) const
		{
			return {};
		}
#endif
        
		virtual bool allowRefCount() const { return true; }

		/** Override this and either clone or swap the captured values. */
		virtual void storeCapturedLocals(NamedValueSet& setFromHolder, bool swap) {};

		virtual void addAsSource(DebugableObjectBase* b, const Identifier& callbackId) { ignoreUnused(b, callbackId); };

		virtual String getComment() const { return {}; }

		virtual Identifier getCallId() const = 0;

		virtual int getNumArguments() const = 0;

	protected:

		Result lastResult;
		ReferenceCountedObject* thisAsRef = nullptr;

		JUCE_DECLARE_WEAK_REFERENCEABLE(CallableObject);
	};

#if USE_BACKEND
	struct RealtimeSafetyInfo
	{
		using CallScope = CallableObject::CallScope;
		using StrictnessLevel = CallableObject::StrictnessLevel;
		using SafetyReport = CallableObject::SafetyReport;

		struct ItemBase: public ReferenceCountedObject
		{
			using Ptr = ReferenceCountedObjectPtr<ItemBase>;
			using List = ReferenceCountedArray<ItemBase>;

			virtual ~ItemBase() {};

			String apiCall;                    // "Console.print" or "*.push" (greedy)
			CallScope scope;                   // from the CallScopeInfo lookup
			String note;                       // callScopeNote, e.g. "allocates MemoryOutputStream"
			Identifier outerHolderType;        // "Callback" or "InlineFunction" — type of outermost holder

			virtual Ptr clone() const = 0;
			virtual String toCallStackString(Processor* p = nullptr) const = 0;

		protected:

			/** Copies all base members into target. Call from subclass clone(). */
			void cloneBaseMembers(ItemBase* target) const
			{
				target->apiCall = apiCall;
				target->scope = scope;
				target->note = note;
				target->outerHolderType = outerHolderType;
			}
		};

		struct Holder
		{
			virtual ~Holder() {}
			virtual RealtimeSafetyInfo* getRealtimeSafetyInfo() = 0;
			virtual Identifier getCallScopeId() const { return {}; }
			virtual Identifier getCallScopeType() const { return {}; }
		};

		ItemBase::List items;
		bool analyzed = false;

		bool isEmpty() const { return items.isEmpty(); }

		bool hasUnsafe() const
		{
			for (auto w : items)
				if (w->scope == CallScope::Unsafe || w->scope == CallScope::Init)
					return true;
			return false;
		}

		bool hasWarning() const
		{
			for (auto w : items)
				if (w->scope == CallScope::Warning)
					return true;
			return false;
		}

		/** Returns a formatted report string for the console.
		 *  Filters based on strictness: Warn includes warning+unsafe, Error same.
		 *  Relaxed returns empty (caller should not call this with Relaxed).
		 */

		String toString(StrictnessLevel l, Processor* p) const;

		/** Encapsulates the full enforcement pattern at call sites.
		 *
		 *  1. Checks callable->isRealtimeSafe() — if false, returns true (structural rejection)
		 *  2. Queries StrictnessLevel via JavascriptProcessor::getStrictnessLevel()
		 *  3. Calls callable->getRealtimeSafetyReport(strictness) → SafetyReport
		 *  4. Logs message if non-empty (both Warn and Strict mode)
		 *  5. Returns true (blocks registration) only if Strict + worstScope is Unsafe/Init
		 */

		static bool check(WeakCallbackHolder::CallableObject* callable, ScriptingObject* caller, const String& context);

	};

	template <int E, int FIndex=0> static ApiClass::DiagnosticResult checkCallbackNumArgs(DiagnosticBase*, const Identifier&, const Array<var>& args)
	{
		if (auto f = dynamic_cast<CallableObject*>(args[FIndex].getObject()))
		{
			auto numActual = f->getNumArguments();

			if (numActual != E)
				return ApiClass::DiagnosticResult::fail("wrong argument count for callback").withExpectation(String(numActual) + " params", String(E) + " params");
		}

		return ApiClass::DiagnosticResult::unknown();
	}

#endif
	struct CallableObjectManager
	{
		virtual ~CallableObjectManager() {};

		template <typename T> T* getRegisteredCallableObject(int index)
		{
			static_assert(std::is_base_of<CallableObject, T>(), "not a base class");

			if (isPositiveAndBelow(index, registeredObjects.size()))
			{
				return dynamic_cast<T*>(registeredObjects[index].get());
			}

			return nullptr;
		}

		int getNumRegisteredCallableObjects() const { return registeredObjects.size(); }

		void registerCallableObject(CallableObject* obj)
		{
			registeredObjects.addIfNotAlreadyThere(obj);
		}

		void deregisterCallableObject(CallableObject* obj)
		{
			registeredObjects.removeAllInstancesOf(obj);
		}

		template <typename T> void addCallbackObjectClearListener(T& obj, const std::function<void(T&, bool)>& f)
		{
			clearMessageBroadcaster.addListener(obj, f, false);
		}

	protected:

		void clearCallableObjects()
		{
			registeredObjects.clear();
			clearMessageBroadcaster.sendMessage(sendNotificationAsync, true);
		}

	private:

		LambdaBroadcaster<bool> clearMessageBroadcaster;
		Array<WeakReference<CallableObject>> registeredObjects;
	};

	WeakCallbackHolder(ProcessorWithScriptingContent* p, ApiClass* parentObject, const var& callback, int numExpectedArgs);

	/** @internal: used by the scripting thread. */
	WeakCallbackHolder(const WeakCallbackHolder& copy);

	WeakCallbackHolder(WeakCallbackHolder&& other);

	~WeakCallbackHolder();

	WeakCallbackHolder& operator=(WeakCallbackHolder&& other);

	/** Call the function with the given arguments. */
	void call(var* arguments, int numArgs);

	void call(const var::NativeFunctionArgs& args);
	
	Result callSync(const var::NativeFunctionArgs& args, var* returnValue = nullptr);

	/** Call the functions synchronously. */
	Result callSync(var* arguments, int numArgs, var* returnValue=nullptr);

	/** Call the function with one argument that can be converted to a var. */
	template <typename T> void call1(const T& arg1)
	{
		var a(arg1);
		call(&a, 1);
	}

	/** Calls the function with any iteratable var container. */
	template <typename ContainerType> void call(const ContainerType& t)
	{
		// C++ Motherf%!(%...
		call(const_cast<var*>(&*t.begin()), (int)t.size());
	}

	void call(Array<var> v)
	{
		call(const_cast<var*>(v.getRawDataPointer()), v.size());
	}

	/** @internal: used by the scripting thread. */
	Result operator()(JavascriptProcessor* p);

	void setHighPriority()
	{
		highPriority = true;
	}

	/** Increases the reference count for the callback object. 
		Use this in order to assure the liveness of the callback, but beware of leaking. 
	*/
	void incRefCount();

	DebugInformationBase* createDebugObject(const String& n) const;

	void decRefCount()
	{
		anonymousFunctionRef = var();
	}

	void addAsSource(DebugableObjectBase* sourceObject, const String& callbackId, bool lookupVariableNameLater=true);

	void addProfileSources(DebugInformationBase::Ptr p);

	void clear();

	operator bool() const
	{
		return weakCallback.get() != nullptr && engineToUse.get() != nullptr;
	}

	void setThisObject(ReferenceCountedObject* thisObj);

	void setThisObjectRefCounted(const var& t);

	bool matches(const var& f) const;

	void reportError(const Result& r);

	void setTrackIndex(uint64_t trackIndexToUse)
	{
		trackIndex = trackIndexToUse;
	}

#if USE_BACKEND
	void addCallbackDiagnostic(DiagnosticBase* c, const Identifier& methodName, int fIndex = 0)
	{
		auto numArgs = numExpectedArgs;

		c->addDiagnostic(methodName, [numArgs, fIndex](DiagnosticBase*, const Identifier&, const Array<var>& args)
		{
			if (auto f = dynamic_cast<CallableObject*>(args[fIndex].getObject()))
			{
				auto numActual = f->getNumArguments();

				if (numActual != numArgs)
					return ApiClass::DiagnosticResult::fail("wrong argument count for callback").withExpectation(String(numActual) + " params", String(numArgs) + " params");
			}

			return ApiClass::DiagnosticResult::unknown();
		});
	}
#endif

private:

	var getThisObject();

	Identifier cid;
	uint64_t trackIndex = 0;

	bool highPriority = false;
	int numExpectedArgs;
	Result r;
	Array<var> args;
	var anonymousFunctionRef;
	NamedValueSet capturedLocals;
	WeakReference<CallableObject> weakCallback;
	WeakReference<DebugableObjectBase> thisObject;

	var refCountedThisObject;

	WeakReference<HiseJavascriptEngine> engineToUse;
	ProfileCollection::ID pTrigger = -1;
	ProfileCollection::ID pCall = -1;
};

class AssignableDotObject
{
public:

	virtual ~AssignableDotObject() {};

	/** Override this method and assign the new value to the given id. */
	virtual bool assign(const Identifier& id, const var& newValue) = 0;

	/** Override this method and return the given id. */
	virtual var getDotProperty(const Identifier& id) const = 0;
};


/** @internal A interface class for objects that can be used with the [] operator in Javascript.
	
*
*	It uses a cached look up index on compilation to accelerate the look up.
*/
class AssignableObject
{
public:

	/** Use this to create a child data in the watch table. */
	struct IndexedValue
	{
		IndexedValue(AssignableObject* obj_, int idx): index(idx), obj(obj_) {}

		var operator()()
		{
			if (obj.get() != nullptr)
				return obj->getAssignedValue(index);

			return var();
		}

		Identifier getId() const
		{
			String s = "%PARENT%";
			s << "[" << String(index) << "]";
			return Identifier(s);
		}

	private:

		const int index;
		WeakReference<AssignableObject> obj;
	};

	virtual ~AssignableObject() {};

	/** Assign the value to the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	virtual void assign(const int index, var newValue) = 0;

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	virtual var getAssignedValue(int index) const = 0;

	/** Overwrite this and return an index that can be used to look up the value when the script is executed. */
	virtual int getCachedIndex(const var &indexExpression) const = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(AssignableObject);
};

struct JSONConversionHelpers: public valuetree::Helpers
{
	static bool isPluginState(const ValueTree& v) { return v.getType() == Identifier("ControlData"); };

	static var convertBase64Data(const String& d, const ValueTree& cTree);

	static String convertDataToBase64(const var& d, const ValueTree& cTree);
};



struct ValueTreeConverters
{
	static String convertDynamicObjectToBase64(const var& object, const Identifier& id, bool compress);;

	/** This converts a dynamic object to a value tree.
	
		If the JSON object contains arrays, the value tree will create child trees with the
		same name (minus a optional `s` at the end to indicate plural vs. singular). 
		If the array is only a simple number / string, it will be stored as `value` property.

		Example:

		{
		  "Object":
		  {
			"Property1": "SomeValue  
		  },
		  "ListElements": [
		  12,
		  14,
		  []
		  ]
		}
	*/
	static ValueTree convertDynamicObjectToValueTree(const var& object, const Identifier& id);

	static String convertValueTreeToBase64(const ValueTree& v, bool compress);

	static var convertBase64ToDynamicObject(const String& base64String, bool isCompressed);

	static ValueTree convertBase64ToValueTree(const String& base64String, bool isCompressed);

	static var convertValueTreeToDynamicObject(const ValueTree& v);

	static var convertFlatValueTreeToVarArray(const ValueTree& v);

	static ValueTree convertVarArrayToFlatValueTree(const var& ar, const Identifier& rootId, const Identifier& childId);

	static void copyDynamicObjectPropertiesToValueTree(ValueTree& v, const var& obj, bool skipArray=false);

	static void copyValueTreePropertiesToDynamicObject(const ValueTree& v, var& obj);

	static var convertContentPropertiesToDynamicObject(const ValueTree& v);

	static ValueTree convertDynamicObjectToContentProperties(const var& d);

	static var convertScriptNodeToDynamicObject(ValueTree v);

	static ValueTree convertDynamicObjectToScriptNodeTree(var obj);

	static var convertStringIfNumeric(const var& value);

private:

	

	static void v2d_internal(var& object, const ValueTree& v);

	static void d2v_internal(ValueTree& v, const Identifier& id, const var& object);;

	static void a2v_internal(ValueTree& v, const Identifier& id, const Array<var>& list);

	static void v2a_internal(var& object, ValueTree& v, const Identifier& id);

	static bool isLikelyVarArray(const ValueTree& v);

};




} // namespace hise
#endif  // SCRIPTINGBASEOBJECTS_H_INCLUDED
