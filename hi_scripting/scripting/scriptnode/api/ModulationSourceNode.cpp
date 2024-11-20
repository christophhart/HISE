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

namespace scriptnode
{
using namespace juce;
using namespace hise;



ModulationSourceNode::ModulationSourceNode(DspNetwork* n, ValueTree d) :
	WrapperNode(n, d),
	ConnectionSourceManager(n, getModulationTargetTree())
{
	initConnectionSourceListeners();
}

juce::ValueTree ModulationSourceNode::getModulationTargetTree()
{
	auto vt = getValueTree().getChildWithName(PropertyIds::ModulationTargets);

	if (!vt.isValid())
	{
		vt = ValueTree(PropertyIds::ModulationTargets);
		getValueTree().addChild(vt, -1, getUndoManager());
	}

	return vt;
}

var ModulationSourceNode::addModulationConnection(var source, Parameter* n)
{
	if (getValueTree().getChildWithName(PropertyIds::SwitchTargets).isValid())
		return WrapperNode::addModulationConnection(source, n);
	else
		return addTarget(n);
}

void ModulationSourceNode::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);

	prepareWasCalled = true;

	rebuildCallback();
}

void ModulationSourceNode::rebuildCallback()
{
	if (!prepareWasCalled)
		return;

	auto p = getParameterHolder();

	if (p == nullptr)
		return;

	auto mp = ConnectionBase::createParameterFromConnectionTree(this, getModulationTargetTree(), isUsingNormalisedRange());

    // we need to pass in the target node for the clone container to work...
    auto firstId = getModulationTargetTree().getChild(0)[PropertyIds::NodeId].toString();
    auto tn = getRootNetwork()->getNodeWithId(firstId);
    
	p->setParameter(tn, mp);
}

ModulationSourceBaseComponent::ModulationSourceBaseComponent(PooledUIUpdater* updater) :
	SimpleTimer(updater, true)
{
	unscaledPath.loadPathFromData(ScriptnodeIcons::unscaledMod, SIZE_OF_PATH(ScriptnodeIcons::unscaledMod));

	dragPath.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));

	setRepaintsOnMouseActivity(true);
	setMouseCursor(createMouseCursor());

	setSize(256, 28);
}

void ModulationSourceBaseComponent::paint(Graphics& g)
{
	Colour c = Colour(0xFF717171);

	if (isMouseOver(true))
		c = c.withMultipliedBrightness(1.4f);

	if (isMouseButtonDown(true))
		c = c.withMultipliedBrightness(1.4f);

	drawDragArea(g, getLocalBounds().toFloat(), c);
}

juce::Image ModulationSourceBaseComponent::createDragImage()
{
	auto img = createDragImageStatic(false);
	return img;
}

juce::Image ModulationSourceBaseComponent::createDragImageStatic(bool shouldFill)
{
	auto sf = Desktop::getInstance().getDisplays().getMainDisplay().scale;

	Image img(Image::ARGB, roundToInt(sf * 28.0), roundToInt(sf * 28.0), true);
	Graphics g(img);

	if (shouldFill)
	{
		Path p;
		p.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));
		p.scaleToFit(0.0f, 0.0f, 28.0f * sf, 28.0f * sf, true);
		g.setColour(Colours::white.withAlpha(0.9f));
		g.fillPath(p);
	}

	return img;
}

void ModulationSourceBaseComponent::drawDragArea(Graphics& g, Rectangle<float> b, Colour c, String text)
{
	b = b.reduced(1.0f);

	g.setColour(c);
	g.drawRoundedRectangle(b, b.getHeight() / 2.0f, 1.0f);
	g.setFont(GLOBAL_BOLD_FONT());

	g.fillPath(dragPath);

	getSourceNodeFromParent();

	if (sourceNode != nullptr && !sourceNode->isUsingNormalisedRange())
		g.fillPath(unscaledPath);

	if (text.isEmpty())
		text = "Drag to modulation target";

	if(GLOBAL_BOLD_FONT().getStringWidth(text) < b.getWidth() * 0.8f)
		g.drawText(text, b, Justification::centred);

	
}

