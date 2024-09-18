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

namespace hise
{
#if !USE_BACKEND
	struct FrontendHostFactory;
#endif
}

namespace scriptnode
{
using namespace juce;
using namespace hise;



struct NodeFactory;

struct DeprecationChecker
{
	enum class DeprecationId
	{
		OK,
		OpTypeNonSet,
		ConverterNotIdentity,
		numDeprecationIds
	};

	DeprecationChecker(DspNetwork* n_, ValueTree v_);

	static String getErrorMessage(int id);

	DspNetwork* n;
	ValueTree v;
	bool notOk = false;

	void throwIf(DeprecationId id);

	bool check(DeprecationId id);
};



class ScriptnodeExceptionHandler
{
	struct Item
	{
		bool operator==(const Item& other) const;

		String toString(const String& customErrorMessage) const;

		WeakReference<NodeBase> node;
		Error error;
		
	};

public:

	static void validateMidiProcessingContext(NodeBase* b);

	bool isOk() const noexcept;

	void addCustomError(NodeBase* n, Error::ErrorCode c, const String& errorMessage);

	void addError(NodeBase* n, Error e, const String& errorMessage = {});

	void removeError(NodeBase* n, Error::ErrorCode errorToRemove=Error::numErrorCodes);

	static String getErrorMessage(Error e);

	String getErrorMessage(const NodeBase* n = nullptr) const;

	LambdaBroadcaster<NodeBase*, Error> errorBroadcaster;

private:

	String customErrorMessage;
	Array<Item> items;
};


class SnexSource;




/** A network of multiple DSP objects that are connected using a graph. */
class DspNetwork : public ConstScriptingObject,
				   public Timer,
				   public AssignableObject,
				   public ControlledObject,
				   public NodeBase::Holder
{
public:

	struct AnonymousNodeCloner
	{
		AnonymousNodeCloner(DspNetwork& p, NodeBase::Holder* other);

		~AnonymousNodeCloner();

		NodeBase::Ptr clone(NodeBase::Ptr p);

		DspNetwork& parent;
		WeakReference<NodeBase::Holder> prevHolder;
	};

	struct VoiceSetter
	{
		VoiceSetter(DspNetwork& p, int newVoiceIndex):
			internalSetter(*p.getPolyHandler(), newVoiceIndex)
		{}

	private:

		const snex::Types::PolyHandler::ScopedVoiceSetter internalSetter;
	};

	struct NoVoiceSetter
	{
		NoVoiceSetter(DspNetwork& p):
			internalSetter(*p.getPolyHandler())
		{}

	private:

		const snex::Types::PolyHandler::ScopedAllVoiceSetter internalSetter;
	};

    struct IdChange
    {
		bool operator==(const IdChange& other) const
		{
			return oldId == other.oldId && newId == other.newId;
		}

        String oldId;
        String newId;
    };
    
	class Holder
	{
	public:

		Holder();

		virtual ~Holder();;

		DspNetwork* getOrCreate(const ValueTree& v);

        void unload();
        
		DspNetwork* getOrCreate(const String& id);
		StringArray getIdList();
		void saveNetworks(ValueTree& d) const;
		void restoreNetworks(const ValueTree& d);

		virtual bool isPolyphonic() const;;

		void clearAllNetworks();

		void setActiveNetwork(DspNetwork* n);

		ScriptParameterHandler* getCurrentNetworkParameterHandler(const ScriptParameterHandler* contentHandler) const;

		DspNetwork* getActiveOrDebuggedNetwork() const;

		DspNetwork* getActiveNetwork() const;

		void setProjectDll(dll::ProjectDll::Ptr pdll);

		dll::ProjectDll::Ptr projectDll;

		ExternalDataHolder* getExternalDataHolder();

		void setExternalDataHolderToUse(ExternalDataHolder* newHolder);

		void setVoiceKillerToUse(snex::Types::VoiceResetter* vk_);

		SimpleReadWriteLock& getNetworkLock();

		DspNetwork* addEmbeddedNetwork(DspNetwork* parent, const ValueTree& v, ExternalDataHolder* holderToUse);

		DspNetwork* getDebuggedNetwork();;
		const DspNetwork* getDebuggedNetwork() const;;

		void toggleDebug();

	protected:

		ReferenceCountedArray<DspNetwork> embeddedNetworks;

		SimpleReadWriteLock connectLock;

		WeakReference<snex::Types::VoiceResetter> vk;

		ExternalDataHolder* dataHolder = nullptr;

		WeakReference<DspNetwork> debuggedNetwork;
		WeakReference<DspNetwork> activeNetwork;

		ReferenceCountedArray<DspNetwork> networks;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
	};

	DspNetwork(ProcessorWithScriptingContent* p, ValueTree data, bool isPolyphonic, ExternalDataHolder* dataHolder=nullptr);
	~DspNetwork();

    /** The faust manager will handle the IDE editing features by sending out compilation and selection messages to its registered listeners. */
    struct FaustManager
    {
        struct FaustListener
        {
            virtual ~FaustListener() {};
            
            /** This message will be sent out synchronously when the faust file is selected for editing. */
            virtual void faustFileSelected(const File& f) = 0;
            
			/** This message will be sent out synchronously before compileFaustCode and can be overriden to indicate a pending compilation. */
			virtual void preCompileFaustCode(const File& f) {};

            /** This message is sent out on the sample loading thread when the faust code needs to be recompiled. */
            virtual Result compileFaustCode(const File& f) = 0;
            
            /** This message will be sent out on the message thread after the faust code was recompiled. */
            virtual void faustCodeCompiled(const File& f, const Result& compileResult) = 0;
            
            JUCE_DECLARE_WEAK_REFERENCEABLE(FaustListener);
        };
        
        /** Adds a listener and calls all messages with the current state. */
        void addFaustListener(FaustListener* l);
        
        /** Removes a listener. */
        void removeFaustListener(FaustListener* l);
        
        /** Call this function when you want to set a FAUST file to be selected for editing.
            There is only a single edited file per faust_manager instance and the listeners
            will receive a message that the edited file changed.
        */
        void setSelectedFaustFile(Component* c, const File& f, NotificationType n);
        
        /** Send a message that this file is about to be compiled.
            The listeners will be called with `compileFaustCode()` which can be override
            to implement the actual compilation.
         
            After the compilation the `faustCodeCompiled()` function will be called with the
            file and the compile result.
        */
        void sendCompileMessage(const File& f, NotificationType n);
        
        FaustManager(DspNetwork& n);;
        
        virtual ~FaustManager() {};
        
    private:
        
		hise::SimpleReadWriteLock listenerLock;

		void sendPostCompileMessage();

        Result lastCompileResult;
        File currentFile;
        File lastCompiledFile;
        
		WeakReference<Processor> processor;

        Array<WeakReference<FaustListener>> listeners;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaustManager);
		JUCE_DECLARE_WEAK_REFERENCEABLE(FaustManager);
    } faustManager;
    
#if HISE_INCLUDE_SNEX
	struct CodeManager
	{
		CodeManager(DspNetwork& p);

