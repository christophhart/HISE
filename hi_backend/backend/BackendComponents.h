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

#ifndef BACKENDCOMPONENTS_H_INCLUDED
#define BACKENDCOMPONENTS_H_INCLUDED

namespace hise { using namespace juce;

class BackendProcessorEditor;
class BackendRootWindow;
class ScriptContentContainer;


class MacroParameterTable;

/** A component which shows eight knobs that control eight macro controls and allows editing of the mapped parameters.
*	@ingroup macroControl
*/
class MacroComponent: public Component,
					  public ButtonListener,
					  public Processor::OtherListener,
					  public SliderListener,
					  public LabelListener
{
public:

	MacroComponent(BackendRootWindow* rootWindow);;

	~MacroComponent();

	SET_GENERIC_PANEL_ID("MacroControls");

	struct MacroControlPopupData
	{
		int itemId;
		ModulatorSynthChain *chain;
		int macroIndex;
	};

	void mouseDown(const MouseEvent &e);

	void addSynthChainToPopup(ModulatorSynthChain *parent, PopupMenu &p, Array<MacroControlPopupData> &popupData);
	

	void setSynthChain(ModulatorSynthChain *synthChainToControl)
	{
		synthChain = synthChainToControl;
	}

	void checkActiveButtons()
	{
		for(int i = 0; i < editButtons.size(); i++)
		{
			const bool on = synthChain->hasActiveParameters(i);

			macroNames[i]->setColour(Label::ColourIds::backgroundColourId, on ? Colours::black.withAlpha(0.1f) : Colours::transparentBlack );			
			macroNames[i]->setColour(Label::ColourIds::textColourId, on ? Colours::white : Colours::white.withAlpha(0.4f) );			
			macroNames[i]->setEnabled(on);
		}

		for(int i = 0; i < macroKnobs.size(); i++)
		{
			macroKnobs[i]->setEnabled(synthChain->hasActiveParameters(i));
		}

		for(int i = 0; i < macroNames.size(); i++)
		{
			if(macroNames[i]->getText() != synthChain->getMacroControlData(i)->getMacroName())
			{
				macroNames[i]->setText(synthChain->getMacroControlData(i)->getMacroName(), dontSendNotification);
			}
		}

	}

	void otherChange(Processor* ) override
	{
		for(int i = 0; i < macroKnobs.size(); i++)
		{
			macroKnobs[i]->setValue(synthChain->getMacroControlData(i)->getCurrentValue(), dontSendNotification);
		}

		checkActiveButtons();
	}
	
	MacroParameterTable* getMainTable();

	int getCurrentHeight()
	{
#if HISE_IOS
		return 200;
#else
		return 90;
#endif
	}

	void resized();

	void sliderValueChanged(Slider *s)
	{
		const int macroIndex = macroKnobs.indexOf(s);

		processor->getMainSynthChain()->setMacroControl(macroIndex, (float)s->getValue(), sendNotification);

		//processor->setParameterNotifyingHost(macroIndex, (float)s->getValue() / 127.0f);
	}

	void labelTextChanged(Label *l)
	{
		for(int i = 0; i< macroNames.size(); i++)
		{
			if(macroNames[i] == l) synthChain->getMacroControlData(i)->setMacroName(l->getText());
		}
		
	}

	void buttonClicked(Button *b);

	private:

	friend class MacroParameterTable;

	ScopedPointer<MacroKnobLookAndFeel> mlaf;

	BackendRootWindow* rootWindow;

	BackendProcessor *processor;

	ModulatorSynthChain *synthChain;

	OwnedArray<Slider> macroKnobs;
	OwnedArray<Label> macroNames;
	OwnedArray<ShapeButton> editButtons;

	
};

class BreadcrumbComponent : public Component,
						    public ControlledObject,
							public MainController::ProcessorChangeHandler::Listener
{
public:
	BreadcrumbComponent(ProcessorEditorContainer* c);;

	~BreadcrumbComponent();

	void moduleListChanged(Processor* /*processorThatWasChanged*/, MainController::ProcessorChangeHandler::EventType type);

	void paint(Graphics &g) override;

	void refreshBreadcrumbs();

	static void newRoot(BreadcrumbComponent& b, Processor* oldRoot, Processor* newRoot)
	{
		b.refreshBreadcrumbs();
	}

    void resized();
    
private:

	class Breadcrumb : public Component
	{
	public:
        
        Breadcrumb(const Processor *p):
        processor(const_cast<Processor*>(p)),
		isOver(false)
        {
			setRepaintsOnMouseActivity(true);
		};

        int getPreferredWidth() const
        {
            if(processor.get() != nullptr)
            {
				Font f = GLOBAL_BOLD_FONT();
                return f.getStringWidth(processor->getId()) + 10.0f;
            }
			return 10.0f;
        }
        
		void paint(Graphics &g) override
		{
            if(processor.get() != nullptr)
            {
				g.setColour(Colours::white.withAlpha(isMouseOver(true) ? 1.0f : 0.6f));
				Font f = GLOBAL_BOLD_FONT();
                g.setFont(f);
                g.drawText(processor->getId(), getLocalBounds(), Justification::centredLeft, true);
            }
		}
        
		void mouseDown(const MouseEvent& /*event*/) override;

    private:
        
        const WeakReference<Processor> processor;
		bool isOver;
	};

	OwnedArray<Breadcrumb> breadcrumbs;

	Component::SafePointer<ProcessorEditorContainer> container;
	JUCE_DECLARE_WEAK_REFERENCEABLE(BreadcrumbComponent);
};

class BaseDebugArea;

/** A table that contains every mapped parameter for the currently edited macro slot.
*	@ingroup debugComponents
*
*	You can change the parameter range and invert it.
*/
class MacroParameterTable      :	public Component,
									public MidiKeyboardFocusTraverser::ParentWithKeyboardFocus,
									public TableListBoxModel
{
public:

	enum ColumnId
	{
		ProcessorId = 1,
		ParameterName,
		Inverted,
		Minimum,
		Maximum,
		numColumns
	};

	MacroParameterTable(BackendRootWindow* /*rootWindow*/)   :
		font (GLOBAL_FONT()),
		data(nullptr)
	{
		setName("Macro Control Parameter List");

		// Create our table component and add it to this component..
		addAndMakeVisible (table);
		table.setModel (this);

		// give it a border

		table.setColour (ListBox::outlineColourId, Colours::black.withAlpha(0.5f));
		table.setColour(ListBox::backgroundColourId, HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));

		table.setOutlineThickness (0);

		laf = new TableHeaderLookAndFeel();

		table.getHeader().setLookAndFeel(laf);
		table.getHeader().setSize(getWidth(), 22);

		table.getViewport()->setScrollBarsShown(true, false, true, false);

		table.getHeader().setInterceptsMouseClicks(false, false);

		table.getHeader().addColumn("Processor", ProcessorId, 90);
		table.getHeader().addColumn("Parameter", ParameterName, 90);
		table.getHeader().addColumn("Inverted", Inverted, 50);
		table.getHeader().addColumn("Min", Minimum, 70);
		table.getHeader().addColumn("Max", Maximum, 70);

		setWantsKeyboardFocus(true);
	}

