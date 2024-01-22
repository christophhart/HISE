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

juce::String NodeComponent::Header::getPowerButtonId(bool getOff) const
{
	auto path = parent.node->getValueTree()[PropertyIds::FactoryPath].toString();

	if (path.startsWith("container."))
	{
		path = path.fromFirstOccurrenceOf("container.", false, false);

		if (getOff)
		{
			if (path.contains("frame") ||
				path.contains("oversample") ||
				path.contains("midi") ||
				path.startsWith("fix")) 
				return "chain";
			else
				return "on";
		}
		else
		{
			if (path == "soft_bypass" ||
				path == "offline")
				return "on";

			return path;
		}
	}
		
	return "on";
}



NodeComponent::Header::Header(NodeComponent& parent_) :
	parent(parent_),
	powerButton(getPowerButtonId(false), this, f, getPowerButtonId(true)),
	deleteButton("delete", this, f),
	parameterButton("parameter", this, f),
	freezeButton("freeze", this, f)
{
	powerButton.setToggleModeWithColourChange(true);
	
	powerButtonUpdater.setCallback(parent.node->getValueTree(), { PropertyIds::Bypassed},
		valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(NodeComponent::Header::updatePowerButtonState));

	colourUpdater.setCallback(parent.node->getValueTree(), { PropertyIds::NodeColour }, valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(NodeComponent::Header::updateColour));

	dynamicPowerUpdater.setTypesToWatch({ PropertyIds::Nodes, PropertyIds::Connections });

	dynamicPowerUpdater.setCallback(parent.node->getRootNetwork()->getValueTree(), valuetree::AsyncMode::Asynchronously, [this](ValueTree v, bool wasAdded)
	{
		if (parent.node != nullptr)
		{
			auto on = parent.node->getDynamicBypassSource(true).isEmpty();
			powerButton.setVisible(on);

			if (auto dng = findParentComponentOfClass<DspNetworkGraph>())
				dng->repaint();
		}
	});

	addAndMakeVisible(powerButton);
	addAndMakeVisible(deleteButton);
	addAndMakeVisible(parameterButton);
	addAndMakeVisible(freezeButton);

	freezeButton.setToggleModeWithColourChange(true);

	parameterButton.setToggleModeWithColourChange(true);
	parameterButton.setToggleStateAndUpdateIcon(parent.dataReference[PropertyIds::ShowParameters]);
	parameterButton.setVisible(dynamic_cast<NodeContainer*>(parent.node.get()) != nullptr);

	freezeButton.setEnabled(parent.node->getRootNetwork()->canBeFrozen());

	freezeButton.setToggleStateAndUpdateIcon(parent.node->getRootNetwork()->isFrozen());
	
	if (!freezeButton.isEnabled())
		freezeButton.setAlpha(0.1f);
}


void NodeComponent::Header::buttonClicked(Button* b)
{
	if (b == &powerButton)
	{
		parent.node->setValueTreeProperty(PropertyIds::Bypassed, !b->getToggleState());
	}
	if (b == &deleteButton)
	{
		parent.node->getRootNetwork()->deselect(parent.node.get());

		parent.dataReference.getParent().removeChild(
			parent.dataReference, parent.node->getUndoManager());
	}
	if (b == &freezeButton)
	{
		parent.node->getRootNetwork()->setUseFrozenNode(b->getToggleState());
		parent.repaint();
	}
	if (b == &parameterButton)
	{
		parent.dataReference.setProperty(PropertyIds::ShowParameters, b->getToggleState(), nullptr);
	}
}


void NodeComponent::Header::updatePowerButtonState(Identifier id, var newValue)
{
	powerButton.setToggleStateAndUpdateIcon(!(bool)newValue);
	repaint();
}

void NodeComponent::Header::mouseDoubleClick(const MouseEvent& e)
{
	if (powerButton.getBoundsInParent().expanded(2).contains(e.getPosition()))
	{
		parent.node->connectToBypass({});
		findParentComponentOfClass<DspNetworkGraph>()->repaint();
		return;
	}

	parent.dataReference.setProperty(PropertyIds::Folded, !parent.isFolded(), nullptr);
	parent.getParentComponent()->repaint();
}


void NodeComponent::Header::resized()
{
	auto b = getLocalBounds();

	powerButton.setBounds(b.removeFromLeft(getHeight()).reduced(3));

	parameterButton.setBounds(b.removeFromLeft(getHeight()).reduced(3));

	deleteButton.setBounds(b.removeFromRight(getHeight()).reduced(3));
	freezeButton.setBounds(deleteButton.getBounds());

	powerButton.setVisible(!parent.isRoot());
	deleteButton.setVisible(!parent.isRoot());
	freezeButton.setVisible(parent.isRoot());
}