		struct SnexSourceCompileHandler : public snex::ui::WorkbenchData::CompileHandler,
		                                  public ControlledObject,
		                                  public Thread
		{
			using TestBase = snex::ui::WorkbenchData::TestRunnerBase;
			
			struct SnexCompileListener
			{
				virtual ~SnexCompileListener();;

				/** This is called directly after the compilation (if it was OK) and can 
				    be used to run the test and update the UI display. */
				virtual void postCompileSync() = 0;

				JUCE_DECLARE_WEAK_REFERENCEABLE(SnexCompileListener);
			};

			SnexSourceCompileHandler(snex::ui::WorkbenchData* d, ProcessorWithScriptingContent* sp_);;

            ~SnexSourceCompileHandler();

			void processTestParameterEvent(int parameterIndex, double value) final override;;
            Result prepareTest(PrepareSpecs ps, const Array<snex::ui::WorkbenchData::TestData::ParameterEvent>& initialParameters) final override;
			;
			void processTest(ProcessDataDyn& data) final override;;

			void run() override;

			void postCompile(ui::WorkbenchData::CompileResult& lastResult) override;

			bool triggerCompilation() override;
			
			/** This returns the mutex used for synchronising the compilation. 
			
				Any access to a JIT compiled function must be locked using a read lock.

				The write lock will be held for a short period before compiling (where you need to reset 
				the state) and then after the compilation where you need to setup the object.
			*/
			SimpleReadWriteLock& getCompileLock();

			void addCompileListener(SnexCompileListener* l);

			void removeCompileListener(SnexCompileListener* l);

			void setTestBase(TestBase* ownedTest);

		private:

			std::atomic<bool> runTestNext = false;

			ScopedPointer<TestBase> test;

			ProcessorWithScriptingContent* sp;

			hise::SimpleReadWriteLock compileLock;

			Array<WeakReference<SnexCompileListener>> compileListeners;
		};

