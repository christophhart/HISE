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

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;
using namespace snex;
using namespace snex::ui;

class SnexSource : public WorkbenchData::Listener,
					public SimpleRingBuffer::WriterBase
{
public:

	using SnexTestBase = snex::ui::WorkbenchData::TestRunnerBase;

	struct SnexTestBaseHelper
	{
	    static void *getNodeWorkbench(NodeBase* node);
	};


    enum class ErrorLevel
    {
        Uncompiled,
        CompileFail,
        Warning,
        OK,
        numErrorLevel
    };
    
	struct SnexSourceListener
	{
		virtual ~SnexSourceListener() {};
		virtual void wasCompiled(bool ok) {};
		virtual void complexDataAdded(snex::ExternalData::DataType t, int index) {};
		virtual void parameterChanged(int snexParameterId, double newValue) {};

		virtual void complexDataTypeChanged() {};

		JUCE_DECLARE_WEAK_REFERENCEABLE(SnexSourceListener);
	};

	struct SnexParameter : public NodeBase::Parameter
	{
		SnexParameter(SnexSource* n, NodeBase* parent, ValueTree dataTree);
		parameter::dynamic p;
		const int pIndex;
	
		static ValueTree getTreeInNetwork(NodeBase* parent, ValueTree dataTree);

		void sendValueChangeToParentListeners(Identifier id, var newValue);

		valuetree::PropertySyncer syncer;
		valuetree::PropertyListener parentValueUpdater;
		WeakReference<SnexSource> snexSource;
		ValueTree treeInCodeMetadata;
	};

	

	struct HandlerBase
	{
		using ObjectStorageType = ObjectStorage<OpaqueNode::SmallObjectSize, 16>;

		HandlerBase(SnexSource& s, ObjectStorageType& obj_) :
			parent(s),
			obj(obj_)
		{};

		virtual void reset() = 0;
		virtual Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) = 0;
		virtual ~HandlerBase() {};

		snex::jit::FunctionData getFunctionAsObjectCallback(const String& id, bool checkProcessFunctions=true);

		void addObjectPtrToFunction(FunctionData& f);;

		SimpleReadWriteLock& getAccessLock() { return lock; }

	protected:

		NodeBase* getNode() { return parent.parentNode; }
		SnexSource& parent;

		ObjectStorageType& obj;

	private:

		SimpleReadWriteLock lock;
	};


	struct ParameterHandlerLight : public HandlerBase
	{
		ParameterHandlerLight(SnexSource& s, ObjectStorageType& o) :
			HandlerBase(s, o)
		{
			memset(lastValues, 0, sizeof(double)*OpaqueNode::NumMaxParameters);
		};

		virtual ~ParameterHandlerLight() {};

		void reset() override
		{
			SimpleReadWriteLock::ScopedWriteLock sl(getAccessLock());

			for (auto& f : pFunctions)
				f = {};
		}

		void copyLastValuesFrom(ParameterHandlerLight& other)
		{
			memcpy(lastValues, other.lastValues, sizeof(double) *OpaqueNode::NumMaxParameters);
			numParameters = other.numParameters;
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override;

		void setParameterDynamic(int index, double v)
		{
			lastValues[index] = v;
			SimpleReadWriteLock::ScopedReadLock sl(getAccessLock());
			pFunctions[index].callVoid(v);
		}

		template <int P> static void setParameterStatic(void* obj, double v)
		{
			auto typed = static_cast<SnexSource::ParameterHandler*>(obj);
			typed->setParameterDynamic(P, v);
		}

	protected:

		int numParameters = 0;
		span<snex::jit::FunctionData, OpaqueNode::NumMaxParameters> pFunctions;
		double lastValues[OpaqueNode::NumMaxParameters];
	};

	virtual SnexTestBase* createTester() = 0;

	struct ParameterHandler : public ParameterHandlerLight
	{
		ParameterHandler(SnexSource& s, ObjectStorageType& o) :
			ParameterHandlerLight(s, o)
		{};

		void updateParameters(ValueTree v, bool wasAdded);

		void updateParametersForWorkbench(bool shouldAdd);

		void removeSnexParameter(SnexParameter* p);

		void addNewParameter(parameter::data p);

		NodeBase* getNode() { return parent.parentNode; }

		void removeLastParameter()
		{
			parameterTree.removeChild(parameterTree.getNumChildren() - 1, getNode()->getUndoManager());
		}

		void addParameterCode(String& code);


	private:

		ValueTree parameterTree;
		valuetree::ChildListener parameterListener;
	};

	struct ComplexDataHandlerLight : public HandlerBase,
		public scriptnode::data::base
	{
		ComplexDataHandlerLight(SnexSource& parent, ObjectStorageType& o) :
			HandlerBase(parent, o)
		{

		}

		virtual ~ComplexDataHandlerLight() {};

		void reset() override
		{
			SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
			externalFunction = {};
		}

		void setExternalData(const snex::ExternalData& d, int index) override
		{
			base::setExternalData(d, index);

			auto v = (void*)(&d);

			SimpleReadWriteLock::ScopedReadLock l(getAccessLock());
			externalFunction.callVoid(v, index);
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override
		{
			auto newFunction = getFunctionAsObjectCallback("setExternalData");
			auto r = newFunction.validateWithArgs(Types::ID::Void, { Types::ID::Pointer, Types::ID::Pointer, Types::ID::Integer });

			if (r.wasOk())
			{
				SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
				std::swap(newFunction, externalFunction);
			}



			return r;
		}

	protected:

		snex::jit::FunctionData externalFunction;
	};

	struct ComplexDataHandler : public ComplexDataHandlerLight,
		public ExternalDataHolder,
		public hise::ComplexDataUIUpdaterBase::EventListener
	{
		ComplexDataHandler(SnexSource& parent, ObjectStorageType& o) :
			ComplexDataHandlerLight(parent, o)
		{};

		~ComplexDataHandler()
		{
			reset();
		}

		int getNumDataObjects(ExternalData::DataType t) const override;

		FilterDataObject* getFilterData(int index) override;
		Table* getTable(int index) override;
		SliderPackData* getSliderPack(int index) override;
		MultiChannelAudioBuffer* getAudioFile(int index) override;
		SimpleRingBuffer* getDisplayBuffer(int index) override;
		bool removeDataObject(ExternalData::DataType t, int index) override;

		ExternalDataHolder* getDynamicDataHolder(snex::ExternalData::DataType t, int index);

		void reset() override
		{
			ComplexDataHandlerLight::reset();

			ExternalData::forEachType([this](ExternalData::DataType t)
			{
				for (int i = 0; i < getNumDataObjects(t); i++)
				{
					if(auto ct = getComplexBaseType(t, i))
						ct->getUpdater().removeEventListener(this);
				}
			});
		}

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override
		{
			if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
			{
				for (auto l : parent.compileListeners)
				{
					if (l != nullptr)
						l->complexDataTypeChanged();
				}
			}
		}

		bool hasComplexData() const
		{
			return !tables.isEmpty() || !sliderPacks.isEmpty() || !audioFiles.isEmpty();
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override;
		void initialise(NodeBase* n);
		void addOrRemoveDataFromUI(ExternalData::DataType t, bool shouldAdd);
		void dataAddedOrRemoved(ValueTree v, bool wasAdded);
		ValueTree getDataRoot() { return dataTree; }



		static void callExternalDataForAll(ComplexDataHandler& handler, ComplexDataHandlerLight& target, bool getWriteLock=true)
		{
			ExternalData::forEachType([&handler, &target, getWriteLock](ExternalData::DataType t)
			{
				for (int i = 0; i < handler.getNumDataObjects(t); i++)
				{
					auto absoluteIndex = handler.getAbsoluteIndex(t, i);
					ExternalData ed(handler.getComplexBaseType(t, i), absoluteIndex);
					SimpleReadWriteLock::ScopedWriteLock l(ed.obj->getDataLock(), getWriteLock);
					target.setExternalData(ed, absoluteIndex);
				}
			});
		}

	private:

		ValueTree dataTree;

		valuetree::ChildListener dataListeners[(int)ExternalData::DataType::numDataTypes];

		OwnedArray<snex::ExternalDataHolder> tables;
		OwnedArray<snex::ExternalDataHolder> sliderPacks;
		OwnedArray<snex::ExternalDataHolder> displayBuffers;
		OwnedArray<snex::ExternalDataHolder> audioFiles;
	};

	struct CallbackHandlerBase : public HandlerBase,
	                             public SnexTestBaseHelper
	{
		CallbackHandlerBase(SnexSource& p, ObjectStorageType& o) :
			HandlerBase(p, o)
		{};

		virtual ~CallbackHandlerBase() {};

		virtual Result runTest(snex::ui::WorkbenchData::CompileResult& lastResult)
		{ 
			// If you hit this, you haven't overriden the runTest method
			jassertfalse;
			return Result::ok(); 
		}

		virtual void runPrepareTest(PrepareSpecs ps)
		{
			// if you hit this you haven't overriden the runRootTest() method
			jassertfalse;
		}

		virtual void runProcessTest(ProcessDataDyn& d)
		{
			// if you hit this you haven't overriden the runRootTest() method
			jassertfalse;
		}

		virtual void runHiseEventTest(HiseEvent& e) 
		{
			jassertfalse;
		}

		virtual bool runRootTest() const { return false; }

		struct ScopedDeactivator
		{
			ScopedDeactivator(CallbackHandlerBase& p) :
				parent(p),
				prevState(parent.parent.getCallbackHandler().ok.load())
			{
				parent.parent.getCallbackHandler().ok = false;
			};

			~ScopedDeactivator()
			{
				parent.parent.getCallbackHandler().ok = prevState;
			}

			CallbackHandlerBase& parent;
			const bool prevState;
		};

	protected:

		
		friend struct ScopedDeactivator;

		/** Use this in every callback and it will check that the read lock was
			acquired and the compilation was ok. */
		struct ScopedCallbackChecker
		{
			ScopedCallbackChecker(CallbackHandlerBase& p) :
				parent(p)
			{
				if (parent.ok)
					holdsLock = p.getAccessLock().enterReadLock();
			}

			operator bool() { return parent.ok && holdsLock; }

			~ScopedCallbackChecker()
			{
				parent.getAccessLock().exitReadLock(holdsLock);
			}

			bool holdsLock = false;
			CallbackHandlerBase& parent;
		};

		std::atomic<bool> ok = { false };
	};

	template <class T, bool UseRootTest=false> struct Tester: public SnexTestBase,
	                                                          public SnexTestBaseHelper
	{
		Tester(SnexSource& s) :
			dataHandler(s, obj),
			parameterHandler(s, obj),
			callbacks(s, obj),
			original(s)
		{
			static_assert(std::is_base_of<CallbackHandlerBase, T>(), "not a base of CallbackHandlerBase");

			init();
		}

		void init()
		{
			callbacks.reset();
			dataHandler.reset();
			parameterHandler.reset();
			parameterHandler.copyLastValuesFrom(original.getParameterHandler());

			if (auto wb = original.getWorkbench())
			{
				if (auto ptr = wb->getLastResult().mainClassPtr)
				{
					ptr->initialiseObjectStorage(obj);

					callbacks.recompiledOk(ptr);
					parameterHandler.recompiledOk(ptr);
					dataHandler.recompiledOk(ptr);
					ComplexDataHandler::callExternalDataForAll(original.getComplexDataHandler(), dataHandler, false);
				}
			}
		}

		void processTestParameterEvent(int parameterIndex, double value) final override
		{
			parameterHandler.setParameterDynamic(parameterIndex, value);
		};

		Result prepareTest(PrepareSpecs ps, const Array<ParameterEvent>& initialParameters) override
		{
			callbacks.runPrepareTest(ps);

			for (const auto& p : initialParameters)
				processTestParameterEvent(p.parameterIndex, p.valueToUse);
            
            return Result::ok();
		}

		void processTest(ProcessDataDyn& data) override
		{
			callbacks.runProcessTest(data);
		}

		void processHiseEvent(HiseEvent& e) override
		{
			callbacks.runHiseEventTest(e);
		}

		bool shouldProcessEventsManually() const override { return true; }

		void initExternalData(ExternalDataHolder* ) override {}

		bool triggerTest(ui::WorkbenchData::CompileResult& lastResult) override
		{
			if(auto wb = original.getWorkbench())
				wb->triggerPostCompileActions();

			return false;
		}

		Result runTest(ui::WorkbenchData::CompileResult& lastResult) override
		{
			init();

			if (callbacks.runRootTest())
			{
				auto wb = static_cast<snex::ui::WorkbenchManager*>(getNodeWorkbench(original.getParentNode()));

				if (auto rwb = wb->getRootWorkbench())
				{
					auto& td = rwb->getTestData();

					td.setCustomTest(this);

					CallbackHandlerBase::ScopedDeactivator sd(callbacks);

					auto cs = original.parentNode->getRootNetwork()->getCurrentSpecs();

					if (cs.sampleRate <= 0.0)
					{
						cs.sampleRate = 44100.0;
						cs.blockSize = 512;
					}

					td.initProcessing(cs);

					td.processTestData(rwb);

					WeakReference<snex::ui::WorkbenchData> safeW(rwb.get());

					auto f = [safeW]()
					{
						if (safeW.get() != nullptr)
							safeW.get()->postPostCompile();
					};

					MessageManager::callAsync(f);

					return Result::ok();
				}
			}
			else
			{
				return callbacks.runTest(lastResult);
			}

			return Result::ok();
		}

		SnexSource& original;
		HandlerBase::ObjectStorageType obj;
		ComplexDataHandlerLight dataHandler;
		ParameterHandlerLight parameterHandler;
		T callbacks;
	};

	SnexSource() :
		classId(PropertyIds::ClassId, ""),
		parameterHandler(*this, object),
		dataHandler(*this, object),
		lastResult(Result::fail("uninitialised"))
	{
	};

	~SnexSource()
	{
		setWorkbench(nullptr);
	}

	virtual Identifier getTypeId() const = 0;

	virtual void initialise(NodeBase* n)
	{
		parentNode = n;

		getComplexDataHandler().initialise(n);

		compileChecker.setCallback(n->getValueTree(), valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_0(SnexSource::throwScriptnodeErrorIfCompileFail));

		classId.initialise(n);
		classId.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(SnexSource::updateClassId), true);
	}

	void preCompile() override
	{
        errorLevel = ErrorLevel::Uncompiled;
		callbackHandler->reset();
		parameterHandler.reset();
		getComplexDataHandler().reset();
	}

    ErrorLevel getErrorLevel() const { return errorLevel; }
    
	void recompiled(WorkbenchData::Ptr wb) final override;

	void throwScriptnodeErrorIfCompileFail();

	void logMessage(WorkbenchData::Ptr wb, int level, const String& s) override;

	void debugModeChanged(bool isEnabled) override;

	SimpleRingBuffer::Ptr getMainDisplayBuffer()
	{
		if (mainDisplayBuffer == nullptr)
		{
			if (getParentNode() != nullptr)
			{
				mainDisplayBuffer = getComplexDataHandler().getDisplayBuffer(0);
			}
		}

		return mainDisplayBuffer;
	}

	bool preprocess(String& code) override
	{
		jassert(code.contains("setParameter("));

		parameterHandler.addParameterCode(code);

		return true;
	}

	String getId() const
	{
		if (parentNode != nullptr)
		{
			return parentNode->getId();
		}

		return {};
	}

	bool allowProcessing()
	{
		return getWorkbench() != nullptr && !getWorkbench()->getGlobalScope().isDebugModeEnabled();
	}

	StringArray getAvailableClassIds()
	{
		return parentNode->getRootNetwork()->codeManager.getClassList(getTypeId());
	}

	virtual String getEmptyText(const Identifier& id) const = 0;

	void setWorkbench(WorkbenchData::Ptr nb)
	{
		if (wb != nullptr)
			wb->removeListener(this);

		wb = nb;

		if (parentNode != nullptr)
			parameterHandler.updateParametersForWorkbench(wb != nullptr);

		if (wb != nullptr)
		{
			if (auto dc = dynamic_cast<snex::ui::WorkbenchData::DefaultCodeProvider*>(wb->getCodeProvider()))
				dc->defaultFunction = [this](const Identifier& id) { return this->getEmptyText(id); };

			if (auto c = dynamic_cast<DspNetwork::CodeManager::SnexSourceCompileHandler*>(getWorkbench()->getCompileHandler()))
			{
				c->setTestBase(createTester());
			}

			if(parentNode != nullptr)
				wb->getGlobalScope().setPolyphonic(parentNode->getRootNetwork()->isPolyphonic());

			wb->addListener(this);
			wb->triggerRecompile();
		}
	}

	void updateClassId(Identifier, var newValue)
	{
		auto s = newValue.toString();

		if (s.isNotEmpty())
		{
			auto nb = parentNode->getRootNetwork()->codeManager.getOrCreate(getTypeId(), Identifier(newValue.toString()));
			setWorkbench(nb);
		}
	}

	WorkbenchData::Ptr getWorkbench() { return wb; }

	void setClass(const String& newClassName)
	{
		classId.storeValue(newClassName, parentNode->getUndoManager());
		updateClassId({}, newClassName);
	}

	NodeBase* getParentNode() { return parentNode; }

	void addCompileListener(SnexSourceListener* l)
	{
		compileListeners.addIfNotAlreadyThere(l);

		if (getWorkbench() != nullptr)
			l->wasCompiled(lastResult.wasOk());
	}

	void removeCompileListener(SnexSourceListener* l)
	{
		compileListeners.removeAllInstancesOf(l);
	}

	ParameterHandler& getParameterHandler() { return parameterHandler; }
	ComplexDataHandler& getComplexDataHandler() { return dataHandler; }
	CallbackHandlerBase& getCallbackHandler() { jassert(callbackHandler != nullptr); return *callbackHandler; }

	const ParameterHandler& getParameterHandler() const { return parameterHandler; }
	const ComplexDataHandler& getComplexDataHandler() const { return dataHandler; }
	const CallbackHandlerBase& getCallbackHandler() const { jassert(callbackHandler != nullptr); return *callbackHandler; }

	Identifier getCurrentClassId() const { return Identifier(classId.getValue()); }

    /** This function will be called whenever a change occurs in the external data.
     
        During the execution of this function the write access to the external data is locked and it is highly recommended to use a DataReadLock object
        whenever you access the external data in the other callbacks.
     
        @code
        struct my_class
        {
            ExternalData localCopy;
     
            void process(ProcessData<2>& data)
            {
                DataReadLock sl(localCopy);
     
                if(!localCopy.isEmpty() && sl)
                {
                    block b;
                    localCopy.referBlockTo(b, 0);
                        
                    b[10]
    
                }
            }
     
            void setExternalData(const ExternalData& d, int index)
            {
                // This is unnecessary because it's already locked
                // but that's basically what happens here...
                // DataWriteLock sl(d);
     
                localCopy = d;
            }
     
        @endcode
    */
	void setExternalData(const ExternalData& d, int index)
	{
		
	}

    /** This function will be called whenever a parameter changes. It has the parameter index as template argument so you can branch at compile time.
     
     This function might be called from an audio rate modulation so make sure that it's performant enough to handle its use cases.
     Also if you are using the snex::Types::PolyData<T, NumVoices> container for polyphonic states make sure to use its iterator to handle both cases (changing all voices vs. modulating the current voice).
    */
	template <int P> void setParameter(double v)
	{
		getParameterHandler().setParameterStatic<P>(&getParameterHandler(), v);
	}

	void rebuildCallbacksAfterChannelChange(int numChannelsToProcess);

#if 0
	int getNumChannelsToProcess() const
	{
		if (numChannels == -1)
			return parentNode->getNumChannelsToProcess();

		return numChannels;
	}
#endif

protected:

	//int numChannels = -1;

	void setCallbackHandler(CallbackHandlerBase* nonOwnedHandler)
	{
		callbackHandler = nonOwnedHandler;
	}

	Array<WeakReference<SnexSourceListener>> compileListeners;

	void addDummyNodeCallbacks(String& s, bool addEvent, bool addReset, bool addHandleModulation);

	void addDummyProcessFunctions(String& s, bool addFrame, const String& processDataType = {});

	

	static void addSnexNodeId(cppgen::Base& c, const Identifier& id)
	{
		String l;
		l << "SNEX_NODE(" << id << ");";
		c << l;
		c.addEmptyLine();
	}

	static void addDefaultParameterFunction(String& code)
	{
		code << "void setExternalData(const ExternalData& d, int index)\n";
		code << "{\n";
		code << "\t\n";
		code << "}\n";
		code << "\n";
		code << "template <int P> void setParameter(double v)\n";
		code << "{\n";
		code << "\t\n";
		code << "}\n";
	}

protected:

	ObjectStorage<OpaqueNode::SmallObjectSize, 16> object;

private:

    ErrorLevel errorLevel = ErrorLevel::Uncompiled;
    
	SimpleRingBuffer::Ptr mainDisplayBuffer;

	valuetree::ParentListener compileChecker;
	bool processingEnabled = true;

	ParameterHandler parameterHandler;
	ComplexDataHandler dataHandler;
	CallbackHandlerBase* callbackHandler = nullptr;

	int currentChannelCount = 0;

	Result lastResult;

	// This keeps the function alive until recompiled
	snex::JitObject lastCompiledObject;
	NodePropertyT<String> classId;
	snex::ui::WorkbenchData::Ptr wb;
	WeakReference<NodeBase> parentNode;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexSource);
};