	~MacroParameterTable()
	{
		table.getHeader().setLookAndFeel(nullptr);
	}

	SET_GENERIC_PANEL_ID("MacroTable");

	int getNumRows() override
	{
		return data == nullptr ? 0 : data->getNumParameters();
	};

	void updateContent()
	{
		if(data != nullptr) data->clearDanglingProcessors();

		table.updateContent();
	}

	void setMacroController(ModulatorSynthChain::MacroControlData *newData)
	{
		data = newData;

		setName("Macro Control Parameter List: " + (newData != nullptr ? newData->getMacroName() : "Idle"));

		updateContent();

		if(getParentComponent() != nullptr) getParentComponent()->repaint();
	}

	void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override
	{
			
			

		if(rowNumber % 2)
		{
			g.setColour(Colours::black.withAlpha(0.05f));

			g.fillAll();
		}

		if (rowIsSelected)
		{
			
			g.fillAll (Colour(0x44000000));
			

		}
	}

	bool keyPressed(const KeyPress& key) override
	{
		if (key == KeyPress::deleteKey)
		{
			if (data != nullptr)
			{
				data->removeParameter(table.getSelectedRow());
				table.updateContent();
				return true;
			}
		}

		return false;
	}

	void setRangeValue(int row, ColumnId column, double newRangeValue)
	{
		jassert(data != nullptr);

		if(data != nullptr)
		{
			if(column == Minimum) data->getParameter(row)->setRangeStart(newRangeValue);
			else if(column == Maximum) data->getParameter(row)->setRangeEnd(newRangeValue);
		}
	};

