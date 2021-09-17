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
 *   which also must be licensed for commercial applications:
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

class DspNetwork;
struct NodeHolder;
class NodeComponent;
class HardcodedNode;
class RestorableNode;
class NodeBase;

struct HelpManager : ControlledObject
{
	HelpManager(NodeBase* parent, ValueTree d);

	struct Listener
	{
		virtual ~Listener() {};

		virtual void helpChanged(float newWidth, float newHeight) = 0;

		virtual void repaintHelp() = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	void update(Identifier id, var newValue);
	void render(Graphics& g, Rectangle<float> area);
	void addHelpListener(Listener* l);
	void removeHelpListener(Listener* l);

	Rectangle<float> getHelpSize() const;

private:

	void rebuild();

	String lastText;
	Colour highlightColour = Colour(SIGNAL_COLOUR);

	float lastWidth = 300.0f;
	float lastHeight = 0.0f;

	Array<WeakReference<HelpManager::Listener>> listeners;
	ScopedPointer<hise::MarkdownRenderer> helpRenderer;
	valuetree::PropertyListener commentListener;
	valuetree::PropertyListener colourListener;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HelpManager);
};

/** A parameter of a node. */
class Parameter : public ConstScriptingObject
{
public:

	/** Create an object of this type if you don't want to remove any connections when the node is
	    removed from the signal chain (eg. when dragging the node around). 
	*/
	struct ScopedAutomationPreserver
	{
		ScopedAutomationPreserver(NodeBase* n);

		~ScopedAutomationPreserver();

		static bool isPreservingRecursive(NodeBase* n);

	private:

		NodeBase* parent;
		bool prevValue;
	};

	Identifier getObjectName() const override { return PropertyIds::Parameter; }

	Parameter(NodeBase* parent_, const ValueTree& data_);;

	// ======================================================================== API Calls

	/** Adds (and/or) returns a connection from the given data. */
	var addConnectionFrom(var connectionData);

	/** Returns the name of the parameter. */
	String getId() const;

	/** Returns the current value. */
	double getValue() const;

	/** Sets the value immediately and stores it asynchronously. */
	void setValueAndStoreAsync(double newValue);

	// ================================================================== End of API Calls

	ValueTree getConnectionSourceTree(bool forceUpdate);

	bool matchesConnection(const ValueTree& c) const;

	Array<Parameter*> getConnectedMacroParameters() const;

	parameter::dynamic_base_holder* getDynamicParameterAsHolder()
	{
		return dynamic_cast<parameter::dynamic_base_holder*>(dynamicParameter.get());
	}

	parameter::dynamic_base::Ptr getDynamicParameter() const
	{
		jassert(dynamicParameter != nullptr);
		return dynamicParameter;
	}

	void setCallbackNew(parameter::dynamic_base* ownedNew);

	StringArray valueNames;
	NodeBase* parent;
	

	void updateFromValueTree(Identifier, var newValue)
	{
		setValueAndStoreAsync((double)newValue);
	}

	ValueTree data;

	bool isProbed = false;

	void ensureAfterValueCallback(valuetree::PropertyListener& l)
	{
		l.setHighPriorityListener(&valuePropertyUpdater);
	}

	bool isModulated() const { return (bool)data.getProperty(PropertyIds::ModulationTarget, false); }

    
    
private:


	void updateConnectionOnRemoval(ValueTree& c);

	parameter::dynamic_base::Ptr dynamicParameter;

	ValueTree connectionSourceTree;

	struct Wrapper;

	valuetree::PropertyListener opTypeListener;
	valuetree::PropertyListener valuePropertyUpdater;
	valuetree::PropertyListener idUpdater;
	valuetree::PropertyListener modulationStorageBypasser;
	valuetree::RemoveListener   automationRemover;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Parameter);
};

/** A node in the DSP network. */
class NodeBase : public ConstScriptingObject
{
public:

	using Parameter = Parameter;

	using FrameType = snex::Types::dyn<float>;
	using MonoFrameType = snex::Types::span<float, 1>;
	using StereoFrameType = snex::Types::span<float, 2>;
	using Ptr = WeakReference<NodeBase>;
	using List = Array<WeakReference<NodeBase>>;

	struct Holder
	{
		virtual ~Holder() 
		{
			root = nullptr;
			nodes.clear();
		}

		NodeBase* getRootNode() const { return root.get(); }

		void setRootNode(NodeBase::Ptr newRootNode)
		{
			root = newRootNode;
		}

		ReferenceCountedObjectPtr<NodeBase> root;
		ReferenceCountedArray<NodeBase> nodes;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
	};

	struct DynamicBypassParameter : public parameter::dynamic_base
	{
		struct ScopedUndoDeactivator
		{
			ScopedUndoDeactivator(NodeBase* n):
				p(*n)
			{
				prevState = p.enableUndo;
				p.enableUndo = false;
			}

