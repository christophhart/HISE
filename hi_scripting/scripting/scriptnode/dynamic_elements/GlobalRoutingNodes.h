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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;


namespace routing
{

struct GlobalRoutingNodeBase: public NodeBase,
							  public GlobalRoutingManager::SelectableTargetBase
{
	static constexpr auto SlotTypeId = GlobalRoutingManager::SlotBase::SlotType::Signal;
	static constexpr int EditorHeight = 130;

	struct Editor;

	GlobalRoutingNodeBase(DspNetwork* n, ValueTree d);;

	String getTargetId() const override;

	void selectCallback(Component* rootEditor) override;
	void updateConnection(Identifier id, var newValue);
	void initParameters();
	void prepare(PrepareSpecs specs) override;
	void processFrame(FrameType& data) override
	{
		jassertfalse;
	}

	

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

	NodeComponent* createComponent() override;
	
	virtual float getGain() const = 0;

	virtual bool isSource() const = 0;

	SimpleReadWriteLock connectionLock;

	ReferenceCountedObjectPtr<GlobalRoutingManager::Signal> currentSlot;
	GlobalRoutingManager::Ptr globalRoutingManager;
	scriptnode::NodePropertyT<String> slotId;
	PrepareSpecs lastSpecs;
	Result lastResult;

	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalRoutingNodeBase);
};

struct GlobalSendNode : public GlobalRoutingNodeBase
{
	GlobalSendNode(DspNetwork* n, ValueTree d);;

	SN_NODE_ID("global_send");

	static NodeBase* createNode(DspNetwork* n, ValueTree d)
	{
		return new GlobalSendNode(n, d);
	}

	float getGain() const override { return 1.0f; }

	bool isSource() const override { return true; }

	void process(ProcessDataDyn& data) override;

	void reset() override;

	String getNodeDescription() const override { return "Send the signal anywhere in HISE!"; }

	static void setValue(void* obj, double v);

	ParameterDataList createInternalParameterList();;

	float value = 1.0f;
};


struct GlobalCableNode : public ModulationSourceNode,
						 public GlobalRoutingManager::CableTargetBase
{
	static constexpr auto SlotTypeId = GlobalRoutingManager::SlotBase::SlotType::Cable;

	static constexpr int EditorHeight = 32 + UIValues::NodeMargin + 34 + UIValues::NodeMargin + 28;

	struct Editor;

	GlobalCableNode(DspNetwork* n, ValueTree d);;
	~GlobalCableNode();

	SN_NODE_ID("global_cable");

	static NodeBase* createNode(DspNetwork* n, ValueTree d);

	void prepare(PrepareSpecs ps) override;

	bool isUsingNormalisedRange() const override { return true; }

	String getTargetId() const override;

	Path getTargetIcon() const override;

	void selectCallback(Component* rootEditor) override;

	void* getObjectPtr() override { return this; }

	void reset() override {};
	void process(ProcessDataDyn& data) override {};
	void updateConnection(Identifier id, var newValue);
	void initParameters();
	void processFrame(FrameType& data) override;

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;

	void sendValue(double v) override;

	static void setValue(void* obj, double newValue);

	ParameterDataList createInternalParameterList();;

	parameter::dynamic_base_holder* getParameterHolder() override { return &parameterTarget; }

	NodeComponent* createComponent() override;

	SimpleReadWriteLock connectionLock;

	ReferenceCountedObjectPtr<GlobalRoutingManager::Cable> currentCable;
	GlobalRoutingManager::Ptr globalRoutingManager;
	scriptnode::NodePropertyT<String> slotId;

	parameter::dynamic_base_holder parameterTarget;
	float lastValue = 0.0f;

	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalCableNode);
};


}


}