void NodeComponent::Header::mouseDown(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (e.mods.isRightButtonDown())
	{
		parent.handlePopupMenuResult((int)MenuActions::EditProperties);
	}
}

void NodeComponent::Header::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	if (e.mods.isRightButtonDown())
	{
		return;
	}

	auto graph = findParentComponentOfClass<DspNetworkGraph>();

	if (isDragging)
		graph->finishDrag();
	else
	{
		parent.node->getRootNetwork()->addToSelection(parent.node.get(), e.mods);
	}
		
}

void NodeComponent::Header::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	if (isDragging)
	{
		d.dragComponent(&parent, e, nullptr);
		parent.getParentComponent()->repaint();
		bool copyNode = e.mods.isAltDown();

		if (copyNode != parent.isBeingCopied())
			repaint();

		findParentComponentOfClass<DspNetworkGraph>()->updateDragging(parent.getParentComponent()->getLocalPoint(this, e.getPosition()), copyNode);

		return;
	}

	auto distance = e.getDistanceFromDragStart();

	

	

	if (!parent.isRoot() && distance > 25)
	{
		isDragging = true;

		if (findParentComponentOfClass<DspNetworkGraph>()->setCurrentlyDraggedComponent(&parent))
			d.startDraggingComponent(&parent, e);
	}
}


bool NodeComponent::Header::isInterestedInDragSource(const SourceDetails& details)
{
	if (dynamic_cast<cable::dynamic::editor*>(details.sourceComponent.get()) != nullptr)
		return false;

	return dynamic_cast<SoftBypassNode*>(parent.node.get()) != nullptr;
}

void NodeComponent::Header::paint(Graphics& g)
{
	auto textArea = getLocalBounds().toFloat();

	auto leftMargin = 0.0f;
	auto rightMargin = 0.0f;

	g.setColour(parent.getOutlineColour());
	g.fillAll();

	auto b = getLocalBounds();
	b.removeFromLeft(1);
	b.removeFromRight(1);
	b.removeFromTop(1);

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x2b000000)));
	g.fillRect(b);

	
	g.setFont(GLOBAL_BOLD_FONT());

	String s = parent.dataReference[PropertyIds::ID].toString();

	if (parent.node.get()->isPolyphonic())
		s << " [poly]";

	
	if (parent.node->getRootNetwork()->getCpuProfileFlag())
	{
		s << parent.node->getCpuUsageInPercent();
	}

	leftMargin += textArea.getHeight();

	if(parameterButton.isVisible())
		leftMargin += textArea.getHeight();

	if(freezeButton.isVisible())
		rightMargin += textArea.getHeight();


	auto ar = getLocalBounds().toFloat();
	ar.removeFromRight(ar.getHeight());

	rightMargin += textArea.getHeight();

	if (parent.node->isClone())
	{
		g.setColour(Colours::white.withAlpha(parent.node->isBypassed() ? 0.05f : 0.2f));
		auto p = f.createPath("clone");
		f.scalePath(p, ar.removeFromRight(ar.getHeight()).reduced(5.0f));

		rightMargin += textArea.getHeight();

		g.fillPath(p);
	}

	if (parent.node->isProcessingHiseEvent())
	{
		auto hasMidiParent = (parent.node->findParentNodeOfType<MidiChainNode>() != nullptr || parent.node->getRootNetwork()->isPolyphonic()) &&
							 parent.node->findParentNodeOfType<NoMidiChainNode>() == nullptr;

		Path p;
		p.loadPathFromData(HiBinaryData::SpecialSymbols::midiData, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::midiData));

		PathFactory::scalePath(p, ar.removeFromRight(ar.getHeight()).reduced(4.0f));

		rightMargin += textArea.getHeight();

		g.setColour(Colours::white.withAlpha(hasMidiParent ? 0.5f : 0.1f));
		g.fillPath(p);
	}

	if (isHoveringOverBypass)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(powerButton.getBounds().expanded(3).toFloat(), 1.0f);
	}

	textArea.removeFromLeft(jmax(leftMargin, rightMargin));
	textArea.removeFromRight(jmax(leftMargin, rightMargin));

	g.setColour(Colours::white.withAlpha(parent.node->isBypassed() ? 0.5f : 1.0f));
	g.drawText(s, textArea, Justification::centred);
}


NodeComponent::NodeComponent(NodeBase* b) :
	dataReference(b->getValueTree()),
	node(b),
	header(*this)
{
	if (auto en = node->getEmbeddedNetwork())
	{
		addAndMakeVisible(embeddedNetworkBar = new EmbeddedNetworkBar(b));
	}

	node->getRootNetwork()->addSelectionListener(this);

	setName(b->getId());
	addAndMakeVisible(header);
	setOpaque(false);

	repaintListener.setCallback(dataReference, {PropertyIds::ID, PropertyIds::NodeColour},
		valuetree::AsyncMode::Asynchronously,
		[this](Identifier id, var)
	{
		if (id == PropertyIds::NodeColour)
		{
			if (auto dng = findParentComponentOfClass<DspNetworkGraph>())
				dng->repaint();
		}

		repaint();
	});
}


