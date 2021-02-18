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

class NodeFactory;

struct Error
{
	enum ErrorCode
	{
		OK,
		NoMatchingParent,
		ChannelMismatch,
		BlockSizeMismatch,
		IllegalFrameCall,
		IllegalBlockSize,
		SampleRateMismatch,
		InitialisationError,
		numErrorCodes
	};

	ErrorCode error = ErrorCode::OK;
	int expected = 0;
	int actual = 0;
};

class ScriptnodeExceptionHandler
{
	struct Item
	{
		bool operator==(const Item& other) const
		{
			return node.get() == other.node.get();
		}

		String toString() const
		{
			if (node == nullptr || error.error == Error::OK)
				return {};
			else
			{
				String s;
				s << node->getCurrentId() << " - " << getErrorMessage(error);
				return s;
			}
		}

		WeakReference<NodeBase> node;
		Error error;
	};

public:

	bool isOk() const noexcept
	{
		return items.isEmpty();
	}

	void addError(NodeBase* n, Error e)
	{
		for (auto& i : items)
		{
			if (i.node == n)
			{
				i.error = e;
				return;
			}
				
		}

		items.add({ n, e });
	}

	void removeError(NodeBase* n)
	{
		Item i = { n, Error::OK };
		items.removeAllInstancesOf(i);
	}

	static String getErrorMessage(Error e)
	{
		String s;

		s << "**";

		switch (e.error)
		{
		case Error::ChannelMismatch: s << "Channel amount mismatch";  break;
		case Error::BlockSizeMismatch: s << "Blocksize mismatch"; break;
		case Error::IllegalFrameCall: s << "Can't be used in frame processing context"; return s;
		case Error::IllegalBlockSize: s << "Illegal block size: " << String(e.actual); return s;
		case Error::SampleRateMismatch: s << "Samplerate mismatch"; break;
		case Error::InitialisationError: return "Initialisation error";
		case Error::NoMatchingParent:	return "Can't find suitable parent node";
		default:
			break;
		}

		s << "**:  \n`" << String(e.actual) << "` (expected: `" << String(e.expected) << "`)";

		return s;
	}

	String getErrorMessage(NodeBase* n) const
	{
		for (auto& i : items)
		{
			if (i.node == n)
				return i.toString();
		}

		return {};
	}

private:


	Array<Item> items;
};


class SnexSource;




