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

/** A interface class for anything that needs to print out the logs from the
	compiler.

	If you want to enable debug output at runtime, you need to pass it to
	GlobalScope::addDebugHandler.

	If you want to show the compiler output, pass it to Compiler::setDebugHandler
*/
struct DebugHandler
{
	virtual ~DebugHandler() {};

	/** Overwrite this function and print the message somewhere.

		Be aware that this is called synchronously, so it might cause clicks.
	*/
	virtual void logMessage(const juce::String& s) = 0;

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

class BreakpointHandler: public AsyncUpdater
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
		return abortExecution || resumeOnNextCheck;
	}

	bool isRunning() const
	{
		return runExection;
	}

	void breakExecution()
	{
		runExection.store(false);
		resumeOnNextCheck.store(false);
		triggerAsyncUpdate();
	}

	void resume()
	{
		if (!abortExecution)
		{
			runExection.store(true);
			resumeOnNextCheck.store(true);
			triggerAsyncUpdate();
		}
	}

	bool canWait() const
	{
		return true;
	}

	void abort()
	{
		abortExecution.store(true);
	}

	void handleAsyncUpdate()
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
		abortExecution = false;

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
		return active;
	}

	void setActive(bool shouldBeActive)
	{
		active = shouldBeActive;
	}

private:

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

	std::atomic<bool> abortExecution = { false };
	std::atomic<bool> runExection = { true };
	std::atomic<bool> resumeOnNextCheck = { false };
};


class BufferHandler
{
public:

	enum class AccessorTypes
	{
		ZeroAccessor,
		WrapAccessor,
		numAccessorTypes
	};

	template <int FixedSize = -1> struct ZeroAccessor
	{
		static constexpr int Limit = FixedSize;

		ZeroAccessor(block& p) :
			parent(p),
			zero(0.0f)
		{};

		int getLimit() const
		{
			return FixedSize != -1 ? FixedSize : parent.size();
		}

		const float& operator[](int index) const
		{
			if (index < 0 || index >= getLimit())
			{
				zero = 0.0f;
				return zero;
			}

			return parent[index];
		}

		float& operator[](int index)
		{
			if (index < 0 || index >= getLimit())
			{
				zero = 0.0f;
				return zero;
			}

			index %= parent.size();
			return parent[index];
		}

		block& parent;
		mutable float zero;
	};

	template <int FixedSize = -1> struct WrapAccessor
	{
		static constexpr int Limit = FixedSize;

		WrapAccessor(block& p) :
			parent(p)
		{};

		int getLimit() const
		{
			return FixedSize != -1 ? FixedSize : parent.size();
		}

		const float& operator[](int index) const
		{
			index %= getLimit();

			if (index < 0)
				index += getLimit();

			return parent[index];
		}

		float& operator[](int index)
		{
			auto l = getLimit();

			if (l == 0)
			{
				static float zero = 0.0f;
				return zero;
			}
				
			index %= getLimit();
			
			if (index < 0)
				index += getLimit();

			return parent[index];
		}

		block& parent;
	};

	template <class Accessor> struct BlockValue
	{
		BlockValue(block b_) :
			b(b_),
			t(b)
		{};

		BlockValue& operator=(int index)
		{
			currentIndex = index;
			return *this;
		}

		float& operator*()
		{
			return t[currentIndex];
		}

		const float& operator*() const
		{
			return t[currentIndex];
		}

		float operator++(int)
		{
			auto value = t[currentIndex];
			currentIndex++;
			return value;
		}

		float operator++()
		{
			currentIndex++;
			return t[currentIndex];
		}

		float& operator[](int index)
		{
			if (b.size() == 0)
			{
				empty = 0.0f;
				return empty;
			}

			return t[index];
		}

		Accessor t;
		int currentIndex = 0;
		block b;
		float empty;
	};

	virtual ~BufferHandler()
	{
		registeredItems.clear();
	}

	block create(const Identifier& id, int size)
	{
		return registerInternalData(id, size);
	}

	block create(int size)
	{
        Identifier id("internal" + juce::String(registeredItems.size()));
		return registerInternalData(id, size);
	}

	block getAudioFile(int fileIndex, const Identifier& variableName=Identifier())
	{
		updateVariableName(fileIndex, false, variableName);
		return getData(getIdForExternalData(fileIndex, false));
	}

	block getTable(int tableIndex, const Identifier& variableName=Identifier())
	{
		updateVariableName(tableIndex, true, variableName);
		return getData(getIdForExternalData(tableIndex, true));

	}