			~ScopedUndoDeactivator()
			{
				p.enableUndo = prevState;
			}

			NodeBase& p;
			bool prevState;
		};

		DynamicBypassParameter(NodeBase* n, Range<double> enabledRange_) :
			node(n),
			enabledRange(enabledRange_)
		{
			jassert(n != nullptr);

			if (n != nullptr)
				dataTree = n->getValueTree();
		};

		void call(double v) final override
		{
			bypassed = !enabledRange.contains(v) && enabledRange.getEnd() != v;

			ScopedUndoDeactivator sns(node);

			node->setBypassed(bypassed);
		}

		virtual void updateUI()
		{
			if (dataTree.isValid())
				dataTree.setProperty(PropertyIds::Bypassed, bypassed, nullptr);
		};

		bool bypassed = false;
		WeakReference<NodeBase> node;
		Range<double> enabledRange;
	};

	

	NodeBase(DspNetwork* rootNetwork, ValueTree data, int numConstants);;
	virtual ~NodeBase();

	virtual void process(ProcessDataDyn& data) = 0;

	virtual void prepare(PrepareSpecs specs);

	Identifier getObjectName() const final override { return PropertyIds::Node; };

	virtual void processFrame(FrameType& data) = 0;
	
	virtual bool forEach(const std::function<bool(NodeBase::Ptr)>& f)
	{
		return f(this);
	}

	virtual void processMonoFrame(MonoFrameType& data)
	{
		FrameType dynData(data);
		processFrame(dynData);
	}

	template <typename T> T* findParentNodeOfType()
	{
		NodeBase* p = parentNode.get();

		while (p != nullptr)
		{
			if (auto asT = dynamic_cast<T*>(p))
				return asT;

			p = p->parentNode;
		}

		return nullptr;
	}

	virtual void processStereoFrame(StereoFrameType& data)
	{
		FrameType dynData(data);
		processFrame(dynData);
	}

	virtual bool isProcessingHiseEvent() const { return false; }

	virtual void handleHiseEvent(HiseEvent& e)
	{
		ignoreUnused(e);
	}

	/** Reset the node's internal state (eg. at voice start). */
	virtual void reset() = 0;

	virtual NodeComponent* createComponent();

	virtual String getNodeDescription() const { return {}; }

	virtual Rectangle<int> getPositionInCanvas(Point<int> topLeft) const;
	
	virtual ParameterDataList createInternalParameterList() { return {}; };

	NamespacedIdentifier getPath() const;

	// ============================================================================================= BEGIN NODE API

	/** Bypasses the node. */
	virtual void setBypassed(bool shouldBeBypassed);

	/** Checks if the node is bypassed. */
	bool isBypassed() const noexcept;

	/** Returns the index in the parent. */
	int getIndexInParent() const;

	/** Checks if the node is inserted into the signal path. */
	bool isActive(bool checkRecursively) const;

	/** Sets the property of the node. */
	void set(var id, var value);

	/** Returns a property of the node. */
	var get(var id);

	/** Inserts the node into the given parent container. */
	void setParent(var parentNode, int indexInParent);

	/** Returns a reference to a parameter.*/
	var getParameterReference(var indexOrId) const;

	// ============================================================================================= END NODE API

	String getDynamicBypassSource(bool forceUpdate=true) const;

	void setValueTreeProperty(const Identifier& id, const var value);
	void setDefaultValue(const Identifier& id, var newValue);
	void setNodeProperty(const Identifier& id, const var& newValue);
	
	var getNodeProperty(const Identifier& id) ;

	bool hasNodeProperty(const Identifier& id) const;

	Value getNodePropertyAsValue(const Identifier& id);

	virtual bool isPolyphonic() const { return false; }

	bool isBodyShown() const
	{
		if (v_data[PropertyIds::Folded])
			return false;

		if (auto p = getParentNode())
		{
			return p->isBodyShown();
		}

		return true;
	}

	HelpManager& getHelpManager() { return helpManager; }

	void addConnectionToBypass(var dragDetails);

	DspNetwork* getRootNetwork() const;

	/** Not necessarily the DSP network. */
	NodeBase::Holder* getNodeHolder() const;

