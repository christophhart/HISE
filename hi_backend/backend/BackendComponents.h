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

#ifndef BACKENDCOMPONENTS_H_INCLUDED
#define BACKENDCOMPONENTS_H_INCLUDED


class BackendProcessorEditor;
class ScriptContentContainer;


class MacroParameterTable;

/** A component which shows eight knobs that control eight macro controls and allows editing of the mapped parameters.
*	@ingroup macroControl
*/
class MacroComponent: public Component,
					  public ButtonListener,
					  public SafeChangeListener,
					  public SliderListener,
					  public LabelListener
{
public:

	MacroComponent(BackendProcessor *processor_, MacroParameterTable *table_):
		processor(processor_),
		synthChain(processor_->getMainSynthChain()),
		table(table_)
	{
		synthChain->addChangeListener(this);

		mlaf = new MacroKnobLookAndFeel();

		for(int i = 0; i < 8; i++)
		{
			Slider *s = new Slider();
			s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
			s->setName(synthChain->getMacroControlData(i)->getMacroName());
			s->setRange(0.0, 127.0, 1.0);
			//s->setTextBoxStyle(Slider::TextBoxRight, true, 50, 20);
			s->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
			s->setLookAndFeel(mlaf);
			s->setValue(0.0, dontSendNotification);

			macroKnobs.add(s);
			addAndMakeVisible(s);
			s->setTextBoxIsEditable(false);
			s->addMouseListener(this, true);
			s->addListener(this);

			ShapeButton *t = new ShapeButton("", Colours::black.withAlpha(0.5f), Colours::black.withAlpha(0.7f), Colours::white);

			static const unsigned char pathData[] = { 110,109,173,14,46,68,22,127,139,67,98,173,190,50,68,22,223,148,67,177,188,50,68,34,17,164,67,173,14,46,68,34,113,173,67,108,173,102,40,68,34,193,184,67,108,177,108,23,68,22,207,150,67,108,177,20,29,68,22,127,139,67,98,161,196,33,68,34,33,130,67,173,94,
			41,68,22,31,130,67,173,14,46,68,22,127,139,67,99,109,65,105,178,67,152,161,9,68,108,77,27,167,67,152,65,32,68,108,77,91,212,67,152,153,26,68,108,173,190,34,68,36,17,196,67,108,177,196,17,68,24,31,162,67,108,65,105,178,67,152,161,9,68,99,109,164,146,17,
			68,32,11,253,67,108,164,146,17,68,148,59,50,68,108,72,37,131,67,148,59,50,68,108,72,37,131,67,40,119,196,67,108,60,147,234,67,40,119,196,67,108,152,72,5,68,40,119,164,67,108,146,74,70,67,40,119,164,67,108,146,74,70,67,148,59,66,68,108,164,146,33,68,148,
			59,66,68,108,164,146,33,68,20,9,221,67,108,164,146,17,68,32,11,253,67,99,101,0,0 };

			Path path;
			path.loadPathFromData (pathData, sizeof (pathData));


			t->setShape(path, false, false, false);

			

			t->addListener (this);

			/*TextButton *t = new TextButton();
			t->setButtonText("Edit Macro " + String(i +1));
			t->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
			
			t->setColour (TextButton::buttonColourId, Colour (0x884b4b4b));
			t->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
			t->setColour (TextButton::textColourOnId, Colour (0xaaffffff));
			t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));*/
			t->setTooltip("Show Edit Panel for Macro " + String(i + 1));
			t->setClickingTogglesState(true);

			editButtons.add(t);
			addAndMakeVisible(t);

			
			Label *l = new Label ("", synthChain->getMacroControlData(i)->getMacroName());

			l->setFont (GLOBAL_BOLD_FONT());
			l->setJustificationType (Justification::centred);
			l->setEditable (false, true, false);
			l->setColour (Label::backgroundColourId, Colours::black.withAlpha(0.1f));
			l->setColour (Label::outlineColourId, Colour (0x2b000000));
			l->setColour (TextEditor::textColourId, Colours::black);
			l->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
			l->addListener (this);

			macroNames.add(l);
			addAndMakeVisible (l);

			

		}

		changeListenerCallback(synthChain);

	};

	~MacroComponent()
	{
		if(synthChain != nullptr) synthChain->removeChangeListener(this);
	}

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
			macroNames[i]->setColour(Label::ColourIds::textColourId, on ? Colours::black : Colours::black.withAlpha(0.4f) );			
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

	void paint(Graphics &g)
	{
		g.fillAll(Colour(BACKEND_BG_COLOUR));

		g.setColour(Colours::black.withAlpha(0.1f));

		g.drawRoundedRectangle(0.0f, 0.0f, (float) getWidth(), (float) getHeight() + 3.0f, 3.0f, 1.0f);

		g.setColour(Colour(0x433f3f3f));

		g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)getHeight() +3.0f, 3.0f);
	}

	void changeListenerCallback(SafeChangeBroadcaster *);
	

	int getCurrentHeight()
	{
#if HISE_IOS
		return 200;
#else
		return 90;
#endif
	}

	void resized()
	{
		const int macroAmount = macroKnobs.size();

#if HISE_IOS

		const int width = (getWidth()-30) / 4;
		int x = 15;

		for(int i = 0; i < 4; i++)
		{
			macroKnobs[i]->setBounds(x + width / 2- 24, 10, 48, 48);
			macroNames[i]->setBounds(x, 64, width - 28, 20);
			editButtons[i]->setBounds(macroNames[i]->getRight() + 2, 64, 20, 20);

			x += width;
		}

		x = 15;

		for(int i = 4; i < 8; i++)
		{
			macroKnobs[i]->setBounds(x + width / 2- 24, 100, 48, 48);
			macroNames[i]->setBounds(x, 160, width - 28, 20);
			editButtons[i]->setBounds(macroNames[i]->getRight() + 2, 160, 20, 20);

			x += width;
		}


#else

		const int width = getWidth() / macroAmount;
		int x = 0;

		for(int i = 0; i < macroAmount; i++)
		{
			macroKnobs[i]->setBounds(x + width / 2- 24, 10, 48, 48);
			macroNames[i]->setBounds(x + 4, 64, width - 28, 20);
			editButtons[i]->setBounds(macroNames[i]->getRight() + 2, 64, 20, 20);
			
			x += getWidth() / macroAmount;
		}

#endif

		checkActiveButtons();
	}

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

	BackendProcessor *processor;

	ModulatorSynthChain *synthChain;

	OwnedArray<Slider> macroKnobs;
	OwnedArray<Label> macroNames;
	OwnedArray<ShapeButton> editButtons;

	Component::SafePointer<MacroParameterTable> table;

};

