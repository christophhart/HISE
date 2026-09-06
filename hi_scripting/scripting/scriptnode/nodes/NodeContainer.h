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
							public ConnectionSourceManager
	{
		ValueTree getConnectionTree();

		MacroParameter(NodeBase* parentNode, ValueTree data_);;

		void rebuildCallback() override;

		void setDynamicParameter(parameter::dynamic_base::Ptr ownedNew) override;

		void updateInputRange(Identifier, var);
		
		valuetree::PropertyListener inputRangeListener;
		ReferenceCountedObjectPtr<parameter::dynamic_base_holder> pholder;

		bool editEnabled = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(MacroParameter);
	};

	using InjectData = InjectHelpers::InjectData;
	using InjectChecker = InjectHelpers::InjectChecker;

	NodeContainer();

	template <int P> static void setParameterStatic(void* obj, double v)
	{
		auto typed = static_cast<NodeContainer*>(obj);

		if(auto p = typed->asNode()->getParameterFromIndex(P))
			p->setValueAsync(v);
	}

	void resetNodes();

	bool forceNoLock = false;

	bool isLockedContainer() const
	{
		if(forceNoLock)
			return false;

		return (bool)asNode()->getValueTree()[PropertyIds::Locked];
	}

	ParameterDataList createInternalParametersForMacros();

	NodeBase* asNode();
	const NodeBase* asNode() const;

	var addMacroConnection(var source, Parameter* n)
	{
		if (auto sp = dynamic_cast<NodeContainer::MacroParameter*>(asNode()->getParameterFromName(source.toString())))
			return sp->addTarget(n);
		
		return var();
	}

	virtual bool hasFixedParameters() const { return false; }

	void addFixedParameters();

	virtual Component* createLeftTabComponent() const;

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

	struct ContainerInjector
	{
		ContainerInjector(NodeContainer& parent) :
		  container(parent)
		{};

		InjectHelpers::InjectData data;
		PrepareSpecs specs;
		NodeContainer& container;

		var poll()
		{
			InjectData nd;
			bool ready = false;

			{
				SimpleReadWriteLock::ScopedWriteLock sl(lock);

				ready = data.reportReady();

				if (ready)
				{
					std::swap(nd, data);
#if USE_BACKEND
					pending.store(false, std::memory_order_release);
#endif
				}
			}

			if (ready)
				return nd.poll(container.asNode());

			return var();
		}

		void reset()
		{
			data.reset();
#if USE_BACKEND
			pending.store(false, std::memory_order_release);
#endif
		}

		bool hasPendingProbe() const noexcept
		{
#if USE_BACKEND
			return pending.load(std::memory_order_acquire);
#else
			return false;
#endif
		}

		Result inject(const InjectData& d)
		{
			auto numNodes = container.getNodeList().size();

			SimpleReadWriteLock::ScopedWriteLock sl(lock);

			if (data.isActive())
				return Result::fail("another inject call is pending");

			data = d;
			data.prepare(specs);
			data.processMidi = ScriptnodeExceptionHandler::isInMidiProcessingContext(container.asNode());

			data.ensureStorageAllocated(numNodes);

			if (data.probeIndex == -1)
				data.probeIndex = jmax(0, numNodes-1);

#if USE_BACKEND
			pending.store(true, std::memory_order_release);
#endif
			return Result::ok();
		}

		struct ScopedProcessor
		{
			ScopedProcessor(ContainerInjector& ci, ProcessDataDyn& pd) :
				sl(ci.lock),
				data(ci.data)
			{
				data.checkEmpty(pd);
			}

			void processBypassed(ProcessDataDyn& pd)
			{
				BACKEND_ONLY(data.processInject(pd, childIndex));
				BACKEND_ONLY(data.processProbe(pd, childIndex++));
			}

			void process(NodeBase* n, ProcessDataDyn& pd)
			{
				BACKEND_ONLY(data.processInject(pd, childIndex));
				n->process(pd);
				BACKEND_ONLY(data.processProbe(pd, childIndex++));
			}

			void processFrame(NodeBase* n, NodeBase::FrameType& fd)
			{
#if USE_BACKEND
				processFrame(fd, [&]() { n->processFrame(fd); });
#else
				n->processFrame(fd);
#endif
			}

			template <typename Function> void processFrame(NodeBase::FrameType& fd, Function&& processFunction)
			{
#if USE_BACKEND
				float* channels[NUM_MAX_CHANNELS];

				for (int i = 0; i < fd.size(); i++)
					channels[i] = fd.begin() + i;

				ProcessDataDyn pd(channels, 1, fd.size());
				data.processInject(pd, childIndex);
				processFunction();
				data.processProbe(pd, childIndex++);
#else
				ignoreUnused(fd);
				processFunction();
#endif
			}

		private:

			InjectData& data;
			hise::SimpleReadWriteLock::ScopedReadLock sl;
			int childIndex = 0;
		};

		void prepare(PrepareSpecs ps)
		{
			specs = ps;
		}

	private:

		SimpleReadWriteLock lock;
#if USE_BACKEND
		std::atomic<bool> pending { false };
#endif
	};

	/** Complete pending probes without processing this container or its descendants. */
	void processInjectedBypass(ProcessDataDyn& data);

	/** Override this and call DynamicSerialProcessor::inject in each subclass if possible. */
	Result injectNextBuffer(const InjectData& d) 
	{ 
		auto copy = d;
		
		copy.processMidi = ScriptnodeExceptionHandler::isInMidiProcessingContext(asNode());

		auto ok = injector.inject(copy);

		if (ok.failed())
		{
			injector.reset();
			return ok;
		}

		if (d.recursive)
		{
			copy.signal = InjectHelpers::InjectData::TestSignal::Silence;
			copy.injectIndex = 0;
			copy.parameterInjector = nullptr;

			for (auto n : nodes)
			{
				if (auto nc = dynamic_cast<NodeContainer*>(n.get()))
				{
					auto ok = nc->injectNextBuffer(copy);
					
					if (!ok.wasOk())
					{
						injector.reset();
						return ok;
					}
						
				}
			}

			return Result::ok();
		}

		return ok;
	}

	/** Override this and call DynamicSerialProcessor::poll. */
	var pollInjectedBuffer() { return injector.poll(); };

	// ===================================================================================

	void clear();

	NodeBase::List getChildNodesRecursive();
	ValueTree getNodeTree() { return asNode()->getValueTree().getOrCreateChildWithName(PropertyIds::Nodes, asNode()->getUndoManager()); }

	NodeBase::List& getNodeList() { return nodes; }
	const NodeBase::List& getNodeList() const { return nodes; }

	Rectangle<int> getContainerPosition(bool isVerticalContainer, Point<int> topLeft) const;

	ModulationSourceNode* getLockedModNode() const
	{
		for(auto n: getNodeList())
		{
			auto p = n->getPath().toString();

			if(p.contains("locked_mod"))
			{
				return dynamic_cast<ModulationSourceNode*>(n.get());
			}
		}

		return nullptr;
	}

	Rectangle<int> getLockedExtraComponentBounds() const
	{
		if(isLockedContainer())
		{
			if(getLockedModNode() != nullptr)
				return { 0, 0, 256, 22 + 2 * UIValues::NodeMargin + 28};
			else
				return { 0, 0, UIValues::NodeWidth, 22 + UIValues::NodeMargin };
		}
		else
		{
			return {};
		}
	}

	ContainerInjector injector;