	void setInverted(int row, bool value)
	{
		jassert(data != nullptr);

		if(data != nullptr)
		{
			data->getParameter(row)->setInverted(value);
				
		}

	}

	void selectedRowsChanged(int /*lastRowSelected*/) {};

	Component* refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                    Component* existingComponentToUpdate) override
	{
		

		if (columnId == Minimum || columnId == Maximum)
		{
			ValueSliderColumn* slider = dynamic_cast<ValueSliderColumn*> (existingComponentToUpdate);

				
			if (slider == nullptr)
				slider = new ValueSliderColumn(*this);

			ModulatorSynthChain::MacroControlledParameterData *pData = data->getParameter(rowNumber);

			const double value = pData->getParameterRangeLimit(columnId == Maximum);

			slider->setRowAndColumn (rowNumber, (ColumnId)columnId, value, pData->getTotalRange());

			return slider;
		}
		else if(columnId == Inverted)
		{
			InvertedButton* b = dynamic_cast<InvertedButton*> (existingComponentToUpdate);

				
			if (b == nullptr)
				b = new InvertedButton(*this);

			ModulatorSynthChain::MacroControlledParameterData *pData = data->getParameter(rowNumber);

			const bool value = pData->isInverted();

			b->setRowAndColumn(rowNumber, value);

			return b;
		}
		{
			// for any other column, just return 0, as we'll be painting these columns directly.

			jassert (existingComponentToUpdate == nullptr);
			return nullptr;
		}
	}

	void paintCell (Graphics& g, int rowNumber, int columnId,
					int width, int height, bool /*rowIsSelected*/) override
	{
		g.setColour (Colours::white.withAlpha(0.8f));
		g.setFont (font);

		String text;

		if (data->getParameter(rowNumber) == nullptr)
			return;

		if(data->getParameter(rowNumber)->getProcessor() == nullptr)
		{
			return;
		}

		switch(columnId)
		{
		case ProcessorId:	text << data->getParameter(rowNumber)->getProcessor()->getId(); break;
		case ParameterName: text << data->getParameter(rowNumber)->getParameterName(); break;
			

		}

		g.drawText (text, 2, 0, width - 4, height, Justification::centredLeft, true);

	}


	//==============================================================================
	void resized() override
	{
		table.setBounds(getLocalBounds());

			
	}


