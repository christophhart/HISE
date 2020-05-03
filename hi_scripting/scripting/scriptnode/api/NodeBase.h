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

	Identifier getObjectName() const override { return PropertyIds::Parameter; }

	Parameter(NodeBase* parent_, ValueTree& data_);;

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

	bool matchesConnection(const ValueTree& c) const;

	Array<Parameter*> getConnectedMacroParameters() const;

	parameter::dynamic_base_holder& getReferenceToCallback()
	{
		return dbNew;
	}

	const parameter::dynamic_base_holder& getCallback() const
	{
		return dbNew;
	}

	void setCallbackNew(parameter::dynamic_base* ownedNew);

	StringArray valueNames;
	NodeBase* parent;
	ValueTree data;

	void updateFromValueTree(Identifier, var newValue)
	{
		setValueAndStoreAsync((double)newValue);
	}

private:

	parameter::dynamic_base_holder dbNew;

	struct Wrapper;

	double lastValue = 0.0;

	valuetree::PropertyListener opTypeListener;
	valuetree::PropertyListener valuePropertyUpdater;
	valuetree::PropertyListener idUpdater;
	valuetree::PropertyListener modulationStorageBypasser;
	DspHelpers::ParameterCallback dbOld;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Parameter);
};

/** A node in the DSP network. */
class NodeBase : public ConstScriptingObject
{
public:

	using Parameter = Parameter;

	struct DynamicBypassParameter : public parameter::dynamic_base
	{
		DynamicBypassParameter(NodeBase* n, Range<double> enabledRange_) :
			node(n),
			enabledRange(enabledRange_)
		{};

		void call(double v) final override
		{
			node->setBypassed(!enabledRange.contains(v));
		}

		WeakReference<NodeBase> node;
		Range<double> enabledRange;
	};

	using FrameType = snex::Types::dyn<float>;
	using MonoFrameType = snex::Types::span<float, 1>;
	using StereoFrameType = snex::Types::span<float, 2>;
	using Ptr = WeakReference<NodeBase>;
	using List = Array<WeakReference<NodeBase>>;

	NodeBase(DspNetwork* rootNetwork, ValueTree data, int numConstants);;
	virtual ~NodeBase() {}

	virtual void process(ProcessData& data) = 0;

	virtual void prepare(PrepareSpecs specs);

	Identifier getObjectName() const override { return "Node"; };

	virtual void processFrame(FrameType& data) = 0;
	
	virtual void processMonoFrame(MonoFrameType& data)
	{
		FrameType dynData(data);
		processFrame(dynData);
	}

	virtual void processStereoFrame(StereoFrameType& data)
	{
		FrameType dynData(data);
		processFrame(dynData);
	}

	virtual void handleHiseEvent(HiseEvent& e)
	{
		ignoreUnused(e);
	}

	/** Reset the node's internal state (eg. at voice start). */
	virtual void reset() = 0;

	virtual String createCppClass(bool isOuterClass);

	virtual NodeComponent* createComponent();

	virtual Rectangle<int> getPositionInCanvas(Point<int> topLeft) const;
	
	virtual Array<ParameterDataImpl> createInternalParameterList() { return {}; }

	// ============================================================================================= BEGIN NODE API

	/** Bypasses the node. */
	virtual void setBypassed(bool shouldBeBypassed);

	/** Checks if the node is bypassed. */
	bool isBypassed() const noexcept;

	/** Returns the index in the parent. */
	int getIndexInParent() const;

	/** Checks if the node is inserted into the signal path. */
	bool isActive() const { return v_data.getParent().isValid(); }

	/** Sets the property of the node. */
	void set(var id, var value);

	/** Returns a property of the node. */
	var get(var id);

	/** Inserts the node into the given parent container. */
	void setParent(var parentNode, int indexInParent);

	/** Writes the modulation signal into the given buffer. */
	void writeModulationSignal(var buffer);

	/** Creates a buffer object that can be used to display the modulation values. */
	var createRingBuffer();
	
	/** Returns a reference to a parameter.*/
	var getParameterReference(var indexOrId) const;

	// ============================================================================================= END NODE API

	void setValueTreeProperty(const Identifier& id, const var value);
	void setDefaultValue(const Identifier& id, var newValue);
	void setNodeProperty(const Identifier& id, const var& newValue);
	
	var getNodeProperty(const Identifier& id) ;

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

	void setNumChannels(int newNumChannels)
	{
		if (!v_data[PropertyIds::LockNumChannels])
			setValueTreeProperty(PropertyIds::NumChannels, newNumChannels);
	}

	bool hasFixChannelAmount() const;

	int getNumChannelsToProcess() const { return (int)v_data[PropertyIds::NumChannels]; };

	int getNumParameters() const;;
	Parameter* getParameter(const String& id) const;
	Parameter* getParameter(int index) const;

	void addParameter(Parameter* p);
	void removeParameter(int index);

	void setParentNode(Ptr newParentNode)
	{
		parentNode = newParentNode;
	}

	void setCurrentId(const String& newId)
	{
		currentId = newId;
	}

	String getCurrentId() const { return currentId; }

	struct Wrapper;

	virtual Rectangle<int> getExtraComponentBounds() const
	{
		return {};
	}

private:

	WeakReference<ConstScriptingObject> parent;

protected:

	ValueTree v_data;

private:

	bool isCurrentlyMoved = false;

	String currentId;

	HelpManager helpManager;

	valuetree::PropertyListener bypassListener;
	bool bypassState = false;

	ReferenceCountedArray<Parameter> parameters;
	WeakReference<NodeBase> parentNode;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeBase);
};


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

#if OLD_OP

	void setOpTypeFromId(const Identifier& id)
	{
		if (id == OperatorIds::SetValue)
			opType = SetValue;
		else if (id == OperatorIds::Add)
			opType = Add;
		else if (id == OperatorIds::Multiply)
			opType = Multiply;
	}

	Identifier getOpType() const
	{
		static const Identifier ids[OpType::numOpTypes] = { OperatorIds::SetValue, OperatorIds::Multiply, OperatorIds::Add };
		return ids[opType];
	}

#endif

	bool objectDeleted() const override
	{
		return !data.getParent().isValid();
	}

	bool objectExists() const override
	{
		return data.getParent().isValid();
	}

	ValueTree data;

	valuetree::RemoveListener nodeRemoveUpdater;

#if OLD_OP
	OpType opType = SetValue;
#endif

	NormalisableRange<double> connectionRange;
	bool inverted = false;

	ReferenceCountedObjectPtr<NodeBase::Parameter> targetParameter;

protected:

	struct Wrapper;

	void initRemoveUpdater(NodeBase* parent);


};


}
