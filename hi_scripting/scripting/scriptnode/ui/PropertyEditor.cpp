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

	LOAD_PATH_IF_URL("export", HnodeIcons::freezeIcon);
	LOAD_PATH_IF_URL("wrap", HnodeIcons::mapIcon);
	LOAD_PATH_IF_URL("surround", HnodeIcons::injectNodeIcon);

	return p;
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
#if HISE_INCLUDE_SNEX
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
	auto sp = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();

	Component::SafePointer<Component> tmp2 = b;

	auto f = [tmp, mode, sp, tmp2]()
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		if (mode == 0)
		{
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


NodePropertyComponent::Comp::Comp(ValueTree d, NodeBase* n) :
	v(d.getPropertyAsValue(PropertyIds::Value, n->getUndoManager()))
{
	publicButton.getToggleStateValue().referTo(d.getPropertyAsValue(PropertyIds::Public, n->getUndoManager()));
	publicButton.setButtonText("Public");
	addAndMakeVisible(publicButton);
	publicButton.setLookAndFeel(&laf);
	publicButton.setClickingTogglesState(true);

	Identifier propId = Identifier(d[PropertyIds::ID].toString().fromLastOccurrenceOf(".", false, false));

	if (propId == PropertyIds::FillMode || propId == PropertyIds::UseMidi || propId == PropertyIds::UseResetValue || propId == PropertyIds::UseFreqDomain)
	{
		TextButton* t = new TextButton();
		t->setButtonText("Enabled");
		t->setClickingTogglesState(true);
		t->getToggleStateValue().referTo(v);
		t->setLookAndFeel(&laf);

		editor = t;
		addAndMakeVisible(editor);
	}
	else if (propId == PropertyIds::Callback || propId == PropertyIds::Connection)
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
					if (auto sp = this->findParentComponentOfClass<DspNetworkGraph::ScrollableParent>())
					{
						auto value = pTree.getPropertyAsValue(PropertyIds::Value, n->getUndoManager());
						auto pg = new snex::jit::SnexPlayground(value);

						auto bounds = sp->getBounds().reduced(100);

						pg->setSize(jmin(1280, bounds.getWidth()), jmin(800, bounds.getHeight()));
						sp->setCurrentModalWindow(pg, sp->getCurrentTarget());
						return;
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
		te->getTextValue().referTo(v);
		te->setLookAndFeel(&laf);
		editor = te;

	}

	if (editor != nullptr)
		addAndMakeVisible(editor);
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

}

