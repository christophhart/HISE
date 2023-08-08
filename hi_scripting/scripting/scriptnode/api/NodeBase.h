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
	void setValueAsync(double newValue);

	/** Returns the range properties as JSON object. */
	var getRangeObject() const;

	/** Updates the parameter range from the given object. */
	void setRangeFromObject(var propertyObject);

    /** Sets a range property. */
    void setRangeProperty(String id, var newValue);
    
	/** Stores the value synchronously and calls the callback. */
	void setValueSync(double newValue);

	// ================================================================== End of API Calls

	ValueTree getConnectionSourceTree(bool forceUpdate);

	bool matchesConnection(const ValueTree& c) const;

	Array<Parameter*> getConnectedMacroParameters() const;

	parameter::dynamic_base::Ptr getDynamicParameter() const
	{
		jassert(dynamicParameter != nullptr);
		return dynamicParameter;
	}

	virtual void setDynamicParameter(parameter::dynamic_base::Ptr ownedNew);

	StringArray valueNames;
	NodeBase* parent;
	

	void updateFromValueTree(Identifier id, var newValue)
	{
		jassert(id == PropertyIds::Value);
		setValueAsync((double)newValue);
	}

	ValueTree data;

	bool isProbed = false;

	void ensureAfterValueCallback(valuetree::PropertyListener& l)
	{
		l.setHighPriorityListener(&valuePropertyUpdater);
	}

	bool isModulated() const 
	{ 
		return (bool)data.getProperty(PropertyIds::Automated, false);
	}

private:

	void updateRange(Identifier, var);

	void updateConnectionOnRemoval(ValueTree& c);

	parameter::dynamic_base::Ptr dynamicParameter;

	ValueTree connectionSourceTree;

	struct Wrapper;

	valuetree::PropertyListener rangeListener;
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

	using Parameter = scriptnode::Parameter;

	using FrameType = snex::Types::dyn<float>;
	using MonoFrameType = snex::Types::span<float, 1>;
	using StereoFrameType = snex::Types::span<float, 2>;
	using Ptr = WeakReference<NodeBase>;
	using List = Array<WeakReference<NodeBase>>;

	struct Holder
	{
		virtual ~Holder();

		NodeBase* getRootNode() const;

		void setRootNode(NodeBase::Ptr newRootNode);

		ReferenceCountedObjectPtr<NodeBase> root;
		ReferenceCountedArray<NodeBase> nodes;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
	};

	struct DynamicBypassParameter : public parameter::dynamic_base
	{
		struct ScopedUndoDeactivator
		{
			ScopedUndoDeactivator(NodeBase* n);

			~ScopedUndoDeactivator();

			NodeBase& p;
			bool prevState;
		};

		DynamicBypassParameter(NodeBase* n, Range<double> enabledRange_);;

		~DynamicBypassParameter();

		void call(double v) final override;

		bool bypassed = false;
		WeakReference<NodeBase> node;
		Range<double> enabledRange;
		String prevId;
	};

	NodeBase(DspNetwork* rootNetwork, ValueTree data, int numConstants);;
	virtual ~NodeBase();

	virtual void process(ProcessDataDyn& data) = 0;

	virtual void prepare(PrepareSpecs specs);

	Identifier getObjectName() const final override;;

	virtual void processFrame(FrameType& data) = 0;
	
	virtual bool forEach(const std::function<bool(NodeBase::Ptr)>& f);

	virtual void processMonoFrame(MonoFrameType& data);

	template <typename T> T* findParentNodeOfType() const
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

	virtual void processStereoFrame(StereoFrameType& data);

	virtual bool isProcessingHiseEvent() const;

	virtual void handleHiseEvent(HiseEvent& e);

	/** Reset the node's internal state (eg. at voice start). */
	virtual void reset() = 0;

	virtual NodeComponent* createComponent();

	virtual String getNodeDescription() const;

	virtual Rectangle<int> getPositionInCanvas(Point<int> topLeft) const;
	
	virtual ParameterDataList createInternalParameterList();;

	virtual var addModulationConnection(var source, Parameter* targetParameter);

	virtual void processProfileInfo(double cpuUsage, int numSamples);

	NamespacedIdentifier getPath() const;

	// ============================================================================================= BEGIN NODE API

	/** Bypasses the node. */
	virtual void setBypassed(bool shouldBeBypassed);

	/** Connects this node to the given parameter target. sourceInfo is either the parameter name (String) or output slot (integer). */
	var connectTo(var parameterTarget, var sourceInfo);

	/** Connects the bypass button of this node to the given source info ("NodeId.ParameterId"). */
	void connectToBypass(var sourceInfo);

	/** Checks if the node is bypassed. */
	bool isBypassed() const noexcept;

	/** Returns the index in the parent. */
	int getIndexInParent() const;

	/** Checks if the node is inserted into the signal path. */
	bool isActive(bool checkRecursively) const;

	/** Sets the property of the node. */
	void set(var id, var value);

    /** Sets the complex data type at the dataSlot to the given index and data (if embedded). */
    bool setComplexDataIndex(String dataType, int dataSlot, int indexValue);

	/** Returns a property of the node. */
	var get(var id);

	/** Inserts the node into the given parent container. */
	void setParent(var parentNode, int indexInParent);

	/** Returns a reference to a parameter.*/
	var getParameter(var indexOrId) const;

	/** Returns the number of parameters. */
	int getNumParameters() const;;

	/** Returns a list of child nodes if this node is a container. */
	var getChildNodes(bool recursive);

	// ============================================================================================= END NODE API

	void setValueTreeProperty(const Identifier& id, const var value);
	void setDefaultValue(const Identifier& id, var newValue);
	void setNodeProperty(const Identifier& id, const var& newValue);
	
	var getNodeProperty(const Identifier& id) ;

	bool hasNodeProperty(const Identifier& id) const;

	Value getNodePropertyAsValue(const Identifier& id);

	virtual bool isPolyphonic() const;

	bool isBodyShown() const;

	HelpManager& getHelpManager();


	DspNetwork* getRootNetwork() const;

	/** Not necessarily the DSP network. */
	NodeBase::Holder* getNodeHolder() const;

	ValueTree getParameterTree();

	ValueTree getPropertyTree();

	void checkValid() const;
	
	bool isBeingMoved() const;

	NodeBase* getParentNode() const;
	ValueTree getValueTree() const;
	String getId() const;
	UndoManager* getUndoManager(bool returnIfPending=false) const;
    
	Rectangle<int> getBoundsToDisplay(Rectangle<int> originalHeight) const;

	Rectangle<int> getBoundsWithoutHelp(Rectangle<int> originalHeight) const;

    Colour getColour() const;

	struct ParameterIterator
	{
		ParameterIterator(NodeBase& n);
		Parameter** begin() const;
		Parameter** end() const;

	private:

		NodeBase& node;
		const int size = 0;
		int index = 0;
	};
	
	
	Parameter* getParameterFromName(const String& id) const;
	Parameter* getParameterFromIndex(int index) const;

	void addParameter(Parameter* p);
	void removeParameter(int index);
	void removeParameter(const String& id);

	void setParentNode(Ptr newParentNode);

	void setCurrentId(const String& newId);

	String getCurrentId() const;

	static void showPopup(Component* childOfGraph, Component* c);

	struct Wrapper;

	virtual Rectangle<int> getExtraComponentBounds() const;

	static bool sendResizeMessage(Component* c, bool async);
    
    
	double& getCpuFlag();

	String getCpuUsageInPercent() const;

	bool isClone() const;

	void setEmbeddedNetwork(NodeBase::Holder* n);

	DspNetwork* getEmbeddedNetwork();
	const DspNetwork* getEmbeddedNetwork() const;

	bool& getPreserveAutomationFlag();

	int getCurrentChannelAmount() const;;
    
    virtual int getNumChannelsToDisplay() const;;
    
	String getDynamicBypassSource(bool forceUpdate) const;

	int getCurrentBlockRate() const;

	void setSignalPeaks(float* p, int numChannels, bool postSignal);

	float getSignalPeak(int channel, bool post) const;

