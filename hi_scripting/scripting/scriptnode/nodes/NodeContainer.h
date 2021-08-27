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


struct NodeContainer : public AssignableObject
{
	struct MacroParameter : public NodeBase::Parameter,
							public SnexDebugHandler,
							public PooledUIUpdater::SimpleTimer
	{
		struct Connection: public ConnectionBase
		{
			Connection(NodeBase* parent, MacroParameter* pp, ValueTree d);

			~Connection();

			bool isValid() const { return targetParameter.get() != nullptr || nodeToBeBypassed.get() != nullptr; };

			bool matchesTarget(const Parameter* target) const
			{
				return target == targetParameter.get();
			}

			NodeBase::Ptr nodeToBeBypassed;

		private:

			ValueTree targetNodeData;
			UndoManager* um = nullptr;
			double rangeMultiplerForBypass = 1.0;
			MacroParameter* parentParameter = nullptr;
			valuetree::PropertyListener exprSyncer;
			String expressionCode;
		};

		ValueTree getConnectionTree();

		MacroParameter(NodeBase* parentNode, ValueTree data_);;

		void rebuildCallback();
		void updateRangeForConnection(ValueTree v, Identifier);

		void updateConnectionForExpression(ValueTree v, Identifier)
		{
			rebuildCallback();
		}

		void updateInputRange(Identifier, var);

		void timerCallback() override
		{
			getDynamicParameter()->updateUI();
		}

		

		var addParameterTarget(NodeBase::Parameter* p)
		{
			p->data.setProperty(PropertyIds::Automated, true, p->parent->getUndoManager());

			for (auto c : connections)
			{
				if (c->matchesTarget(p))
					return var(c);
			}

			ValueTree newC(PropertyIds::Connection);
			newC.setProperty(PropertyIds::NodeId, p->parent->getId(), nullptr);
			newC.setProperty(PropertyIds::ParameterId, p->getId(), nullptr);
			RangeHelpers::storeDoubleRange(newC, false, RangeHelpers::getDoubleRange(p->data), nullptr);
			newC.setProperty(PropertyIds::Expression, "", nullptr);

			getConnectionTree().addChild(newC, -1, p->parent->getUndoManager());

			return var(connections.getLast());
		}

		void logMessage(int level, const String& s) override
		{
			debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), s);
		}

		bool matchesTarget(const Parameter* target) const;

		valuetree::ChildListener connectionListener;
		valuetree::RecursivePropertyListener rangeListener;
		valuetree::RecursivePropertyListener expressionListener;
		valuetree::PropertyListener inputRangeListener;

		ReferenceCountedArray<Connection> connections;
		bool initialised = false;
		bool editEnabled = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(MacroParameter);
	};

	NodeContainer();

	template <int P> static void setParameterStatic(void* obj, double v)
	{
		auto typed = static_cast<NodeContainer*>(obj);

		if(auto p = typed->asNode()->getParameter(P))
			p->setValueAndStoreAsync(v);
	}

	void resetNodes();

	ParameterDataList createInternalParametersForMacros();

	NodeBase* asNode();
	const NodeBase* asNode() const;

	void prepareContainer(PrepareSpecs& ps);

	void prepareNodes(PrepareSpecs ps);

	bool shouldCreatePolyphonicClass() const;

	virtual Colour getContainerColour() const { return Colours::transparentBlack; }

	virtual bool isPolyphonic() const;

	virtual void assign(const int index, var newValue) override;

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	var getAssignedValue(int index) const override
	{
		return var(nodes[index]);
	}

	virtual int getBlockSizeForChildNodes() const { return originalBlockSize; }
	virtual double getSampleRateForChildNodes() const { return originalSampleRate; }

	virtual int getCachedIndex(const var &indexExpression) const override;

	bool forEachNode(const std::function<bool(NodeBase::Ptr)> & f);

	// ===================================================================================

	void clear();

	NodeBase::List getChildNodesRecursive();
	ValueTree getNodeTree() { return asNode()->getValueTree().getOrCreateChildWithName(PropertyIds::Nodes, asNode()->getUndoManager()); }

	NodeBase::List& getNodeList() { return nodes; }
	const NodeBase::List& getNodeList() const { return nodes; }

	Rectangle<int> getContainerPosition(bool isVerticalContainer, Point<int> topLeft) const;

protected:

	void initListeners(bool initParameterListener=true);

	friend class ContainerComponent;

	NodeBase::List nodes;

	double originalSampleRate = 0.0;
	int originalBlockSize = 0;

	virtual void channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange) { ignoreUnused(nodeThatCausedLayoutChange); };

	valuetree::ChildListener nodeListener;
	valuetree::ChildListener parameterListener;
	valuetree::RecursivePropertyListener channelListener;

private:

	void nodeAddedOrRemoved(ValueTree v, bool wasAdded);
	void parameterAddedOrRemoved(ValueTree v, bool wasAdded);
	void updateChannels(ValueTree v, Identifier id);

	PolyHandler* lastVoiceIndex = nullptr;
	bool channelRecursionProtection = false;
};

class SerialNode : public NodeBase,
				   public NodeContainer
{
public:

	class DynamicSerialProcessor: public HiseDspBase
	{
	public:

		SN_GET_SELF_AS_OBJECT(DynamicSerialProcessor);

		DynamicSerialProcessor() = default;

		DynamicSerialProcessor(const DynamicSerialProcessor& other);

		bool handleModulation(double&);
		void handleHiseEvent(HiseEvent& e);
		void initialise(NodeBase* p);
		void reset();
		void prepare(PrepareSpecs);

		template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
		{
			for (auto n : parent->getNodeList())
			{
				auto& dd = data.template as<ProcessDataDyn>();
				n->process(dd);
			}
		}

		template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
		{
			jassert(parent != nullptr);

			NodeBase::FrameType dd(data.begin(), data.size());

			for (auto n : parent->getNodeList())
				n->processFrame(dd);
		}

		void createParameters(ParameterDataList& ) override {};

		NodeContainer* parent;
	};

	bool forEach(const std::function<bool(NodeBase::Ptr)>& f) override
	{
		return forEachNode(f);
	}

	SerialNode(DspNetwork* root, ValueTree data);

	NodeComponent* createComponent() override;

	ParameterDataList createInternalParameterList() override
	{
		return NodeContainer::createInternalParametersForMacros();
	}

	

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
	{
		return getBoundsToDisplay(getContainerPosition(isVertical.getValue(), topLeft));
	}

	NodePropertyT<bool> isVertical;
};

class ParallelNode : public NodeBase,
	public NodeContainer
{
public:

	ParallelNode(DspNetwork* root, ValueTree data);
	NodeComponent* createComponent() override;
	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

	bool forEach(const std::function<bool(NodeBase::Ptr)>& f) override
	{
		return forEachNode(f);
	}
};

class NodeContainerFactory : public NodeFactory
{
public:

	NodeContainerFactory(DspNetwork* parent);
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("container"); };
};

}