NodeComponent::~NodeComponent()
{
	if(node != nullptr)
		node->getRootNetwork()->removeSelectionListener(this);

	node = nullptr;
}


void NodeComponent::paint(Graphics& g)
{

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff353535)));

	g.fillAll();

	auto b = getLocalBounds().toFloat();
	b.removeFromTop(header.getHeight());

	drawTopBodyGradient(g, b, JUCE_LIVE_CONSTANT_OFF(0.15f));

	g.setColour(getOutlineColour());
	g.drawRect(getLocalBounds().toFloat(), 1.0f);

    g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x69181818)));
    
    
    b.removeFromLeft(2.0f);
    b.removeFromRight(2.0f);
    b.removeFromBottom(2.0f);
    b.removeFromTop(1.0f);
    
    g.drawRect(b, 1.0f);
}


void NodeComponent::paintOverChildren(Graphics& g)
{
	if (isRoot() && node->getRootNetwork()->isFrozen())
	{
		auto b = getLocalBounds().reduced(1);
		b.removeFromTop(header.getHeight());

		if (header.parameterButton.getToggleState())
			b.removeFromTop(UIValues::ParameterHeight);

		auto hashMatches = node->getRootNetwork()->hashMatches();

		g.setColour(hashMatches ? Colour(0xEE171717) : Colour(0xEE221111));
		g.fillRect(b);

		g.setFont(GLOBAL_BOLD_FONT());

		if (!hashMatches)
		{
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("The compiled node doesn't match the interpreted network. Recompile this node in order to ensure consistent behaviour", b.toFloat(), Justification::centred);
		}

		Path fp;
		fp.loadPathFromData(HnodeIcons::freezeIcon, SIZE_OF_PATH(HnodeIcons::freezeIcon));
		auto pSize = jmin(200, getWidth(), getHeight());
		auto bl = b.withSizeKeepingCentre(pSize, pSize).toFloat();
		PathFactory::scalePath(fp, bl);

		g.setColour(Colours::white.withAlpha(0.1f));
		g.strokePath(fp, PathStrokeType(4.0f));
		g.fillPath(fp);
		
		auto tb = bl.removeFromBottom(24.0f).translated(0.0f, 40.0f);
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawText("Using the project DLL node", tb, Justification::centred);


	}

	if (isSelected())
	{
		UnblurryGraphics ug(g, *this, true);

		g.setColour(Colour(SIGNAL_COLOUR));
		ug.draw1PxRect(getLocalBounds().toFloat());
	}

	if (isBeingCopied())
	{
		Path p;
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::addIcon,
			sizeof(HiBinaryData::ProcessorEditorHeaderIcons::addIcon));

		auto b = getLocalBounds().toFloat().withSizeKeepingCentre(32, 32);

		p.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), true);
		g.setColour(Colours::white.withAlpha(0.6f));
		g.fillPath(p);
	}

	auto& exceptionHandler = node->getRootNetwork()->getExceptionHandler();

	auto error = exceptionHandler.getErrorMessage(node.get());

	if (error.isNotEmpty())
	{
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xaa683333)));

		g.drawRect(getLocalBounds().reduced(1), 1);

		auto b = getLocalBounds();
		b.removeFromTop(header.getHeight());

		g.fillRect(b.reduced(1));

		auto f = GLOBAL_BOLD_FONT();

		g.setFont(f);
		
		MarkdownRenderer mp(error);
		mp.getStyleData().fontSize = 13.0f;
		
		mp.parse();
		mp.getHeightForWidth(getWidth() - 20.0f);

		mp.draw(g, b.toFloat().reduced(20.0f));
	}
}


void NodeComponent::resized()
{
	auto b = getLocalBounds();

	header.setBounds(b.removeFromTop(24));

	if (embeddedNetworkBar != nullptr)
		embeddedNetworkBar->setBounds(b.removeFromTop(24));
}


hise::MarkdownLink NodeComponent::getLink() const
{
	if (node != nullptr)
	{
		auto path = node->getValueTree()[PropertyIds::FactoryPath].toString().replaceCharacter('.', '/');
		String s;
		
		s << "scriptnode/list/" << path << "/";

		return { File(), s };
	}

	return {};
}

void NodeComponent::selectionChanged(const NodeBase::List& selection)
{
	bool nowSelected = selection.contains(node.get());

	if (wasSelected != nowSelected)
	{
		wasSelected = nowSelected;
		repaint();
	}
}




