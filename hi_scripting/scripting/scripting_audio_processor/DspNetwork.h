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

#ifndef SCRIPTEDAUDIOPROCESSOR_H_INCLUDED
#define SCRIPTEDAUDIOPROCESSOR_H_INCLUDED

namespace hise
{

class ProcessorWithScriptingContent;

namespace scriptnode
{
using namespace juce;

#define DECLARE_ID(x) static const Identifier x(#x);

namespace PropertyIds
{
DECLARE_ID(Network);
DECLARE_ID(Node);
DECLARE_ID(HiseFXNode);
DECLARE_ID(ID);
DECLARE_ID(Type);
DECLARE_ID(Folded);
DECLARE_ID(FactoryPath);
DECLARE_ID(SerialNode);
DECLARE_ID(MultiChannelNode);
DECLARE_ID(SplitNode);
DECLARE_ID(DspNode);
DECLARE_ID(NumChannels);
}

#undef DECLARE_ID

class DspNetwork;

struct ProcessData
{
	float** data = nullptr;
	int numChannels = 0;
	int size = 0;
};

class NodeComponent;

struct NodeBase: public ConstScriptingObject
{
	class AsyncPropertyListener: private ValueTree::Listener,
								 private SafeChangeBroadcaster,
							     private SafeChangeListener
	{
	public:

		using ExternalCallback = std::function<void(int index, const var& newValue)>;

		AsyncPropertyListener(NodeBase& parent_);

		void registerIdWithCallback(int index, const Identifier& id, var defaultValue, const ExternalCallback& callback)
		{
			if (parent.data.hasProperty(id))
			{
				defaultValue = parent.data[id];
				callback(index, defaultValue);
			}
			else
				parent.data.setPropertyExcludingListener(this, id, defaultValue, nullptr);

			registeredIds.set(index, { index, id, defaultValue, callback });

			jassert(registeredIds[index].index == index);
		}

		void changePropertyAndNotifyAsync(int index, const var& newValue)
		{
			if (isPositiveAndBelow(index, registeredIds.size()))
			{
				auto& r = registeredIds.getReference(index);

				if (r.value != newValue)
				{
					r.value = newValue;
					r.changedFromExternal = true;
					sendPooledChangeMessage();
				}
			}
		}

	private:

		struct ValueChange
		{
			ValueChange() {};

			ValueChange(int index_, Identifier id_, const var& defaultValue, const ExternalCallback& callback_):
				index(index_),
				id(id_),
				value(defaultValue),
				callback(callback_)
			{}

			int index;
			Identifier id;
			var value;
			ExternalCallback callback;
			bool changedFromValueTree = false;
			bool changedFromExternal = false;
		};

		void changeListenerCallback(SafeChangeBroadcaster*) override
		{
			for (auto& c : registeredIds)
			{
				if (c.changedFromValueTree && c.callback)
				{
					c.callback(c.index, c.value);
					c.changedFromValueTree = false;
				}

				if (c.changedFromExternal)
				{
					parent.data.setPropertyExcludingListener(this, c.id, c.value, parent.getUndoManager());
					c.changedFromExternal = false;
				}
			}
		}

		void valueTreePropertyChanged(ValueTree& v, const Identifier& id) override 
		{
			if (v != parent.data)
				return;

			for (auto& c : registeredIds)
			{
				if (c.id == id)
				{
					c.value = v[id];
					c.changedFromValueTree = true;
					sendPooledChangeMessage();
				}
			}
		}

		void valueTreeChildAdded(ValueTree&, ValueTree&) override {};
		void valueTreeChildRemoved(ValueTree&, ValueTree&, int) override {};
		void valueTreeChildOrderChanged(ValueTree&, int, int) override {};
		void valueTreeParentChanged(ValueTree&) override {};

		Array<ValueChange> registeredIds;

		NodeBase& parent;
	};

	static constexpr int HeaderHeight = 24;
	static constexpr int NodeWidth = 256;
	static constexpr int NodeHeight = 48;
	static constexpr int NodeMargin = 10;

	NodeBase(DspNetwork* rootNetwork, ValueTree data, int numConstants);;

	using Ptr = WeakReference<NodeBase>;
	using List = Array<WeakReference<NodeBase>>;

	DspNetwork* getRootNetwork() const;

	virtual ~NodeBase() {}
	
	virtual void process(ProcessData& data) = 0;

	virtual void prepare(double sampleRate, int blockSize) = 0;

	bool isConnected() const { return data.getParent().isValid(); }

	void setDefaultValue(const Identifier& id, var newValue);

	virtual NodeComponent* createComponent();

	virtual Rectangle<int> getPositionInCanvas(Point<int> topLeft) const
	{
		return Rectangle<int>(NodeWidth, NodeHeight)
			.withPosition(topLeft)
			.reduced(NodeMargin);
	}