juce::MouseCursor ModulationSourceBaseComponent::createMouseCursor()
{
	return MouseCursor(MouseCursor::CrosshairCursor);
}

void ModulationSourceBaseComponent::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	if (getSourceNodeFromParent() == nullptr)
		return;

	auto ng = findParentComponentOfClass<DspNetworkGraph>();

	DragAndDropContainer* container = nullptr;

	if(ng->isShowingRootNode())
		container = dynamic_cast<DragAndDropContainer*>(ng->root.get());
	else
		container = ng;

	if (container != nullptr)
	{
		// We need to be able to drag it anywhere...
		//while (auto pc = DragAndDropContainer::findParentDragContainerFor(dynamic_cast<Component*>(container)))
//			container = pc;

		var d;

		auto details = new DynamicObject();

		details->setProperty(PropertyIds::ID, sourceNode->getId());
		details->setProperty(PropertyIds::Automated, true);

		container->startDragging(var(details), this, ScaledImage(createDragImage()));

		ZoomableViewport::checkDragScroll(e, false);

		findParentComponentOfClass<DspNetworkGraph>()->dragOverlay.setEnabled(true);
	}
}


void ModulationSourceBaseComponent::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	ZoomableViewport::checkDragScroll(e, true);

	findParentComponentOfClass<DspNetworkGraph>()->dragOverlay.setEnabled(false);
}

void ModulationSourceBaseComponent::resized()
{
	auto b = getLocalBounds();
	auto p = b.removeFromLeft(b.getHeight()).toFloat().reduced(4.0f);
	PathFactory::scalePath(dragPath, p);
	auto p2 = b.removeFromRight(b.getHeight()).toFloat().reduced(4.0f);
	PathFactory::scalePath(unscaledPath, p2);

	getProperties().set("circleOffsetX", p.getCentreX() - (float)(getWidth() / 2));
	getProperties().set("circleOffsetY", -0.5f * (float)getHeight() -3.0f);

}

void ModulationSourceBaseComponent::mouseDown(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (getSourceNodeFromParent() == nullptr)
		return;

	if (e.mods.isRightButtonDown())
	{
		auto pe = new MacroPropertyEditor(sourceNode, sourceNode->getValueTree(), PropertyIds::ModulationTargets);

		pe->setName("Edit Modulation Targets");
        
        
        auto g = findParentComponentOfClass<ZoomableViewport>();
        auto b = g->getLocalArea(this, getLocalBounds());
        
		g->setCurrentModalWindow(pe, b);

	}
}

scriptnode::ModulationSourceNode* ModulationSourceBaseComponent::getSourceNodeFromParent() const
{
	if (sourceNode == nullptr)
	{
		if (auto pc = findParentComponentOfClass<NodeComponent>())
		{
			if(auto container = dynamic_cast<NodeContainer*>(pc->node.get()))
				sourceNode = container->getLockedModNode();
			else
				sourceNode = dynamic_cast<ModulationSourceNode*>(pc->node.get());
		}
	}

	return sourceNode;
}


ModulationSourcePlotter::ModulationSourcePlotter(PooledUIUpdater* updater) :
	ModulationSourceBaseComponent(updater)
{
	
	p.setSpecialLookAndFeel(new data::ui::pimpl::complex_ui_laf(), true);

	start();
	setOpaque(true);
	setSize(256, ModulationSourceNode::ModulationBarHeight);
	addAndMakeVisible(p);
}

void ModulationSourcePlotter::timerCallback()
{
	if (auto nc = findParentComponentOfClass<NodeComponent>())
		p.setColour(ModPlotter::ColourIds::pathColour, nc->header.colour);

	stop();
		
	Colour(0).withMultipliedSaturation(2.0);

	skip = !skip;

	if (skip)
		return;

	//p.refresh();
}