	ValueTree getParameterTree() { return v_data.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager()); }
	
	ValueTree getPropertyTree() { return v_data.getOrCreateChildWithName(PropertyIds::Properties, getUndoManager()); }

	void checkValid() const;
	
	bool isBeingMoved() const
	{
		auto pNode = getParentNode();

		while (pNode != nullptr)
		{
			if (pNode->isCurrentlyMoved)
				return true;

			pNode = pNode->getParentNode();
		}

		return isCurrentlyMoved;
	}

	NodeBase* getParentNode() const;
	ValueTree getValueTree() const;
	String getId() const;
	UndoManager* getUndoManager() const;
    
	Rectangle<int> getBoundsToDisplay(Rectangle<int> originalHeight) const;

	Rectangle<int> getBoundsWithoutHelp(Rectangle<int> originalHeight) const;

    Colour getColour() const;

	

	

	int getNumParameters() const;;
	Parameter* getParameter(const String& id) const;
	Parameter* getParameter(int index) const;

	void addParameter(Parameter* p);
	void removeParameter(int index);

	void setParentNode(Ptr newParentNode);

	void setCurrentId(const String& newId)
	{
		currentId = newId;
	}

	String getCurrentId() const { return currentId; }

	static void showPopup(Component* childOfGraph, Component* c);

	struct Wrapper;

	virtual Rectangle<int> getExtraComponentBounds() const
	{
		return {};
	}

	double& getCpuFlag() { return cpuUsage; }

	String getCpuUsageInPercent() const;

	void setIsUINodeOfDuplicates(bool on)
	{
		uiNodeOfDuplicates = on;
		forEach([&](NodeBase::Ptr p)
		{
			if (p == this)
				return false;

			p->setIsUINodeOfDuplicates(on); return false; 
		});
	}

	bool isUINodeOfDuplicate() const { return uiNodeOfDuplicates; }

	void setEmbeddedNetwork(NodeBase::Holder* n);

	DspNetwork* getEmbeddedNetwork();
	const DspNetwork* getEmbeddedNetwork() const;

	bool& getPreserveAutomationFlag() { return preserveAutomation; }

    int getCurrentChannelAmount() const { return lastSpecs.numChannels; };
    
protected:

	ValueTree v_data;
	PrepareSpecs lastSpecs;

private:

	bool preserveAutomation = false;
	bool enableUndo = true;
	mutable String dynamicBypassId;
	

	void updateFrozenState(Identifier id, var newValue);

	bool containsNetwork = false;

	valuetree::PropertyListener frozenListener;

	bool uiNodeOfDuplicates = false;

	WeakReference<NodeBase::Holder> embeddedNetwork;
	WeakReference<NodeBase::Holder> parent;
	WeakReference<NodeBase::Holder> subHolder;
	
	double cpuUsage = 0.0;

	bool isCurrentlyMoved = false;

	String currentId;

	HelpManager helpManager;
	
	CachedValue<bool> bypassState;

	ReferenceCountedArray<Parameter> parameters;
	WeakReference<NodeBase> parentNode;
	

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeBase);
};

#define ENABLE_NODE_PROFILING 1

struct DummyNodeProfiler
{
	DummyNodeProfiler(NodeBase* unused)
	{
		ignoreUnused(unused);
	}
};

struct RealNodeProfiler
{
	RealNodeProfiler(NodeBase* n);

	~RealNodeProfiler()
	{
		if (enabled)
		{
			auto delta = Time::getMillisecondCounterHiRes() - start;
			profileFlag = profileFlag * 0.9 + 0.1 * delta;
		}
	}

	bool enabled;
	double& profileFlag;
	double start;
};

#if ENABLE_NODE_PROFILING
using NodeProfiler = RealNodeProfiler;
#else
using NodeProfiler = DummyNodeProfiler;
#endif

/** A connection between two parameters or a parameter and a modulation source. */
class ConnectionBase : public ConstScriptingObject
{
public:

	enum OpType
	{
		SetValue,
		Multiply,
		Add,
		numOpTypes
	};

	ConnectionBase(ProcessorWithScriptingContent* p, ValueTree data_);;

	virtual ~ConnectionBase() {};

	Identifier getObjectName() const override { return PropertyIds::Connection; };

	// ============================================================================== API Calls

	/** Set a property (use the constant ids of this object for valid ids). This operation is not undoable! */
	void set(String id, var newValue)
	{
		data.setProperty(Identifier(id), newValue, nullptr);
	}

	/** Get a property (use the constant ids of this object for valid ids). */
	var get(String id) const
	{
		return data.getProperty(Identifier(id), var());
	}

	/** Returns the last value that was sent over this connection. */
	virtual double getLastValue() const { return 0.0; }

	/** Returns the target parameter of this connection. */
	var getTarget() const { return var(targetParameter); }

	/** Returns true if this connection is between a modulation signal and a parameter. */
	virtual bool isModulationConnection() const { return false; }

	// ============================================================================== End of API Calls

	

	bool objectDeleted() const override
	{
		return !data.getParent().isValid();
	}

	bool objectExists() const override
	{
		return data.getParent().isValid();
	}

	static parameter::dynamic_chain* createParameterFromConnectionTree(NodeBase* n, const ValueTree& connectionTree, bool throwIfNotFound=false);

	ValueTree data;

	valuetree::RemoveListener nodeRemoveUpdater;
	valuetree::RemoveListener sourceRemoveUpdater;

	InvertableParameterRange connectionRange;

	ReferenceCountedObjectPtr<NodeBase::Parameter> targetParameter;

protected:

	struct Wrapper;

	void initRemoveUpdater(NodeBase* parent);


};


}
