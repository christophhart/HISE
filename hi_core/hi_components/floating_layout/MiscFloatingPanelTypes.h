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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#ifndef MISCFLOATINGPANELTYPES_H_INCLUDED
#define MISCFLOATINGPANELTYPES_H_INCLUDED

class FloatingTile;

#define SET_PANEL_NAME(x) static Identifier getPanelId() { static const Identifier id(x); return id;}; Identifier getIdentifierForBaseClass() const override { return getPanelId(); };
#define GET_PANEL_NAME(className) className::getPanelId()				  

class Note : public Component,
	public FloatingTileContent,
	public TextEditor::Listener
{
public:

	SET_PANEL_NAME("Note");

	Note(FloatingTile* p);

	void resized() override;

	ValueTree exportAsValueTree() const override;;

	void restoreFromValueTree(const ValueTree& v) override;;

	void labelTextChanged(Label* l);

	int getFixedHeight() const override
	{
		return 150;
	}

private:

	PopupLookAndFeel plaf;

	ScopedPointer<TextEditor> editor;
};



class EmptyComponent : public Component,
					   public FloatingTileContent
{
public:

	SET_PANEL_NAME("Empty");

	EmptyComponent(FloatingTile* p) :
		FloatingTileContent(p)
	{
		Random r;

		setInterceptsMouseClicks(false, true);

		addCategory();
		flexBox = FlexBox(FlexBox::Direction::column, FlexBox::Wrap::noWrap, FlexBox::AlignContent::center, FlexBox::AlignItems::center, FlexBox::JustifyContent::center);

		c = Colour(r.nextInt()).withAlpha(0.1f);
	};

	void paint(Graphics& g) override;

	void resized();

	void mouseDown(const MouseEvent& event);

	class Overlay : public Component
	{
		Overlay()
		{
			
		}
	};

private:

	struct Category: public Component
	{
		Category(FlexItem& item):
			properties(item)
		{
			flexBox = FlexBox(FlexBox::Direction::row, FlexBox::Wrap::wrap, FlexBox::AlignContent::center, FlexBox::AlignItems::center, FlexBox::JustifyContent::center);

			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
			addItem();
		}

		void resized();

		void paint(Graphics& g) override
		{
			g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0));
			g.setColour(Colours::white.withAlpha(0.1f));
			g.drawText("Category", 0, 0, getWidth(), 20, Justification::centred);
			g.fillRect(0, 0, getWidth(), 20);
		}

		struct Dummy : public Component
		{
			Dummy(FlexItem& item) :
				properties(item)
			{

			};

			void paint(Graphics& g)
			{
				g.fillAll(Colours::white.withAlpha(0.1f));
			}

			FlexItem& properties;
		};

		void addItem()
		{
			flexBox.items.add(FlexItem(60, 60).withMargin(10));

			auto& flexItem = flexBox.items.getReference(flexBox.items.size() - 1);

			

			auto panel = new Dummy(flexItem);
			elements.add(panel);
			flexItem.associatedComponent = panel;
			addAndMakeVisible(panel);
		}
		
		OwnedArray<Dummy> elements;

		FlexBox flexBox;

		FlexItem& properties;
	};


	void addCategory();
	
	FlexBox flexBox;
	OwnedArray<Category> categories;

	Colour c;
};

class SpacerPanel : public FloatingTileContent,
					public Component
{
public:

	SET_PANEL_NAME("Spacer");

	SpacerPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		setInterceptsMouseClicks(false, false);
	}

	bool showTitleInPresentationMode() const override { return false; }

};

class MidiKeyboardPanel : public FloatingTileContent,
						  public Component,
					      public ComponentWithKeyboard
{
public:

	SET_PANEL_NAME("Keyboard");

	MidiKeyboardPanel(FloatingTile* parent);

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	CustomKeyboard* getKeyboard() const override
	{
		return keyboard;
	}

	~MidiKeyboardPanel()
	{
		keyboard = nullptr;
	}

	void resized() override
	{
		keyboard->setBounds(0, 0, getWidth(), 72);
	}

	int getFixedHeight() const override { return 72; }

private:
	
	ScopedPointer<CustomKeyboard> keyboard;
};


class ModulatorSynthChain;

