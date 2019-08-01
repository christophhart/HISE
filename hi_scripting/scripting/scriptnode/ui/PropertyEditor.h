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

		if (d[PropertyIds::ID].toString() == PropertyIds::Code.toString())
			setPreferredHeight(400);
	}

	struct Comp : public Component,
				  public ComboBoxListener,
				  public Value::Listener,
				  public ButtonListener
	{
		Comp(ValueTree d, NodeBase* n):
			v(d.getPropertyAsValue(PropertyIds::Value, n->getUndoManager()))
		{
			publicButton.getToggleStateValue().referTo(d.getPropertyAsValue(PropertyIds::Public, n->getUndoManager()));
			publicButton.setButtonText("Public");
			addAndMakeVisible(publicButton);
			publicButton.setLookAndFeel(&laf);
			publicButton.setClickingTogglesState(true);

			compileButton.setButtonText("Apply");
			addAndMakeVisible(compileButton);
			compileButton.setLookAndFeel(&laf);
			compileButton.addListener(this);
			
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
			else if (propId == PropertyIds::Code)
			{
				codeDoc = new CodeDocument();
				codeDoc->replaceAllContent(v.toString());
				tokeniser = new CPlusPlusCodeTokeniser();
				editor = new CodeEditorComponent(*codeDoc, tokeniser.get());
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

		void buttonClicked(Button* b) override
		{
			v.setValue(codeDoc->getAllContent());
		}

		void resized() override
		{
			auto b = getLocalBounds();

			if (dynamic_cast<CodeEditorComponent*>(editor.get()) == nullptr)
				publicButton.setBounds(b.removeFromRight(40));
			else
			{
				compileButton.setBounds(b.removeFromBottom(24));
				publicButton.setVisible(false);
			}
				
			
			if(editor != nullptr)
				editor->setBounds(b);
		}

		Value v;

		ScopedPointer<CodeDocument> codeDoc;
		ScopedPointer<CodeTokeniser> tokeniser;
		ScopedPointer<Component> editor;
		TextButton publicButton;
		TextButton compileButton;
		HiPropertyPanelLookAndFeel laf;
		
		
	} comp;

	void refresh() override {};
};

struct MultiColumnPropertyPanel : public Component
{
	MultiColumnPropertyPanel()
	{

	}

	void setUseTwoColumns(bool shouldUseTwoColumns)
	{
		useTwoColumns = shouldUseTwoColumns;
	}

	void addProperties(Array<PropertyComponent*> props)
	{
		for (auto p : props)
		{
			addAndMakeVisible(p);
			properties.add(p);
			contentHeight += p->getPreferredHeight();

			if (useTwoColumns)
			{
				p->setColour(TextPropertyComponent::backgroundColourId, Colour(0xFFAAAAAA));
				p->getChildComponent(0)->setColour(ComboBox::backgroundColourId, Colour(0xFFAAAAAA));
			}
				
		}

	}

	void resized()
	{
		int y = 0;

		if (useTwoColumns)
		{
			int x = 0;
			auto w = getWidth() / 2;

			for (auto p : properties)
			{
				auto h = p->getPreferredHeight();
				
				p->setBounds(x, y, w, h);

				if(x == w)
					y += h;

				x += w;

				if (x == getWidth())
					x = 0;
			}
		}
		else
		{
			for (auto p : properties)
			{
				auto h = p->getPreferredHeight();
				p->setBounds(0, y, getWidth(), h);

				y += h;
			}
		}
		
	}

	int getTotalContentHeight() const { return contentHeight / (useTwoColumns ? 2 : 1); }

	bool useTwoColumns = true;
	
	int contentHeight = 0;

	OwnedArray<PropertyComponent> properties;
};

struct PropertyEditor : public Component
{
	PropertyEditor(NodeBase* n, bool useTwoColumns, ValueTree data, Array<Identifier> hiddenIds = {})
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

		for (auto prop : n->getPropertyTree())
		{
			if (!(bool)prop[PropertyIds::Public] && prop[PropertyIds::ID].toString().contains("."))
			{
				continue;
			}

			auto nt = new NodePropertyComponent(n, prop);
			newProperties.add(nt);
		}

		p.setUseTwoColumns(useTwoColumns);
		p.addProperties(newProperties);

		addAndMakeVisible(p);
		p.setLookAndFeel(&plaf);
		setSize(400, p.getTotalContentHeight());
	}

	void resized() override
	{
		p.setBounds(getLocalBounds());
	}

	HiPropertyPanelLookAndFeel plaf;
	MultiColumnPropertyPanel p;
};


struct NodePopupEditor : public Component,
						 public ButtonListener
{
	struct Factory : public PathFactory
	{
		String getId() const { return "edit"; }

		Path createPath(const String& s) const override;

	} factory;

	NodePopupEditor(NodeComponent* nc_) :
		nc(nc_),
		editor(nc->node, false, nc->node->getValueTree()),
		exportButton("export", this, factory),
		wrapButton("wrap", this, factory),
		surroundButton("surround", this, factory)
	{
		setName("Edit Node Properties");

		addAndMakeVisible(editor);
		addAndMakeVisible(exportButton);
		addAndMakeVisible(wrapButton);
		addAndMakeVisible(surroundButton);
		setWantsKeyboardFocus(true);
		setSize(editor.getWidth(), editor.getHeight() + 50);
	}

	bool keyPressed(const KeyPress& key) override
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
		if (key == KeyPress::tabKey)
		{
			editor.getChildComponent(0)->grabKeyboardFocus();
			return true;
		}
	}

	void buttonClicked(Button* b) override;

	void resized() override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(50);

		auto w3 = getWidth() / 3;

		wrapButton.setBounds(top.removeFromLeft(w3).withSizeKeepingCentre(32, 32));
		surroundButton.setBounds(top.removeFromLeft(w3).withSizeKeepingCentre(32, 32));
		exportButton.setBounds(top.removeFromLeft(w3).withSizeKeepingCentre(32, 32));

		editor.setBounds(b);
	}

	

	Component::SafePointer<NodeComponent> nc;
	PropertyEditor editor;
	HiseShapeButton exportButton, wrapButton, surroundButton;

};

}
