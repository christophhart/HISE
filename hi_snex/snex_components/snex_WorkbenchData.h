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

	void handleHiseEvent(HiseEvent& e)
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

class WorkbenchData : public ReferenceCountedObject,
	public DebugHandler,
	public ApiProviderBase::Holder
{
public:

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

	void setEnabled(bool ) override {};

	void logMessage(int level, const juce::String& s) override
	{
		if (!getGlobalScope().isDebugModeEnabled() && level > 1)
			return;

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

    ApiProviderBase* getProviderBase() override
    {
        return &getLastResultReference();
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

		virtual void debugModeChanged(bool isEnabled) {};

		virtual void logMessage(WorkbenchData::Ptr wb, int level, const String& s)
		{
			ignoreUnused(wb, level, s);
		};

		

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

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubItemBase);
	};

	struct CompileResult: public ApiProviderBase
	{
		enum class DataTypes
		{
			Integer,
			Float,
			Double,
			Struct,
			Span,
			Dyn
		};

		struct DataEntry : public DebugInformationBase
		{
			DataEntry(TypeInfo t, void* dataPtr, const Identifier& id_):
				type(t),
				data(dataPtr),
				id(id_)
			{

			}

			virtual String getTextForName() const
			{
				return id.toString();
			}

			String getTextForType() const override { return type.toString(); }

			String getTextForDataType() const override
			{
				return type.toString();
			}

			int getType() const override;;

			String getTextForValue() const override;

			String getCodeToInsert() const { return "unused"; }

			int getNumChildElements() const override;

			DebugInformationBase::Ptr getChildElement(int index) override;

			TypeInfo type;
			void* data;
			Identifier id;
		};

		CompileResult() :
			compileResult(Result::ok())
		{};

		bool compiledOk() const
		{
			return compileResult.wasOk();
		}

		void rightClickCallback(const MouseEvent& e, Component* componentToNotify)
		{

		}

		void setDataPtrForDebugging(void* d)
		{
			dataPtr = d;
			
		}

		void getColourAndLetterForType(int type, Colour& colour, char& letter) override
		{
			switch ((DataTypes)type)
			{
			case DataTypes::Integer: 
				colour = Types::Helpers::getColourForType(Types::ID::Integer);
				letter = 'I';
				break;
			case DataTypes::Float:
				colour = Types::Helpers::getColourForType(Types::ID::Float);
				letter = 'F';
				break;
			case DataTypes::Double:
				colour = Types::Helpers::getColourForType(Types::ID::Double);
				letter = 'D';
				break;
			case DataTypes::Span:
				colour = Types::Helpers::getColourForType(Types::ID::Block);
				letter = 'S';
				break;
			case DataTypes::Dyn:
				colour = Types::Helpers::getColourForType(Types::ID::Block);
				letter = 'D';
				break;
			case DataTypes::Struct:
				colour = Types::Helpers::getColourForType(Types::ID::Block);
				letter = 'C';
				break;
			}
		}

		int getNumDebugObjects() const override;

		DebugInformationBase::Ptr getDebugInformation(int index);

		Result compileResult;
		String assembly;
		JitObject obj;
        
        ComplexType::Ptr mainClassPtr;

		scriptnode::ParameterDataList parameters;
		JitCompiledNode::Ptr lastNode;

		void* dataPtr = nullptr;
	};

	struct TestRunnerBase
	{
		struct ParameterEvent
		{
			ParameterEvent() {};

			ParameterEvent(int timeStamp_, int parameterIndex_, double v) :
				timeStamp(timeStamp_),
				parameterIndex(parameterIndex_),
				valueToUse(v)
			{};

			bool operator<(const ParameterEvent& other) const
			{
				return timeStamp < other.timeStamp;
			}

			ParameterEvent(const var& obj) :
				timeStamp(obj.getProperty("Timestamp", 0)),
				parameterIndex(obj.getProperty("Index", 0)),
				valueToUse(obj.getProperty("Value", 0.0))
			{

			}

			var toJson() const
			{
				auto obj = new DynamicObject();

				obj->setProperty("Index", parameterIndex);
				obj->setProperty("Value", valueToUse);
				obj->setProperty("Timestamp", timeStamp);

				return var(obj);
			}

			int timeStamp = 0;
			int parameterIndex = 0;
			double valueToUse = 0.0;
		};

		virtual ~TestRunnerBase() {};

		virtual Result runTest(ui::WorkbenchData::CompileResult& lastResult) { return Result::ok(); };

		/** Override this function and call the parameter method. */
		virtual void processTestParameterEvent(int parameterIndex, double value) = 0;

		virtual Result prepareTest(PrepareSpecs ps, const Array<ParameterEvent>& initialParameters) = 0;

		virtual void processTest(ProcessDataDyn& data) = 0;

		virtual bool shouldProcessEventsManually() const { return false; }

		virtual void processHiseEvent(HiseEvent& e) {};

		virtual void initExternalData(ExternalDataHolder* h)
		{
			jassertfalse;
		}

		virtual bool triggerTest(ui::WorkbenchData::CompileResult& lastResult)
		{
			runTest(lastResult);
			return true;
		}

		JUCE_DECLARE_WEAK_REFERENCEABLE(TestRunnerBase);
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
		{
			rebuildTestSignal(dontSendNotification);
		};

		struct HiseEventEvent
		{
			HiseEventEvent(HiseEvent& e_):
				e(e_)
			{};

			HiseEvent e;
		};

		using ParameterEvent = TestRunnerBase::ParameterEvent;

		bool testWasOk() const
		{
			return testResult.wasOk();
		}

		void saveCurrentTestOutput();

		bool saveToFile()
		{
			if (currentTestFile.existsAsFile())
			{
				return currentTestFile.replaceWithText(JSON::toString(toJSON()));
			}

			return false;
		}

		bool loadFromFile(const File& fileToLoad)
		{
			currentTestFile = fileToLoad;
			auto json = JSON::parse(fileToLoad);

			if (json.isObject())
				return fromJSON(json);

			return false;
		}

		var toJSON() const;

		bool fromJSON(const var& jsonData, NotificationType runTest=sendNotificationSync);

		bool shouldRunTest() const
		{
			return testSourceData.getNumSamples() > 0;
		}

		int getNumTestEvents(bool getParameterAmount) const
		{
			return getParameterAmount ? parameterEvents.size() : hiseEvents.getNumUsed();
		}

		TestRunnerBase* getCustomTest() { return customTester.get(); }

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
		
		void processInChunks(const std::function<void()>& f);

		Result processTestData(WorkbenchData::Ptr data);

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
		AudioSampleBuffer testReferenceData;
		
		File currentTestFile;
		File testInputFile;
		File testOutputFile;

		double cpuUsage = 0.0;

		TestSignalMode currentTestSignalType = TestSignalMode::Ramp;
		int testSignalLength = 4096;

		String getErrorMessage() const
		{
			return testResult.getErrorMessage();
		}

		void initProcessing(PrepareSpecs specsToUse)
		{
            ps = specsToUse;
			ps.voiceIndex = parent.getGlobalScope().getPolyHandler();
            
            int numChannelsTestSignal = testSourceData.getNumChannels();
            
            if(numChannelsTestSignal != ps.numChannels)
                rebuildTestSignal(dontSendNotification);
            
            ps.blockSize = jmin(ps.blockSize, testSourceData.getNumSamples());
		}

		PrepareSpecs getPrepareSpecs() { return ps; }

		Table* getTable(int index) override
		{
			if (isPositiveAndBelow(index, tables.size()))
			{
				return tables[index].get();
			}

			tables.add(new SampleLookupTable());

			sendMessageToListeners(true);

			return tables.getLast().get();
		}

		SliderPackData* getSliderPack(int index) override
		{
			if (isPositiveAndBelow(index, sliderPacks.size()))
			{
				return sliderPacks[index].get();
			}

			sliderPacks.add(new SliderPackData(nullptr, updater));
			sliderPacks.getLast()->setNumSliders(16);

			sendMessageToListeners(true);

			return sliderPacks.getLast().get();
		}

		MultiChannelAudioBuffer* getAudioFile(int index) override
		{
			if (isPositiveAndBelow(index, buffers.size()))
			{
				return buffers[index].get();
			}

			auto b = new MultiChannelAudioBuffer();
			b->setProvider(dataProvider);
			buffers.add(b);

			sendMessageToListeners(true);

			return buffers.getLast().get();
		}

		FilterDataObject* getFilterData(int index) override
		{
			if (isPositiveAndBelow(index, filterData.size()))
				return filterData[index].get();

			auto fd = new FilterDataObject();
			filterData.add(fd);
			sendMessageToListeners(true);
			return filterData.getLast().get();
		}

		SimpleRingBuffer* getDisplayBuffer(int index) override
		{
			if (isPositiveAndBelow(index, displayBuffers.size()))
				return displayBuffers[index].get();

			auto fd = new SimpleRingBuffer();
			displayBuffers.add(fd);
			sendMessageToListeners(true);
			return displayBuffers.getLast().get();
		}


		int getNumDataObjects(ExternalData::DataType t) const override
		{
			if (t == ExternalData::DataType::SliderPack)
				return sliderPacks.size();
			if (t == ExternalData::DataType::Table)
				return tables.size();
			if (t == ExternalData::DataType::AudioFile)
				return buffers.size();
			if (t == ExternalData::DataType::FilterCoefficients)
				return filterData.size();
			if (t == ExternalData::DataType::DisplayBuffer)
				return displayBuffers.size();

			return 0;
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

            return false;
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

		void setMultichannelDataProvider(MultiChannelAudioBuffer::DataProvider* p)
		{
			dataProvider = p;
		}

		void setCustomTest(TestRunnerBase* newCustomTester)
		{
			customTester = newCustomTester;
		}

	private:
		
		WeakReference<TestRunnerBase> customTester;

		MultiChannelAudioBuffer::DataProvider::Ptr dataProvider;

		int getHiseEventInSampleRange(Range<int> r, int lastIndex, HiseEvent& e)
		{
			int before = lastIndex;

			lastIndex = jmax(0, lastIndex);

			for (int i = lastIndex; i < hiseEvents.getNumUsed(); i++)
			{
				if (r.contains(hiseEvents.getEvent(i).getTimeStamp()) && before != i)
				{
					e = hiseEvents.getEvent(i);
					return i;
				}
			}

			return -1;
		}

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
		ReferenceCountedArray<FilterDataObject> filterData;
		ReferenceCountedArray<SimpleRingBuffer> displayBuffers;

		File testRootDirectory;

		JUCE_DECLARE_NON_COPYABLE(TestData);
	};

	

	

	struct CompileHandler : public SubItemBase,
						    public TestRunnerBase
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
            Compiler::Ptr cc = createCompiler();

			CompileResult r;
			r.obj = cc->compileJitObject(codeToCompile);
			r.assembly = cc->getAssemblyCode();
			r.compileResult = cc->getCompileResult();

			NamespacedIdentifier mainObjectId(getParent()->getInstanceId());

			int voiceAmount = 1;

			if (auto ph = getParent()->getGlobalScope().getPolyHandler())
				if (ph->isEnabled())
					voiceAmount = NUM_POLYPHONIC_VOICES;
			
			TemplateParameter tp(voiceAmount);

			r.mainClassPtr = cc->getComplexType(mainObjectId, {tp}, true);

			return r;
		}

		

		


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

		virtual bool providesCode() const { return false; }

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
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CodeProvider);
	};

	struct DefaultCodeProvider: public CodeProvider
	{
		DefaultCodeProvider(WorkbenchData* parent, const File& f_):
			CodeProvider(parent),
			f(f_)
		{
			
		}

		std::function<String(const Identifier&)> defaultFunction = WorkbenchData::getDefaultNodeTemplate;

		String loadCode() const override
		{
			auto parent = getParent();

			if (parent != nullptr && f.existsAsFile())
			{
				auto code = f.loadFileAsString();

				if (code.isEmpty())
				{
					code = defaultFunction(getInstanceId());
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

			return false;
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

		for (auto o : OptimizationIds::Helpers::getDefaultIds())
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
		if (compileHandler == nullptr)
			return;

		compileHandler->postCompile(lastCompileResult);

		callAsyncWithSafeCheck([](WorkbenchData* d)
		{
			d->postPostCompile();
		}, true);
	}

	void postPostCompile()
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->postPostCompile(this);
		}
	}

    
    
	void callAsyncWithSafeCheck(const std::function<void(WorkbenchData* d)>& f, bool callSyncIfMessageThread=false);

	void postCompile()
	{
		if (getLastResult().compiledOk())
		{
            
            
			//gegetLastResult().rebuildDebugInformation();
			rebuild();
		}

		getGlobalScope().sendRecompileMessage();

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

	CompileResult& getLastResultReference() { return lastCompileResult; }

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

	int getNumChannels() const { return numChannels; }

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

	void setCompileHandler(CompileHandler* ownedNewCompileHandler, NotificationType recompile = dontSendNotification)
	{
		compileHandler = ownedNewCompileHandler;

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

	void setDebugMode(bool shouldBeEnabled, NotificationType n)
	{
		if (shouldBeEnabled != memory.isDebugModeEnabled())
		{
			memory.setDebugMode(shouldBeEnabled);

			if (n != dontSendNotification)
			{
				for (auto l : listeners)
				{
					if (l.get() != nullptr)
						l->debugModeChanged(shouldBeEnabled);
				}
			}
		}
	}

	void setIsCppPreview(bool shouldBeCppPreview)
	{
		cppPreview = shouldBeCppPreview;
	}

	bool isCppPreview() const
	{
		return cppPreview;
	}

private:

	bool cppPreview = false;

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




class WorkbenchComponent : public Component,
	public WorkbenchData::Listener
{
public:

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
	ValueTreeCodeProvider(snex::ui::WorkbenchData* data, int numChannelsToCompile) :
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
    int numChannelsToCompile;
};

class WorkbenchManager final: public AsyncUpdater
{
public:

	using LogFunction = std::function<void(int, const String&)>;

	~WorkbenchManager() {};

	struct WorkbenchChangeListener
	{
		virtual ~WorkbenchChangeListener() {};

		virtual void workbenchChanged(WorkbenchData::Ptr newWorkbench) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(WorkbenchChangeListener);
	};

	void addListener(WorkbenchChangeListener* l)
	{
		listeners.addIfNotAlreadyThere(l);

		l->workbenchChanged(currentWb);
	}

	void removeListener(WorkbenchChangeListener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	WorkbenchData::Ptr getWorkbenchDataForCodeProvider(WorkbenchData::CodeProvider* p, bool ownCodeProvider);

	void resetToRoot()
	{
		setCurrentWorkbench(rootWb, false);
	}

	void setCurrentWorkbench(WorkbenchData::Ptr newWorkbench, bool setAsRoot);

	WorkbenchData::Ptr getCurrentWorkbench() { return currentWb; }
	
	WorkbenchData::Ptr getRootWorkbench() { return rootWb; }

	void removeWorkbench(WorkbenchData::Ptr p)
	{
		data.removeObject(p);

		if (currentWb == p || p == rootWb)
			setCurrentWorkbench(nullptr, p == rootWb);
	}

private:

	void handleAsyncUpdate() override;

	ReferenceCountedArray<WorkbenchData> data;
	WorkbenchData::Ptr currentWb;
	WorkbenchData::Ptr rootWb;

	LogFunction logFunction;

	Array<WeakReference<WorkbenchChangeListener>> listeners;

	OwnedArray<WorkbenchData::CodeProvider> codeProviders;

	JUCE_DECLARE_WEAK_REFERENCEABLE(WorkbenchManager);
};


}
}