		snex::ui::WorkbenchData::Ptr getOrCreate(const Identifier& typeId, const Identifier& classId);

		ValueTree getParameterTree(const Identifier& typeId, const Identifier& classId);

		StringArray getClassList(const Identifier& id, const String& fileExtension = "*.h");

	private:

		struct Entry
		{
			Entry(const Identifier& t, const File& targetFile, ProcessorWithScriptingContent* sp);

			Entry(const Identifier& t, const ExternalScriptFile::Ptr& embeddedFile, ProcessorWithScriptingContent* sp);

			const Identifier type;
			const File parameterFile;

			ScopedPointer<snex::ui::WorkbenchData::CodeProvider> cp;

			snex::ui::WorkbenchData::Ptr wb;

			void parameterAddedOrRemoved(ValueTree, bool);

			void propertyChanged(ValueTree, Identifier);

			ValueTree parameterTree;
			
		private:

			ExternalScriptFile::ResourceType resourceType;

			void init(snex::ui::WorkbenchData::CodeProvider* codeProvider, const ValueTree& pTree, ProcessorWithScriptingContent* sp)
			{
				cp = codeProvider;
				wb = new snex::ui::WorkbenchData();
				wb->setCodeProvider(cp, dontSendNotification);
				wb->setCompileHandler(new SnexSourceCompileHandler(wb.get(), sp));

				parameterTree = pTree;

				if(!parameterTree.isValid())
					parameterTree = ValueTree(PropertyIds::Parameters);

				pListener.setCallback(parameterTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Entry::parameterAddedOrRemoved));
				propListener.setCallback(parameterTree, RangeHelpers::getRangeIds(), valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Entry::propertyChanged));
			}

			void updateFile();

			ExternalScriptFile::Ptr parameterExternalFile;

			valuetree::ChildListener pListener;
			valuetree::RecursivePropertyListener propListener;
		};

		OwnedArray<Entry> entries;

		File getCodeFolder() const;

		DspNetwork& parent;
	} codeManager;