	void registerTable(int index, block b)
	{
		registerExternalData(getIdForExternalData(index, true), b);
	}

	void registerAudioFile(int index, block b)
	{
		registerExternalData(getIdForExternalData(index, false), b);
	}

	block getData(const Identifier& id)
	{
		for (auto& i : registeredItems)
		{
			if (i->id == id || i->variableName == id)
				return i->b;
		}

		throw String("Didn't find buffer data");
	}

	/** This is called before every compilation and removes all internal buffers. */
	void reset()
	{
		registerExternalItems();

		for (int i = 0; i < registeredItems.size(); i++)
		{
			if (registeredItems[i]->allocatedSize != 0)
				registeredItems.removeObject(registeredItems[i--]);
		}
	}

	virtual void registerExternalItems() {};
	
private:

	void updateVariableName(int index, bool isTable, const Identifier& variableName)
	{
		auto id = getIdForExternalData(index, isTable);

		for (auto& i : registeredItems)
		{
			if (i->id == id)
			{
				i->variableName = variableName;
			}
		}
	}

	Identifier getIdForExternalData(int index, bool isTable)
	{
		return Identifier((isTable ? "Table" : "AudioFile") + String(index));
	}

	block registerExternalData(const Identifier& id, block b)
	{
		for (auto& i : registeredItems)
			if (i->id == id)
				return i->b;

		auto newItem = new Item();
		newItem->id = id;
		newItem->b = b;
		newItem->isConst = true;

		registeredItems.add(newItem);
		return newItem->b;
	}

	block getExternalData(const Identifier& id)
	{
		for (auto& i : registeredItems)
		{
			if (i->allocatedSize > 0)
				continue;

			if (i->variableName == id)
			{
				if (i->isConst)
					throw String("Can't use non-const access");

				return i->b;
			}
		}

		throw String("Didn't find external data");
	}

	block registerInternalData(const Identifier& id, int size)
	{
		for (auto& i : registeredItems)
		{
			if (i->variableName == id)
			{
				if (size == i->allocatedSize)
					return i->b;
				else
				{
					i->allocatedSize = size;
					i->internalData.realloc(size, sizeof(float));
					FloatVectorOperations::clear(i->internalData, size);
					i->b = block(i->internalData, size);
					return i->b;
				}
			}
		}

		auto newItem = new Item();

		newItem->variableName = id;
		newItem->internalData.allocate(size, true);
		newItem->allocatedSize = size;
		FloatVectorOperations::clear(newItem->internalData, size);
		newItem->b = block(newItem->internalData, size);
		newItem->isConst = false;

		registeredItems.add(newItem);
		return newItem->b;
	}

	struct Item
	{
		Item() :
			id(Identifier()),
			b({})
		{

		};

		bool isUsed() const
		{
			return variableName.isValid();
		}

		Identifier variableName;
		Identifier id;
		block b;
		bool isConst;

		HeapBlock<float> internalData;
		int allocatedSize = 0;
	};

	OwnedArray<Item> registeredItems;
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
	public BaseScope
{
public:

	GlobalScope(int numVariables = 1024);

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

	void logMessage(const String& message)
	{
		for (auto dh : debugHandlers)
			dh->logMessage(message);
	}

	/** Add an optimization pass ID that will be added to each compiler that uses this scope. 
		
		Use one of the IDs defined in the namespace OptimizationIds.
	*/
	void addOptimization(const Identifier& passId)
	{
		optimizationPasses.addIfNotAlreadyThere(passId);
	}

	void clearOptimizations()
	{
		optimizationPasses.clear();
	}

	const Array<Identifier>& getOptimizationPassList() const
	{
		return optimizationPasses;
	}

	BufferHandler& getBufferHandler() { return *bufferHandler.get(); }

	void setBufferHandler(BufferHandler* newBufferHandler)
	{
		bufferHandler = newBufferHandler;
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

private:

	ScopedPointer<FunctionClass> consoleFunctions;

	BreakpointHandler breakPointHandler;
	WeakReference<BaseScope> currentClassScope;

	ScopedPointer<BufferHandler> bufferHandler;

	Array<Identifier> optimizationPasses;

	Array<WeakReference<DebugHandler>> debugHandlers;

	Array<WeakReference<ObjectDeleteListener>> deleteListeners;

	OwnedArray<FunctionClass> objectClassesWithJitCallableFunctions;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalScope);
	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalScope);
};



}
}
