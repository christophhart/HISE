/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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

#pragma once

namespace snex {
namespace jit {
using namespace juce;

/** This object contains information about possible runtime issues that
    are catches when the SafeChecks are enabled. 
	
	In order to use it in the compiler, fetch the address of the global
	runtime error object and overwrite the bytes as shown below with 
	meaningful codes:
	
	Byte 0: error code (should be an ErrorType enum value)
	Byte 4: line number
	Byte 8: col number
	Byte 12: special data 1 (if used)
	Byte 16: special data 2 (if used) 
*/
struct RuntimeError
{
	enum class ErrorType
	{
		OK,
		DynAccessOutOfBounds,
		DynReferIllegalSize,
		DynReferIllegalOffset,
		IntegerDivideByZero,
		WhileLoop,
		NullptrAccess
	};

	bool wasOk() const noexcept { return errorType == (int)ErrorType::OK; }

	String toString() const noexcept
	{
		String s;

		s << "Line " << String(lineNumber) << "(" << String(colNumber) << "): ";
		
		switch ((ErrorType)errorType)
		{
		case ErrorType::IntegerDivideByZero: s << "Runtime error (division by zero)"; break;
		case ErrorType::WhileLoop: s << "Runtime error (endless while loop)"; break;
		case ErrorType::DynAccessOutOfBounds: s << "dyn operator[] out of bounds - " << String(data1) << ", limit: " << String(data2); break;
		case ErrorType::DynReferIllegalOffset: s << "referTo: illegal offset for dynamic source: " << String(data2) << "(total size: " + String(data1) << ")"; break;
		case ErrorType::DynReferIllegalSize: s << "referTo: illegal size for dynamic source (" << String(data1) << ")"; break;
        default: break;
		}

		return s;
	}

	int errorType = 0;
	int lineNumber = 0;
	int colNumber = 0;
	int data1 = 0;
	int data2 = 0;
};

/** A external definition that can be added to the preprocessor. */
struct ExternalPreprocessorDefinition
{
	using List = Array<ExternalPreprocessorDefinition>;

	enum class Type
	{
		Definition,
		Macro,
		Empty,
		numTypes
	};

	bool operator==(const ExternalPreprocessorDefinition& other) const
	{
		return name.compareNatural(other.name) == 0;
	}

	Type t;
	String name;
	String value;
	String description;
	int charNumber = -1;
	String fileName;
};

/** A interface class for anything that needs to print out the logs from the
	compiler.

	If you want to enable debug output at runtime, you need to pass it to
	GlobalScope::addDebugHandler.

	If you want to show the compiler output, pass it to Compiler::setDebugHandler
*/
struct DebugHandler
{
	struct Tokeniser : public juce::CodeTokeniser
	{
		int readNextToken(CodeDocument::Iterator& source) override;
		CodeEditorComponent::ColourScheme getDefaultColourScheme() override;
	};

	virtual ~DebugHandler() {};

	/** Overwrite this function and print the message somewhere.

		Be aware that this is called synchronously, so it might cause clicks.
	*/
	virtual void logMessage(int level, const juce::String& s) = 0;

	/** Override this function and implement some kind of (asynchronous) blinking
	    on the UI.
	*/
	virtual void blink(int line) {};

	virtual void recompiled() {};

	virtual void clearLogger() {};

	/** Override this and change the behaviour for the handler depending on its supposed enablement. */
	virtual void setEnabled(bool shouldBeEnabled)
	{
		ignoreUnused(shouldBeEnabled);
	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(DebugHandler);
};

class BaseCompiler;

struct DataPool
{
	struct Item
	{
		Symbol s;
		int offset;
	};


	juce::HeapBlock<char> data;
};

class BreakpointHandler
{
public:

	enum EventType
	{
		Break,
		Resume,
		numEventTypes
	};

	struct Listener
	{
		virtual ~Listener() {};

		virtual void eventHappened(BreakpointHandler* handler, EventType type) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	struct Entry
	{
		juce::String toString() const
		{
			juce::String s;
			s << id.toString();
			s << " (" << Types::Helpers::getTypeName(currentValue.getType()) << "): ";
			s << Types::Helpers::getCppValueString(currentValue);
			s << "\n";
            
            return s;
		}

		Symbol id;
		VariableStorage currentValue;
		BaseScope::ScopeType scope;
		bool changed = false;
		bool isUsed = false;
	};

	bool shouldResume() const
	{
		if (executingThread != nullptr)
		{
			jassert(Thread::getCurrentThread() == executingThread);

			if (executingThread->threadShouldExit())
				return true;
		}

		return resumeOnNextCheck;
	}

	bool isRunning() const
	{
		return runExection;
	}

