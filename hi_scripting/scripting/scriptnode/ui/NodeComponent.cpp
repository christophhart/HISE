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
			if (path.contains("frame") || path.contains("oversample"))
				return "chain";
			else
				return "on";
		}
		else
			return path;
	}
		
	return "on";
}



NodeComponent::Header::Header(NodeComponent& parent_) :
	parent(parent_),
	powerButton(getPowerButtonId(false), this, f, getPowerButtonId(true)),
	deleteButton("delete", this, f),
	parameterButton("parameter", this, f)
{
	powerButton.setToggleModeWithColourChange(true);
	
	powerButtonUpdater.setCallback(parent.node->getValueTree(), { PropertyIds::Bypassed, PropertyIds::DynamicBypass},
		valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(NodeComponent::Header::updatePowerButtonState));

	colourUpdater.setCallback(parent.node->getValueTree(), { PropertyIds::NodeColour }, valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(NodeComponent::Header::updateColour));

	addAndMakeVisible(powerButton);
	addAndMakeVisible(deleteButton);
	addAndMakeVisible(parameterButton);
	
	parameterButton.setToggleModeWithColourChange(true);
	parameterButton.setToggleStateAndUpdateIcon(parent.dataReference[PropertyIds::ShowParameters]);
	parameterButton.setVisible(dynamic_cast<NodeContainer*>(parent.node.get()) != nullptr);
}


void NodeComponent::Header::buttonClicked(Button* b)
{
	if (b == &powerButton)
	{
		parent.node->setValueTreeProperty(PropertyIds::Bypassed, !b->getToggleState());
	}
	if (b == &deleteButton)
	{
		parent.dataReference.getParent().removeChild(
			parent.dataReference, parent.node->getUndoManager());
	}
	if (b == &parameterButton)
	{
		parent.dataReference.setProperty(PropertyIds::ShowParameters, b->getToggleState(), nullptr);
	}
}


void NodeComponent::Header::updatePowerButtonState(Identifier id, var newValue)
{
	if (id == PropertyIds::DynamicBypass)
	{
		auto nv = newValue.toString().isEmpty();
		powerButton.setVisible(nv);
	}
	else
	{
		powerButton.setToggleStateAndUpdateIcon(!(bool)newValue);
		repaint();
	}
}

void NodeComponent::Header::mouseDoubleClick(const MouseEvent& event)
{
	parent.dataReference.setProperty(PropertyIds::Folded, !parent.isFolded(), nullptr);
	parent.getParentComponent()->repaint();
}


void NodeComponent::Header::resized()
{
	auto b = getLocalBounds();

	powerButton.setBounds(b.removeFromLeft(getHeight()).reduced(3));

	parameterButton.setBounds(b.removeFromLeft(getHeight()).reduced(3));

	deleteButton.setBounds(b.removeFromRight(getHeight()).reduced(3));

	powerButton.setVisible(!parent.isRoot());
	deleteButton.setVisible(!parent.isRoot());
}

void NodeComponent::Header::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);
		
		parent.fillContextMenu(m);

		int result = m.show();

		if(result > 0)
			parent.handlePopupMenuResult(result);
	}
}

void NodeComponent::Header::mouseUp(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		return;
	}

	auto graph = findParentComponentOfClass<DspNetworkGraph>();

	if (isDragging)
		graph->finishDrag();
	else
	{
		parent.node->getRootNetwork()->addToSelection(parent.node, e.mods);
	}
		
}

void NodeComponent::Header::mouseDrag(const MouseEvent& e)
{
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

	if (distance > 25)
	{
		isDragging = true;

		if (findParentComponentOfClass<DspNetworkGraph>()->setCurrentlyDraggedComponent(&parent))
			d.startDraggingComponent(&parent, e);
	}
}


void NodeComponent::Header::paint(Graphics& g)
{
	g.setColour(parent.getOutlineColour());
	g.fillAll();

	
	g.setFont(GLOBAL_BOLD_FONT());



	String s = parent.dataReference[PropertyIds::ID].toString();

	if (parent.node.get()->isPolyphonic())
		s << " [poly]";

	

	auto textWidth = GLOBAL_BOLD_FONT().getStringWidthFloat(s) + 10.0f;

	auto colourBounds = getLocalBounds().reduced(3, 6);

	if (powerButton.isVisible())
		colourBounds.removeFromLeft(powerButton.getWidth() + 6);

	if (parameterButton.isVisible())
		colourBounds.removeFromLeft(parameterButton.getWidth() + 6);

	if (deleteButton.isVisible())
		colourBounds.removeFromRight(deleteButton.getWidth() + 6);

	if (!colour.isTransparent())
	{
		g.setColour(colour);
		
		auto textBounds = getLocalBounds().toFloat().withSizeKeepingCentre(textWidth, (float)getHeight());

		auto colourL = colourBounds.toFloat().withRight(textBounds.getX());
		auto colourR = colourBounds.toFloat().withLeft(textBounds.getRight());

		g.fillRoundedRectangle(colourL, 2.0f);
		g.fillRoundedRectangle(colourR, 2.0f);
	}

	


	g.setColour(Colours::white.withAlpha(parent.node->isBypassed() ? 0.5f : 1.0f));
	g.drawText(s, getLocalBounds(), Justification::centred);

	if (isHoveringOverBypass)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(powerButton.getBounds().expanded(3).toFloat(), 1.0f);
	}
}