	ValueTree getValueTree() const { return data; }

	String getId() const { return data[PropertyIds::ID].toString(); }

	UndoManager* getUndoManager();

	Rectangle<int> reduceHeightIfFolded(Rectangle<int> originalHeight) const;

	int getNumChannelsToProcess() const { return (int)data[PropertyIds::NumChannels]; };

protected:

	ValueTree data;

private:

	WeakReference<ConstScriptingObject> parent;
	WeakReference<NodeBase> parentNode;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeBase);
};

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

		StringArray getIdList()
		{
			StringArray sa;

			for (auto n : networks)
				sa.add(n->getId());

			return sa;
		}

		void saveNetworks(ValueTree& d) const;

		void restoreNetworks(const ValueTree& d);

	protected:

		ReferenceCountedArray<DspNetwork> networks;
	};

	DspNetwork(ProcessorWithScriptingContent* p, ValueTree data);
	~DspNetwork();

	void setNumChannels(int newNumChannels)
	{
		signalPath->getValueTree().setProperty(PropertyIds::NumChannels, newNumChannels, nullptr);
	}

	Identifier getObjectName() const override { return "DspNetwork"; };

	String getDebugName() const override { return "DSP Network"; }
	String getDebugValue() const override { return getId(); }

	void rightClickCallback(const MouseEvent& e, Component* c) override;

	NodeBase* getNodeForValueTree(const ValueTree& v);

	NodeBase::List getListOfUnconnectedNodes() const;

	StringArray getListOfAllAvailableModuleIds() const;

	// ===============================================================================

	/** Initialise processing of all nodes. */
	void prepareToPlay(double sampleRate, double blockSize);

	/** Process the given channel array with the node network. */
	void processBlock(var data);

	/** Creates a node. If a node with the id already exists, it returns this node. */
	var create(String path, String id);

	/** Returns a reference to the node with the given id. */
	var get(String id);

	String getId() const { return data[PropertyIds::ID].toString(); }

	ValueTree getValueTree() const { return data; };

	CriticalSection& getConnectionLock() { return connectLock; }

private:

	CriticalSection connectLock;

	ValueTree data;

	NodeBase* createFromValueTree(ValueTree d);

	float* currentData[NUM_MAX_CHANNELS];
	friend class DspNetworkGraph;

	struct Wrapper;

	DynamicObject::Ptr loader;

	ReferenceCountedArray<NodeBase> nodes;

	ReferenceCountedObjectPtr<NodeBase> signalPath;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetwork);
};

struct NodeContainer : public NodeBase,
					   public ValueTree::Listener,
					   public AssignableObject
{
	enum ChangeAction
	{
		Insert,
		Remove
	};

	NodeContainer(DspNetwork* root, ValueTree data);
	~NodeContainer();

	void prepare(double sampleRate, int blockSize) override
	{
		for (auto n : nodes)
			n->prepare(sampleRate, blockSize);
	}

	virtual void assign(const int index, var newValue) override;

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	var getAssignedValue(int index) const override
	{
		return var(nodes[index]);
	}

	virtual int getCachedIndex(const var &indexExpression) const override
	{
		return (int)indexExpression;
	}

	// ===================================================================================

	void clear()
	{
		getValueTree().removeAllChildren(getUndoManager());
	}

	void valueTreePropertyChanged(ValueTree& , const Identifier& ) override {}
	void valueTreeChildAdded(ValueTree& parentTree, ValueTree& child) override;
	void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& child, int) override;
	void valueTreeChildOrderChanged(ValueTree& , int , int ) override {};
	void valueTreeParentChanged(ValueTree&) override {};

protected:

	friend class ContainerComponent;

	List nodes;
};

class SerialNode : public NodeContainer
{
public:

	static constexpr int PinHeight = 24;

	SerialNode(DspNetwork* root, ValueTree data);

	Identifier getObjectName() const override { return "SerialNode"; };

	NodeComponent* createComponent() override;

	void process(ProcessData& data) final override;

	void valueTreePropertyChanged(ValueTree& v, const Identifier& id) override
	{
		if (v == data && id == PropertyIds::NumChannels)
		{
			for (auto n : nodes)
				n->getValueTree().setProperty(PropertyIds::NumChannels, v[id], nullptr);
		}
	}

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;
};

class ParallelNode : public NodeContainer
{
public:

	ParallelNode(DspNetwork* root, ValueTree data):
		NodeContainer(root, data)
	{}

	NodeComponent* createComponent() override;

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;
};

class SplitNode : public ParallelNode
{
public:

	SplitNode(DspNetwork* root, ValueTree data) :
		ParallelNode(root, data)
	{};