void NodeComponent::fillContextMenu(PopupMenu& m)
{
	m.addItem((int)MenuActions::ExportAsSnippet, "Export as snippet");
	m.addItem((int)MenuActions::EditProperties, "Edit Properties");

#if 0
	if (auto hc = node.get()->getAsRestorableNode())
	{
		m.addItem((int)MenuActions::UnfreezeNode, "Unfreeze hardcoded node");
	}
	else if (node->getValueTree().hasProperty(PropertyIds::FreezedId) ||
		node->getValueTree().hasProperty(PropertyIds::FreezedPath))
	{
		m.addItem((int)MenuActions::FreezeNode, "Replace with hardcoded version");
	}
#endif

	m.addSectionHeader("Wrap into container");
	m.addItem((int)MenuActions::WrapIntoChain, "Chain");
	m.addItem((int)MenuActions::WrapIntoSplit, "Split");
	m.addItem((int)MenuActions::WrapIntoMulti, "Multi");
	m.addItem((int)MenuActions::WrapIntoFrame, "Frame");
	m.addItem((int)MenuActions::WrapIntoOversample4, "Oversample(4x)");

	m.addSectionHeader("Surround with nodes");
	m.addItem((int)MenuActions::SurroundWithFeedback, "send / receive");
	m.addItem((int)MenuActions::SurroundWithMSDecoder, "ms_encode / ms_decode");
}