NodeComponent::NodeComponent(NodeBase* b) :
	dataReference(b->getValueTree()),
	node(b),
	header(*this)
{
	node->getRootNetwork()->addSelectionListener(this);

	setName(b->getId());
	addAndMakeVisible(header);
	setOpaque(true);

	repaintListener.setCallback(dataReference, {PropertyIds::ID, PropertyIds::NumChannels},
		valuetree::AsyncMode::Asynchronously,
		[this](Identifier id, var)
	{
		repaint();
		
		if (id == PropertyIds::NumChannels && getParentComponent() != nullptr)
			getParentComponent()->repaint();
	});
}


NodeComponent::~NodeComponent()
{
	node->getRootNetwork()->removeSelectionListener(this);
}


void NodeComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF444444));
	g.setColour(getOutlineColour());
	g.drawRect(getLocalBounds());
}


void NodeComponent::paintOverChildren(Graphics& g)
{
	if (isSelected())
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(getLocalBounds(), 1);

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
}


void NodeComponent::resized()
{
	auto b = getLocalBounds();

	header.setBounds(b.removeFromTop(24));
}


void NodeComponent::selectionChanged(const NodeBase::List& selection)
{
	bool nowSelected = selection.contains(node);

	if (wasSelected != nowSelected)
	{
		wasSelected = nowSelected;
		repaint();
	}
}

