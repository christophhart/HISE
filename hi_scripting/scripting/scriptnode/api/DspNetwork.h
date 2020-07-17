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

/** A network of multiple DSP objects that are connected using a graph. */
class DspNetwork : public ConstScriptingObject,
				   public Timer
{
public:

	struct VoiceSetter
	{
		VoiceSetter(DspNetwork& p, int newVoiceIndex):
			n(p),
			previous(p.voiceIndex)
		{
			jassert(previous == -1);
			n.voiceIndex = newVoiceIndex;
		}

		~VoiceSetter()
		{
			n.voiceIndex = previous;
		}

	private:

		DspNetwork& n;
		int previous;
	};

	class Holder
	{
	public:

		virtual ~Holder() {};

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

	protected:

		WeakReference<DspNetwork> activeNetwork;

		ReferenceCountedArray<DspNetwork> networks;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
	};

	DspNetwork(ProcessorWithScriptingContent* p, ValueTree data, bool isPolyphonic);
	~DspNetwork();

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

	bool isCurrentlyRenderingVoice() const noexcept { return isPolyphonic() && voiceIndex != -1; }

	bool isRenderingFirstVoice() const noexcept { return !isPolyphonic() || voiceIndex == 0; }

	bool isForwardingControlsToParameters() const
	{
		return forwardControls;
	}

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

	int* getVoiceIndexPtr() { return &voiceIndex; }

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

private:

#if USE_BACKEND
	bool enableUndo = true;
#else
	// disable undo on compiled plugins unless explicitely stated
	bool enableUndo = false; 
#endif

	bool forwardControls = true;

	UndoManager um;

	const bool isPoly;

	int voiceIndex;

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