protected:

	ValueTree v_data;
	PrepareSpecs lastSpecs;

private:

    span<span<float, NUM_MAX_CHANNELS>, 2> signalPeaks;
    
	void updateBypassState(Identifier, var newValue);

	int lastBlockSize = 0;

	bool preserveAutomation = false;
	bool enableUndo = true;
	
	mutable String dynamicBypassId;

	void updateFrozenState(Identifier id, var newValue);

	bool containsNetwork = false;

	valuetree::PropertyListener frozenListener;
	valuetree::PropertyListener bypassListener;

	bool bypassState = false;

	WeakReference<NodeBase::Holder> embeddedNetwork;
	WeakReference<NodeBase::Holder> parent;
	WeakReference<NodeBase::Holder> subHolder;
	
	double cpuUsage = 0.0;

	bool isCurrentlyMoved = false;

	String currentId;

	HelpManager helpManager;
	
	ReferenceCountedArray<Parameter> parameters;
	WeakReference<NodeBase> parentNode;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeBase);
};

#define ENABLE_NODE_PROFILING 1

struct DummyNodeProfiler
{
	DummyNodeProfiler(NodeBase* unused, int unused2)
	{
		ignoreUnused(unused, unused2);
	}
};

struct ProcessDataPeakChecker
{
    ProcessDataPeakChecker(NodeBase* n, ProcessDataDyn& d_);
    ~ProcessDataPeakChecker();
    
	void check(bool post);
    
    NodeBase& p;
    ProcessDataDyn& d;
};