void NodeComponent::handlePopupMenuResult(int result)
{
#if USE_BACKEND
	if (result == (int)MenuActions::CreateScreenShot)
	{
		auto mc = node->getScriptProcessor()->getMainController_();

		auto docRepo = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject().getSetting(HiseSettings::Documentation::DocRepository).toString();

		if (docRepo.isNotEmpty())
		{
			auto targetDirectory = File(docRepo).getChildFile("images/scriptnode/");

			auto imageFile = targetDirectory.getChildFile(node->getId()).withFileExtension("png");

			targetDirectory.createDirectory();

			auto g = findParentComponentOfClass<DspNetworkGraph>();

			auto imgBounds = g->getLocalArea(this, getLocalBounds());

			auto img = g->createComponentSnapshot(imgBounds, true);

			PNGImageFormat pngFormat;
			FileOutputStream fos(imageFile);
			if (pngFormat.writeImageToStream(img, fos))
			{
				PresetHandler::showMessageWindow("Screenshot added to repository", "The screenshot was saved at " + imageFile.getFullPathName());
			}
		}
	}
	if (result == (int)MenuActions::EditProperties)
	{
		auto n = new NodePopupEditor(this);
		
		auto g = findParentComponentOfClass<ZoomableViewport>();
		auto b = g->getLocalArea(this, getLocalBounds());

		g->setCurrentModalWindow(n, b);
	}
	if (result == (int)MenuActions::ExportAsSnippet)
	{
		auto data = "ScriptNode" + ValueTreeConverters::convertValueTreeToBase64(node->getValueTree(), true);

		SystemClipboard::copyTextToClipboard(data);
		PresetHandler::showMessageWindow("Copied to clipboard", "The node was copied to the clipboard");
	}
	if (result == (int)MenuActions::UnfreezeNode)
	{
		DspNetworkGraph::Actions::unfreezeNode(node.get());
	}
	if (result == (int)MenuActions::FreezeNode)
	{
		DspNetworkGraph::Actions::freezeNode(node.get());
	}
	if (result == (int)MenuActions::WrapIntoDspNetwork)
	{
		if (node->getRootNetwork()->getRootNode() == node.get())
		{
			PresetHandler::showMessageWindow("Nope", "You don't need to wrap the root node.  \n> Just tick the `AllowCompilation` flag in the properties, save this network and export the DLL");
			return;
		}

		auto wType = PopupHelpers::isWrappable(node.get());

		if (wType == 2)
		{
			auto id = node->getId();

			struct ConnectionState
			{
				ConnectionState(ValueTree v, bool isInput):
					sourceTree(v),
					input(isInput)
				{

				}

				void removeOldConnection(NodeBase::Ptr old)
				{
					oldParent = sourceTree.getParent();
					oldParent.removeChild(sourceTree, old->getUndoManager());
				}

				void addNewConnection(NodeBase::Ptr n)
				{
					if (input)
					{
						auto oldId = sourceTree[PropertyIds::NodeId].toString();
						auto newId = n->getId();
						sourceTree.setProperty(PropertyIds::NodeId, newId, n->getUndoManager());
						oldParent.addChild(sourceTree, -1, n->getUndoManager());
					}
					else
					{
						auto tTree = n->getValueTree().getOrCreateChildWithName(PropertyIds::ModulationTargets, n->getUndoManager());
						tTree.addChild(sourceTree, -1, n->getUndoManager());
					}
					
				}

				ValueTree sourceTree;
				ValueTree oldParent;
				const bool input;
			};

			Array<ConnectionState> existingConnections;

			for (auto p: NodeBase::ParameterIterator(*node))
			{
				auto sourceConnection = p->getConnectionSourceTree(true);

				if(sourceConnection.isValid())
					existingConnections.add(ConnectionState(sourceConnection, true));
			}

			if (auto modNode = dynamic_cast<ModulationSourceNode*>(node.get()))
			{
				for (auto c : modNode->getModulationTargetTree())
				{
					existingConnections.add(ConnectionState(c, false));
				}
			}

			for (auto& c : existingConnections)
			{
				c.removeOldConnection(node.get());
			}

			if (id == node->getPath().getIdentifier().toString())
			{
				id = PresetHandler::getCustomName(id, "Enter a customized name for the node");
			}

			String newId = id + "_";

			node->setValueTreeProperty(PropertyIds::ID, newId);

			PopupHelpers::wrapIntoChain(node.get(), MenuActions::WrapIntoChain, id);

			auto pn = node->getParentNode();
			pn->getValueTree().setProperty(PropertyIds::ShowParameters, true, node->getUndoManager());

			if (auto modNode = dynamic_cast<ModulationSourceNode*>(node.get()))
			{
				String pmodId = id + "_pm";
				var pmodvar = node->getRootNetwork()->create("routing.public_mod", pmodId);

				auto pmod = dynamic_cast<NodeBase*>(pmodvar.getObject());

				pmod->setParent(var(pn), 1);

				jassert(modNode != nullptr);

				modNode->addModulationConnection(0, pmod->getParameterFromIndex(0));
			}

			for(auto p: NodeBase::ParameterIterator(*node))
				pn->getParameterTree().addChild(p->data.createCopy(), -1, node->getUndoManager());

			for (int i = 0; i < node->getNumParameters(); i++)
			{
				auto m = dynamic_cast<NodeContainer::MacroParameter*>(pn->getParameterFromIndex(i));
				m->addTarget(node->getParameterFromIndex(i));
			}

			PopupHelpers::wrapIntoNetwork(pn, true);

			for (auto& c : existingConnections)
				c.addNewConnection(pn);
		}

		if (wType == 1)
		{
			

			PopupHelpers::wrapIntoNetwork(node.get(), PresetHandler::showYesNoWindow("Compile this network", "Do you want to include the new network into the compilation?", PresetHandler::IconType::Question));
		}
	}
	if (result >= (int)MenuActions::WrapIntoChain && result <= (int)MenuActions::WrapIntoOversample4)
	{
		PopupHelpers::wrapIntoChain(node.get(), (MenuActions)result);
	}
	if (result >= (int)MenuActions::SurroundWithFeedback && result <= (int)MenuActions::SurroundWithMSDecoder)
	{
		bool addFeedback = result == (int)MenuActions::SurroundWithFeedback;

		auto network = node->getRootNetwork();

		String firstNode, secondNode;

		if (addFeedback)
		{
			firstNode = "routing.receive";
			secondNode = "routing.send";
		}
		else
		{
			firstNode = "routing.ms_decode";
			secondNode = "routing.ms_encode";
		}

		auto thisTree = node->getValueTree();
		auto parent = thisTree.getParent();
		String firstId;

		if (auto fn = dynamic_cast<NodeBase*>(network->create(firstNode, "").getObject()))
		{
			int insertIndex = parent.indexOf(thisTree);
			parent.addChild(fn->getValueTree(), insertIndex, node->getUndoManager());

			firstId = fn->getId();
		}

		if (auto sn = dynamic_cast<NodeBase*>(network->create(secondNode, "").getObject()))
		{
			int insertIndex = parent.indexOf(thisTree);
			parent.addChild(sn->getValueTree(), insertIndex + 1, node->getUndoManager());

			sn->setNodeProperty(PropertyIds::Connection, firstId);
		}
	}
#endif
	
}

juce::Colour NodeComponent::getOutlineColour() const
{
	if (isRoot())
		return dynamic_cast<const Processor*>(node->getScriptProcessor())->getColour();

	auto& exceptionHandler = node->getRootNetwork()->getExceptionHandler();

	if (!exceptionHandler.isOk())
	{
		auto error = exceptionHandler.getErrorMessage(node.get());

		if (error.isNotEmpty())
				return JUCE_LIVE_CONSTANT_OFF(Colour(0xFFFF0000));
	}

	if (!header.colour.isTransparent())
		return header.colour;

	return header.isDragging ? Colour(0x88444444) : Colour(0xFF555555);
}

juce::Colour NodeComponent::getHeaderColour() const
{
	if (!header.colour.isTransparent())
		return header.colour;

	return Colours::white;
}