#endif

	void setNumChannels(int newNumChannels);

	void createAllNodesOnce();

	Identifier getObjectName() const override { return "DspNetwork"; };

	String getDebugName() const override { return "DspNetwork"; }
	String getDebugValue() const override { return getId(); }
	
	NodeBase* getNodeForValueTree(const ValueTree& v);
	NodeBase::List getListOfUnconnectedNodes() const;

	ValueTree getListOfAvailableModulesAsTree() const;

	StringArray getListOfAllAvailableModuleIds() const;
	StringArray getListOfUsedNodeIds() const;
	StringArray getListOfUnusedNodeIds() const;
	StringArray getFactoryList() const;

	void assign(const int, var newValue) override { reportScriptError("Can't assign to this expression"); };

	var getAssignedValue(int index) const override;

	int getCachedIndex(const var &indexExpression) const override;

	NodeBase::Holder* getCurrentHolder() const;

	void registerOwnedFactory(NodeFactory* ownedFactory);

	NodeBase::List getListOfNodesWithPath(const NamespacedIdentifier& id, bool includeUnusedNodes);

	template <class T> NodeBase::List getListOfNodesWithType(bool includeUsedNodes)
	{
		NodeBase::List list;

		for (auto n : nodes)
		{
			if ((includeUsedNodes || isInSignalPath(n)) && dynamic_cast<T*>(n) != nullptr)
				list.add(n);
		}

		return list;
	}

	void reset();

	void handleHiseEvent(HiseEvent& e);

	void process(ProcessDataDyn& data);

	void process(AudioSampleBuffer& b, HiseEventBuffer* e);

	bool isPolyphonic() const { return isPoly; }

	bool hasTail() const;

	bool isSuspendedOnSilence() const;

	bool handleModulation(double& v);

	Identifier getParameterIdentifier(int parameterIndex);

	// ===============================================================================

	/** Defines whether the UI controls of this script control the parameters or regular script callbacks. */
	void setForwardControlsToParameters(bool shouldForward);

	/** Initialise processing of all nodes. */
	void prepareToPlay(double sampleRate, double blockSize);

	/** Process the given channel array with the node network. */
	void processBlock(var data);

	/** Creates and returns a node with the given path (`factory.node`). If a node with the id already exists, it returns this node. */
	var create(String path, String id);

	/** Creates a node, names it automatically and adds it at the end of the given parent. */
	var createAndAdd(String path, String id, var parent);

	/** Creates multiple nodes from the given JSON object. */
	var createFromJSON(var jsonData, var parent);

	/** Returns a reference to the node with the given id. */
	var get(var id) const;

	/** Removes all nodes. */
	void clear(bool removeNodesFromSignalChain, bool removeUnusedNodes);

	/** Undo the last action. */
	bool undo();

	void checkValid() const
	{
		if (parentHolder == nullptr)
			reportScriptError("Parent of DSP Network is deleted");
	}

	bool isBeingDebugged() const
	{
		return parentHolder->getDebuggedNetwork() == this;
	}

	/** Creates a test object for this network. */
	var createTest(var testData);

	/** Deletes the node if it is not in a signal path. */
	bool deleteIfUnused(String id);

	/** Sets the parameters of this node according to the JSON data. */
	bool setParameterDataFromJSON(var jsonData);

	Array<Parameter*> getListOfProbedParameters();

	String getId() const { return data[PropertyIds::ID].toString(); }

	ValueTree getValueTree() const { return data; };

	SimpleReadWriteLock& getConnectionLock() { return parentHolder->getNetworkLock(); }
	bool updateIdsInValueTree(ValueTree& v, StringArray& usedIds);
	NodeBase* createFromValueTree(bool createPolyIfAvailable, ValueTree d, bool forceCreate=false);
	bool isInSignalPath(NodeBase* b) const;

	bool isCurrentlyRenderingVoice() const noexcept { return isPolyphonic() && getPolyHandler()->getVoiceIndex() != -1; }

	bool isRenderingFirstVoice() const noexcept { return !isPolyphonic() || getPolyHandler()->getVoiceIndex() == 0; }

    bool isInitialised() const noexcept { return initialised; };

	bool isForwardingControlsToParameters() const
	{
		return forwardControls;
	}

	PrepareSpecs getCurrentSpecs() const { return currentSpecs; }

	NodeBase* getNodeWithId(const String& id) const;

	void checkIfDeprecated();

	Result checkBeforeCompilation();

	struct SelectionListener
	{
		virtual ~SelectionListener() {};
		virtual void selectionChanged(const NodeBase::List& selection) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SelectionListener);
	};

	void addSelectionListener(SelectionListener* l) { if(selectionUpdater != nullptr) selectionUpdater->listeners.addIfNotAlreadyThere(l); }
	void removeSelectionListener(SelectionListener* l) { if(selectionUpdater != nullptr) selectionUpdater->listeners.removeAllInstancesOf(l); }

	bool isSelected(NodeBase* node) const { return selection.isSelected(node); }

	void deselect(NodeBase* node);

	void deselectAll() { selection.deselectAll(); }

	void addToSelection(NodeBase* node, ModifierKeys mods);

	SelectedItemSet<NodeBase::Ptr>& getRawSelection() { return selection; };

	NodeBase::List getSelection() const { return selection.getItemArray(); }

    void zoomToSelection(Component* c);

	void fillSnexObjects(StringArray& indexList);

	static scriptnode::dll::FactoryBase* createStaticFactory();

	struct NetworkParameterHandler : public hise::ScriptParameterHandler
	{
		int getNumParameters() const final override { return root->getNumParameters(); }
		Identifier getParameterId(int index) const final override
		{
			return Identifier(root->getParameterFromIndex(index)->getId());
		}

		int getParameterIndexForIdentifier(const Identifier& id) const final override
		{
			for(int i = 0; i < root->getNumParameters(); i++)
			{
				if (root->getId() == id.toString())
					return i;
			}

			return -1;
		}

		float getParameter(int index) const final override
		{
			if(isPositiveAndBelow(index, getNumParameters()))
				return (float)root->getParameterFromIndex(index)->getValue();

			return 0.0f;
		}

		void setParameter(int index, float newValue) final override
		{
			if(isPositiveAndBelow(index, getNumParameters()))
				root->getParameterFromIndex(index)->setValueAsync((double)newValue);
		}

		NodeBase::Ptr root;
	} networkParameterHandler;

	ValueTree cloneValueTreeWithNewIds(const ValueTree& treeToClone, Array<IdChange>& idChanges, bool changeIds);

	void setEnableUndoManager(bool shouldBeEnabled);

	ScriptnodeExceptionHandler& getExceptionHandler()
	{
		return exceptionHandler;
	}

	const ScriptnodeExceptionHandler& getExceptionHandler() const
	{
		return exceptionHandler;
	}

	//int* getVoiceIndexPtr() { return &voiceIndex; }

	void timerCallback() override
	{
		um.beginNewTransaction();
	}

	

	void changeNodeId(ValueTree& c, const String& oldId, const String& newId, UndoManager* um);

	UndoManager* getUndoManager(bool returnIfPending=false);

	double getOriginalSampleRate() const noexcept { return originalSampleRate; }

	Array<WeakReference<SnexSource>>& getSnexObjects() 
	{
		return snexObjects;
	}

	ExternalDataHolder* getExternalDataHolder() { return dataHolder; }

	

	void setExternalDataHolder(ExternalDataHolder* newHolder)
	{
		dataHolder = newHolder;
	}

	bool& getCpuProfileFlag() { return enableCpuProfiling; };

	void setVoiceKiller(VoiceResetter* newVoiceKiller)
	{
		if (isPolyphonic())
			getPolyHandler()->setVoiceResetter(newVoiceKiller);	
	}

	void setUseFrozenNode(bool shouldBeEnabled);

	bool canBeFrozen() const { return projectNodeHolder.loaded; }

	bool isFrozen() const { return projectNodeHolder.isActive(); }

	bool hashMatches();

	void setExternalData(const snex::ExternalData & d, int index);

	ScriptParameterHandler* getCurrentParameterHandler();

	Holder* getParentHolder() { return parentHolder; }

	DspNetwork* getParentNetwork() { return parentNetwork.get(); }
	const DspNetwork* getParentNetwork() const { return parentNetwork.get(); }

	void setParentNetwork(DspNetwork* p)
	{
		parentNetwork = p;
	}

	PolyHandler* getPolyHandler();

	const PolyHandler* getPolyHandler() const;

	ModValue& getNetworkModValue() { return networkModValue; }

	void addPostInitFunction(const std::function<bool(void)>& f)
	{
		postInitFunctions.add(f);
	}

	void runPostInitFunctions();

	static void initKeyPresses(Component* root);



    bool isSignalDisplayEnabled() const { return signalDisplayEnabled; }
    
    void setSignalDisplayEnabled(bool shouldBeEnabled)
    {
        signalDisplayEnabled = shouldBeEnabled;
    }
    
	String getNonExistentId(String id, StringArray& usedIds) const;

	