	void breakExecution()
	{
		if (!shouldAbort())
		{
			runExection.store(false);
			resumeOnNextCheck.store(false);
			sendMessage();
		}
	}

	void resume()
	{
		if (executingThread != nullptr)
		{
			if (executingThread->threadShouldExit())
				return;

			executingThread->notify();
		}

		runExection.store(true);
		resumeOnNextCheck.store(true);

		
		sendMessage();
	}

	bool shouldAbort() const
	{
		if (executingThread != nullptr)
			return executingThread->threadShouldExit();

		return false;
	}

	bool canWait() const
	{
		if (executingThread != nullptr)
			return !executingThread->threadShouldExit();
		
		return true;
	}

	void abort()
	{
		if (executingThread != nullptr)
			executingThread->signalThreadShouldExit();
	}

	void sendMessage()
	{
		auto e = runExection ? EventType::Resume : EventType::Break;
		sendMessageToListeners(e);
	}

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	int getNumEntries() const 
	{ 
		return numEntries;
	}

	Entry getEntry(const Symbol& id) const 
	{ 
		for (int i = 0; i < 128; i++)
		{
			if (dumpTable[i].id == id)
				return dumpTable[i];
		}

		return {};
	}
	Entry getEntry(int index) const { return dumpTable[index]; }

	void clearTable()
	{
		for (int i = 0; i < 128; i++)
			dumpTable[i].isUsed = false;

		numEntries = 0;
	}

	void* getNextFreeTable(const Symbol& ref)
	{
		for (int i = 0; i < 128; i++)
		{
			if (!dumpTable[i].isUsed || dumpTable[i].id == ref)
			{
				if(!dumpTable[i].isUsed)
					numEntries++;

				dumpTable[i].isUsed = true;
                dumpTable[i].id = ref;
				dumpTable[i].scope = BaseScope::Class; 
				dumpTable[i].currentValue = VariableStorage(ref.typeInfo.getType(), 0);

				return dumpTable[i].currentValue.getDataPointer();
			}
		}

		return nullptr;
	}

	uint64* getLineNumber()
	{
		return &lineNumber;
	}
	
	bool isActive() const
	{
		return enabled && executingThread != nullptr && Thread::getCurrentThread() == executingThread;
	}

	void setExecutingThread(Thread* threadToUse)
	{
		executingThread = threadToUse;
	}

	void setEnabled(bool shouldBeEnabled)
	{
		enabled = shouldBeEnabled;
	}

private:

	Thread* executingThread = nullptr;

	bool enabled;
	bool active = false;
	uint64 lineNumber = 0;
	
	Entry dumpTable[128];
	int numEntries = 0;

	void sendMessageToListeners(EventType t)
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->eventHappened(this, t);
		}
	}

	Array<WeakReference<Listener>> listeners;