void NodeComponent::handlePopupMenuResult(int result)
{
	if (result == (int)MenuActions::ExportAsCpp)
	{
		const String c = node->createCppClass(true);

		CodeHelpers::addFileToCustomFolder(node->getId(), c);

		SystemClipboard::copyTextToClipboard(c);

		String cppClassName = "custom." + node->getId();
		node->setValueTreeProperty(PropertyIds::FreezedPath, cppClassName);
	}
	if (result == (int)MenuActions::EditProperties)
	{
		auto n = new PropertyEditor(node, node->getValueTree());
		auto g = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();
		auto b = g->getLocalArea(this, header.getBounds());

		CallOutBox::launchAsynchronously(n, b, g);
	}
	if (result == (int)MenuActions::ExportAsSnippet)
	{
		auto data = "ScriptNode" + ValueTreeConverters::convertValueTreeToBase64(node->getValueTree(), true);

		SystemClipboard::copyTextToClipboard(data);
		PresetHandler::showMessageWindow("Copied to clipboard", "The node was copied to the clipboard");
	}
	if (result == (int)MenuActions::UnfreezeNode)
	{
		if (auto hc = node->getAsHardcodedNode())
		{
			// Check if there is already a node that was unfrozen

			for (auto n : node->getRootNetwork()->getListOfUnconnectedNodes())
			{
				if (n->getValueTree()[PropertyIds::FreezedId].toString() == node->getId())
				{
					auto newTree = n->getValueTree();
					auto oldTree = node->getValueTree();
					auto um = node->getUndoManager();

					auto f = [oldTree, newTree, um]()
					{
						auto p = oldTree.getParent();

						int position = p.indexOf(oldTree);
						p.removeChild(oldTree, um);
						p.addChild(newTree, position, um);
					};

					MessageManager::callAsync(f);
					return;
				}
			}

			auto t = hc->getSnippetText();

			if (t.isNotEmpty())
			{
				auto newTree = ValueTreeConverters::convertBase64ToValueTree(t, true);
				newTree = node->getRootNetwork()->cloneValueTreeWithNewIds(newTree);
				newTree.setProperty(PropertyIds::FreezedPath, node->getValueTree()[PropertyIds::FactoryPath], nullptr);
				newTree.setProperty(PropertyIds::FreezedId, node->getId(), nullptr);

				{
					auto oldTree = node->getValueTree();
					auto um = node->getUndoManager();

					node->getRootNetwork()->createFromValueTree(true, newTree, true);

					auto f = [oldTree, newTree, um]()
					{
						auto p = oldTree.getParent();

						int position = p.indexOf(oldTree);
						p.removeChild(oldTree, um);
						p.addChild(newTree, position, um);
					};

					MessageManager::callAsync(f);
				}
			}
		}
	}
	if (result == (int)MenuActions::FreezeNode)
	{
		auto freezedId = node->getValueTree()[PropertyIds::FreezedId].toString();

		if (freezedId.isNotEmpty())
		{
			if (auto fn = dynamic_cast<NodeBase*>(node->getRootNetwork()->get(freezedId).getObject()))
			{
				auto newTree = fn->getValueTree();
				auto oldTree = node->getValueTree();
				auto um = node->getUndoManager();

				auto f = [oldTree, newTree, um]()
				{
					auto p = oldTree.getParent();

					int position = p.indexOf(oldTree);
					p.removeChild(oldTree, um);
					p.addChild(newTree, position, um);
				};

				MessageManager::callAsync(f);
			}

			return;
		}

		auto freezedPath = node->getValueTree()[PropertyIds::FreezedPath].toString();

		if (freezedPath.isNotEmpty())
		{
			auto newNode = node->getRootNetwork()->create(freezedPath, node->getId() + "_freezed", node->isPolyphonic());

			if (auto nn = dynamic_cast<NodeBase*>(newNode.getObject()))
			{
				auto newTree = nn->getValueTree();
				auto oldTree = node->getValueTree();
				auto um = node->getUndoManager();

				auto f = [oldTree, newTree, um]()
				{
					auto p = oldTree.getParent();

					int position = p.indexOf(oldTree);
					p.removeChild(oldTree, um);
					p.addChild(newTree, position, um);
				};

				MessageManager::callAsync(f);
			}
		}
	}
	if (result >= (int)MenuActions::WrapIntoChain && result <= (int)MenuActions::WrapIntoOversample4)
	{
		auto framePath = "container.frame" + String(node->getNumChannelsToProcess()) + "_block";

		StringArray ids = { "container.chain", "container.split", "container.multi",
						      framePath, "container.oversample4x" };

		int idIndex = result - (int)MenuActions::WrapIntoChain;

		auto path = ids[idIndex];

		
		int index = 1;
		
		auto newId = "wrap" + node->getId() + String(index);

		auto network = node->getRootNetwork();

		while (network->get(newId).isObject())
		{
			index++;
			newId = "wrap" + node->getId() + String(index);
		}
		
		auto newNode = node->getRootNetwork()->create(path, newId, node->isPolyphonic());

		if (auto newContainer = dynamic_cast<NodeBase*>(newNode.getObject()))
		{
			auto containerTree = newContainer->getValueTree();

			auto um = node->getUndoManager();

			auto selection = network->getSelection();

			if (selection.isEmpty() || !selection.contains(node))
			{
				auto nodeTree = node->getValueTree();
				auto parent = nodeTree.getParent();
				auto index = parent.indexOf(nodeTree);

				parent.removeChild(nodeTree, um);
				containerTree.getChildWithName(PropertyIds::Nodes).addChild(nodeTree, -1, um);

				jassert(!containerTree.getParent().isValid());

				parent.addChild(containerTree, index, um);
			}
			else
			{
				auto parent = selection.getFirst()->getValueTree().getParent();
				auto index = parent.indexOf(selection.getFirst()->getValueTree());

				for (auto n : selection)
				{
					n->getValueTree().getParent().removeChild(n->getValueTree(), um);
					containerTree.getChildWithName(PropertyIds::Nodes).addChild(n->getValueTree(), -1, um);
				}

				parent.addChild(containerTree, index, um);
			}
		}
	}
}

juce::Colour NodeComponent::getOutlineColour() const
{
	if (isRoot())
		return dynamic_cast<const Processor*>(node->getScriptProcessor())->getColour();

	return header.isDragging ? Colour(0x88444444) : Colour(0xFF555555);
}

bool NodeComponent::isRoot() const
{
	return !isDragged() && dynamic_cast<DspNetworkGraph*>(getParentComponent()) != nullptr;
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
	return node->getRootNetwork()->isSelected(node);
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


juce::Path NodeComponent::Factory::createPath(const String& id) const
{
	Path p;
	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

	LOAD_PATH_IF_URL("on", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	LOAD_PATH_IF_URL("fold", HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon);
	LOAD_PATH_IF_URL("delete", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
	LOAD_PATH_IF_URL("move", ColumnIcons::moveIcon);
	LOAD_PATH_IF_URL("parameter", HiBinaryData::SpecialSymbols::macros);
	LOAD_PATH_IF_URL("split", ScriptnodeIcons::splitIcon);
	LOAD_PATH_IF_URL("chain", ScriptnodeIcons::chainIcon);
	LOAD_PATH_IF_URL("multi", ScriptnodeIcons::multiIcon);
	LOAD_PATH_IF_URL("mod", ScriptnodeIcons::modIcon);
	
	if (url.contains("frame"))
	{
		p.loadPathFromData(ScriptnodeIcons::frameIcon, sizeof(ScriptnodeIcons::frameIcon));
	}



	return p;
}


}

