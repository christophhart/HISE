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
	public ApiProviderBase::Holder
{
	static String getDefaultNodeTemplate(const Identifier& mainClass);

	static String getDefaultCode(bool getTestCode);
	using Ptr = ReferenceCountedObjectPtr<WorkbenchData>;
	using WeakPtr = WeakReference<WorkbenchData>;

	/** Formats the log event to print nicely on a console. */
	String convertToLogMessage(int level, const String& s);

	void blink(int line) override
	{
		pendingBlinks.insert(line);

		MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(WorkbenchData::handleBlinks));
	}

	void handleBlinks();

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

	ApiProviderBase* getProviderBase() override 
	{
		if(lastCompileResult.compiledOk())
			return &lastCompileResult.obj; 

		return nullptr;
	}

	void handleBreakpoints(const Identifier& codeFile, Graphics& g, Component* c) override
	{
		jassertfalse;

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

		/** This is called directly after the compilation. Be aware that any
		    test that is performed in postCompile() might not be executed.
		*/
		virtual void recompiled(WorkbenchData::Ptr wb) {};

		/** Override this and modify the code that will be passed to the compiler. */
		virtual bool preprocess(String& code) { return false; }

		/** This is called after the CompileHandler::postCompile callback so if
		    you rely on a test execution, use this callback instead. */
		virtual void postPostCompile(WorkbenchData::Ptr wb) {};

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

	/** Base class for all sub items (compile handlers, code providers, etc. */
	struct SubItemBase
	{
		virtual ~SubItemBase() {};

		WorkbenchData* getParent() { return parent.get(); }
		const WorkbenchData* getParent() const { return parent.get(); }

		

	protected:
		SubItemBase(WorkbenchData* d) :
			parent(d)
		{};

	private:

		friend class WorkbenchData;

		WorkbenchData::WeakPtr parent;
	};

	struct CompileResult
	{
		CompileResult() :
			compileResult(Result::ok()),
			testResult(Result::ok())
		{};

		bool compiledOk() const
		{
			return compileResult.wasOk();
		}

		bool testWasOk() const
		{
			return testResult.wasOk();
		}

		bool allOk() const
		{
			return compiledOk() && testWasOk();
		}

		Result compileResult;
		Result testResult;
		String assembly;
		JitObject obj;
	};

	struct CompileHandler : public SubItemBase
	{
		CompileHandler(WorkbenchData* d) :
			SubItemBase(d)
		{};

		virtual ~CompileHandler() {};

		/** This causes a synchronous compilation.

			You can override this method to postpone the compilation
			to a background thread. If you do so, you will need to call

			@returns true if the compilation was executed synchronously
			and false if it was deferred.
		*/
		virtual bool triggerCompilation()
		{
			return getParent()->handleCompilation();
		}

		/** Compiles the given code and returns a default JIT object. 
		
			Override this for custom compilation tasks. This function will
			be called synchronously or on a background thread depending on the
			implementation of triggerCompilation().
		*/
		virtual CompileResult compile(const String& codeToCompile)
		{
			ScopedPointer<Compiler> cc = createCompiler();

			CompileResult r;
			r.obj = cc->compileJitObject(codeToCompile);
			r.assembly = cc->getAssemblyCode();
			r.compileResult = cc->getCompileResult();

			return r;
		}

		/** Override this method if you want to perform synchronous
		    tasks directly after a successful compilation. 
			
			You can use it to run the code and allow breakpoints.
			
		*/
		virtual void postCompile(CompileResult& lastResult) {};

		virtual Compiler::Ptr createCompiler()
		{
			auto p = getParent();
			Compiler::Ptr cc = new Compiler(p->getGlobalScope());

			SnexObjectDatabase::registerObjects(*cc, p->numChannels);
			cc->setDebugHandler(p);
			return cc;
		}

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(CompileHandler);
	};

	/** A code provider will be used to fetch and store the data
	
	*/
	struct CodeProvider: public SubItemBase
	{
		CodeProvider(WorkbenchData* d) :
			SubItemBase(d)
		{};

		virtual ~CodeProvider() {};

		virtual String loadCode() const = 0;

		/** You can override this method and supply a custom preprocessing
			which will not be return by loadCode() (and thus not be saved).
		*/
		virtual bool preprocess(String& code) 
		{
			ignoreUnused(code);
			return false; 
		}

		virtual bool saveCode(const String& s) = 0;

		/** Override this method and return the instance id. This will be used to find the main class in nodes. */
		virtual Identifier getInstanceId() const = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(CodeProvider);
	};

	struct DefaultCodeProvider: public CodeProvider
	{
		DefaultCodeProvider(WorkbenchData* parent, const File& f_):
			CodeProvider(parent),
			f(f_)
		{
			
		}

		String loadCode() const override
		{
			auto parent = getParent();

			if (parent != nullptr && f.existsAsFile())
			{
				auto code = f.loadFileAsString();

				if (code.isEmpty())
				{
					code = WorkbenchData::getDefaultNodeTemplate(getInstanceId());
					f.replaceWithText(code);
				}

				return code;
			}
			
			return {};
		}

		bool saveCode(const String& s) override
		{
			auto parent = getParent();
			if (parent != nullptr && f.existsAsFile())
				return f.replaceWithText(s);
		}

		Identifier getInstanceId() const override
		{
			if (f.existsAsFile())
				return Identifier(f.getFileNameWithoutExtension());

			return {};
		}

		File f;
	};


	struct ValueBasedCodeProvider : public CodeProvider
	{
		ValueBasedCodeProvider(snex::ui::WorkbenchData* d, Value& v, const Identifier& id_) :
			CodeProvider(d),
			id(id_)
		{
			codeValue.referTo(v);
		}

		String loadCode() const override
		{
			auto c = codeValue.toString();

			if (c.isEmpty())
				c = defaultCode;

			return c;
		}

		bool saveCode(const String& s) override
		{
			codeValue.setValue(s);
			return true;
		}

		Identifier getInstanceId() const override { return id; }

		Identifier id;
		Value codeValue;
		const String defaultCode;
	};

	WorkbenchData() :
		memory(1024)
	{
		memory.addDebugHandler(this);
	};

	~WorkbenchData()
	{
		compileHandler = nullptr;
	}

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

		if (compileHandler != nullptr)
			compileHandler->triggerCompilation();
		else
			handleCompilation();
	}

	void triggerPostCompileActions()
	{
		compileHandler->postCompile(lastCompileResult);
		postPostCompile();
	}

	void postPostCompile()
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->postPostCompile(this);
		}
	}

	void postCompile()
	{
		if (getLastResult().compiledOk())
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

	bool handleCompilation();

	void setUseFileAsContentSource(const File& f)
	{
		codeProvider = new DefaultCodeProvider(this, f);
	}

	bool setCode(const String& s, NotificationType recompileOnOk)
	{
		if (codeProvider != nullptr)
		{
			auto ok = codeProvider->saveCode(s);

			if (ok && recompileOnOk != dontSendNotification)
				triggerRecompile();

			return ok;
		}

		return false;
	}

	String getCode() const
	{
		if (codeProvider != nullptr)
			return codeProvider->loadCode();

		return "";
	}

	GlobalScope& getGlobalScope() { return memory; }
	const GlobalScope& getGlobalScope() const { return memory; }

	CompileResult getLastResult() const { return lastCompileResult; }
	JitObject getLastJitObject() const { return lastCompileResult.obj; }
	String getLastAssembly() const { return lastCompileResult.assembly; }

	Identifier getInstanceId() const { return codeProvider->getInstanceId(); }

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void setNumChannels(int newNumChannels)
	{
		if (numChannels != newNumChannels)
		{
			numChannels = newNumChannels;
			triggerRecompile();
		}
	}

	void setCodeProvider(CodeProvider* newCodeProvider, NotificationType recompile=dontSendNotification)
	{
		codeProvider = newCodeProvider;
		codeProvider->parent = this;

		if (recompile != dontSendNotification)
			triggerRecompile();
	}

	void setCompileHandler(CompileHandler* newCompileHandler, NotificationType recompile = dontSendNotification)
	{
		compileHandler = newCompileHandler;

		if (recompile != dontSendNotification)
			triggerRecompile();
	}

	bool operator==(CodeProvider* p) const
	{
		return getInstanceId() == p->getInstanceId();
	}

	CompileHandler* getCompileHandler() { return compileHandler; }

private:

	hise::UnorderedStack<int> pendingBlinks;

	GlobalScope memory;
	int numChannels = 2;

	WeakReference<Holder> holder;

	WeakReference<CodeProvider> codeProvider;
	ScopedPointer<CompileHandler> compileHandler;
	CompileResult lastCompileResult;

	Array<WeakReference<Listener>> listeners;

	friend class WorkbenchComponent;
	friend class WorkbenchManager;

	JUCE_DECLARE_WEAK_REFERENCEABLE(WorkbenchData);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkbenchData);
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

