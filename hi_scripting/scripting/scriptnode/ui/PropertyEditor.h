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


struct NodePropertyComponent : public PropertyComponent
{
	NodePropertyComponent(NodeBase* n, ValueTree d) :
		PropertyComponent(d[PropertyIds::ID].toString()),
		comp(d, n)
	{
		addAndMakeVisible(comp);
	}

	struct Comp : public Component,
				  public ComboBoxListener,
				  public Value::Listener
	{
		Comp(ValueTree d, NodeBase* n):
			v(d.getPropertyAsValue(PropertyIds::Value, n->getUndoManager()))
		{
			publicButton.getToggleStateValue().referTo(d.getPropertyAsValue(PropertyIds::Public, n->getUndoManager()));
			publicButton.setButtonText("Public");
			addAndMakeVisible(publicButton);
			publicButton.setLookAndFeel(&laf);
			publicButton.setClickingTogglesState(true);

			Identifier propId = Identifier(d[PropertyIds::ID].toString().fromLastOccurrenceOf(".", false, false));

			if (propId == PropertyIds::FillMode || propId == PropertyIds::UseMidi || propId == PropertyIds::UseResetValue)
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
			else
			{
				auto te = new TextEditor();
				te->getTextValue().referTo(v);
				te->setLookAndFeel(&laf);
				editor = te;
			}

			if(editor != nullptr)
				addAndMakeVisible(editor);

		}

		StringArray getListForId(const Identifier& id, NodeBase* n)
		{
			if (id == PropertyIds::Callback)
			{
				if (auto jp = dynamic_cast<JavascriptProcessor*>(n->getScriptProcessor()))
				{
					return jp->getScriptEngine()->getInlineFunctionNames(1);
				}
			}
			else if (id == PropertyIds::Connection)
			{
				return routing::Factory::getSourceNodeList(n);
			}

			return {};
		}

		void valueChanged(Value& value)
		{
			if (auto cb = dynamic_cast<ComboBox*>(editor.get()))
				cb->setText(value.getValue(), dontSendNotification);
		}

		void comboBoxChanged(ComboBox* c) override
		{
			v.setValue(c->getText());
		}

		void resized() override
		{
			auto b = getLocalBounds();
			publicButton.setBounds(b.removeFromRight(40));
			
			if(editor != nullptr)
				editor->setBounds(b);
		}

		Value v;
		ScopedPointer<Component> editor;
		TextButton publicButton;
		HiPropertyPanelLookAndFeel laf;
	} comp;

	void refresh() override {};
};

struct PropertyEditor : public Component
{
	PropertyEditor(NodeBase* n, ValueTree data, Array<Identifier> hiddenIds = {})
	{
		Array<PropertyComponent*> newProperties;

		for (int i = 0; i < data.getNumProperties(); i++)
		{
			auto id = data.getPropertyName(i);

			if (hiddenIds.contains(id))
				continue;

			auto nt = PropertyHelpers::createPropertyComponent(n->getScriptProcessor(), data, id, n->getUndoManager());

			newProperties.add(nt);
		}

		for (auto prop : n->getPropertyTree())
		{
			if (!(bool)prop[PropertyIds::Public] && prop[PropertyIds::ID].toString().contains("."))
			{
				continue;
			}

			auto nt = new NodePropertyComponent(n, prop);
			newProperties.add(nt);
		}

		p.addProperties(newProperties);

		addAndMakeVisible(p);
		p.setLookAndFeel(&plaf);
		setSize(300, p.getTotalContentHeight());
	}

	void resized() override
	{
		p.setBounds(getLocalBounds());
	}

	HiPropertyPanelLookAndFeel plaf;
	PropertyPanel p;
};


}