protected:

	void initListeners(bool initParameterListener=true);

	friend class ContainerComponent;

	ReferenceCountedArray<NodeBase> ownedReference;
	NodeBase::List nodes;

	double originalSampleRate = 0.0;
	int originalBlockSize = 0;

	virtual void channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange) { ignoreUnused(nodeThatCausedLayoutChange); };

	valuetree::ChildListener nodeListener;
	valuetree::ChildListener parameterListener;
	valuetree::RecursivePropertyListener channelListener;

	PolyHandler* lastVoiceIndex = nullptr;

private:

	void nodeAddedOrRemoved(ValueTree v, bool wasAdded);
	void parameterAddedOrRemoved(ValueTree v, bool wasAdded);
	void updateChannels(ValueTree v, Identifier unused);

	
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
		void initialise(ObjectWithValueTree* p);
		void reset();
		void prepare(PrepareSpecs);

		template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
		{
			int index = 0;
			auto& dd = data.template as<ProcessDataDyn>();

			ContainerInjector::ScopedProcessor sp(parent->injector, dd);

			for (auto n : parent->getNodeList())
				sp.process(n, dd);
		}

		template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
		{
			jassert(parent != nullptr);

			NodeBase::FrameType dd(data.begin(), data.size());

#if USE_BACKEND
			if (!parent->injector.hasPendingProbe())
			{
				for (auto n : parent->getNodeList())
					n->processFrame(dd);

				return;
			}

			float* channels[NUM_MAX_CHANNELS];

			for (int i = 0; i < dd.size(); i++)
				channels[i] = dd.begin() + i;

			ProcessDataDyn pd(channels, 1, dd.size());
			ContainerInjector::ScopedProcessor sp(parent->injector, pd);

			for (auto n : parent->getNodeList())
				sp.processFrame(n, dd);
#else
			for (auto n : parent->getNodeList())
				n->processFrame(dd);
#endif
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

	var addModulationConnection(var source, Parameter* targetParameter) override
	{
		return NodeContainer::addMacroConnection(source, targetParameter);
	}

	Rectangle<int> getExtraComponentBounds() const override
	{
		return getLockedExtraComponentBounds();
	}

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

	NodePropertyT<bool> isVertical;
};

class ParallelNode : public NodeBase,
	public NodeContainer
{
public:

	ParallelNode(DspNetwork* root, ValueTree data);
	NodeComponent* createComponent() override;
	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

	var addModulationConnection(var source, Parameter* targetParameter) override
	{
		return NodeContainer::addMacroConnection(source, targetParameter);
	}

	Rectangle<int> getExtraComponentBounds() const override
	{
		return getLockedExtraComponentBounds();
	}

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