struct ValueTreeCodeProvider : public snex::ui::WorkbenchData::CodeProvider,
	public Timer
{
	ValueTreeCodeProvider(snex::ui::WorkbenchData* data) :
		CodeProvider(data)
	{
		timerCallback();
		startTimer(1000);
	}
	void timerCallback() override;

	bool saveCode(const String& s) override
	{
		customCode = s;
		getParent()->triggerRecompile();
		return false;
	}

	Identifier getInstanceId() const override;

	String loadCode() const override
	{
		if (customCode.isNotEmpty())
			return customCode;

		rebuild();
		return lastResult.code;
	}

	void rebuild() const;

	mutable cppgen::ValueTreeBuilder::BuildResult lastResult;

	ValueTree lastTree;
	String customCode;
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

	WorkbenchData::Ptr getWorkbenchDataForCodeProvider(WorkbenchData::CodeProvider* p, bool ownCodeProvider)
	{
		ScopedPointer<WorkbenchData::CodeProvider> owned = p;

		for (auto w : data)
		{
			if (*w == p)
			{
				w->addListener(this);
				setCurrentWorkbench(w);

				if (!ownCodeProvider)
					owned.release();
				
				return w;
			}
		}

		WorkbenchData::Ptr w = new WorkbenchData();

		w->setCodeProvider(p, dontSendNotification);
		w->addListener(this);
		setCurrentWorkbench(w);

		if(ownCodeProvider)
			codeProviders.add(owned.release());

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

	OwnedArray<WorkbenchData::CodeProvider> codeProviders;

	JUCE_DECLARE_WEAK_REFERENCEABLE(WorkbenchManager);
};


}
}