private:

	String initialId;

	void checkId(const Identifier& id, const var& newValue);

	valuetree::PropertyListener idGuard;

    bool signalDisplayEnabled = false;
    
	Array<std::function<bool()>> postInitFunctions;

	ModValue networkModValue;

	bool enableCpuProfiling = false;

	WeakReference<ExternalDataHolder> dataHolder;
	WeakReference<DspNetwork> parentNetwork;

	PrepareSpecs currentSpecs;

	Array<WeakReference<SnexSource>> snexObjects;

	double originalSampleRate = 0.0;

	ScriptnodeExceptionHandler exceptionHandler;

#if USE_BACKEND
	bool enableUndo = true;
#else
	// disable undo on compiled plugins unless explicitely stated
	bool enableUndo = false; 
#endif

	bool forwardControls = true;

	UndoManager um;

	const bool isPoly;

	CachedValue<bool> hasTailProperty;
	CachedValue<bool> canBeSuspendedProperty;

	snex::Types::DllBoundaryTempoSyncer tempoSyncer;
	snex::Types::PolyHandler polyHandler;

	SelectedItemSet<NodeBase::Ptr> selection;

	struct SelectionUpdater : public ChangeListener
	{
		SelectionUpdater(DspNetwork& parent_);
		~SelectionUpdater();

		void changeListenerCallback(ChangeBroadcaster* ) override;

		Array<WeakReference<SelectionListener>> listeners;

		DspNetwork& parent;

		valuetree::RecursiveTypedChildListener deleteChecker;
	};

    bool initialised = false;
    
	ScopedPointer<SelectionUpdater> selectionUpdater;

	OwnedArray<NodeFactory> ownedFactories;

	Array<WeakReference<NodeFactory>> nodeFactories;

	

	valuetree::RecursivePropertyListener idUpdater;
	valuetree::RecursiveTypedChildListener exceptionResetter;

    valuetree::RecursiveTypedChildListener sortListener;

	WeakReference<Holder> parentHolder;

	ValueTree data;

	float* currentData[NUM_MAX_CHANNELS];
	friend class DspNetworkGraph;

	struct Wrapper;

	DynamicObject::Ptr loader;

	WeakReference<NodeBase::Holder> currentNodeHolder;

	bool createAnonymousNodes = false;

	struct ProjectNodeHolder: public hise::ScriptParameterHandler
	{
		ProjectNodeHolder(DspNetwork& parent);

		Identifier getParameterId(int index) const override;

		int getParameterIndexForIdentifier(const Identifier& id) const override
		{
			return network.networkParameterHandler.getParameterIndexForIdentifier(id);
		}

		int getNumParameters() const override;

		void setParameter(int index, float newValue) override;

		float getParameter(int index) const override;;

		~ProjectNodeHolder();

		bool isActive() const;

		void prepare(PrepareSpecs ps);

		void process(ProcessDataDyn& data);

		bool handleModulation(double& modValue);

		void setEnabled(bool shouldBeEnabled);

		void init(dll::StaticLibraryHostFactory* staticLibrary);

		void init(dll::ProjectDll::Ptr dllToUse);

		bool hashMatches = false;

		float parameterValues[OpaqueNode::NumMaxParameters];
		DspNetwork& network;
		dll::ProjectDll::Ptr dll;
		OpaqueNode n;
		bool loaded = false;
		bool forwardToNode = false;
	} projectNodeHolder;
    
	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetwork);
};


