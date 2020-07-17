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



/** A node in the DSP network. */
class NodeBase : public ConstScriptingObject
{
public:

	struct HelpManager: ControlledObject
	{
		HelpManager(NodeBase& parent, ValueTree d);

		struct Listener
		{
			virtual ~Listener() {};

			virtual void helpChanged(float newWidth, float newHeight) = 0;

			virtual void repaintHelp() = 0;

		private:

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

		void update(Identifier id, var newValue)
		{
			if (id == PropertyIds::NodeColour)
			{
				highlightColour = PropertyHelpers::getColourFromVar(newValue);
				if (highlightColour.isTransparent())
					highlightColour = Colour(SIGNAL_COLOUR);

				if (helpRenderer != nullptr)
				{
					helpRenderer->getStyleData().headlineColour = highlightColour;
					
					helpRenderer->setNewText(lastText);

					for (auto l : listeners)
					{
						if (l != nullptr)
							l->repaintHelp();
					}
				}
			}
			else if (id == PropertyIds::Comment)
			{
				lastText = newValue.toString();
				rebuild();
			}
			else if (id == PropertyIds::CommentWidth)
			{
				lastWidth = (float)newValue;

				rebuild();
			}
		}

		void render(Graphics& g, Rectangle<float> area)
		{
			if (helpRenderer != nullptr && !area.isEmpty())
			{
				area.removeFromLeft(10.0f);
				g.setColour(Colours::black.withAlpha(0.1f));
				g.fillRoundedRectangle(area, 2.0f);
				helpRenderer->draw(g, area.reduced(10.0f));
			}
				
		}

		void addHelpListener(Listener* l)
		{
			listeners.addIfNotAlreadyThere(l);
			l->helpChanged(lastWidth + 30.0f, lastHeight + 20.0f);
		}

		void removeHelpListener(Listener* l)
		{
			listeners.removeAllInstancesOf(l);
		}

		Rectangle<float> getHelpSize() const
		{
			return { 0.0f, 0.0f, lastHeight > 0.0f ? lastWidth + 30.0f : 0.0f, lastHeight + 20.0f };
		}

	private:

		void rebuild()
		{
			if (lastText.isNotEmpty())
			{
				helpRenderer = new MarkdownRenderer(lastText);
				helpRenderer->setDatabaseHolder(dynamic_cast<MarkdownDatabaseHolder*>(getMainController()));
				helpRenderer->getStyleData().headlineColour = highlightColour;
				helpRenderer->setDefaultTextSize(15.0f);
				helpRenderer->parse();
				lastHeight = helpRenderer->getHeightForWidth(lastWidth);
			}
			else
			{
				helpRenderer = nullptr;
				lastHeight = 0.0f;
			}

			for (auto l : listeners)
			{
				if (l != nullptr)
					l->helpChanged(lastWidth + 30.0f, lastHeight);
			}
		}

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

	using Ptr = WeakReference<NodeBase>;
	using List = Array<WeakReference<NodeBase>>;

	/** A parameter of a node. */
	class Parameter: public ConstScriptingObject
	{
	public:

		Identifier getObjectName() const override { return PropertyIds::Parameter; }

		Parameter(NodeBase* parent_, ValueTree& data_);;
		
		void addModulationValue(double newValue);
		void multiplyModulationValue(double newValue);

		void clearModulationValues();

		// ======================================================================== API Calls

		/** Adds (and/or) returns a connection from the given data. */
		var addConnectionFrom(var connectionData);

		/** Returns the name of the parameter. */
		String getId() const;

		/** Returns the current value (without modulation). */
		double getValue() const;

		/** Returns the last value including modulation. */
		double getModValue() const;

		/** Sets the value immediately and stores it asynchronously. */
		void setValueAndStoreAsync(double newValue);

		// ================================================================== End of API Calls

		bool matchesConnection(const ValueTree& c) const;

		Array<Parameter*> getConnectedMacroParameters() const;

		void prepare(PrepareSpecs specs)
		{
			value.prepare(specs);
		}

		DspHelpers::ParameterCallback& getReferenceToCallback();
		DspHelpers::ParameterCallback getCallback() const;
		void setCallback(const DspHelpers::ParameterCallback& newCallback);

		StringArray valueNames;
		NodeBase::Ptr parent;
		ValueTree data;

		void updateFromValueTree(Identifier, var newValue)
		{
			setValueAndStoreAsync((double)newValue);
		}

	private:

		struct Wrapper;

		double lastValue = 0.0;

		static void nothing(double) {};
		void storeValue();

		valuetree::PropertyListener opTypeListener;
		valuetree::PropertyListener valuePropertyUpdater;
		valuetree::PropertyListener idUpdater;
		valuetree::PropertyListener modulationStorageBypasser;
		DspHelpers::ParameterCallback db = nothing;
		LockFreeUpdater valueUpdater;

		struct ParameterValue
		{
			double modAddValue = 0.0;
			double modMulValue = 1.0;
			double value = 0.0;
			double lastValue = 0.0;

			bool updateLastValue()
			{
				auto thisValue = getModValue();
				std::swap(thisValue, lastValue);
				return thisValue != lastValue;
			}

			double getModValue() const
			{
				return (value + modAddValue) * modMulValue;
			}

		};
		
		PolyData<ParameterValue, NUM_POLYPHONIC_VOICES> value;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Parameter);
	};

	NodeBase(DspNetwork* rootNetwork, ValueTree data, int numConstants);;
	virtual ~NodeBase() {}

	virtual void process(ProcessData& data) = 0;

	virtual void prepare(PrepareSpecs specs) = 0;

	Identifier getObjectName() const final override { return PropertyIds::Node; };

	void prepareParameters(PrepareSpecs specs);

	virtual void processSingle(float* frameData, int numChannels);

	virtual void handleHiseEvent(HiseEvent& e)
	{
		ignoreUnused(e);
	}

	/** Reset the node's internal state (eg. at voice start). */
	virtual void reset() = 0;

	virtual String createCppClass(bool isOuterClass);

	virtual NodeComponent* createComponent();

	virtual Rectangle<int> getPositionInCanvas(Point<int> topLeft) const;
	
	virtual HardcodedNode* getAsHardcodedNode() { return nullptr; }

	virtual RestorableNode* getAsRestorableNode() { return nullptr; }

	// ============================================================================================= BEGIN NODE API

	/** Bypasses the node. */
	void setBypassed(bool shouldBeBypassed);

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

    
    virtual void postInit() {};
    
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

private:

	WeakReference<ConstScriptingObject> parent;

protected:

	

	ValueTree v_data;

private:

	bool isCurrentlyMoved = false;

	String currentId;

	HelpManager helpManager;

	CachedValue<bool> bypassed;
	bool pendingBypassState;
	LockFreeUpdater bypassUpdater;

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

	bool objectDeleted() const override
	{
		return !data.getParent().isValid();
	}

	bool objectExists() const override
	{
		return data.getParent().isValid();
	}

	ValueTree data;

	valuetree::PropertyListener opTypeListener;
	valuetree::RemoveListener nodeRemoveUpdater;

	OpType opType = SetValue;

	NormalisableRange<double> connectionRange;
	bool inverted = false;

	ReferenceCountedObjectPtr<NodeBase::Parameter> targetParameter;

protected:

	struct Wrapper;

	void initRemoveUpdater(NodeBase* parent);


};


}
