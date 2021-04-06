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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace parameter
{

	dynamic_list::MultiOutputConnection::MultiOutputConnection(NodeBase* n, ValueTree switchTarget) :
		ConnectionBase(n->getScriptProcessor(), switchTarget),
		parentNode(n)
	{
		jassert(switchTarget.getType() == PropertyIds::SwitchTarget);
		connectionTree = switchTarget.getOrCreateChildWithName(PropertyIds::Connections, n->getUndoManager());

		initRemoveUpdater(n);

		connectionListener.setCallback(connectionTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(MultiOutputConnection::updateConnections));

		auto ids = RangeHelpers::getRangeIds();
		ids.add(PropertyIds::Expression);

		connectionPropertyListener.setCallback(connectionTree, ids, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(MultiOutputConnection::updateConnectionProperty));

		ok = rebuildConnections(true);
	}

	void dynamic_list::MultiOutputConnection::updateConnectionProperty(ValueTree, Identifier)
	{
		rebuildConnections(false);
	}

	bool dynamic_list::MultiOutputConnection::rebuildConnections(bool tryAgainIfFail)
	{
		try
		{
			ScopedPointer<dynamic_chain> dc = createParameterFromConnectionTree(parentNode, connectionTree, true);

			if (auto fp = dc->getFirstIfSingle())
				p.setParameter(fp);
			else
				p.setParameter(dc.release());

			return true;
		}
		catch (String& s)
		{
			if (tryAgainIfFail)
			{
				ReferenceCountedObjectPtr<MultiOutputConnection> safeThis = this;

				MessageManager::callAsync([safeThis]()
					{
						if (safeThis.get() != nullptr)
							safeThis.get()->rebuildConnections(false);
					});
			}

			return false;
		}
	}

	void dynamic_list::MultiOutputConnection::updateConnections(ValueTree v, bool wasAdded)
	{
		rebuildConnections(true);
	}

	bool dynamic_list::MultiOutputConnection::matchesTarget(NodeBase::Parameter* np)
	{
		if (p.base == nullptr)
			return false;

		if (np->getTreeWithValue() == p.base->dataTree)
			return true;

		if (auto dc = dynamic_cast<parameter::dynamic_chain*>(p.base.get()))
		{
			for (auto d : dc->targets)
				if (d->dataTree == np->getTreeWithValue())
					return true;
		}

		return false;
	}

	void dynamic_list::initialise(NodeBase* n)
	{
		parentNode = n;

		switchTree = n->getValueTree().getOrCreateChildWithName(PropertyIds::SwitchTargets, n->getUndoManager());

		connectionUpdater.setCallback(switchTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(dynamic_list::updateConnections));

		numParameters.initialise(n);
		numParameters.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic_list::updateParameterAmount));

		rebuildMultiOutputConnections();
	}

	void dynamic_list::rebuildMultiOutputConnections()
	{
		targets.clear();

		for (auto c : switchTree)
			targets.add(new MultiOutputConnection(parentNode, c));
	}

	int dynamic_list::getNumParameters() const
	{
		return targets.size();
	}

	void dynamic_list::updateConnections(ValueTree v, bool wasAdded)
	{
		if (deferUpdate)
			return;

		rebuildMultiOutputConnections();
	}

	void dynamic_list::updateParameterAmount(Identifier id, var newValue)
	{
		auto numToUse = numParameters.getValue();

		int numToRemove = switchTree.getNumChildren() - numToUse;
		int numToAdd = -numToRemove;

		if (numToAdd == 0)
			return;

		ScopedValueSetter<bool> svs(deferUpdate, true);

		auto size = getNumParameters();

		if (numToRemove > 0)
		{
			for (int i = 0; i < numToRemove; i++)
			{
				switchTree.removeChild(switchTree.getNumChildren() - 1, parentNode->getUndoManager());
			}
		}
		else
		{
			for (int i = 0; i < numToAdd; i++)
			{
				ValueTree sv(PropertyIds::SwitchTarget);
				ValueTree cv(PropertyIds::Connections);
				sv.addChild(cv, -1, nullptr);
				switchTree.addChild(sv, -1, parentNode->getUndoManager());
			}
		}

		rebuildMultiOutputConnections();
	}

	juce::Path ui::Factory::createPath(const String& url) const
	{
		Path p;
		LOAD_PATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
		LOAD_PATH_IF_URL("delete", SampleMapIcons::deleteSamples);
		LOAD_PATH_IF_URL("drag", ColumnIcons::targetIcon);
		LOAD_PATH_IF_URL("edit", ColumnIcons::moveIcon);

		return p;
	}

	namespace ui
	{
		struct dynamic_list_editor::MultiConnectionEditor::ConnectionEditor : public Component,
			public ButtonListener
		{
			ConnectionEditor(NodeBase* n_, ValueTree connectionTree, int index_, int numParameters_) :
				deleteButton("delete", this, f),
				n(n_),
				editor(n_, true, connectionTree, RangeHelpers::getHiddenIds()),
				c(connectionTree),
				index(index_),
				numParameters(numParameters_)
			{
				addAndMakeVisible(deleteButton);
				addAndMakeVisible(editor);

				setSize(editor.getWidth(), editor.p.getTotalContentHeight() + 24);
			}

			void buttonClicked(Button* b) override
			{
				c.getParent().removeChild(c, n->getUndoManager());
			}

			void paint(Graphics& g) override
			{
				auto b = getLocalBounds();
				auto top = b.removeFromTop(24);

				g.setColour(MultiOutputDragSource::getFadeColour(index, numParameters));
				g.fillRoundedRectangle(top.removeFromLeft(top.getHeight()).toFloat().reduced(1.0f), 2.0f);
				g.setColour(Colours::white);
				g.setFont(GLOBAL_BOLD_FONT());
				String title;
				title << c[PropertyIds::NodeId].toString() << "." << c[PropertyIds::ParameterId].toString();
				g.drawText(title, top.toFloat(), Justification::centred);
			}

			void resized() override
			{
				auto b = getLocalBounds();

				auto top = b.removeFromTop(24);
				deleteButton.setBounds(top.removeFromRight(24).reduced(2));
				editor.setBounds(b);
			}

			int index;
			int numParameters;

			NodeBase::Ptr n;
			Factory f;
			ValueTree c;
			PropertyEditor editor;
			HiseShapeButton deleteButton;
		};

		struct dynamic_list_editor::MultiConnectionEditor::OutputEditor : public Component
		{
			valuetree::ChildListener removeListener;

			void rebuildEditors(ValueTree v, bool wasAdded)
			{
				editors.clear();

				int y = 0;

				for (auto v : c->connectionTree)
				{
					editors.add(new ConnectionEditor(c->parentNode, v, index, numParameters));

					auto e = editors.getLast();
					e->setTopLeftPosition(0, y);
					addAndMakeVisible(e);
					y += e->getHeight();
				}

				setSize(MultiConnectionEditor::ViewportWidth - 16, y);
			}

			OutputEditor(parameter::dynamic_list::MultiOutputConnection* c_, int index_, int numParameters_) :
				c(c_),
				index(index_),
				numParameters(numParameters_)
			{
				rebuildEditors({}, true);
				removeListener.setCallback(c->connectionTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(OutputEditor::rebuildEditors));
			}

			int index;
			int numParameters;

			parameter::dynamic_list::MultiOutputConnection* c;
			OwnedArray<ConnectionEditor> editors;
		};

		struct dynamic_list_editor::MultiConnectionEditor::WrappedOutputEditor : public Component
		{
			WrappedOutputEditor(parameter::dynamic_list::MultiOutputConnection* c)
			{
				index = c->data.getParent().indexOf(c->data);

				setName("Output " + String(index + 1));

				auto n = new OutputEditor(c, index, c->data.getParent().getNumChildren());
				vp.setViewedComponent(n);
				addAndMakeVisible(vp);

				containsSomething = n->editors.size() > 0;

				setSize(ViewportWidth, jmin(500, vp.getViewedComponent()->getHeight()));
			}

			bool containsSomething = false;

			void resized() override
			{
				vp.setBounds(getLocalBounds());
			}

			int index = 0;
			Viewport vp;
		};

		dynamic_list_editor::MultiConnectionEditor::MultiConnectionEditor(parameter::dynamic_list* l)
		{
			setName("Edit Connections");

			int maxHeight = 0;

			for (auto t : l->targets)
			{
				ScopedPointer<WrappedOutputEditor> n = new WrappedOutputEditor(t);

				if (n->containsSomething)
				{
					maxHeight = jmax(maxHeight, n->getHeight());
					addAndMakeVisible(n);
					editors.add(n.release());
				}
			}

			setSize(editors.size() * ViewportWidth, jmin(500, maxHeight));
		}

		void dynamic_list_editor::MultiConnectionEditor::resized()
		{
			auto b = getLocalBounds();
			for (auto v : editors)
			{
				v->setBounds(b.removeFromLeft(ViewportWidth));
			}
		}

		dynamic_list_editor::dynamic_list_editor(parameter::dynamic_list* l, PooledUIUpdater* updater) :
			ScriptnodeExtraComponent<parameter::dynamic_list>(l, updater),
			addButton("add", this, f),
			removeButton("delete", this, f),
			editButton("edit", this, f)
		{
			addButton.setTooltip("Add a connection output");
			removeButton.setTooltip("Remove the last connection output");

			addAndMakeVisible(addButton);
			addAndMakeVisible(removeButton);
			addAndMakeVisible(editButton);
		}

		void dynamic_list_editor::timerCallback()
		{
			for (auto o : getObject()->targets)
				o->p.updateUI();

			if (dragSources.size() != getObject()->getNumParameters())
			{
				dragSources.clear();

				for (int i = 0; i < getObject()->getNumParameters(); i++)
				{
					auto n = new DragComponent(getObject(), i);
					dragSources.add(n);
					addAndMakeVisible(n);
				}

				resized();
			}
		}

		void dynamic_list_editor::buttonClicked(Button* b)
		{
			if (b == &editButton)
			{
				auto n = new MultiConnectionEditor(getObject());

				NodeBase::showPopup(this, n);

				return;
			}

			int newValue = 0;

			if (b == &addButton)
				newValue = jmin(getObject()->getNumParameters() + 1, 8);

			if (b == &removeButton)
			{
				newValue = jmax(0, getObject()->getNumParameters() - 1);
			}

			getObject()->parentNode->setNodeProperty(PropertyIds::NumParameters, newValue);
		}

		void dynamic_list_editor::resized()
		{
			auto b = getLocalBounds();

			b.removeFromTop(5);
			auto top = b.removeFromTop(UIConstants::ButtonHeight);

			b.removeFromTop(5);
			b.removeFromBottom(10);

			addButton.setBounds(top.removeFromLeft(UIConstants::ButtonHeight).reduced(2));
			removeButton.setBounds(top.removeFromLeft(UIConstants::ButtonHeight).reduced(2));
			editButton.setBounds(top.removeFromLeft(UIConstants::ButtonHeight).reduced(2));

			if (dragSources.size() > 0)
			{
				auto wPerDrag = b.getWidth() / dragSources.size();

				for (auto d : dragSources)
					d->setBounds(b.removeFromLeft(wPerDrag));
			}
		}

		dynamic_list_editor::DragComponent::DragComponent(parameter::dynamic_list* parent, int index_) :
			index(index_),
			pdl(parent)
		{
			n = dynamic_cast<ModulationSourceNode*>(pdl->parentNode);
			p = Factory().createPath("drag");

			setRepaintsOnMouseActivity(true);

			setMouseCursor(ModulationSourceBaseComponent::createMouseCursor());
		}

		bool dynamic_list_editor::DragComponent::matchesParameter(NodeBase::Parameter* p) const
		{
			if (auto t = pdl->targets[index])
			{
				return t->matchesTarget(p);
			}

			return false;
		}

		void dynamic_list_editor::DragComponent::mouseDown(const MouseEvent& e)
		{
			if (e.mods.isRightButtonDown())
			{
				if (auto t = pdl->targets[index])
				{
					NodeBase::showPopup(this, new MultiConnectionEditor::WrappedOutputEditor(t));
				}
			}
		}

		void dynamic_list_editor::DragComponent::mouseDrag(const MouseEvent& e)
		{
			if (e.mods.isRightButtonDown())
			{
				return;
			}

			auto container = findParentComponentOfClass<ContainerComponent>();

			auto root = pdl->parentNode->getRootNetwork()->getRootNode();

			while (container != nullptr && container->node != root)
			{
				container = container->findParentComponentOfClass<ContainerComponent>();
			}

			if (container != nullptr)
			{
				var d;

				DynamicObject::Ptr details = new DynamicObject();

				auto nodeId = pdl->parentNode->getId();

				details->setProperty(PropertyIds::ID, nodeId);
				details->setProperty(PropertyIds::ParameterId, index);
				details->setProperty(PropertyIds::SwitchTarget, true);

				container->startDragging(var(details), this, ModulationSourceBaseComponent::createDragImageStatic(false));
				findParentComponentOfClass<DspNetworkGraph>()->repaint();
			}
		}

		void dynamic_list_editor::DragComponent::mouseUp(const MouseEvent& event)
		{
			findParentComponentOfClass<DspNetworkGraph>()->repaint();
		}

		void dynamic_list_editor::DragComponent::resized()
		{
			auto b = getLocalBounds().withSizeKeepingCentre(24, 24);


			auto pathBounds = b.toFloat().reduced(2.0f).translated(-12.0f, 0.0f);

			Factory::scalePath(p, pathBounds);

			auto xOffset = pathBounds.getCentreX() - (float)getWidth() / 2.0f;
			auto yOffset = pathBounds.getCentreY() - (float)getHeight() - JUCE_LIVE_CONSTANT(3.f);

			getProperties().set("circleOffsetX", xOffset);
			getProperties().set("circleOffsetY", yOffset);
		}

		void dynamic_list_editor::DragComponent::paint(Graphics& g)
		{
			auto b = getLocalBounds().toFloat().reduced(1.0f);

			g.setColour(Colours::black.withAlpha(0.1f));
			g.fillRoundedRectangle(b, b.getHeight() / 2.0f);

			float alpha = 0.5f;

			if (isMouseOver())
				alpha += 0.1f;

			if (isMouseButtonDown())
				alpha += 0.2f;

			auto c = getFadeColour(index, getNumOutputs());

			g.setColour(c.withAlpha(alpha));
			g.fillPath(p);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(String(index + 1), getLocalBounds().toFloat(), Justification::centredTop);
		}

	}

	
}

}
