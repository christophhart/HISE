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


juce::Path NodePopupEditor::Factory::createPath(const String& s) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(s);

	Path p;

	LOAD_EPATH_IF_URL("export", HnodeIcons::freezeIcon);
	LOAD_EPATH_IF_URL("wrap", HnodeIcons::mapIcon);
	LOAD_EPATH_IF_URL("surround", HnodeIcons::injectNodeIcon);

	return p;
}


NodePopupEditor::NodePopupEditor(NodeComponent* nc_) :
	nc(nc_),
	editor(nc->node.get(), false, nc->node->getValueTree(), { PropertyIds::Bypassed} ),
	networkEditor(nc->node.get(), false, nc->node->getRootNetwork()->getValueTree(), { PropertyIds::ID }, false),
	exportButton("export", this, factory),
	wrapButton("wrap", this, factory),
	surroundButton("surround", this, factory)
{
	setName("Edit Node Properties");

	addAndMakeVisible(editor);
	addAndMakeVisible(networkEditor);
	addAndMakeVisible(exportButton);
	addAndMakeVisible(wrapButton);
	addAndMakeVisible(surroundButton);
	setWantsKeyboardFocus(true);
	setSize(editor.getWidth(), editor.getHeight() + networkEditor.getHeight() + 50);
}

bool NodePopupEditor::keyPressed(const KeyPress& key)
{
	if (key.getKeyCode() == 'w' || key.getKeyCode() == 'W')
	{
		buttonClicked(&wrapButton);
		return true;
	}
	if (key.getKeyCode() == 's' || key.getKeyCode() == 'S')
	{
		buttonClicked(&surroundButton);
		return true;
	}
	if (key.getKeyCode() == 'e' || key.getKeyCode() == 'E')
	{
		buttonClicked(&exportButton);
		return true;
	}
	if (key.getKeyCode() == 'o' || key.getKeyCode() == 'O')
	{
#if HISE_INCLUDE_SNEX && OLD_JIT_STUFF
		if (auto sp = this->findParentComponentOfClass<DspNetworkGraph::ScrollableParent>())
		{
			auto n = nc->node;

			auto f = [this, n, sp]()
			{
				auto propTree = n.get()->getPropertyTree().getChildWithProperty(PropertyIds::ID, PropertyIds::Code.toString());

				if (propTree.isValid())
				{
					auto bh = new HiseBufferHandler(dynamic_cast<Processor*>(n.get()->getScriptProcessor()));

					auto pg = new snex::jit::SnexPlayground(propTree.getPropertyAsValue(PropertyIds::Value, n->getUndoManager()), bh);

					auto bounds = sp->getBounds().reduced(100);

					pg->setSize(jmin(1280, bounds.getWidth()), jmin(800, bounds.getHeight()));
					sp->setCurrentModalWindow(pg, sp->getCurrentTarget());
				}

				
			};

			MessageManager::callAsync(f);
			
			return true;
		}
#else
        return false;
#endif
	}
	if (key == KeyPress::tabKey)
	{
		editor.getChildComponent(0)->grabKeyboardFocus();
		return true;
	}

	return false;
}

