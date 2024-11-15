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



class WrapperNode : public NodeBase
{
public:

	void initParameterData(ParameterDataList& pData);

	/** This is called during the initialisation of the wrapper node
	    if:

		1. The node is wrapped in a wrap::data wrapper and
		2. The register function was created with the AddDataOffsetToUIPtr template argument set to true.

		It will then use the byte offset for the opaque void* pointer that is supplied as first parameter
		of the static createExtraComponent function, so you can safely static cast it to the expected data
		type (the one that you registered the node with).
	*/
	void setUIOffset(size_t o)
	{
		uiOffset = o;
	}

	std::function<Component*(void*, PooledUIUpdater* updater)> extraComponentFunction;

    
    
	virtual var addModulationConnection(var source, Parameter* targetParameter);

	void setCachedSize(int extraWidth, int extraHeight)
	{ 
		cachedExtraWidth = extraWidth;
		cachedExtraHeight = extraHeight;
	}

protected:

	WrapperNode(DspNetwork* parent, ValueTree d);;

	NodeComponent* createComponent() override;

	Component* createExtraComponent();

	Rectangle<int> getExtraComponentBounds() const;

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;
	

	virtual void* getObjectPtr() = 0;
	
private:

	size_t uiOffset = 0;

	mutable int cachedExtraWidth = -1;
	mutable int cachedExtraHeight = -1;
};
    
class ModulationSourceNode : public WrapperNode,
							 public ConnectionSourceManager
{
public:

	static constexpr int ModulationBarHeight = 60;

	ValueTree getModulationTargetTree();

	ModulationSourceNode(DspNetwork* n, ValueTree d);;

	var addModulationConnection(var source, Parameter* targetParameter) override;

	virtual bool isUsingNormalisedRange() const = 0;
	virtual parameter::dynamic_base_holder* getParameterHolder() { return nullptr; }

	void rebuildCallback() override;

	void prepare(PrepareSpecs ps) override;

private:

	bool prepareWasCalled = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ModulationSourceNode);
};

struct MultiOutputDragSource
{
	static Colour getFadeColour(int index, int numPaths)
	{
		if (numPaths == 0)
			return Colours::transparentBlack;

		auto hue = (float)index / (float)numPaths;

		auto saturation = JUCE_LIVE_CONSTANT_OFF(0.3f);
		auto brightness = JUCE_LIVE_CONSTANT_OFF(1.0f);
		auto minHue =	  JUCE_LIVE_CONSTANT_OFF(0.2f);
		auto maxHue =	  JUCE_LIVE_CONSTANT_OFF(0.8f);
		auto alpha =	  JUCE_LIVE_CONSTANT_OFF(0.4f);

		hue = jmap(hue, minHue, maxHue);

		return Colour::fromHSV(hue, saturation, brightness, alpha);
	}

    virtual ~MultiOutputDragSource() {};
    
	virtual NodeBase* getNode() const = 0;
	virtual int getOutputIndex() const = 0;
	virtual int getNumOutputs() const = 0;

	virtual bool matchesParameter(NodeBase::Parameter* p) const = 0;

	Component* asComponent() { return dynamic_cast<Component*>(this); }
};

struct ModulationSourceBaseComponent : public ComponentWithMiddleMouseDrag,
	public PooledUIUpdater::SimpleTimer
{
	ModulationSourceBaseComponent(PooledUIUpdater* updater);;
	ModulationSourceNode* getSourceNodeFromParent() const;

	using ObjectType = HiseDspBase;

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new ModulationSourceBaseComponent(updater);
	}

	void timerCallback() override {};
	void paint(Graphics& g) override;
	Image createDragImage();

	static Image createDragImageStatic(bool fill=true);

	void drawDragArea(Graphics& g, Rectangle<float> area, Colour c, String text=String());

	static MouseCursor createMouseCursor();

	void mouseDown(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent&) override;
	void mouseUp(const MouseEvent& e) override;
	void resized() override;

protected:

	Path dragPath;
	Path unscaledPath;

	mutable WeakReference<ModulationSourceNode> sourceNode;
};





struct ModulationSourcePlotter : ModulationSourceBaseComponent
{
	using ObjectType = void;

	static Component* createExtraComponent(void* , PooledUIUpdater* updater)
	{
		return new ModulationSourcePlotter(updater);
	}

	ModulationSourcePlotter(PooledUIUpdater* updater);

	void timerCallback() override;

	void resized() override
	{
		p.setBounds(getLocalBounds());
	}

	bool skip = false;

	ModPlotter p;
};

template <typename T> struct TypedModulationSourcePlotter: public ModulationSourcePlotter
{
	using ObjectType = T;
};

}