class PanelWithProcessorConnection : public FloatingTileContent,
									 public Component,
									 public ComboBox::Listener,
									 public Processor::DeleteListener
{
public:

	PanelWithProcessorConnection(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		addAndMakeVisible(connectionSelector = new ComboBox());

		connectionSelector->addListener(this);

		getMainSynthChain()->getMainController()->skin(*connectionSelector);

		StringArray items;
		fillProcessorList(items);

		connectionSelector->addItemList(items, 1);
	}

	void resized() override
	{
		connectionSelector->setBounds(0, 0, 128, 24);

		if (content != nullptr)
		{
			content->setBounds(0, 0, getWidth(), getHeight()-24);
		}
	}
	
	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		const String id = comboBoxThatHasChanged->getText();

		auto p = ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), id);
		connectToProcessor(p);
	}

	virtual void processorDeleted(Processor* deletedProcessor)
	{
		connectToProcessor(nullptr);
	}

	virtual void updateChildEditorList(bool forceUpdate) {}

	void fillProcessorList(StringArray& arrayToFill)
	{
		Processor::Iterator<LookupTableProcessor> iter(getMainSynthChain(), false);

		while (auto p = iter.getNextProcessor())
		{
			arrayToFill.add(dynamic_cast<Processor*>(p)->getId());
		}
	}

	Processor* getProcessor() { return connectedProcessor.get(); }
	const Processor* getProcessor() const { return connectedProcessor.get(); }

	ModulatorSynthChain* getMainSynthChain();

	const ModulatorSynthChain* getMainSynthChain() const;

	virtual void processorChanged()
	{
		if (getProcessor() == nullptr)
		{
			content = nullptr;

			connectionSelector->clear();
			StringArray sa;
			fillProcessorList(sa);
			connectionSelector->addItemList(sa, 1);
		}
		else
		{
			getProcessor()->addDeleteListener(this);

			auto ltp = dynamic_cast<LookupTableProcessor*>(getProcessor());

			addAndMakeVisible(content = new TableEditor(ltp->getTable(0)));
		}

		resized();
	}

	void connectToProcessor(Processor* p)
	{
		if (connectedProcessor.get() != nullptr)
		{
			connectedProcessor->removeDeleteListener(this);
		}

		jassert(p == nullptr || ProcessorHelpers::is<LookupTableProcessor>(p));

		connectedProcessor = p;
		processorChanged();
	}

protected:

private:


	ScopedPointer<ComboBox> connectionSelector;

	WeakReference<Processor> connectedProcessor;

	ScopedPointer<Component> content;
};




class TableEditorPanel : public PanelWithProcessorConnection
{
public:

	TableEditorPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	SET_PANEL_NAME("TableEditor");

};



#define SET_GENERIC_PANEL_ID(x) static Identifier getGenericPanelId() { static const Identifier id(x); return x;}

template <class ContentType> class GenericPanel : public Component,
										   public FloatingTileContent
{
public:

	static Identifier getPanelId() { return ContentType::getGenericPanelId(); } 
	
	Identifier getIdentifierForBaseClass() const override { return getPanelId(); }

	GenericPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		setInterceptsMouseClicks(false, true);

		addAndMakeVisible(component = new ContentType());
	}

	void resized() override
	{
		component->setBounds(2, 24, jmax<int>(0, getWidth() - 4), jmax<int>(0, getHeight() - 26));
	}

private:

	ScopedPointer<ContentType> component;
};

class ConsolePanel : public FloatingTileContent,
					 public Component
{
public:

	SET_PANEL_NAME("Console");

	ConsolePanel(FloatingTile* parent);

	void resized() override
	{
		console->setBounds(getLocalBounds().withTrimmedTop(16));
	}

	


private:

	ScopedPointer<Console> console;

};

struct BackendCommandIcons
{
	static Path getIcon(int commandId);
};



class ApplicationCommandButtonPanel : public FloatingTileContent,
									  public Component
{
public:


	ApplicationCommandButtonPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		addAndMakeVisible(b = new ShapeButton("Icon", Colours::white.withAlpha(0.3f), Colours::white.withAlpha(0.5f), Colours::white));

		b->setVisible(false);
	}

	SET_PANEL_NAME("Icon");

	void resized() override
	{
		b->setBounds(getLocalBounds());
	}

	void setCommand(int commandID);

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

private:

	ScopedPointer<ShapeButton> b;
};



#endif  // MISCFLOATINGPANELTYPES_H_INCLUDED