/** A network of multiple DSP objects that are connected using a graph. */
class DspNetwork : public ConstScriptingObject,
				   public Timer
{
public:

	struct VoiceSetter
	{
		VoiceSetter(DspNetwork& p, int newVoiceIndex):
			internalSetter(p.voiceIndex, newVoiceIndex)
		{}

	private:

		const snex::Types::PolyHandler::ScopedVoiceSetter internalSetter;
	};

	class Holder
	{
	public:

		virtual ~Holder() {};

		DspNetwork* getOrCreate(const ValueTree& v);

		DspNetwork* getOrCreate(const String& id);
		StringArray getIdList();
		void saveNetworks(ValueTree& d) const;
		void restoreNetworks(const ValueTree& d);

		virtual bool isPolyphonic() const { return false; };

		void setActiveNetwork(DspNetwork* n)
		{
			activeNetwork = n;
		}

		ScriptParameterHandler* getCurrentNetworkParameterHandler(const ScriptParameterHandler* contentHandler) const
		{
			if (auto n = getActiveNetwork())
			{
				if (n->isForwardingControlsToParameters())
					return const_cast<ScriptParameterHandler*>(static_cast<const ScriptParameterHandler*>(&n->networkParameterHandler));
			}

			return const_cast<ScriptParameterHandler*>(contentHandler);
		}

		DspNetwork* getActiveNetwork() const
		{
			return activeNetwork.get();
		}

		void setProjectDll(dll::ProjectDll::Ptr pdll)
		{
			projectDll = pdll;
		}

		dll::ProjectDll::Ptr projectDll;

		ExternalDataHolder* getExternalDataHolder()
		{
			return dataHolder;
		}

		void setExternalDataHolderToUse(ExternalDataHolder* newHolder)
		{
			dataHolder = newHolder;
		}

	protected:

		ExternalDataHolder* dataHolder = nullptr;

		WeakReference<DspNetwork> activeNetwork;

		ReferenceCountedArray<DspNetwork> networks;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
	};

	DspNetwork(ProcessorWithScriptingContent* p, ValueTree data, bool isPolyphonic);
	~DspNetwork();

	struct CodeManager
	{
		CodeManager(DspNetwork& p):
			parent(p)
		{
			
		}

		struct SnexSourceCompileHandler : public snex::ui::WorkbenchData::CompileHandler,
										  public ControlledObject,
									      public Thread
		{
			struct SnexCompileListener
			{
				virtual ~SnexCompileListener() {};

				/** This is called directly after the compilation (if it was OK) and can 
				    be used to run the test and update the UI display. */
				virtual void postCompileSync() = 0;

				JUCE_DECLARE_WEAK_REFERENCEABLE(SnexCompileListener);
			};

			SnexSourceCompileHandler(snex::ui::WorkbenchData* d, ProcessorWithScriptingContent* sp_);;

			void processTestParameterEvent(int parameterIndex, double value) final override {};
			void prepareTest(PrepareSpecs ps, const Array<snex::ui::WorkbenchData::TestData::ParameterEvent>& initialParameters) final override {};
			void processTest(ProcessDataDyn& data) final override {};

			void run() override;

			bool triggerCompilation() override;
			
			/** This returns the mutex used for synchronising the compilation. 
			
				Any access to a JIT compiled function must be locked using a read lock.

				The write lock will be held for a short period before compiling (where you need to reset 
				the state) and then after the compilation where you need to setup the object.
			*/
			SimpleReadWriteLock& getCompileLock() { return compileLock; }

			void addCompileListener(SnexCompileListener* l)
			{
				compileListeners.addIfNotAlreadyThere(l);
			}

			void removeCompileListener(SnexCompileListener* l)
			{
				compileListeners.removeAllInstancesOf(l);
			}

		private:

			ProcessorWithScriptingContent* sp;

			hise::SimpleReadWriteLock compileLock;

			Array<WeakReference<SnexCompileListener>> compileListeners;
		};

		snex::ui::WorkbenchData::Ptr getOrCreate(const Identifier& typeId, const Identifier& classId)
		{
			using namespace snex::ui;

			for (auto e : entries)
			{
				if (e->wb->getInstanceId() == classId && e->type == typeId)
					return e->wb;
			}

			auto targetFile = getCodeFolder().getChildFile(typeId.toString()).getChildFile(classId.toString()).withFileExtension("h");
			entries.add(new Entry(typeId, targetFile, parent.getScriptProcessor()));
			return entries.getLast()->wb;
		}

		ValueTree getParameterTree(const Identifier& typeId, const Identifier& classId)
		{
			for (auto e : entries)
			{
				if (e->type == typeId && e->wb->getInstanceId() == classId)
					return e->parameterTree;
			}

			jassertfalse;
			return {};
		}
		
		StringArray getClassList(const Identifier& id)
		{
			auto f = getCodeFolder();

			if (id.isValid())
				f = f.getChildFile(id.toString());

			StringArray sa;

			for (auto& l : f.findChildFiles(File::findFiles, true, "*.h"))
			{
				sa.add(l.getFileNameWithoutExtension());
			}

			return sa;
		}

	private:

		struct Entry
		{
			Entry(const Identifier& t, const File& targetFile, ProcessorWithScriptingContent* sp):
				type(t),
				parameterFile(targetFile.withFileExtension("xml"))
			{
				targetFile.create();

				cp = new snex::ui::WorkbenchData::DefaultCodeProvider(wb, targetFile);
				wb = new snex::ui::WorkbenchData();
				wb->setCodeProvider(cp, dontSendNotification);
				wb->setCompileHandler(new SnexSourceCompileHandler(wb, sp));

				if (ScopedPointer<XmlElement> xml = XmlDocument::parse(parameterFile))
					parameterTree = ValueTree::fromXml(*xml);
				else
					parameterTree = ValueTree(PropertyIds::Parameters);

				pListener.setCallback(parameterTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Entry::parameterAddedOrRemoved));
				propListener.setCallback(parameterTree, RangeHelpers::getRangeIds(), valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Entry::propertyChanged));
			}

			const Identifier type;
			const File parameterFile;

			ScopedPointer<snex::ui::WorkbenchData::CodeProvider> cp;

			

			snex::ui::WorkbenchData::Ptr wb;

			void parameterAddedOrRemoved(ValueTree, bool)
			{
				updateFile();
			}

			void propertyChanged(ValueTree, Identifier)
			{
				updateFile();
			}

			ValueTree parameterTree;
			
		private:

			void updateFile()
			{
				ScopedPointer<XmlElement> xml = parameterTree.createXml();
				parameterFile.replaceWithText(xml->createDocument(""));
			}

			valuetree::ChildListener pListener;
			valuetree::RecursivePropertyListener propListener;
		};

		OwnedArray<Entry> entries;

		File getCodeFolder() const;

		DspNetwork& parent;
	} codeManager;

	void setNumChannels(int newNumChannels);

	Identifier getObjectName() const override { return "DspNetwork"; };

	String getDebugName() const override { return "DspNetwork"; }
	String getDebugValue() const override { return getId(); }
	void rightClickCallback(const MouseEvent& e, Component* c) override;

	NodeBase* getNodeForValueTree(const ValueTree& v);
	NodeBase::List getListOfUnconnectedNodes() const;

	ValueTree getListOfAvailableModulesAsTree() const;

	StringArray getListOfAllAvailableModuleIds() const;
	StringArray getListOfUsedNodeIds() const;
	StringArray getListOfUnusedNodeIds() const;
	StringArray getFactoryList() const;

	void registerOwnedFactory(NodeFactory* ownedFactory);

	NodeBase::List getListOfNodesWithPath(const NamespacedIdentifier& id, bool includeUnusedNodes)
	{
		NodeBase::List list;

		for (auto n : nodes)
		{
			auto path = n->getPath();

			if ((includeUnusedNodes || isInSignalPath(n)) && path == id)
				list.add(n);
		}

		return list;
	}

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

	void process(AudioSampleBuffer& b, HiseEventBuffer* e);

	bool isPolyphonic() const { return isPoly; }

	NodeBase* getRootNode() { return signalPath.get(); }
	const NodeBase* getRootNode() const { return signalPath.get(); }

	void setRootNode(NodeBase::Ptr newRootNode)
	{
		signalPath = newRootNode;
	}

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

	/** Returns a reference to the node with the given id. */
	var get(var id) const;

	/** Any scripting API call has to be checked using this method. */
	void checkValid() const
	{
		if (parentHolder == nullptr)
			reportScriptError("Parent of DSP Network is deleted");
	}

	/** Deletes the node if it is not in a signal path. */
	bool deleteIfUnused(String id);

	String getId() const { return data[PropertyIds::ID].toString(); }

	ValueTree getValueTree() const { return data; };

	CriticalSection& getConnectionLock() { return connectLock; }
	bool updateIdsInValueTree(ValueTree& v, StringArray& usedIds);
	NodeBase* createFromValueTree(bool createPolyIfAvailable, ValueTree d, bool forceCreate=false);
	bool isInSignalPath(NodeBase* b) const;

	bool isCurrentlyRenderingVoice() const noexcept { return isPolyphonic() && voiceIndex.getVoiceIndex() != -1; }

	bool isRenderingFirstVoice() const noexcept { return !isPolyphonic() || voiceIndex.getVoiceIndex() == 0; }

	bool isForwardingControlsToParameters() const
	{
		return forwardControls;
	}

	PrepareSpecs getCurrentSpecs() const { return currentSpecs; }

	NodeBase* getNodeWithId(const String& id) const;

	struct SelectionListener
	{
		virtual ~SelectionListener() {};
		virtual void selectionChanged(const NodeBase::List& selection) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SelectionListener);
	};

	void addSelectionListener(SelectionListener* l) { if(selectionUpdater != nullptr) selectionUpdater->listeners.addIfNotAlreadyThere(l); }
	void removeSelectionListener(SelectionListener* l) { if(selectionUpdater != nullptr) selectionUpdater->listeners.removeAllInstancesOf(l); }

	bool isSelected(NodeBase* node) const { return selection.isSelected(node); }

	void deselect(NodeBase* node)
	{
		selection.deselect(node);
	}

	void deselectAll() { selection.deselectAll(); }

	void addToSelection(NodeBase* node, ModifierKeys mods);

	NodeBase::List getSelection() const { return selection.getItemArray(); }

	struct NetworkParameterHandler : public hise::ScriptParameterHandler
	{
		int getNumParameters() const final override { return root->getNumParameters(); }
		Identifier getParameterId(int index) const final override
		{
			return Identifier(root->getParameter(index)->getId());
		}

		float getParameter(int index) const final override
		{
			if(isPositiveAndBelow(index, getNumParameters()))
				return (float)root->getParameter(index)->getValue();

			return 0.0f;
		}

		void setParameter(int index, float newValue) final override
		{
			if(isPositiveAndBelow(index, getNumParameters()))
				root->getParameter(index)->setValueAndStoreAsync((double)newValue);
		}

		NodeBase::Ptr root;
	} networkParameterHandler;

	ValueTree cloneValueTreeWithNewIds(const ValueTree& treeToClone);

	void setEnableUndoManager(bool shouldBeEnabled)
	{
		enableUndo = shouldBeEnabled;
		if (enableUndo)
		{
			startTimer(1500);
		}
		else
			stopTimer();
	}

	ScriptnodeExceptionHandler& getExceptionHandler()
	{
		return exceptionHandler;
	}

	//int* getVoiceIndexPtr() { return &voiceIndex; }

	void timerCallback() override
	{
		um.beginNewTransaction();
	}

	void changeNodeId(ValueTree& c, const String& oldId, const String& newId, UndoManager* um);

	UndoManager* getUndoManager()
	{ 
		if (!enableUndo)
			return nullptr;

		if (um.isPerformingUndoRedo())
			return nullptr;
		else
			return &um;
	}

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

private:

	bool enableCpuProfiling = false;

	WeakReference<ExternalDataHolder> dataHolder;

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

	snex::Types::PolyHandler voiceIndex;

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

	ScopedPointer<SelectionUpdater> selectionUpdater;

	OwnedArray<NodeFactory> ownedFactories;

	Array<WeakReference<NodeFactory>> nodeFactories;

	String getNonExistentId(String id, StringArray& usedIds) const;

	valuetree::RecursivePropertyListener idUpdater;

	CriticalSection connectLock;

	WeakReference<Holder> parentHolder;

	ValueTree data;

	float* currentData[NUM_MAX_CHANNELS];
	friend class DspNetworkGraph;

	struct Wrapper;

	DynamicObject::Ptr loader;

	ReferenceCountedArray<NodeBase> nodes;

	ReferenceCountedObjectPtr<NodeBase> signalPath;

	

	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetwork);
};



}


