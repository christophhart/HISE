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

	bool isPolyphonic() const { return isPoly; }

	void setPolyphonic(bool shouldBePolyphonic) { isPoly = shouldBePolyphonic; voiceIndex = isPoly ? -1 : 0; }

	NodeBase* getRootNode() { return signalPath.get(); }

	Identifier getParameterIdentifier(int parameterIndex);

	// ===============================================================================

	/** Initialise processing of all nodes. */
	void prepareToPlay(double sampleRate, double blockSize);

	/** Process the given channel array with the node network. */
	void processBlock(var data);

	/** Creates a node. If a node with the id already exists, it returns this node. */
	var create(String path, String id, bool createPolyNode);

	/** Returns a reference to the node with the given id. */
	var get(String id) const;

	String getId() const { return data[PropertyIds::ID].toString(); }

	ValueTree getValueTree() const { return data; };

	CriticalSection& getConnectionLock() { return connectLock; }
	bool updateIdsInValueTree(ValueTree& v, StringArray& usedIds);
	NodeBase* createFromValueTree(bool createPolyIfAvailable, ValueTree d, bool forceCreate=false);
	bool isInSignalPath(NodeBase* b) const;

	bool isCurrentlyRenderingVoice() const { return isPolyphonic() && voiceIndex != -1; }

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

	struct NetworkParameterHandler : public hise::ScriptParameterHandler
	{
		int getNumParameters() const final override { return root->getNumParameters(); }
		Identifier getParameterId(int index) const final override
		{
			return Identifier(root->getParameter(index)->getId());
		}

		float getParameter(int index) const final override 
		{ 
			return (float)root->getParameter(index)->getValue();
		}

		void setParameter(int index, float newValue) final override
		{
			root->getParameter(index)->setValueAndStoreAsync((double)newValue);
		}

		NodeBase::Ptr root;
	} networkParameterHandler;

	ValueTree cloneValueTreeWithNewIds(const ValueTree& treeToClone);

private:



	bool isPoly = false;

	int voiceIndex = 0;

	SelectedItemSet<NodeBase::Ptr> selection;

	struct SelectionUpdater : public ChangeListener
	{
		SelectionUpdater(DspNetwork& parent_);
		~SelectionUpdater();

		void changeListenerCallback(ChangeBroadcaster* ) override;

		Array<WeakReference<SelectionListener>> listeners;

		DspNetwork& parent;

	} selectionUpdater;

	OwnedArray<NodeFactory> nodeFactories;

	String getNonExistentId(String id, StringArray& usedIds) const;

	void updateId(NodeBase* n, const String& newId);

	

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

#define SCRIPTNODE_FACTORY(x, id) static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new x(n, d); }; \
								  static Identifier getStaticId() { return Identifier(id); };

class NodeFactory
{
public:

	NodeFactory(DspNetwork* n) :
		network(n)
	{};

    virtual ~NodeFactory() {};
    
	using CreateCallback = std::function<NodeBase*(DspNetwork*, ValueTree)>;
	using PostCreateCallback = std::function<void(NodeBase* n)>;
	using IdFunction = std::function<Identifier()>;

	StringArray getModuleList() const
	{
		StringArray sa;
		String prefix = getId().toString() + ".";

		for (const auto& item : monoNodes)
		{
			sa.add(prefix + item.id().toString());
		}

		return sa;
	}

	template <class T> void registerNodeRaw(const PostCreateCallback& cb = {})
	{
		Item newItem;
		newItem.cb = T::createNode;
		newItem.id = T::getStaticId;
		newItem.pb = cb;

		monoNodes.add(newItem);
	}

	template <class T> void registerNode(const PostCreateCallback& cb = {})
	{
		using WrappedT = HiseDspNodeBase<T>;

        Item newItem;
        newItem.cb = WrappedT::createNode;
        newItem.id = WrappedT::getStaticId;
        newItem.pb = cb;
        
		monoNodes.add(newItem);
	};

	template <class MonoT, class PolyT> void registerPolyNode(const PostCreateCallback& cb = {})
	{
		using WrappedPolyT = HiseDspNodeBase<PolyT>;
		using WrappedMonoT = HiseDspNodeBase<MonoT>;

		{
			Item newItem;
			newItem.cb = WrappedPolyT::createNode;
			newItem.id = WrappedPolyT::getStaticId;
			newItem.pb = cb;

			polyNodes.add(newItem);
		}

		{
			Item newItem;
			newItem.cb = WrappedMonoT::createNode;
			newItem.id = WrappedMonoT::getStaticId;
			newItem.pb = cb;

			monoNodes.add(newItem);
		}
	};

	virtual Identifier getId() const = 0;

	NodeBase* createNode(ValueTree data, bool createPolyIfAvailable) const;

	void setNetworkToUse(DspNetwork* n)
	{
		network = n;
	}

private:

	struct Item
	{
		CreateCallback cb;
		IdFunction id;
		PostCreateCallback pb;
	};

	Array<Item> monoNodes;
	Array<Item> polyNodes;

	WeakReference<DspNetwork> network;
};

class SingletonFactory : public NodeFactory,
	public juce::DeletedAtShutdown
{
public:

	SingletonFactory() :
		NodeFactory(nullptr)
	{};
};

#define DEFINE_FACTORY_FOR_NAMESPACE NodeFactory* Factory::instance = nullptr; \
	NodeFactory* Factory::getInstance(DspNetwork* n) \
	{ if (instance == nullptr) instance = new Factory(); return instance; }

#define DECLARE_SINGLETON_FACTORY_FOR_NAMESPACE(name) class Factory : private SingletonFactory \
{ \
public: \
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER(#name); } \
	static NodeFactory* getInstance(DspNetwork* n); \
	static NodeFactory* instance; \
}; 


template <class FactoryClass, class T> struct RegisterAtFactory
{
	RegisterAtFactory() { FactoryClass::getInstance(nullptr)->registerNode<T>(); }
};

template <class FactoryClass, class T, class PolyT> struct PolyRegisterAtFactory
{
	PolyRegisterAtFactory() { FactoryClass::getInstance(nullptr)->registerPolyNode<T, PolyT>(); }
};

#define REGISTER_POLY PolyRegisterAtFactory<Factory, instance<1>, instance<NUM_POLYPHONIC_VOICES>> reg;
#define REGISTER_MONO RegisterAtFactory<Factory, instance> reg;

}


