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
		int maxWidth = CONTAINER_WIDTH;

		if (getWidth() < maxWidth)
			keyboard->setBounds(0, 0, getWidth(), 72);
		else
			keyboard->setBounds((getWidth() - maxWidth) / 2, 0, maxWidth, 72);
	}

	int getFixedHeight() const override { return 72; }

private:
	
	ScopedPointer<CustomKeyboard> keyboard;
};


class ModulatorSynthChain;

class PanelWithProcessorConnection : public FloatingTileContent,
									 public Component,
									 public ComboBox::Listener,
									 public Processor::DeleteListener,
									 public SafeChangeListener
{
public:

	class ProcessorConnection : public UndoableAction
	{
	public:

		ProcessorConnection(PanelWithProcessorConnection* panel_, Processor* newProcessor_, int newIndex_) :
			panel(panel_),
			newProcessor(newProcessor_),
			newIndex(newIndex_)
		{
			oldIndex = panel->currentIndex;
			oldProcessor = panel->currentProcessor.get();
		}

		bool perform() override
		{
			if (panel.getComponent() != nullptr)
			{
				panel->currentIndex = newIndex;
				panel->setCurrentProcessor(newProcessor.get());
				panel->refreshContent();
				return true;
			}
			
			return false;
		}

		bool undo() override
		{
			if (panel.getComponent())
			{
				panel->currentIndex = oldIndex;
				panel->setCurrentProcessor(oldProcessor.get());
				panel->refreshContent();
				return true;
			}

			return false;
		}

	private:

		Component::SafePointer<PanelWithProcessorConnection> panel;
		WeakReference<Processor> oldProcessor;
		WeakReference<Processor> newProcessor;

		int oldIndex = -1;
		int newIndex = -1;

	};

	PanelWithProcessorConnection(FloatingTile* parent);

	virtual ~PanelWithProcessorConnection();

	void paint(Graphics& g) override;

	void changeListenerCallback(SafeChangeBroadcaster* b) override
	{
		refreshConnectionList();
	}

	void resized() override;
	
	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

	virtual void processorDeleted(Processor* deletedProcessor)
	{
		setContentWithUndo(nullptr, -1);
	}

	void refreshConnectionList();

	void refreshIndexList();

	template <class ContentType> ContentType* getContent() { return dynamic_cast<ContentType*>(content.get()); };

	virtual void updateChildEditorList(bool forceUpdate) {}

	Processor* getProcessor() { return currentProcessor.get(); }
	const Processor* getProcessor() const { return currentProcessor.get(); }

	/** Use the connected processor for filling the index list (!= the current processor which is shown). */
	Processor* getConnectedProcessor() { return connectedProcessor.get(); }
	const Processor* getConnectedProcessor() const { return connectedProcessor.get(); }

	ModulatorSynthChain* getMainSynthChain();

	const ModulatorSynthChain* getMainSynthChain() const;

	void setContentWithUndo(Processor* newProcessor, int newIndex);

	void refreshContent()
	{
		if (getConnectedProcessor())
			connectionSelector->setText(getConnectedProcessor()->getId(), dontSendNotification);
		else
			connectionSelector->setSelectedId(1);

		indexSelector->setSelectedId(currentIndex + 2, dontSendNotification);

		if (getProcessor() == nullptr || hasSubIndex() && currentIndex == -1)
		{
			content = nullptr;
		}
		else
		{
			getProcessor()->addDeleteListener(this);

			addAndMakeVisible(content = createContentComponent(currentIndex));
		}

		auto titleToUse = hasCustomTitle() ? getCustomTitle() : getTitle();

		if (getProcessor())
		{
			titleToUse << ": " << getConnectedProcessor()->getId();
		}

		setDynamicTitle(titleToUse);

		resized();
		repaint();

		contentChanged();
	}

	

	virtual Component* createContentComponent(int index) = 0;

	virtual void contentChanged() {};

	virtual void fillModuleList(StringArray& moduleList) = 0;

	virtual void fillIndexList(StringArray& indexList) {};

	virtual bool hasSubIndex() const { return false; }

	void setCurrentProcessor(Processor* p)
	{
		if (currentProcessor.get() != nullptr)
		{
			currentProcessor->removeDeleteListener(this);
		}
		
		currentProcessor = p;
		connectedProcessor = currentProcessor;
	}

	void setConnectionIndex(int newIndex)
	{
		currentIndex = newIndex;
	}

protected:

	template <class ProcessorType> void fillModuleListWithType(StringArray& moduleList)
	{
		Processor::Iterator<ProcessorType> iter(getMainSynthChain(), false);

		while (auto p = iter.getNextProcessor())
		{
			moduleList.add(dynamic_cast<Processor*>(p)->getId());
		}
	}


private:

	Component::SafePointer<BackendRootWindow> rootWindow;

	bool listInitialised = false;

	ScopedPointer<ComboBox> connectionSelector;
	ScopedPointer<ComboBox> indexSelector;

	int currentIndex = -1;
	int previousIndex = -1;
	int nextIndex = -1;

	WeakReference<Processor> currentProcessor;
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

	Component* createContentComponent(int index) override
	{
		auto ltp = dynamic_cast<LookupTableProcessor*>(getProcessor());
		return new TableEditor(ltp->getTable(index));
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<LookupTableProcessor>(moduleList);
	}

	bool hasSubIndex() const override { return true; }

	void fillIndexList(StringArray& indexList) override
	{
		auto ltp = dynamic_cast<LookupTableProcessor*>(getConnectedProcessor());

		for (int i = 0; i < ltp->getNumTables(); i++)
		{
			indexList.add("Table " + String(i + 1));
		}
	}
};