struct OpaqueNetworkHolder
{
	SN_GET_SELF_AS_OBJECT(OpaqueNetworkHolder);

	bool isPolyphonic() const;

	SN_EMPTY_INITIALISE;
	

	OpaqueNetworkHolder();

	~OpaqueNetworkHolder();

	void handleHiseEvent(HiseEvent& e);

	bool handleModulation(double& modValue);

	void process(ProcessDataDyn& d);

	void reset();

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		// this might be the most inefficient code ever but we need
		// to allow frame based processing of wrapped networks
		float* channels[NUM_MAX_CHANNELS];

		for (int i = 0; i < d.size(); i++)
			channels[i] = d.begin() + i;

		ProcessDataDyn pd(channels, 1, d.size());

		ownedNetwork->process(pd);
	}

	void prepare(PrepareSpecs ps);

	void createParameters(ParameterDataList& l);

	void setCallback(parameter::data& d, int index);

	template <int P> static void setParameterStatic(void* obj, double v)
	{
		auto t = static_cast<OpaqueNetworkHolder*>(obj);
		t->ownedNetwork->getCurrentParameterHandler()->setParameter(P, (float)v);
	}

	void setNetwork(DspNetwork* n);

	DspNetwork* getNetwork();

	void setExternalData(const ExternalData& d, int index);

private:

	struct DeferedDataInitialiser
	{
		ExternalData d;
		int index;
	};

	Array<DeferedDataInitialiser> deferredData;
	ReferenceCountedObjectPtr<DspNetwork> ownedNetwork;
};

struct HostHelpers
{
    struct NoExtraComponent
    {
        static Component* createExtraComponent(void* , PooledUIUpdater*) { return nullptr; }
    };
    
	static int getNumMaxDataObjects(const ValueTree& v, snex::ExternalData::DataType t);

	static void setNumDataObjectsFromValueTree(OpaqueNode& on, const ValueTree& v);

	template <typename WrapperType> static NodeBase* initNodeWithNetwork(DspNetwork* p, ValueTree nodeTree, const ValueTree& embeddedNetworkTree, bool useMod)
	{
		auto t = dynamic_cast<WrapperType*>(WrapperType::template createNode<OpaqueNetworkHolder, NoExtraComponent, false, false>(p, nodeTree));

		auto& on = t->getWrapperType().getWrappedObject();
		setNumDataObjectsFromValueTree(on, embeddedNetworkTree);
		auto ed = t->setOpaqueDataEditor(useMod);

		auto onh = static_cast<OpaqueNetworkHolder*>(on.getObjectPtr());
		onh->setNetwork(p->getParentHolder()->addEmbeddedNetwork(p, embeddedNetworkTree, ed));

		ParameterDataList pList;
		onh->createParameters(pList);
		on.fillParameterList(pList);

		t->postInit();
		auto asNode = dynamic_cast<NodeBase*>(t);
		asNode->setEmbeddedNetwork(onh->getNetwork());

		return asNode;
	}
};


