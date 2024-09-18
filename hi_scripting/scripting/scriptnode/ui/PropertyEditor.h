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
				  public Value::Listener,
				  public TextEditor::Listener
	{
		Comp(ValueTree d, NodeBase* n);

		~Comp()
		{
			if (auto te = dynamic_cast<TextEditor*>(editor.get()))
				textEditorFocusLost(*te);
		}

		StringArray getListForId(const Identifier& id, NodeBase* n);

		void textEditorReturnKeyPressed(TextEditor& te) 
		{
			v.setValue(te.getText());
		}

		void textEditorEscapeKeyPressed(TextEditor& te)
		{
			te.setText(v.getValue().toString(), dontSendNotification);
		}

		void textEditorFocusLost(TextEditor& te)
		{
			v.setValue(te.getText());
		}

		void valueChanged(Value& value)
		{
			if (auto cb = dynamic_cast<ComboBox*>(editor.get()))
				cb->setText(value.getValue(), dontSendNotification);
			if (auto te = dynamic_cast<TextEditor*>(editor.get()))
				te->setText(value.getValue(), dontSendNotification);
		}

		void comboBoxChanged(ComboBox* c) override
		{
			v.setValue(c->getText());
		}

		void resized() override
		{
			auto b = getLocalBounds();

			if(editor != nullptr)
				editor->setBounds(b);
		}

		Value v;

		ScopedPointer<Component> editor;
		NodeBase::Ptr jitNode;
		
		HiPropertyPanelLookAndFeel laf;
		
	} comp;

	void refresh() override {};
};

struct MultiColumnPropertyPanel : public Component
{
	MultiColumnPropertyPanel()
	{

	}

	void enableProperties(const Array<Identifier>& ids, bool shouldBeEnabled)
	{
		for (auto p : properties)
		{
			Identifier pId(p->getName());

			if (ids.contains(pId))
			{
				p->setVisible(shouldBeEnabled);
			}
		}
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

	void resized();

	int getTotalContentHeight() const;

	bool useTwoColumns = true;
	
	int contentHeight = 0;

	OwnedArray<PropertyComponent> properties;
};

struct PropertyEditor : public Component
{
	PropertyEditor(NodeBase* n, bool useTwoColumns, ValueTree data, Array<Identifier> hiddenIds = {}, bool includeProperties=true);

	void updateSize()
	{
		setSize(400, p.getTotalContentHeight());
	}

	void enableProperties(Array<Identifier>& ids, bool shouldBeEnabled)
	{
		p.enableProperties(ids, shouldBeEnabled);
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

	NodePopupEditor(NodeComponent* nc_);

	bool keyPressed(const KeyPress& key) override;

	void buttonClicked(Button* b) override;

	void resized() override;

	void paint(Graphics& g) override;

	Rectangle<float> globalTextArea;

	Component::SafePointer<NodeComponent> nc;
	PropertyEditor editor;
	PropertyEditor networkEditor;
	HiseShapeButton exportButton, wrapButton, surroundButton;
};



}