class SliderPackPanel : public PanelWithProcessorConnection
{
public:

	SliderPackPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{
	};

	SET_PANEL_NAME("SliderPackEditor");

	Component* createContentComponent(int index) override
	{
		auto p = dynamic_cast<SliderPackProcessor*>(getProcessor());

		auto sp = new SliderPack(p->getSliderPackData(0));

		sp->setOpaque(true);
		sp->setColour(Slider::backgroundColourId, Colour(0xff333333));

		return sp;
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<SliderPackProcessor>(moduleList);
	}

	void resized() override;
	
};


class CodeEditorPanel : public PanelWithProcessorConnection
					    
{
public:

	CodeEditorPanel(FloatingTile* parent);;

	~CodeEditorPanel();

	SET_PANEL_NAME("ScriptEditor");

	Component* createContentComponent(int index) override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<JavascriptProcessor>(moduleList);
	}

	bool hasSubIndex() const override { return true; }

	void fillIndexList(StringArray& indexList) override;

	void gotoLocation(Processor* p, const String& fileName, int charNumber);

private:

	ScopedPointer<JavascriptTokeniser> tokeniser;
};

class ScriptContentPanel: public PanelWithProcessorConnection
{
public:

	struct Canvas;

	class Editor : public Component
	{
	public:

		Editor(Processor* p);

		void resized() override
		{
			viewport->setBounds(getLocalBounds());
		}

	public:

		ScopedPointer<Viewport> viewport;
	};


	ScriptContentPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	SET_PANEL_NAME("ScriptContent");

	Component* createContentComponent(int /*index*/) override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<JavascriptProcessor>(moduleList);
	}

private:

};


class ScriptWatchTablePanel : public PanelWithProcessorConnection
{
public:

	ScriptWatchTablePanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	SET_PANEL_NAME("ScriptWatchTable");

	Component* createContentComponent(int /*index*/) override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<JavascriptProcessor>(moduleList);
	}

private:

};

class GlobalConnectorPanel : public PanelWithProcessorConnection
{
public:


	GlobalConnectorPanel(FloatingTile* parent):
		PanelWithProcessorConnection(parent)
	{

	}

	SET_PANEL_NAME("GlobalConnectorPanel");

	int getFixedHeight() const override { return 18; }

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	bool hasSubIndex() const override { return false; }

	Component* createContentComponent(int index) override
	{
		return new Component();
	}

	void contentChanged() override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<JavascriptProcessor>(moduleList);
	};

private:

};

class SampleMapEditorPanel : public PanelWithProcessorConnection,
							 public SafeChangeListener
{
public:

	SampleMapEditorPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	

	SET_PANEL_NAME("SampleMapEditor");

	void changeListenerCallback(SafeChangeBroadcaster* b);

	bool hasSubIndex() const override { return false; }

	Component* createContentComponent(int index) override;

	
	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<ModulatorSampler>(moduleList);
	};

	void contentChanged() override;

};



class SampleEditorPanel : public PanelWithProcessorConnection,
						  public SafeChangeListener
{
public:

	SampleEditorPanel(FloatingTile* parent);;

	SET_PANEL_NAME("SampleEditor");

	void changeListenerCallback(SafeChangeBroadcaster* b);

	
	Component* createContentComponent(int index) override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<ModulatorSampler>(moduleList);
	};

	void contentChanged() override;

private:

	struct EditListener;

	ScopedPointer<EditListener> editListener;

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

		addAndMakeVisible(component = new ContentType(getRootWindow()));
	}

	~GenericPanel()
	{
		component = nullptr;
	}

	void resized() override
	{
		component->setBounds(getParentShell()->getContentBounds());
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

	void resized() override;

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
