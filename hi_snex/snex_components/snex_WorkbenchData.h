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

	
	struct TestData: public AsyncUpdater,
					 public ExternalDataHolder
	{
		enum class TestSignalMode
		{
			Empty,
			DC1,
			Ramp,
			FastRamp,
			OneKhzSine,
			OneKhzSaw,
			Impulse,
			SineSweep,
			Noise,
			CustomFile,
			numTestSignals
		};

		static StringArray getSignalTypeList()
		{
			return { "Empty", "0dB Static", "Ramp", "FastRamp", "1kHz Sine", "1kHz Saw", "Impulse", "Sine Sweep", "Noise", "Custom" };
		}

		struct TestListener
		{
			virtual ~TestListener() {};

			virtual void testSignalChanged() {};

			virtual void testEventsChanged() {};

			JUCE_DECLARE_WEAK_REFERENCEABLE(TestListener);
		};

		TestData(WorkbenchData& p) :
			testResult(Result::ok()),
			parent(p)
		{};

		struct HiseEventEvent
		{
			HiseEventEvent(HiseEvent& e_):
				e(e_)
			{};

			HiseEvent e;
		};

		struct ParameterEvent
		{
			ParameterEvent() {};

			ParameterEvent(int timeStamp_, int parameterIndex_, double v):
				timeStamp(timeStamp_),
				parameterIndex(parameterIndex_),
				valueToUse(v)
			{};

			bool operator<(const ParameterEvent& other) const
			{
				return timeStamp < other.timeStamp;
			}

			ParameterEvent(const var& obj):
				timeStamp(obj.getProperty("Timestamp", 0)),
				parameterIndex(obj.getProperty("Index", 0)),
				valueToUse(obj.getProperty("Value", 0.0))
			{
				
			}

			var toJson() const
			{
				DynamicObject::Ptr obj = new DynamicObject();
				
				obj->setProperty("Index", parameterIndex);
				obj->setProperty("Value", valueToUse);
				obj->setProperty("Timestamp", timeStamp);

				return var(obj);
			}

			int timeStamp = 0;
			int parameterIndex = 0;
			double valueToUse = 0.0;
		};

		bool testWasOk() const
		{
			return testResult.wasOk();
		}

		var toJSON() const;

		bool fromJSON(const var& jsonData);

		bool shouldRunTest() const
		{
			return testSourceData.getNumSamples() > 0;
		}

		int getNumTestEvents(bool getParameterAmount) const
		{
			return getParameterAmount ? parameterEvents.size() : hiseEvents.getNumUsed();
		}

		void addTestEvent(const HiseEvent& e)
		{
			hiseEvents.addEvent(e);
			sendMessageToListeners(true);

			parent.triggerPostCompileActions();
		}

		HiseEvent getTestHiseEvent(int index) const { return hiseEvents.getEvent(index); }
		ParameterEvent getParameterEvent(int index) const { return parameterEvents[index]; }

		void addTestEvent(const ParameterEvent& p)
		{
			parameterEvents.addUsingDefaultSort(p);
			sendMessageToListeners(true);

			parent.triggerPostCompileActions();
		}

		void removeTestEvent(int index, bool isParameter, NotificationType notifyListeners=sendNotification)
		{
			if (isParameter)
				parameterEvents.remove(index);
			else
				hiseEvents.popEvent(index);

			if (notifyListeners != dontSendNotification)
			{
				sendMessageToListeners(true);
				parent.triggerPostCompileActions();
			}
		}

		void handleAsyncUpdate()
		{
			for (auto l : listeners)
			{
				if (l != nullptr)
				{
					if (sendEventMessage)
						l->testEventsChanged();
					if(sendSignalMessage)
						l->testSignalChanged();
				}
			}

			sendEventMessage = false;
			sendSignalMessage = false;
		}

		void sendMessageToListeners(bool eventMessage)
		{
			if (eventMessage)
				sendEventMessage = true;
			else
				sendSignalMessage = true;

			triggerAsyncUpdate();
		}

		void rebuildTestSignal(NotificationType triggerTest=sendNotification);

		void addListener(TestListener* l)
		{
			listeners.addIfNotAlreadyThere(l);
		}

		void removeListener(TestListener* l)
		{
			listeners.removeAllInstancesOf(l);
		}
		
		void processTestData(WorkbenchData::Ptr data);

		void clear(NotificationType notify = dontSendNotification)
		{
			testSourceData.setSize(0, 0);
			hiseEvents.clear();
			parameterEvents.clear();

			clearAllDataObjects();

			if (notify != dontSendNotification)
			{
				parent.triggerPostCompileActions();
				sendMessageToListeners(true);
				sendMessageToListeners(false);
			}
		}

		WorkbenchData& parent;

		AudioSampleBuffer testSourceData;
		AudioSampleBuffer testOutputData;
		

		File testInputFile;
		File testOutputFile;

		double cpuUsage = 0.0;

		TestSignalMode currentTestSignalType;
		int testSignalLength = 1024;

		String getErrorMessage() const
		{
			return testResult.getErrorMessage();
		}

		void initProcessing(int blockSize, double sampleRate)
		{
			ps.numChannels = 2;
			ps.blockSize = jmin(blockSize, testSourceData.getNumSamples());
			ps.sampleRate = sampleRate;
			ps.voiceIndex = parent.getGlobalScope().getPolyHandler();
		}

		PrepareSpecs getPrepareSpecs() { return ps; }

		

		Table* getTable(int index) override
		{
			if (isPositiveAndBelow(index, tables.size()))
			{
				return tables[index];
			}

			tables.add(new SampleLookupTable());

			sendMessageToListeners(true);

			return tables.getLast();
		}

		SliderPackData* getSliderPack(int index) override
		{
			if (isPositiveAndBelow(index, sliderPacks.size()))
			{
				return sliderPacks[index];
			}

			sliderPacks.add(new SliderPackData(nullptr, updater));
			sliderPacks.getLast()->setNumSliders(16);

			sendMessageToListeners(true);

			return sliderPacks.getLast();
		}

		MultiChannelAudioBuffer* getAudioFile(int index) override
		{
			if (isPositiveAndBelow(index, buffers.size()))
			{
				return buffers[index];
			}

			buffers.add(new MultiChannelAudioBuffer(updater, AudioSampleBuffer()));

			sendMessageToListeners(true);

			return buffers.getLast();
		}

		int getNumDataObjects(ExternalData::DataType t) const override
		{
			if (t == ExternalData::DataType::SliderPack)
				return sliderPacks.size();
			if (t == ExternalData::DataType::Table)
				return tables.size();
			if (t == ExternalData::DataType::AudioFile)
				return buffers.size();
		}

		bool removeDataObject(ExternalData::DataType t, int index) override
		{
			if (t == ExternalData::DataType::Table)
			{
				if (isPositiveAndBelow(index, tables.size()))
				{
					tables.remove(index);
					return true;
				}

				return false;
			}

			if (t == ExternalData::DataType::SliderPack)
			{
				if (isPositiveAndBelow(index, sliderPacks.size()))
				{
					sliderPacks.remove(index);
					return true;
				}

				return false;
			}

			if (t == ExternalData::DataType::AudioFile)
			{
				if (isPositiveAndBelow(index, buffers.size()))
				{
					buffers.remove(index);
					return true;
				}

				return false;
			}
		}

		void setUpdater(PooledUIUpdater* nonOwnedUpdater)
		{
			updater = nonOwnedUpdater;
		}

		File getTestRootDirectory() const 
		{
			jassert(testRootDirectory.isDirectory());
			return testRootDirectory; 
		}

		void setTestRootDirectory(const File& newRootDir)
		{
			jassert(newRootDir.isDirectory());
			testRootDirectory = newRootDir;
		}

	private:
		
		int getParameterInSampleRange(Range<int> r, int lastIndex, ParameterEvent& pToFill) const
		{
			lastIndex = jmax(0, lastIndex);

			for (int i = lastIndex; i < parameterEvents.size(); i++)
			{
				if (r.contains(parameterEvents[i].timeStamp))
				{
					pToFill = parameterEvents[i];
					return i;
				}
			}

			return -1;
		}

		PrepareSpecs ps;

		bool sendEventMessage = false;
		bool sendSignalMessage = false;

		Result testResult;

		HiseEventBuffer hiseEvents;
		Array<ParameterEvent> parameterEvents;

		Array<WeakReference<TestListener>> listeners;

		PooledUIUpdater* updater = nullptr;

		ReferenceCountedArray<Table> tables;
		ReferenceCountedArray<SliderPackData> sliderPacks;
		ReferenceCountedArray<MultiChannelAudioBuffer> buffers;

		File testRootDirectory;

		JUCE_DECLARE_NON_COPYABLE(TestData);
	};

	struct CompileResult
	{
		/** This data is used by the ParameterList component to change the parameters. */
		struct DynamicParameterData
		{
			using List = Array<DynamicParameterData>;

			cppgen::ParameterEncoder::Item data;
			std::function<void(double)> f;
		};

		CompileResult() :
			compileResult(Result::ok())
		{};

		bool compiledOk() const
		{
			return compileResult.wasOk();
		}

		Result compileResult;
		String assembly;
		JitObject obj;
		DynamicParameterData::List parameters;
		JitCompiledNode::Ptr lastNode;
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

		/** Override this function and call the parameter method. */
		virtual void processTestParameterEvent(int parameterIndex, double value) = 0;

		virtual void prepareTest(PrepareSpecs ps) = 0;

		virtual void processTest(ProcessDataDyn& data) = 0;


		virtual void postCompile(ui::WorkbenchData::CompileResult& lastResult)
		{
			
		}

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
		memory(),
		currentTestData(*this)
	{
		memory.addDebugHandler(this);

		for (auto o : OptimizationIds::getAllIds())
			memory.addOptimization(o);
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

	CodeProvider* getCodeProvider() const
	{
		return codeProvider;
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

	TestData& getTestData() { return currentTestData; }
	const TestData& getTestData() const { return currentTestData; }

private:

	hise::UnorderedStack<int> pendingBlinks;

	GlobalScope memory;
	int numChannels = 2;

	WeakReference<Holder> holder;

	WeakReference<CodeProvider> codeProvider;
	ScopedPointer<CompileHandler> compileHandler;
	TestData currentTestData;
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