void NodePopupEditor::buttonClicked(Button* b)
{
	int mode = 0;
	if (b == &wrapButton)
		mode = 1;
	if (b == &surroundButton)
		mode = 2;

	auto tmp = nc.getComponent();
	auto sp = findParentComponentOfClass<ZoomableViewport>();

	Component::SafePointer<Component> tmp2 = b;

	auto f = [tmp, mode, sp, tmp2]()
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		if (mode == 0)
		{
#if 0
			if (tmp->node->getAsRestorableNode())
			{
				m.addItem((int)NodeComponent::MenuActions::UnfreezeNode, "Unfreeze Node");
				m.addSeparator();
			}

			auto freezedId = tmp->node->getValueTree()[PropertyIds::FreezedId].toString();

			if (freezedId.isNotEmpty())
			{
				m.addItem((int)NodeComponent::MenuActions::FreezeNode, "Freeze Node (discard changes)");
				m.addSeparator();
			}
#endif

			m.addSectionHeader("Export Node");
			m.addItem((int)NodeComponent::MenuActions::ExportAsCpp, "Export as custom CPP class");
			m.addItem((int)NodeComponent::MenuActions::ExportAsCppProject, "Export as project CPP class");
			m.addItem((int)NodeComponent::MenuActions::ExportAsSnippet, "Export as Base64 snippet");
			m.addItem((int)NodeComponent::MenuActions::CreateScreenShot, "Create screenshot");
		}
		else if (mode == 1)
		{
			m.addSectionHeader("Move into container");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoChain, "Wrap into chain");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoSplit, "Wrap into split");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoMulti, "Wrap into multi");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoFrame, "Wrap into frame");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoFix32, "Wrap into fix32");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoMidiChain, "Wrap into midichain");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoCloneChain, "Wrap into clone");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoNoMidiChain, "Wrap into nomidi");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoNoMidiChain, "Wrap into soft bypass");
			m.addItem((int)NodeComponent::MenuActions::WrapIntoOversample4, "Wrap into oversample4");
		}
		else
		{
			m.addSectionHeader("Surround with Node pair");
			m.addItem((int)NodeComponent::MenuActions::SurroundWithFeedback, "Surround with feedback");
			m.addItem((int)NodeComponent::MenuActions::SurroundWithMSDecoder, "Surround with M/S");
		}

		int result = m.showAt(tmp2.getComponent());

		if (result != 0)
		{
			tmp->handlePopupMenuResult(result);
			sp->setCurrentModalWindow(nullptr, {});
		}
	};

	MessageManager::callAsync(f);
}




void NodePopupEditor::resized()
{
	auto b = getLocalBounds();

	auto top = b.removeFromTop(50);

	auto w3 = getWidth() / 3;

	wrapButton.setBounds(top.removeFromLeft(w3).withSizeKeepingCentre(32, 32));
	surroundButton.setBounds(top.removeFromLeft(w3).withSizeKeepingCentre(32, 32));
	exportButton.setBounds(top.removeFromLeft(w3).withSizeKeepingCentre(32, 32));

	editor.setBounds(b.removeFromTop(editor.getHeight()));

	globalTextArea = b.removeFromTop(28).toFloat();

	networkEditor.setBounds(b);
}

void NodePopupEditor::paint(Graphics& g)
{
	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.8f));
	g.drawText("DSP Network Properties", globalTextArea, Justification::centred);
}

NodePropertyComponent::Comp::Comp(ValueTree d, NodeBase* n) :
	v(d.getPropertyAsValue(PropertyIds::Value, n->getUndoManager()))
{
	Identifier propId = Identifier(d[PropertyIds::ID].toString().fromLastOccurrenceOf(".", false, false));

	if (propId == PropertyIds::FillMode || propId == PropertyIds::UseResetValue || propId == PropertyIds::UseFreqDomain)
	{
		TextButton* t = new TextButton();
		t->setButtonText("Enabled");
		t->setClickingTogglesState(true);
		t->getToggleStateValue().referTo(v);
		t->setLookAndFeel(&laf);

		editor = t;
		addAndMakeVisible(editor);
	}
	else if (propId == PropertyIds::Callback)
	{
		Array<var> values;

		StringArray sa = getListForId(propId, n);

		for (auto s : sa)
			values.add(s);

		auto c = new ComboBox();

		c->addItemList(sa, 1);
		c->addListener(this);
		v.addListener(this);

		editor = c;

		valueChanged(v);
	}
#if HISE_INCLUDE_SNEX
	else if (propId == PropertyIds::Code)
	{
        TextButton* jp = new TextButton("Edit Code");

		{
			jp->onClick = [this, n]()
			{
				
				auto pTree = n->getPropertyTree().getChildWithProperty(PropertyIds::ID, PropertyIds::Code.toString());
				if (pTree.isValid())
				{
					if (auto sp = this->findParentComponentOfClass<ZoomableViewport>())
					{
						if (auto m = static_cast<snex::ui::WorkbenchManager*>(n->getScriptProcessor()->getMainController_()->getWorkbenchManager()))
						{
							auto v = n->getPropertyTree().getChildWithProperty(PropertyIds::ID, PropertyIds::Code.toString()).getPropertyAsValue(PropertyIds::Value, n->getUndoManager());

							auto cp = new snex::ui::WorkbenchData::ValueBasedCodeProvider(nullptr, v, n->getId());

							auto wb = m->getWorkbenchDataForCodeProvider(cp, true);

							auto pg = new snex::jit::SnexPlayground(wb.get());

							auto bounds = sp->getBounds().reduced(100);

							pg->setSize(jmin(1280, bounds.getWidth()), jmin(800, bounds.getHeight()));
							sp->setCurrentModalWindow(pg, sp->getCurrentTarget());
							return;
						}
					}
				}
			};
		}

		editor = jp;
	}
#endif
	else
	{
		auto te = new TextEditor();
		te->setLookAndFeel(&laf);
		te->addListener(this);
		editor = te;
		valueChanged(v);
		v.addListener(this);
	}

	if (editor != nullptr)
		addAndMakeVisible(editor);
}