#if !USE_FRONTEND

struct DspNetworkListeners
{
	struct Base : public valuetree::AnyListener
	{
		template <bool AllowRootParameterChange> static bool isValueProperty(const ValueTree& v, const Identifier& id)
		{
			if (v.getType() == PropertyIds::Parameter && id == PropertyIds::Value)
			{
				if (AllowRootParameterChange)
				{
					auto possibleRoot = v.getParent().getParent().getParent();

					if (possibleRoot.getType() == PropertyIds::Network)
						return false;
				}

				return !v[PropertyIds::Automated];
			}

			return true;
		}

		Base(DspNetwork* n, bool isSync) :
			AnyListener(isSync ? valuetree::AsyncMode::Synchronously : valuetree::AsyncMode::Asynchronously),
			network(n),
			creationTime(Time::getMillisecondCounter())
		{}

		void postInit(bool sync)
		{
			if(!sync)
				setMillisecondsBetweenUpdate(1000);

			setRootValueTree(network->getValueTree());
		}

		virtual ~Base()
		{

		}

		void anythingChanged(CallbackType cb) override
		{
			auto t = Time::getMillisecondCounter();

			if ((t - creationTime) < 2000)
				return;

			changed = true;
			networkChanged();
		}

		virtual bool isChanged() const { return changed; }
		virtual void networkChanged() = 0;

	protected:

		uint32 creationTime;

		bool changed = false;
		WeakReference<DspNetwork> network;
	};

	struct LambdaAtNetworkChange : public Base
	{
		LambdaAtNetworkChange(scriptnode::DspNetwork* n) :
			Base(n, true)
		{
			setPropertyCondition(Base::isValueProperty<true>);
			postInit(true);
		}

		void networkChanged() override
		{
			if (additionalCallback)
				additionalCallback();
		}

		std::function<void()> additionalCallback;
	};

	struct PatchAutosaver : public Base
	{
		PatchAutosaver(scriptnode::DspNetwork* n, const File& directory) :
			Base(n, false)
		{
			setPropertyCondition(Base::isValueProperty<false>);
			postInit(false);

			auto id = "autosave_" + n->getRootNode()->getId();
			d = directory.getChildFile(id).withFileExtension("xml");
		}

		~PatchAutosaver()
		{
			if (d.existsAsFile())
				d.deleteFile();
		}

		bool isChanged() const override { return Base::isChanged() && d.existsAsFile(); }

		void closeAndDelete(bool forceDelete)
		{
			if (changed || forceDelete)
			{
				anythingChanged(valuetree::AnyListener::PropertyChange);

				auto ofile = d.getSiblingFile(network->getRootNode()->getId()).withFileExtension(".xml");
				auto backup = ofile.loadFileAsString();
				auto ok = ofile.deleteFile();

				if(ok)
					ok = d.moveFileTo(ofile);

				if (!ok)
					ofile.replaceWithText(backup);

				creationTime = Time::getMillisecondCounter();
			}
		}

		void networkChanged() override
		{
			auto saveCopy = network->getValueTree().createCopy();

			DspNetworkListeners::PatchAutosaver::removeDanglingConnections(saveCopy);

			valuetree::Helpers::forEach(saveCopy, stripValueTree);

			auto xml = saveCopy.createXml();

			d.replaceWithText(xml->createDocument(""));
		}

	private:

		static void removeIfDefault(ValueTree v, const Identifier& id, const var& defaultValue)
		{
			if (v[id] == defaultValue)
				v.removeProperty(id, nullptr);
		};

		static bool removePropIfDefault(ValueTree v, const Identifier& id, const var& defaultValue)
		{
			if (v[PropertyIds::ID].toString() == id.toString() &&
				v[PropertyIds::Value] == defaultValue)
			{
				return true;
			}

			return false;
		};

		static void removeIfNoChildren(ValueTree v)
		{
			if (v.isValid() && v.getNumChildren() == 0)
				v.getParent().removeChild(v, nullptr);
		}

		static void removeIfDefined(ValueTree v, const Identifier& id, const Identifier& definedId)
		{
			if (v.hasProperty(definedId))
				v.removeProperty(id, nullptr);
		}

	public:

		static void removeDanglingConnections(ValueTree& v);

		static bool stripValueTree(ValueTree& v);

		File d;
	};
};
#endif



}


