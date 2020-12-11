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
using namespace juce;

struct Node
{
	Node(const JitObject& o, ComplexType::Ptr nodeClass)
	{
		data.allocate(nodeClass->getRequiredAlignment() + nodeClass->getRequiredByteSize(), true);

		ComplexType::InitData d;

		d.callConstructor = nodeClass->hasConstructor();
		d.initValues = nodeClass->makeDefaultInitialiserList();
		d.dataPointer = data.get() + nodeClass->getRequiredAlignment();

		FunctionClass::Ptr fc = nodeClass->getFunctionClass();

		auto callbackIds = Types::ScriptnodeCallbacks::getIds(fc->getClassName());

		for (int i = 0; i < Types::ScriptnodeCallbacks::numFunctions; i++)
		{
			callbacks[i] = fc->getNonOverloadedFunction(callbackIds[i]);
			callbacks[i].object = d.dataPointer;
		}

		nodeClass->initialise(d);
	}

	void process(Types::ProcessDataDyn& data)
	{
		callbacks[Types::ScriptnodeCallbacks::ProcessFunction].callVoid(&data);
	}

	void reset()
	{
		callbacks[Types::ScriptnodeCallbacks::ResetFunction].callVoid();
	}

	void prepare(Types::PrepareSpecs ps)
	{
		callbacks[Types::ScriptnodeCallbacks::PrepareFunction].callVoid(&ps);
	}

	void handleEvent(HiseEvent& e)
	{
		callbacks[Types::ScriptnodeCallbacks::HandleEventFunction].callVoid(&e);
	}

	JitObject obj;
	ComplexType::Ptr nodeClass;

	HeapBlock<uint8> data;

	FunctionData callbacks[Types::ScriptnodeCallbacks::ID::numFunctions];
};

namespace ui
{
using namespace Types;
using namespace jit;

struct WorkbenchData : public ReferenceCountedObject,
	public DebugHandler,
	public ApiProviderBase::Holder,
	public AsyncUpdater
{
	static String getDefaultCode(bool getTestCode);
	static String getDefaultNodeTemplate(const Identifier& mainClass);
	static String getTestTemplate();

	using Ptr = ReferenceCountedObjectPtr<WorkbenchData>;
	using WeakPtr = WeakReference<WorkbenchData>;

	/** Formats the log event to print nicely on a console. */
	String convertToLogMessage(int level, const String& s);

	void logMessage(int level, const juce::String& s) override
	{
		if (MessageManager::getInstance()->isThisTheMessageThread())
		{
			for (auto l : listeners)
			{
				if (l != nullptr)
					l->logMessage(this, level, s);
			}
		}
		else
		{
			WeakPtr wp(this);

			MessageManager::callAsync([wp, level, s]()
			{
				if (wp != nullptr)
					wp.get()->logMessage(level, s);
			});
		}
	}

	ValueTree createApiTree() override
	{
		return getLastJitObject().createValueTree();
	}

	ApiProviderBase* getProviderBase() override { return &lastObject; }

	void handleBreakpoints(const Identifier& codeFile, Graphics& g, Component* c) override
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
			{
				if (c == dynamic_cast<Component*>(l.get()))
				{
					l->drawBreakpoints(g);
					return;
				}
			}
		}
	}

	struct Listener
	{
		virtual ~Listener() {};

		virtual void preCompile() {};

		virtual void recompiled(WorkbenchData::Ptr wb) = 0;

		virtual void drawBreakpoints(Graphics& g) {};

		virtual void logMessage(WorkbenchData::Ptr wb, int level, const String& s)
		{
			ignoreUnused(wb, level, s);
		};

		virtual void workbenchChanged(Ptr oldWorkBench, Ptr newWorkbench)
		{
			if (oldWorkBench != nullptr)
				oldWorkBench->removeListener(this);

			if (newWorkbench != nullptr)
				newWorkbench->addListener(this);
		}

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};



	using LoadFunction = std::function<String()>;
	using PreprocessFunction = std::function<String(const String&)>;
	using SaveFunction = std::function<bool(const String& s)>;
	using CompileManager = std::function<bool(void)>;
	using CompileProcedure = std::function<Result(void)>;
	

	WorkbenchData() :
		memory(1024),
		lastResult(Result::ok())
	{
		memory.addDebugHandler(this);
	};

	/** Call this if you want to recompile the current workbench.

		It either recompiles it synchronously or calls a lambda that might
		defer the compilation to a background thread.

	*/
	void triggerRecompile()
	{
		for (auto& l : listeners)
		{
			if (l != nullptr)
				l->preCompile();
		}

		if (compileManager)
			compileManager();
		else
			handleCompilation();
	}

	void handleAsyncUpdate()
	{
		if (lastResult.wasOk())
		{
			getLastJitObject().rebuildDebugInformation();
			rebuild();
		}

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->recompiled(this);
		}
	}

	/** You can add additional callbacks that will be executed after a successful compilation. 
	
		You can customize the way the code is being compiled (and executed) by adding other functions
	*/
	void setCompileProcedure(const CompileProcedure& f)
	{
		compileProcedure = f;
	}

	Result compileTestCase();

	Result defaultCompilation();

	Result compileJitNode();

	bool handleCompilation()
	{
		if (!getGlobalScope().getBreakpointHandler().isRunning())
		{
			jassertfalse;
#if 0
			getGlobalScope().getBreakpointHandler().abort();

			runThread.stopThread(1000);

			while (runThread.isThreadRunning())
			{
				Thread::getCurrentThread()->wait(800);
			}

			memory.getBreakpointHandler().clearTable();
#endif
		}

		jassert(compileProcedure);

		lastResult = compileProcedure();

		triggerAsyncUpdate();

		return true;
	}

	void setUseFileAsContentSource(const File& f)
	{
		connectedFile = f;

		setContentFunctions([f]() { return f.loadFileAsString(); }, [f](const String& s) { return f.replaceWithText(s); });
	}

	/** Set a compile function. This function must call `handleCompilation` at some point and return true or false if the compilation was done synchronously. */
	void setCompileManager(const CompileManager& f)
	{
		compileManager = f;
	}

	void setContentFunctions(const LoadFunction& lf, const SaveFunction& sf)
	{
		loadFunction = lf;
		saveFunction = sf;
	}

	bool setCode(const String& s, NotificationType recompileOnOk)
	{
		if (saveFunction)
		{
			auto ok = saveFunction(s);

			if (ok && recompileOnOk != dontSendNotification)
				triggerRecompile();

			return ok;
		}

		return false;
	}

	String getCode() const
	{
		if (loadFunction)
			return loadFunction();

		return "";
	}

	GlobalScope& getGlobalScope() { return memory; }
	const GlobalScope& getGlobalScope() const { return memory; }

	Result getLastResult() const { return lastResult; }
	JitObject getLastJitObject() const { return lastObject; }
	String getLastAssembly() const { return lastAssembly; }

	JitCompiledNode::Ptr getCompiledNode() { return compiledNode; }

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	File getConnectedFile() const { return connectedFile; }

	void setNumChannels(int newNumChannels)
	{
		if (numChannels != newNumChannels)
		{
			numChannels = newNumChannels;
			triggerRecompile();
		}
	}

	void setPreprocessFunction(const PreprocessFunction& f)
	{
		preprocessFunction = f;
	}