juce::StringArray NodePropertyComponent::Comp::getListForId(const Identifier& id, NodeBase* n)
{
	if (id == PropertyIds::Callback)
	{
		if (auto jp = dynamic_cast<JavascriptProcessor*>(n->getScriptProcessor()))
		{
			return jp->getScriptEngine()->getInlineFunctionNames(1);
		}
	}


	return {};
}

void MultiColumnPropertyPanel::resized()
{
	int y = 0;

	if (useTwoColumns)
	{
		int x = 0;
		auto w = getWidth() / 2;

		for (auto p : properties)
		{
			if (!p->isVisible())
				continue;

			auto h = p->getPreferredHeight();

			p->setBounds(x, y, w, h);

			if (x == w)
				y += h;

			x += w;

			if (x == getWidth())
				x = 0;
		}

		if (properties.size() % 2 != 0)
		{
			properties.getLast()->setSize(getWidth(), properties.getLast()->getHeight());
		}
	}
	else
	{
		for (auto p : properties)
		{
			if (!p->isVisible())
				continue;

			auto h = p->getPreferredHeight();
			p->setBounds(0, y, getWidth(), h);

			y += h;
		}
	}
}


int MultiColumnPropertyPanel::getTotalContentHeight() const
{
	if (!useTwoColumns)
		return contentHeight;

	int columnIndex = 0;

	int h = 0;

	for (auto p : properties)
	{
		if (!p->isVisible())
			continue;

		if (columnIndex % 2 == 0)
			h += p->getPreferredHeight();

		columnIndex++;
	}

	return h;
}



PropertyEditor::PropertyEditor(NodeBase* n, bool useTwoColumns, ValueTree data, Array<Identifier> hiddenIds, bool includeProperties)
{
	plaf.propertyBgColour = Colours::transparentBlack;

	Array<PropertyComponent*> newProperties;

	for (int i = 0; i < data.getNumProperties(); i++)
	{
		auto id = data.getPropertyName(i);

		if (hiddenIds.contains(id))
			continue;

		auto nt = PropertyHelpers::createPropertyComponent(n->getScriptProcessor(), data, id, n->getUndoManager());

		newProperties.add(nt);
	}

	// Only add Node properties in two-column mode (aka Connection Editor)...
	if (includeProperties && !useTwoColumns)
	{
		for (auto prop : n->getPropertyTree())
		{
			auto nt = new NodePropertyComponent(n, prop);
			newProperties.add(nt);
		}
	}


	p.setUseTwoColumns(useTwoColumns);
	p.addProperties(newProperties);

	addAndMakeVisible(p);
	p.setLookAndFeel(&plaf);
	updateSize();
}

}