	std::atomic<bool> runExection = { true };
	std::atomic<bool> resumeOnNextCheck = { false };
};


/** The global scope that is passed to the compiler and contains the global variables
	and all registered objects.

	It has the following additional features:

	 - a listener system that notifies its Listeners when a registered object is deleted.
	   This is useful to invalidate JIT compiled functions that access this object's
	   function
	 - a dynamic list of JIT callable objects that can be registered and called from
	   JIT code. Objects derived from FunctionClass can subscribe and unsubscribe to this
	   scope and will be resolved as member function calls.

	You have to pass a reference to an object of this scope to the Compiler.
*/
class GlobalScope : public FunctionClass,
				    public BaseScope,
					public AsyncUpdater
{
public:

	GlobalScope();

	struct ObjectDeleteListener
	{
		virtual ~ObjectDeleteListener() {};

		virtual void objectWasDeleted(const NamespacedIdentifier& id) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(ObjectDeleteListener);
	};

	
	void registerObjectFunction(FunctionClass* objectClass);

	void deregisterObject(const NamespacedIdentifier& id);

	void registerFunctionsToNamespaceHandler(NamespaceHandler& handler);

	bool hasFunction(const NamespacedIdentifier& id) const override;

	void addMatchingFunctions(Array<FunctionData>& matches, const NamespacedIdentifier& symbol) const override;

	void addObjectDeleteListener(ObjectDeleteListener* l);
	void removeObjectDeleteListener(ObjectDeleteListener* l);

	Map getMap() override;

	static GlobalScope* getFromChildScope(BaseScope* scope)
	{
		auto parent = scope->getParent();

		while (parent != nullptr)
		{
			scope = parent;
			parent = parent->getParent();
		}

		return dynamic_cast<GlobalScope*>(scope);
	}

	FunctionClass* getGlobalFunctionClass(const NamespacedIdentifier& id)
	{
		for (auto c : objectClassesWithJitCallableFunctions)
			if (c->getClassName() == id)
				return c;

		return nullptr;
	}
	
	void addDebugHandler(DebugHandler* handler)
	{
		debugHandlers.addIfNotAlreadyThere(handler);
	}

	void removeDebugHandler(DebugHandler* handler)
	{
		debugHandlers.removeAllInstancesOf(handler);
	}

	void sendBlinkMessage(int lineNumber);

	void logMessage(const String& message);

	/** Add an optimization pass ID that will be added to each compiler that uses this scope. 
		
		Use one of the IDs defined in the namespace OptimizationIds.
	*/
	void addOptimization(const juce::String& passId);

	void clearOptimizations();


	const StringArray& getOptimizationPassList() const
	{
		return optimizationPasses;
	}

	void setCurrentClassScope(BaseScope* b)
	{
		currentClassScope = b;
	}

	BaseScope* getCurrentClassScope()
	{
		return currentClassScope;
	}

	BreakpointHandler& getBreakpointHandler() { return breakPointHandler; }

	
	/** This returns the address to the runtime error flag. 
		
		If the NoSafeChecks optimisation is disabled, you can implement
		some safe checks that can write an error into this flag. 
		The return value is the 64bit absolute address that points to the
		RunTimeError object in use...


	*/
	uint64_t getRuntimeErrorFlag() { return reinterpret_cast<uint64_t>(&currentRuntimeError.errorType); }

	void sendRecompileMessage()
	{
		for (auto dh : debugHandlers)
		{
			if (dh != nullptr)
				dh->recompiled();
		}
	}

	bool isRuntimeErrorCheckEnabled() const noexcept { return !optimizationPasses.contains(OptimizationIds::NoSafeChecks); }

	bool checkRuntimeErrorAfterExecution();

	Result getRuntimeError() const { return runtimeError; }

	void setPreprocessorDefinitions(var d, bool clearExisting=false);

	void setPreprocessorDefinitions(const ExternalPreprocessorDefinition::List& d, bool clearExisting=false);

	ExternalPreprocessorDefinition::List getPreprocessorDefinitions() const { return preprocessorDefinitions; }

	static ExternalPreprocessorDefinition::List getDefaultDefinitions();

	Types::PolyHandler* getPolyHandler()
	{
		return &polyHandler;
	}

	void setPolyphonic(bool shouldBePolyphonic)
	{
		if (shouldBePolyphonic != polyHandler.isEnabled())
		{
			polyHandler.setEnabled(shouldBePolyphonic);

			for (auto& epd : preprocessorDefinitions)
			{
				if (epd.name == "NUM_POLYPHONIC_VOICES")
				{
					epd.value = String(shouldBePolyphonic ? NUM_POLYPHONIC_VOICES : 1);
					break;
				}
			}
		}
	}

	void addNoInliner(const Identifier& id)
	{
		noInliners.addIfNotAlreadyThere(id);
	}

	bool shouldInlineFunction(const Identifier& id)
	{
		if (!optimizationPasses.contains(OptimizationIds::Inlining))
			return false;

		if (noInliners.contains(id))
			return false;

		return true;
	}

	void setDebugMode(bool shouldBeDebugMode)
	{
		if (debugMode != shouldBeDebugMode)
		{
			debugMode = shouldBeDebugMode;

			getBreakpointHandler().setEnabled(shouldBeDebugMode);

			for (auto d : debugHandlers)
			{
				if (d.get() != nullptr)
					d->setEnabled(shouldBeDebugMode);
			}
		}
	}

	bool isDebugModeEnabled() const { return debugMode; }

    void setUseInterpreter(bool shouldUseInterpreter) { interpreterMode = shouldUseInterpreter; }
    
    bool isUsingInterpreter() const { return interpreterMode; }
    
	void clearDebugMessages();

private:

	void handleAsyncUpdate();

	bool clearNext = false;
	hise::SimpleReadWriteLock messageLock;
	Array<String> pendingMessages;

	Map currentMap;

    bool interpreterMode = false;
    bool debugMode = false;

	Array<Identifier> noInliners;

	Types::PolyHandler polyHandler;
	
	ExternalPreprocessorDefinition::List preprocessorDefinitions;

	ComplexType::Ptr blockType;

	RuntimeError currentRuntimeError;
	Result runtimeError;

	BreakpointHandler breakPointHandler;
	WeakReference<BaseScope> currentClassScope;

	StringArray optimizationPasses;

	Array<WeakReference<DebugHandler>> debugHandlers;

	Array<WeakReference<ObjectDeleteListener>> deleteListeners;

	ReferenceCountedArray<FunctionClass> objectClassesWithJitCallableFunctions;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalScope);
	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalScope);
};



}
}