private:

	class ValueSliderColumn: public Component,
						public SliderListener
	{
	public:
		ValueSliderColumn(MacroParameterTable &table):
			owner(table)
		{
			addAndMakeVisible(slider = new Slider());

			slider->setLookAndFeel(&laf);
			slider->setSliderStyle (Slider::LinearBar);
			slider->setTextBoxStyle (Slider::TextBoxLeft, true, 80, 20);
			slider->setColour (Slider::backgroundColourId, Colour (0x38ffffff));
			slider->setColour (Slider::thumbColourId, Colour (SIGNAL_COLOUR));
			slider->setColour (Slider::rotarySliderOutlineColourId, Colours::black);
			slider->setColour (Slider::textBoxOutlineColourId, Colour (0x38ffffff));
			slider->setColour (Slider::textBoxTextColourId, Colours::black);
			slider->setTextBoxIsEditable(true);

			slider->addListener(this);
		}

		void resized()
		{
			slider->setBounds(getLocalBounds());
		}

		void setRowAndColumn (const int newRow, ColumnId column, double value, NormalisableRange<double> range)
		{
			row = newRow;
			columnId = column;

			slider->setRange(range.start, range.end, 0.1);

			slider->setValue(value, dontSendNotification);
		}

	private:

		void sliderValueChanged (Slider *) override
		{
			owner.setRangeValue(row, columnId, slider->getValue());
		}

	private:
		MacroParameterTable &owner;

		HiPropertyPanelLookAndFeel laf;
				
		int row;
		ColumnId columnId;
		ScopedPointer<Slider> slider;
	};

	class InvertedButton: public Component,
							public ButtonListener
	{
	public:

		InvertedButton(MacroParameterTable &owner_):
			owner(owner_)
		{
			addAndMakeVisible(t = new TextButton("Inverted"));
			t->setButtonText("Inverted");
			t->setLookAndFeel(&laf);
			t->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
			t->addListener (this);
			t->setTooltip("Invert the range of the macro control for this parameter.");
			t->setColour (TextButton::buttonColourId, Colour (0x88000000));
			t->setColour (TextButton::buttonOnColourId, Colour (0x88FFFFFF));
			t->setColour (TextButton::textColourOnId, Colour (0xaa000000));
			t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));			
			
			t->setClickingTogglesState(true);
		};

		void resized()
		{
			t->setBounds(getLocalBounds());
		}

		void setRowAndColumn (const int newRow, bool value)
		{
			row = newRow;
				
			t->setToggleState(value, dontSendNotification);

			t->setButtonText(value ? "Inverted" : "Normal");

		}

		void buttonClicked(Button *b)
		{
			t->setButtonText(b->getToggleState() ? "Inverted" : "Normal");
			owner.setInverted(row, b->getToggleState());
		};

	private:

		MacroParameterTable &owner;
				
		int row;
		ColumnId columnId;
		ScopedPointer<TextButton> t;

		HiPropertyPanelLookAndFeel laf;


	};

	TableListBox table;     // the table component itself
	Font font;

	ScopedPointer<TableHeaderLookAndFeel> laf;

	ModulatorSynthChain::MacroControlData *data;

	int numRows;            // The number of rows of data we've got

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroParameterTable)
};


/** A viewport that caches its content when it begins scrolling */
class CachedViewport : public Component,
	public DragAndDropTarget
{
public:

	CachedViewport();

	enum ColourIds
	{
		backgroundColourId = 0x1004
	};

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;

	void itemDragEnter(const SourceDetails &dragSourceDetails) override;

	void itemDragExit(const SourceDetails &dragSourceDetails) override;

	void itemDropped(const SourceDetails &dragSourceDetails) override;;

	void showPreloadMessage(bool shouldShow)
	{
		isPreloading = shouldShow;
		repaint();
	}

	void paint(Graphics& g)
	{
		g.fillAll(Colour(0xFF333333));

		if (isPreloading)
		{
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Loading Instrument...", viewport->getLocalBounds(), Justification::centred);
		}
	}

	void resized();

	
private:

	class InternalViewport : public Viewport
	{
	public:


		InternalViewport();


		bool isCurrentlyScrolling;

		Image cachedImage;

		void paint(Graphics &g);

	};



public:

	std::atomic<bool> isPreloading;

	ScopedPointer<InternalViewport> viewport;

	bool dragNew;

};

