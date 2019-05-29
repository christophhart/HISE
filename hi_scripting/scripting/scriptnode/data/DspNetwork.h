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
	public DebugableObject
{
public:

	class Holder
	{
	public:

		virtual ~Holder() {};

		DspNetwork* getOrCreate(const String& id);
		StringArray getIdList();
		void saveNetworks(ValueTree& d) const;
		void restoreNetworks(const ValueTree& d);

		void setActiveNetwork(DspNetwork* n)
		{
			activeNetwork = n;
		}

		DspNetwork* getActiveNetwork() const
		{
			return activeNetwork.get();
		}

	protected:

		WeakReference<DspNetwork> activeNetwork;

		ReferenceCountedArray<DspNetwork> networks;
	};

	DspNetwork(ProcessorWithScriptingContent* p, ValueTree data);
	~DspNetwork();

	void setNumChannels(int newNumChannels);

	Identifier getObjectName() const override { return "DspNetwork"; };

	String getDebugName() const override { return "DSP Network"; }
	String getDebugValue() const override { return getId(); }
	void rightClickCallback(const MouseEvent& e, Component* c) override;

	NodeBase* getNodeForValueTree(const ValueTree& v);
	NodeBase::List getListOfUnconnectedNodes() const;

	StringArray getListOfAllAvailableModuleIds() const;
	StringArray getListOfUsedNodeIds() const;
	StringArray getListOfUnusedNodeIds() const;

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

	// ===============================================================================

	/** Initialise processing of all nodes. */
	void prepareToPlay(double sampleRate, double blockSize);

	/** Process the given channel array with the node network. */
	void processBlock(var data);

	/** Creates a node. If a node with the id already exists, it returns this node. */
	var create(String path, String id);

	/** Returns a reference to the node with the given id. */
	var get(String id) const;

	String getId() const { return data[PropertyIds::ID].toString(); }

	ValueTree getValueTree() const { return data; };

	CriticalSection& getConnectionLock() { return connectLock; }
	void updateIdsInValueTree(ValueTree& v, StringArray& usedIds);
	NodeBase* createFromValueTree(ValueTree d, bool forceCreate=false);
	bool isInSignalPath(NodeBase* b) const;

	NodeBase* getNodeWithId(const String& id) const;

	struct SelectionListener
	{
		virtual ~SelectionListener() {};
		virtual void selectionChanged(const NodeBase::List& selection) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SelectionListener);
	};

	void addSelectionListener(SelectionListener* l) { selectionUpdater.listeners.addIfNotAlreadyThere(l); }
	void removeSelectionListener(SelectionListener* l) { selectionUpdater.listeners.removeAllInstancesOf(l); }

	bool isSelected(NodeBase* node) const { return selection.isSelected(node); }

	void addToSelection(NodeBase* node, ModifierKeys mods);

	NodeBase::List getSelection() const { return selection.getItemArray(); }

private:

	SelectedItemSet<NodeBase::Ptr> selection;

	struct SelectionUpdater : public ChangeListener
	{
		SelectionUpdater(DspNetwork& parent_) :
			parent(parent_)
		{
			WeakReference<DspNetwork> weakParent;

			auto f = [weakParent, this]()
			{
				if (weakParent != nullptr)
					weakParent.get()->selection.addChangeListener(this);
			};

			MessageManager::callAsync(f);
		}

		~SelectionUpdater()
		{
			parent.selection.removeChangeListener(this);
		}

		void changeListenerCallback(ChangeBroadcaster* ) override
		{
			auto& list = parent.selection.getItemArray();
			
			for (auto l : listeners)
			{
				if (l != nullptr)
					l->selectionChanged(list);
			}
		}

		Array<WeakReference<SelectionListener>> listeners;

		DspNetwork& parent;

	} selectionUpdater;

	OwnedArray<NodeFactory> nodeFactories;

	String getNonExistentId(String id, StringArray& usedIds) const;

	CriticalSection connectLock;

	ValueTree data;

	float* currentData[NUM_MAX_CHANNELS];
	friend class DspNetworkGraph;

	struct Wrapper;

	DynamicObject::Ptr loader;

	ReferenceCountedArray<NodeBase> nodes;

	ReferenceCountedObjectPtr<NodeBase> signalPath;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetwork);
};

#define SCRIPTNODE_FACTORY(x, id) static NodeBase* createNode(DspNetwork* n, ValueTree& d) { return new x(n, d); }; \
								  static Identifier getStaticId() { return Identifier(id); };

class NodeFactory
{
public:

	NodeFactory(DspNetwork* n) :
		network(n)
	{};

	using CreateCallback = std::function<NodeBase*(DspNetwork*, ValueTree)>;
	using PostCreateCallback = std::function<void(NodeBase* n)>;
	using IdFunction = std::function<Identifier()>;

	StringArray getModuleList() const
	{
		StringArray sa;
		String prefix = getId().toString() + ".";

		for (const auto& item : registeredItems)
		{
			sa.add(prefix + item.id().toString());
		}

		return sa;
	}

	template <class T> void registerNode(const PostCreateCallback& cb = {})
	{
		registeredItems.add({ T::createNode, T::getStaticId, cb });
	};

	virtual Identifier getId() const = 0;

	NodeBase* createNode(ValueTree data) const;

private:

	struct Item
	{
		CreateCallback cb;
		IdFunction id;
		PostCreateCallback pb;
	};

	Array<Item> registeredItems;

	WeakReference<DspNetwork> network;
};

}