#ifndef ALLOW_FRAME_SIGNAL_CHECK
#define ALLOW_FRAME_SIGNAL_CHECK 1
#endif

struct FrameDataPeakChecker
{
	FrameDataPeakChecker(NodeBase* n, float* d, int s);

	~FrameDataPeakChecker();

	void check(bool post);

	NodeBase& p;
	dyn<float> b;
};

struct RealNodeProfiler
{
	RealNodeProfiler(NodeBase* n, int numSamples);

	~RealNodeProfiler();

	NodeBase* node;
	bool enabled;
	double& profileFlag;
	double start;
	const int numSamples;
};

#if ENABLE_NODE_PROFILING
using NodeProfiler = RealNodeProfiler;
#else
using NodeProfiler = DummyNodeProfiler;
#endif

struct ConnectionSourceManager
{
	/** This object listens to the source and target node and removes the connection if one of the 
	    nodes is being deleted. 

		It's lifetime is tied to the Connection valuetree and is the only entity that sets the Automated flag (TODO)
	*/
	struct CableRemoveListener
	{
		void removeCable(ValueTree& v);

		ValueTree findTargetNodeData(const ValueTree& recursiveTree);

		CableRemoveListener(ConnectionSourceManager& parent, ValueTree connectionData, ValueTree sourceNodeData);

		~CableRemoveListener();

        bool initListeners();
        
		ValueTree data;
		ValueTree sourceNode;
		ValueTree targetNode;
		ValueTree targetParameterTree;

		ConnectionSourceManager& parent;

		valuetree::RemoveListener targetRemoveUpdater;
		valuetree::RemoveListener sourceRemoveUpdater;
		valuetree::PropertyListener targetRangeListener;

        JUCE_DECLARE_WEAK_REFERENCEABLE(CableRemoveListener);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CableRemoveListener);
	};

	struct Helpers
	{
		static ValueTree getOrCreateConnection(ValueTree connectionTree, const String& nodeId, const String& parameterId, UndoManager* um);
		static ValueTree findParentNodeTree(const ValueTree& v);
	};

	ConnectionSourceManager(DspNetwork* n_, ValueTree connectionsTree_);

	virtual ~ConnectionSourceManager()
	{
		connections.clear();
	}

	/** Call this in your subclass constructor (it will call the virtual method rebuildCallback() so 
	    the subclass needs to be defined. 
	*/
	bool isConnectedToSource(const Parameter* target) const;

	var addTarget(NodeBase::Parameter* p);

protected:

	void initConnectionSourceListeners();

	/** Override this method and rebuild the parameter callback. */
	virtual void rebuildCallback() = 0;

private:

	void connectionChanged(ValueTree v, bool wasAdded);

	WeakReference<DspNetwork> n;
	ValueTree connectionsTree;
	OwnedArray<CableRemoveListener> connections;
	valuetree::ChildListener connectionListener;
	bool initialised = false;
};




/** A connection between two parameters or a parameter and a modulation source. These objects are
	supposed to be used in the scripting engine and contain NO task whatsoever.
	
	TODO: Make a hover  info box that shows the connection update rate
	      - search the common container
		  - get the process specs of the container
*/
class ConnectionBase final: public ConstScriptingObject
{
public:

	enum ConnectionSource
	{
		MacroParameter,
		SingleOutputModulation,
		MultiOutputModulation,
		numConnectionSources
	};

	ConnectionBase(DspNetwork* n, ValueTree data_);;
	~ConnectionBase() {};

	Identifier getObjectName() const override { return PropertyIds::Connection; };

	// ============================================================================== API Calls

	/** Returns the target parameter of this connection. */
	var getTarget() const;

	/** Returns the source node. If getSignalSource is true, it searches the node that creates the modulation signal. */
	var getSourceNode(bool getSignalSource) const;

	/** Removes this connection. */
	void disconnect();

	/** Returns the connection type. */
	int getConnectionType() const;

	/** Returns the update rate for the modulation connection. */
	int getUpdateRate() const;

	/** Checks if the connection is still valid. */
	bool isConnected() const;

	// ============================================================================== End of API Calls

	bool objectDeleted() const override
	{
		return !data.getParent().isValid();
	}

	bool objectExists() const override
	{
		return data.getParent().isValid();
	}

	static parameter::dynamic_base::Ptr createParameterFromConnectionTree(NodeBase* n, const ValueTree& connectionTree, bool scaleInput);

	struct Helpers
	{
		static NodeBase* findRealSource(NodeBase* source);

		static ValueTree findCommonParent(ValueTree v1, ValueTree v2);
	};

private:

	WeakReference<DspNetwork> network;
	WeakReference<NodeBase> sourceNode;
	WeakReference<NodeBase> sourceInSignalChain;
	WeakReference<NodeBase> commonContainer;
	ConnectionSource type;

	ValueTree data;
	WeakReference<NodeBase::Parameter> targetParameter;

	struct Wrapper;
};


}