void NodeComponent::drawTopBodyGradient(Graphics& g, Rectangle<float> b, float alpha/*=0.1f*/, float height/*=20.0f*/)
{
	b = b.removeFromTop(height);
	g.setGradientFill(ColourGradient(Colours::black.withAlpha(alpha), 0.0f, b.getY(), Colours::transparentBlack, 0.0f, b.getBottom(), false));
	g.fillRect(b);
}

bool NodeComponent::isRoot() const
{
	return node->getRootNetwork()->getRootNode() == node.get();
}

bool NodeComponent::isFolded() const
{
	return (bool)dataReference[PropertyIds::Folded];
}

bool NodeComponent::isDragged() const
{
	if(auto ng = findParentComponentOfClass<DspNetworkGraph>())
		return ng->currentlyDraggedComponent == this;

	return false;
}

bool NodeComponent::isSelected() const
{
	return node->getRootNetwork()->isSelected(node.get());
}


bool NodeComponent::isBeingCopied() const
{
	return isDragged() && findParentComponentOfClass<DspNetworkGraph>()->copyDraggedNode;
}


DeactivatedComponent::DeactivatedComponent(NodeBase* b) :
	NodeComponent(b)
{
	header.setEnabled(false);
	setOpaque(true);
}


void DeactivatedComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF555555));
	g.setColour(Colour(0xFF888888));
	g.drawRect(getLocalBounds());
}


void DeactivatedComponent::resized()
{

}


Component* NodeComponentFactory::createComponent(NodeBase* node)
{
	return dynamic_cast<juce::Component*>(node->createComponent());
}

String NodeComponentFactory::getId() const
{ return "Scriptnode"; }

juce::Array<hise::PathFactory::Description> NodeComponentFactory::getDescription() const
{
	Array<Description> d;

	auto addD = [&d](const String& name)
	{
		d.add({ name, name });
	};

	addD("on");
	addD("fold");
	addD("delete");
	addD("move");
	addD("goto");
	addD("parameter");
	addD("split");
	addD("chain");
	addD("multi");
	addD("modchain");
	addD("midichain");
	addD("oversample2x");
	addD("oversample4x");
	addD("oversample8x");
	addD("newnode");
	addD("oldnode");
	addD("clipboard");

	return d;
}