private:

	int numChannels = 2;

	WeakReference<Holder> holder;

	CompileProcedure compileProcedure;

	File connectedFile;

	String lastAssembly;
	Result lastResult;
	JitObject lastObject;

	JitCompiledNode::Ptr compiledNode;

	Array<WeakReference<Listener>> listeners;
	PreprocessFunction preprocessFunction;
	LoadFunction loadFunction;
	SaveFunction saveFunction;
	CompileManager compileManager;

	GlobalScope memory;

	friend class WorkbenchComponent;
	friend class WorkbenchManager;

	JUCE_DECLARE_WEAK_REFERENCEABLE(WorkbenchData);
};


struct WorkbenchComponent : public Component,
	public WorkbenchData::Listener
{
	WorkbenchComponent(WorkbenchData* d, bool isOwner = false) :
		workbench(d)
	{
		if (isOwner)
			strongPtr = d;
	}

	~WorkbenchComponent()
	{
		if (workbench != nullptr)
		{
			// You have to remove it in your subclass destructor!
			jassert(!workbench->listeners.contains(this));
		}
	}

	WorkbenchData* getWorkbench()
	{
		jassert(workbench != nullptr);
		return workbench.get();
	}

	const WorkbenchData* getWorkbench() const
	{
		jassert(workbench != nullptr);
		return workbench.get();
	}

	GlobalScope& getGlobalScope()
	{
		return getWorkbench()->getGlobalScope();;
	}

	virtual int getFixedHeight() const { return -1; };

	const GlobalScope& getGlobalScope() const
	{
		return getWorkbench()->getGlobalScope();;
	}

private:

	WorkbenchData::Ptr strongPtr;
	WorkbenchData::WeakPtr workbench;

	JUCE_DECLARE_WEAK_REFERENCEABLE(WorkbenchComponent);
};

struct WorkbenchManager : public WorkbenchData::Listener
{
	using LogFunction = std::function<void(int, const String&)>;

	virtual ~WorkbenchManager() {};

	void recompiled(WorkbenchData::Ptr p) override
	{
		setCurrentWorkbench(p);
	}

	void logMessage(WorkbenchData::Ptr p, int level, const String& s) override
	{
		if (p.get() == currentWb.get() && logFunction)
			logFunction(level, s);
	};

	WorkbenchData::Ptr getWorkbenchDataForFile(const File& f)
	{
		for (auto w : data)
		{
			if (w->getConnectedFile() == f)
			{
				w->addListener(this);
				setCurrentWorkbench(w);
				return w;
			}
		}

		WorkbenchData::Ptr w = new WorkbenchData();
		w->setUseFileAsContentSource(f);
		w->addListener(this);
		setCurrentWorkbench(w);

		data.add(w);

		return w;
	}

	void workbenchChanged(WorkbenchData::Ptr oldWorkBench, WorkbenchData::Ptr newWorkbench) override;

	void setCurrentWorkbench(WorkbenchData::Ptr newWorkbench)
	{
		if (currentWb.get() != newWorkbench.get())
		{
			registerAllComponents(newWorkbench);

			for (auto l : registeredComponents)
				l->workbenchChanged(currentWb.get(), newWorkbench);
		}
	}

	void setLogFunction(const LogFunction& f)
	{
		logFunction = f;
	}

	void registerAllComponents(WorkbenchData::Ptr p)
	{
		for (int i = 0; i < registeredComponents.size(); i++)
		{
			if (registeredComponents[i] == nullptr)
				registeredComponents.remove(i--);
		}

		for (auto l : p->listeners)
		{
			if(l != nullptr)
				registeredComponents.addIfNotAlreadyThere(l);
		}
	}

	LogFunction logFunction;

	ReferenceCountedArray<WorkbenchData> data;

	WorkbenchData::WeakPtr currentWb;

	Array<WeakReference<WorkbenchData::Listener>> registeredComponents;

	JUCE_DECLARE_WEAK_REFERENCEABLE(WorkbenchManager);
};


}
}