namespace multipage
{

#define MULTIPAGE_BIND_CPP(className, methodName) state->bindCallback(#methodName, BIND_MEMBER_FUNCTION_1(className::methodName));

struct EncodedDialogBase: public Component,
						  public QuasiModalComponent,
						  public multipage::HardcodedDialogWithStateBase,
					      public ControlledObject
{
	EncodedDialogBase(BackendRootWindow* bpe_, bool addBorder=true);

	struct Factory: public PathFactory
	{
		Path createPath(const String& url) const override
		{
			Path p;
			LOAD_EPATH_IF_URL("close", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
			return p;
		}
	} factory;

	void writeState(const Identifier& id, const var& value)
	{
		state->globalState.getDynamicObject()->setProperty(id, value);
	}

	var readState(const Identifier& id) const
	{
		return state->globalState[id];
	}

	virtual void bindCallbacks() = 0;

	void loadFrom(const String& d)
	{
		MemoryBlock mb;
		mb.fromBase64Encoding(d);
		MemoryInputStream mis(mb, false);
		MonolithData md(&mis);

		state = new State(var());
        
        try
        {
            addAndMakeVisible(dialog = md.create(*state, true));
        }
        catch(String& e)
        {
            PresetHandler::showMessageWindow ("Error loading dialog", e, PresetHandler::IconType::Error);
            
            
        }

		

        if(dialog != nullptr)
        {
            dialog->setFinishCallback([this]()
            {
                findParentComponentOfClass<ModalBaseWindow>()->clearModalComponent();
            });

            bindCallbacks();

            setSize(dialog->getWidth()+2*(int)addBorder, dialog->getHeight()+2*(int)addBorder);

            dialog->showFirstPage();

			Component::callRecursive<simple_css::FlexboxComponent>(this, [this](simple_css::FlexboxComponent* c)
			{
				auto id = simple_css::FlexboxComponent::Helpers::getIdSelectorFromComponentClass(c).name;

				if(id == "header" && c->isVisible())
				{
					c->setInterceptsMouseClicks(true, true);
					this->dragger = new WindowDragger(rootWindow, this, c);
					return true;
				}

				return false;
			});
        }
	}

	void closeAndPerform(const std::function<void()>& f)
	{
		dialog->setFinishCallback(f);

		MessageManager::callAsync([this]()
		{
			dialog->navigate(true);
		});
	}

	void setElementProperty(const String& listId, const Identifier& id, const var& newValue)
	{
		if(auto pb = dialog->findPageBaseForID(listId))
		{
			pb->getInfoObject().getDynamicObject()->setProperty(id, newValue);
			pb->updateInfoProperty(id);
		}
	}

	void paint(Graphics& g) override
	{
		

		g.fillAll(Colour(0xFF333333));

		if(addBorder)
		{
			g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF474747)));
			g.drawRect(getLocalBounds(), 1);
		}
		
	}

	const bool addBorder = true;

	void resized() override
	{
		auto b = getLocalBounds().reduced((int)addBorder);

        if(dialog != nullptr)
            dialog->setBounds(b);

		closeButton.setBounds(b.removeFromRight(34).removeFromTop(34).reduced(8));
		closeButton.toFront(false);
	}

	void navigate(int pageIndex, bool shouldSubmit)
	{
		SafeAsyncCall::call<EncodedDialogBase>(*this, [pageIndex, shouldSubmit](EncodedDialogBase& db)
		{
			if(shouldSubmit)
			{
				db.state->currentPageIndex = pageIndex-1;
				db.dialog->navigate(true);
			}
			else
			{
				db.state->currentPageIndex = pageIndex;
				db.dialog->refreshCurrentPage();
			}
		});
	}
	
protected:

	ScopedPointer<State> state;
	ScopedPointer<Dialog> dialog;

	HiseShapeButton closeButton;

private:

	

	struct WindowDragger: public MouseListener,
					      public ComponentBoundsConstrainer
	{
		WindowDragger(Component* rw, Component* dialog_, Component* draggedComponent_):
		  rootWindow(rw),
		  dialog(dialog_),
		  draggedComponent(draggedComponent_)
		{
			draggedComponent->addMouseListener(this, true);
		};

		~WindowDragger()
		{
			if(draggedComponent.getComponent() != nullptr)
				draggedComponent->removeMouseListener(this);
		}

		void mouseDown(const MouseEvent& e) override
		{
			dragger.startDraggingComponent(dialog, e);
		}

		void mouseDrag(const MouseEvent& e) override
		{
			dragger.dragComponent(dialog, e, this);
		}

		void checkBounds (Rectangle<int>& bounds,
                              const Rectangle<int>& previousBounds,
                              const Rectangle<int>& limits,
                              bool isStretchingTop,
                              bool isStretchingLeft,
                              bool isStretchingBottom,
                              bool isStretchingRight) override
		{
			bounds = bounds.constrainedWithin(rootWindow->getLocalBounds());
		}

		Component::SafePointer<Component> draggedComponent;
		Component::SafePointer<Component> dialog;
		Component::SafePointer<Component> rootWindow;
		
		ComponentDragger dragger;
	};

	ScopedPointer<WindowDragger> dragger;

	Component* rootWindow;

	JUCE_DECLARE_WEAK_REFERENCEABLE(EncodedDialogBase);
};

} // namespace multipage
} // namespace hise

#endif  // BACKENDCOMPONENTS_H_INCLUDED