juce::Path NodeComponentFactory::createPath(const String& id) const
{
	Path p;
	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

	LOAD_EPATH_IF_URL("on", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	LOAD_EPATH_IF_URL("fold", HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon);
	LOAD_EPATH_IF_URL("delete", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
	LOAD_PATH_IF_URL("move", ColumnIcons::moveIcon);
	LOAD_PATH_IF_URL("goto", ColumnIcons::targetIcon);
	LOAD_EPATH_IF_URL("parameter", HiBinaryData::SpecialSymbols::macros);
	LOAD_EPATH_IF_URL("split", ScriptnodeIcons::splitIcon);
	LOAD_EPATH_IF_URL("freeze", HnodeIcons::freezeIcon);
	LOAD_EPATH_IF_URL("chain", ScriptnodeIcons::chainIcon);
	LOAD_EPATH_IF_URL("branch", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	LOAD_EPATH_IF_URL("multi", ScriptnodeIcons::multiIcon);
	LOAD_EPATH_IF_URL("modchain", ScriptnodeIcons::modIcon);
	LOAD_EPATH_IF_URL("midichain", HiBinaryData::SpecialSymbols::midiData);
	LOAD_EPATH_IF_URL("oversample2x", ScriptnodeIcons::os2Icon);
	LOAD_EPATH_IF_URL("oversample4x", ScriptnodeIcons::os4Icon);
	LOAD_EPATH_IF_URL("oversample8x", ScriptnodeIcons::os8Icon);
	LOAD_EPATH_IF_URL("clipboard", SampleMapIcons::pasteSamples);
	LOAD_EPATH_IF_URL("newnode", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
	LOAD_EPATH_IF_URL("oldnode", EditorIcons::swapIcon);
	LOAD_EPATH_IF_URL("clone", SampleMapIcons::copySamples);

	if (url.startsWith("fix"))
		p.loadPathFromData(ScriptnodeIcons::fixIcon, ScriptnodeIcons::fixIcon_Size);
	
	if (url.contains("frame"))
		p.loadPathFromData(ScriptnodeIcons::frameIcon, ScriptnodeIcons::frameIcon_Size);
	
	return p;
}



juce::Path NodeComponent::EmbeddedNetworkBar::Factory::createPath(const String& url) const
{
	Path p;
	LOAD_EPATH_IF_URL("freeze", HnodeIcons::freezeIcon);
	LOAD_PATH_IF_URL("goto", ColumnIcons::openWorkspaceIcon);
	LOAD_EPATH_IF_URL("warning", EditorIcons::warningIcon);
	return p;
}

NodeComponent::EmbeddedNetworkBar::EmbeddedNetworkBar(NodeBase* n) :
	parentNode(n),
	embeddedNetwork(n->getEmbeddedNetwork()),
	warningButton("warning", this, f),
	freezeButton("freeze", this, f),
	gotoButton("goto", this, f)
{
	jassert(embeddedNetwork != nullptr);

	addAndMakeVisible(warningButton);

	warningButton.setVisible(!n->getEmbeddedNetwork()->hashMatches() & embeddedNetwork->canBeFrozen());

	addAndMakeVisible(gotoButton);
	addAndMakeVisible(freezeButton);

	if (!embeddedNetwork->canBeFrozen())
	{
		freezeButton.setEnabled(false);
		freezeButton.setAlpha(0.1f);
	}
	else
	{
		freezeUpdater.setCallback(parentNode->getValueTree(), { PropertyIds::Frozen }, valuetree::AsyncMode::Asynchronously,
			BIND_MEMBER_FUNCTION_2(EmbeddedNetworkBar::updateFreezeState));
	}
	
	freezeButton.setToggleModeWithColourChange(true);
	freezeButton.setToggleStateAndUpdateIcon(parentNode->getValueTree()[PropertyIds::Frozen]);

	setSize(100, 24);
}

void NodeComponent::EmbeddedNetworkBar::buttonClicked(Button* b)
{
	if (b == &warningButton)
	{

	}
	if (b == &freezeButton)
	{
		parentNode->setValueTreeProperty(PropertyIds::Frozen, b->getToggleState());
	}
	if (b == &gotoButton)
	{
		findParentComponentOfClass<ZoomableViewport>()->setNewContent(new DspNetworkGraph(embeddedNetwork), getParentComponent());
	}
}

void NodeComponent::EmbeddedNetworkBar::resized()
{
	auto b = getLocalBounds();
	freezeButton.setBounds(b.removeFromRight(b.getHeight()).reduced(2));
	if (warningButton.isVisible())
		warningButton.setBounds(b.removeFromRight(b.getHeight()).reduced(2));

	gotoButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(4));
}

void NodeComponent::EmbeddedNetworkBar::updateFreezeState(const Identifier& id, const var& newValue)
{
	freezeButton.setToggleStateAndUpdateIcon((bool)newValue);
}

int NodeComponent::PopupHelpers::isWrappable(NodeBase* n)
{
	if (n == nullptr)
		return 0;

	auto nodeTree = n->getValueTree();

	auto p = n->getPath();

	auto isOptionalSnex = snex::cppgen::CustomNodeProperties::nodeHasProperty(nodeTree, PropertyIds::IsOptionalSnexNode);

	if(isOptionalSnex)
	{
		auto mode = n->getNodeProperty(PropertyIds::Mode).toString();

		isOptionalSnex = mode == "Custom";
	}

	auto isSnex = p.getIdentifier().toString().contains("snex");
	auto isChain = p == NamespacedIdentifier::fromString("container::chain");
	auto isExpression = p.getIdentifier().toString().endsWith("expr");

	if (isChain)
		return 1;

	if (isSnex || isOptionalSnex || isExpression)
		return 2;

	return 0;
}

void NodeComponent::PopupHelpers::wrapIntoNetwork(NodeBase* node, bool makeCompileable)
{
#if USE_BACKEND
	ValueTree nData(PropertyIds::Network);

	auto rootTree = node->getRootNetwork()->getValueTree();

	

	// mirror all properties from the parent network;
	for (int i = 0; i < rootTree.getNumProperties(); i++)
		nData.setProperty(rootTree.getPropertyName(i), rootTree.getProperty(rootTree.getPropertyName(i)), nullptr);

	nData.setProperty(PropertyIds::ID, node->getId(), nullptr);
	nData.addChild(node->getValueTree().createCopy(), -1, nullptr);

	auto ndir = BackendDllManager::getSubFolder(node->getScriptProcessor()->getMainController_(), BackendDllManager::FolderSubType::Networks);

	auto tFile = ndir.getChildFile(node->getId()).withFileExtension("xml");

	if (makeCompileable)
    {
		nData.setProperty(PropertyIds::AllowCompilation, makeCompileable, nullptr);
        nData.setProperty(PropertyIds::CompileChannelAmount, node->getCurrentChannelAmount(), nullptr);
    }

	if (!tFile.existsAsFile() || PresetHandler::showYesNoWindow("Overwrite file", "Do you want to overwrite the file " + tFile.getFileName()))
	{
		auto xml = nData.createXml();
		tFile.replaceWithText(xml->createDocument(""));

		auto newPath = "project." + node->getId();
		node->setValueTreeProperty(PropertyIds::FactoryPath, newPath);

		PresetHandler::showMessageWindow("Exported chain as new network", "Reload this patch to apply the change");
		return;
	}
#endif
}

void NodeComponent::PopupHelpers::wrapIntoChain(NodeBase* node, MenuActions result, String idToUse)
{
#if USE_BACKEND
	auto framePath = "container.frame" + String(node->getCurrentChannelAmount()) + "_block";

	StringArray ids = { "container.chain", "container.split", "container.multi",
						  framePath, "container.fix32_block", "container.midichain", "container.no_midi", "container.clone", "container.soft_bypass", "container.oversample4x"};

	int idIndex = (int)result - (int)MenuActions::WrapIntoChain;

	auto path = ids[idIndex];

	int index = 1;

	if (idToUse.isEmpty())
	{
		idToUse = "wrap" + node->getId() + String(index);
	}

	auto network = node->getRootNetwork();

	while (network->get(idToUse).isObject())
	{
		index++;
		idToUse = "wrap" + node->getId() + String(index);
	}

	auto newNode = node->getRootNetwork()->create(path, idToUse);

	if (auto newContainer = dynamic_cast<NodeBase*>(newNode.getObject()))
	{
		auto containerTree = newContainer->getValueTree();

		newContainer->setValueTreeProperty(PropertyIds::NodeColour, node->getValueTree()[PropertyIds::NodeColour]);

		auto um = node->getUndoManager();

		auto selection = network->getSelection();

		if (selection.isEmpty() || !selection.contains(node))
		{
			auto nodeTree = node->getValueTree();
			auto parent = nodeTree.getParent();
			auto nIndex = parent.indexOf(nodeTree);

            
            
			parent.removeChild(nodeTree, um);
			containerTree.getChildWithName(PropertyIds::Nodes).addChild(nodeTree, -1, um);

			jassert(!containerTree.getParent().isValid());

			parent.addChild(containerTree, nIndex, um);
		}
		else
		{
			auto parent = selection.getFirst()->getValueTree().getParent();
			auto nIndex = parent.indexOf(selection.getFirst()->getValueTree());

            
            
			for (auto n : selection)
			{
                n->setParent(newContainer, -1);
                
				//n->getValueTree().getParent().removeChild(n->getValueTree(), um);
				//containerTree.getChildWithName(PropertyIds::Nodes).addChild(n->getValueTree(), -1, um);
			}

			parent.addChild(containerTree, nIndex, um);
		}
	}
#endif
}

simple_visualiser::simple_visualiser(NodeBase*, PooledUIUpdater* u) :
	ScriptnodeExtraComponent<NodeBase>(nullptr, u)
{
	setSize(100, 60);
}

scriptnode::NodeBase* simple_visualiser::getNode()
{
	auto nc = findParentComponentOfClass<NodeComponent>();
	auto n = nc->node.get();
	return n;
}

double simple_visualiser::getParameter(int index)
{
	if (auto n = getNode())
	{
		jassert(isPositiveAndBelow(index, n->getNumParameters()));
		return n->getParameterFromIndex(index)->getValue();
	}

	return 0.0;
}

InvertableParameterRange simple_visualiser::getParameterRange(int index)
{
    if (auto n = getNode())
    {
        jassert(isPositiveAndBelow(index, n->getNumParameters()));
        return RangeHelpers::getDoubleRange(n->getParameterFromIndex(index)->data);
    }

    return {};
}

juce::Colour simple_visualiser::getNodeColour()
{
	auto c = findParentComponentOfClass<NodeComponent>()->header.colour;

	if (c == Colours::transparentBlack)
		return Colour(0xFFAAAAAA);

	return c;
}

void simple_visualiser::timerCallback()
{
	original.clear();
	p.clear();
	gridPath.clear();
	rebuildPath(p);

	auto b = getLocalBounds().toFloat().reduced(4.0f);

	if (!p.getBounds().isEmpty())
		p.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);

	if (!original.getBounds().isEmpty())
	{
		original.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);

		auto cp = original;

		float l[2] = { 2.0f, 2.0f };
		PathStrokeType(1.5f).createDashedStroke(original, cp, l, 2);
	}

	if (!gridPath.getBounds().isEmpty())
	{
		gridPath.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);
	}

	repaint();
}

void simple_visualiser::paint(Graphics& g)
{
	if (drawBackground)
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, getLocalBounds().toFloat(), false);

	if (!gridPath.isEmpty())
	{
		UnblurryGraphics ug(g, *this);

		g.setColour(Colours::black.withAlpha(0.5f));
		g.strokePath(gridPath, PathStrokeType(ug.getPixelSize()));
	}

	g.setColour(getNodeColour());

	if (!original.isEmpty())
		g.fillPath(original);

	g.strokePath(p, PathStrokeType(thickness));
}

}