struct SnexComplexDataDisplay : public Component,
	public SnexSource::SnexSourceListener
{
	SnexComplexDataDisplay(SnexSource* s);

	~SnexComplexDataDisplay();

	void wasCompiled(bool ok) {};

	void complexDataAdded(snex::ExternalData::DataType, int)
	{
		rebuildEditors();
	}

	void parameterChanged(int index, double v)
	{

	}

	void rebuildEditors();

	void resized()
	{
		auto b = getLocalBounds();

		for (auto e : editors)
		{
			e->setBounds(b.removeFromTop(100));
		}
	}

	OwnedArray<Component> editors;

	WeakReference<SnexSource> source;
};

struct SnexMenuBar : public ComponentWithMiddleMouseDrag,
	public ButtonListener,
	public ComboBox::Listener,
	public SnexSource::SnexSourceListener,
	public snex::ui::WorkbenchManager::WorkbenchChangeListener,
	public snex::ui::WorkbenchData::Listener

{
	struct Factory : public PathFactory
	{
		Path createPath(const String& p) const override;

		String getId() const override { return {}; }
	} f;

	ComboBox classSelector;
	HiseShapeButton popupButton;
	HiseShapeButton editButton;
	HiseShapeButton addButton;
	HiseShapeButton debugButton;
	HiseShapeButton optimizeButton;
	HiseShapeButton asmButton;
	HiseShapeButton cdp;

	SnexMenuBar(SnexSource* s);

	void wasCompiled(bool ok) override;

	void parameterChanged(int snexParameterId, double newValue) override;

	~SnexMenuBar();

	void debugModeChanged(bool isEnabled) override;;

	void workbenchChanged(snex::ui::WorkbenchData::Ptr newWb);

	void complexDataAdded(snex::ExternalData::DataType t, int index) override;

	void rebuildComboBoxItems();

	void refreshButtonState();

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

	void buttonClicked(Button* b);

	void paint(Graphics& g) override;

	void resized() override;;

	ScriptnodeComboBoxLookAndFeel plaf;
	Path snexIcon;
	Colour iconColour = Colours::white.withAlpha(0.2f);

	bool debugMode = false;

	WeakReference<SnexSource> source;

	snex::ui::WorkbenchData::WeakPtr rootBench;
	snex::ui::WorkbenchData::WeakPtr lastBench;

};


}