	Identifier getObjectName() const override { return "SplitNode"; };

	void prepare(double sampleRate, int blockSize) override
	{
		NodeContainer::prepare(sampleRate, blockSize);
		numSamples = blockSize;
		updateBuffer();
	}

	void process(ProcessData& data) final override
	{
		internalData.set(0, data);

		if (numChannels > 1)
		{
			for (int i = 1; i < numChannels; i++)
			{
				ProcessData d;
				d.numChannels = 2;
				d.size = data.size;
				float* dt[2] = { additionalChannels.getWritePointer(i), additionalChannels.getWritePointer(i + 1) };

				FloatVectorOperations::copy(dt[0], data.data[0], data.size);
				FloatVectorOperations::copy(dt[1], data.data[1], data.size);

				d.data = dt;

				internalData.set(i, d);
			}
		}

		for (int i = 0; i < nodes.size(); i++)
		{
			nodes[i]->process(internalData[i]);
		}

		for (int i = 1; i < internalData.size(); i++)
		{
			FloatVectorOperations::copy(data.data[0], internalData[i].data[0], data.size);
			FloatVectorOperations::copy(data.data[1], internalData[i].data[1], data.size);
		}
	}

private:

	void updateBuffer()
	{
		if ((numChannels - 1) != additionalChannels.getNumChannels())
		{
		}

		internalData.clear();
		internalData.ensureStorageAllocated(numChannels);
	}

	int numChannels = 1;
	int numSamples = 0;
	Array<ProcessData> internalData;
	AudioSampleBuffer additionalChannels;
};

class MultiChannelNode : public ParallelNode
{
public:

	MultiChannelNode(DspNetwork* root, ValueTree data) :
		ParallelNode(root, data)
	{};

	void process(ProcessData& data) override
	{
		int channelIndex = 0;

		for (auto n : nodes)
		{
			int numChannelsThisTime = n->getNumChannelsToProcess();
			int startChannel = channelIndex;
			int endChannel = startChannel + numChannelsThisTime;

			if (endChannel < data.numChannels)
			{
				for (int i = 0; i < numChannelsThisTime; i++)
					currentChannelData[i] = data.data[startChannel + i];

				ProcessData thisData;
				thisData.data = currentChannelData;
				thisData.numChannels = numChannelsThisTime;
				thisData.size = data.size;

				n->process(thisData);
			}

			channelIndex += numChannelsThisTime;
		}
	}

	float* currentChannelData[NUM_MAX_CHANNELS];

	Identifier getObjectName() const override { return "MultiChannelNode"; };
};

class DspNode : public NodeBase,
				public AssignableObject
{
public:

	DspNode(DspNetwork* root, DspFactory* f_, ValueTree data);

	Identifier getObjectName() const override { return "DspNode"; }

	virtual void assign(const int index, var newValue) override
	{
		if (obj != nullptr && isPositiveAndBelow(index, obj->getNumParameters()))
		{
			auto floatValue = (float)newValue;
			FloatSanitizers::sanitizeFloatNumber(floatValue);

			obj->setParameter(index, floatValue);
			parameterUpdater.changePropertyAndNotifyAsync(index, newValue);
		}
		else
			reportScriptError("Cant' find parameter for index " + String(index));
	}

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	var getAssignedValue(int index) const override
	{
		if (obj != nullptr && isPositiveAndBelow(index, obj->getNumParameters()))
		{
			return obj->getParameter(index);
		}
		else
			reportScriptError("Cant' find parameter for index " + String(index));
	}

	virtual int getCachedIndex(const var &indexExpression) const override
	{
		return (int)indexExpression;
	}

	void prepare(double sampleRate, int blockSize) override
	{
		if (obj != nullptr)
			obj->prepareToPlay(sampleRate, blockSize);
	}

	void process(ProcessData& data) final override
	{
		if (obj != nullptr)
			obj->processBlock(data.data, data.numChannels, data.size);
	}

	~DspNode()
	{
		f->destroyDspBaseObject(obj);
	}

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
	{
		if (obj != nullptr)
		{
			int numParameters = obj->getNumParameters();

			int numRows = std::ceil((float)numParameters / 2.0f);

			auto b = Rectangle<int>(0, 0, 256, numRows * 48 + NodeBase::HeaderHeight);

			return b.expanded(NodeBase::NodeMargin).withPosition(topLeft);
		}
	}

	NodeComponent* createComponent() override;

private:

	AsyncPropertyListener parameterUpdater;

	friend class DspNodeComponent;

	String moduleName;

	void initialise();

	DspFactory::Ptr f;
	DspBaseObject* obj = nullptr;

};





class HiseFXNode;



}
}


#endif  // SCRIPTEDAUDIOPROCESSOR_H_INCLUDED