class BreadcrumbComponent : public Component
{
public:
	BreadcrumbComponent()
	{};

	void paint(Graphics &g) override
	{
		g.setColour(Colour(BACKEND_BG_COLOUR_BRIGHT));

		g.fillRoundedRectangle(0.0f,0.0f,(float)getWidth(), (float)getHeight()-8.0f, 3.0f);

		for (int i = 1; i < breadcrumbs.size(); i++)
		{
			g.setColour(Colours::white.withAlpha(0.6f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(">" , breadcrumbs[i]->getRight(), breadcrumbs[i]->getY(), 20, breadcrumbs[i]->getHeight(), Justification::centred, true);
		}
		
	}

	void refreshBreadcrumbs();

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
#if HISE_IOS
				Font f = GLOBAL_BOLD_FONT().withHeight(18.0f);
#else
                Font f = GLOBAL_BOLD_FONT();
#endif
                
                return f.getStringWidth(processor->getId());
            }
			return 0;
        }
        
		void paint(Graphics &g) override
		{
            if(processor.get() != nullptr)
            {
				g.setColour(Colours::white.withAlpha(isMouseOver(true) ? 1.0f : 0.6f));

#if HISE_IOS
				Font f = GLOBAL_BOLD_FONT().withHeight(18.0f);
#else
				Font f = GLOBAL_BOLD_FONT();
#endif

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
};

class BaseDebugArea;

/** A table that contains every mapped parameter for the currently edited macro slot.
*	@ingroup debugComponents
*
*	You can change the parameter range and invert it.
*/
class MacroParameterTable      :	public Component,
									public TableListBoxModel,
									public AutoPopupDebugComponent
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

	MacroParameterTable(BaseDebugArea *area)   :
		AutoPopupDebugComponent(area),
		font (GLOBAL_FONT()),
		data(nullptr)
	{
		setName("Macro Control Parameter List");

		// Create our table component and add it to this component..
		addAndMakeVisible (table);
		table.setModel (this);

		// give it a border

		table.setColour (ListBox::outlineColourId, Colours::black.withAlpha(0.5f));
		table.setColour(ListBox::backgroundColourId, Colour(DEBUG_AREA_BACKGROUND_COLOUR));

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
			
		g.setColour (Colours::white.withAlpha(0.4f));
		g.fillRect (width - 1, 0, 1, height);


		g.setColour (Colours::black);
		g.setFont (font);

		String text;

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

	void resized();

	void paint(Graphics &g)
	{
		if (dragNew)
		{
			g.fillAll(Colours::green.withAlpha(0.02f));
			g.setColour(Colours::black);

			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Drop Preset to load container.", 0, 0, getWidth(), getHeight(), true);
		}

	}

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

	ScopedPointer<InternalViewport> viewport;

	bool dragNew;

};




#endif  // BACKENDCOMPONENTS_H_INCLUDED