juce::Component* WrapperNode::createExtraComponent()
{
	if (extraComponentFunction)
	{
		auto obj = static_cast<uint8*>(getObjectPtr());

		obj += uiOffset;

		PooledUIUpdater* updater = getScriptProcessor()->getMainController_()->getGlobalUIUpdater();
		return extraComponentFunction((void*)obj, updater);
	}

	return nullptr;
}

juce::Rectangle<int> WrapperNode::getExtraComponentBounds() const
{
	if (cachedExtraHeight == -1)
	{
		ScopedPointer<Component> c = const_cast<WrapperNode*>(this)->createExtraComponent();

		if (c != nullptr)
		{
			cachedExtraWidth = c->getWidth();
			cachedExtraHeight = c->getHeight() + UIValues::NodeMargin;
		}
		else
		{
			cachedExtraWidth = 0;
			cachedExtraHeight = 0;
		}

	}

	return { 0, 0, cachedExtraWidth, cachedExtraHeight };
}

void WrapperNode::initParameterData(ParameterDataList& pData)
{
	auto d = getValueTree();

	auto pTree = d.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager());

	int numParameters = pData.size();

	if (pTree.getNumChildren() != 0)
	{
		for (int i = 0; i < numParameters; i++)
		{
			auto idInTree = pTree.getChild(i)[PropertyIds::ID].toString();
			auto idInList = pData[i].info.getId();

			if (idInTree != idInList)
			{
				auto faultyId = d[PropertyIds::ID].toString();

				std::vector<String> treeList;
				std::vector<String> parameterList;

				for (auto c : pTree)
					treeList.push_back(c[PropertyIds::ID].toString());

				for (auto c : pData)
					parameterList.push_back(c.info.getId());

				String errorMessage;

				errorMessage << "Error when loading " << faultyId << ": Wrong parameter list in XML data:  \n";

				errorMessage << "> ";

				for (auto& c : treeList)
					errorMessage << "`" << c << "`, ";

				errorMessage << "  \nExpected parameter list:  \n> ";

				for (auto& p : parameterList)
					errorMessage << "`" << p << "`, ";

#if USE_BACKEND 

				if (MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread())
				{
					PresetHandler::showMessageWindow("Error", errorMessage, PresetHandler::IconType::Error);
				}
				else
				{
                    auto p = dynamic_cast<Processor*>(getRootNetwork()->getScriptProcessor());
                    
                    // Don't want to interupt the loading on another thread
                    debugError(p, errorMessage);					
				}
#else
				// There's a mismatch between parameters in the value tree
				// and the list
				jassertfalse;
#endif

				getRootNetwork()->getExceptionHandler().addCustomError(this, Error::ErrorCode::InitialisationError, errorMessage);

				
			}
		}
	}

	for (auto p : pData)
	{
		auto existingChild = getParameterTree().getChildWithProperty(PropertyIds::ID, p.info.getId());

		if (!existingChild.isValid())
		{
			existingChild = p.createValueTree();
			getParameterTree().addChild(existingChild, -1, getUndoManager());
		}

		auto newP = new Parameter(this, existingChild);

		auto ndb = new parameter::dynamic_base(p.callback);

		newP->setDynamicParameter(ndb);
		newP->valueNames = p.parameterNames;

		addParameter(newP);
	}
}

juce::var WrapperNode::addModulationConnection(var source, Parameter* n)
{
	jassert(getValueTree().getChildWithName(PropertyIds::SwitchTargets).isValid());

	int sourceIndex = (int)source;

	auto cTree = getValueTree().getChildWithName(PropertyIds::SwitchTargets).getChild(sourceIndex).getChildWithName(PropertyIds::Connections);

	auto newC = ConnectionSourceManager::Helpers::getOrCreateConnection(cTree, n->parent->getId(), n->getId(), getUndoManager());

	return var(new ConnectionBase(getRootNetwork(), newC));
}

}

