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

#ifndef SCRIPTWATCHTABLE_H_INCLUDED
#define SCRIPTWATCHTABLE_H_INCLUDED

class ScriptComponentEditPanel;
class ScriptComponentEditListener;

class AutoPopupDebugComponent;


class DebugInformation;
class ScriptingEditor;


/** A table component containing all variables and their current value for the currently debugged ScriptProcessor.
 *	@ingroup debugComponents
 */
class ScriptWatchTable      : public Component,
public TableListBoxModel,
public Timer,
public TextEditor::Listener,
public GlobalScriptCompileListener
{
public:
    
    enum ColumnId
    {
        Type = 1,
		DataType,
        Name,
        Value,
        numColumns
    };
    
	enum class RefreshEvent
	{
		timerCallback = 0,
		filterTextChanged,
		recompiled,
		numRefreshEvents
	};

    ScriptWatchTable(BackendRootWindow *window) ;
    
	SET_GENERIC_PANEL_ID("ScriptWatchTable");

    ~ScriptWatchTable();
    
    void timerCallback();
    
	void scriptWasCompiled(JavascriptProcessor *processor);

    void setScriptProcessor(JavascriptProcessor *p, ScriptingEditor *editor);
    
	void textEditorTextChanged(TextEditor& )
	{
		applySearchFilter();
	}

    int getNumRows() override;
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;
    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override;
    
    String getHeadline() const;
    void resized() override;
    
    void refreshChangeStatus();
    void mouseDoubleClick(const MouseEvent &e) override;
    
	void paint(Graphics &g) override;

	DebugInformation* getDebugInformationForRow(int rowIndex);

private:
    
	void rebuildLines();

	void applySearchFilter();

	ScopedPointer<TextEditor> fuzzySearchBox;

	int numFilteredDebugObjects = 0;

    TableHeaderLookAndFeel laf;
	Array<StringArray> allVariableLines;
	Array<int> filteredIndexes;
    BigInteger changed;
    MainController *controller;
    WeakReference<Processor> processor;
    
    ScopedPointer<TableListBox> table;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptWatchTable);
};


class HiPropertyComponent: public SafeChangeBroadcaster
{
public:
	virtual Identifier getId() const = 0;
	virtual var getValueAsVar() const = 0;
};

class CodeDragger: public Component,
				   public DragAndDropContainer
{
public:

	CodeDragger(ScriptComponentEditPanel *panel_):
		panel(panel_)
	{};

	static String getTag(ReferenceCountedObject*scriptComponent, bool getEndTag);

	static String getText(ReferenceCountedObject*scriptComponent);

	void mouseDown(const MouseEvent &) override;;

	void paint(Graphics &g) override;
	
private:

	String getTextFromPanel();

	String getNameFromPanel();

	ScriptComponentEditPanel *panel;
};


class HiColourPropertyComponent  : public PropertyComponent,
								   public HiPropertyComponent
{
public:
    HiColourPropertyComponent (const String& name,
		const bool canReset, Colour actualColour_, ScriptComponentEditPanel *panel_)
        : PropertyComponent (name),
		  actualColour(actualColour_),
		  id(Identifier(name)),
		  panel(panel_)
    {
        colourPropEditor = new ColourPropEditorComponent (this, canReset);
        addAndMakeVisible (colourPropEditor);
    }

	virtual ~HiColourPropertyComponent();

    virtual void setColour (Colour newColour)
	{
		actualColour = newColour;
		refresh();
		sendChangeMessage();
	}

	Identifier getId() const override {	return id; };

	var getValueAsVar() const override { return (int64)getColour().getARGB(); };

    virtual Colour getColour() const { return actualColour;	};

    virtual void resetToDefault() {};

    void refresh() override { ((ColourPropEditorComponent*) getChildComponent (0))->refresh(); }

    class ColourEditorComponent    : public Component,
                                     public ChangeListener
    {
    public:
		ColourEditorComponent (const bool canReset, HiColourPropertyComponent *parent_)
            : canResetToDefault (canReset),
			parentColourPropertyComponent(parent_)
        {
        }

		~ColourEditorComponent()
		{
			parentColourPropertyComponent = nullptr;
		}

        void paint (Graphics& g);

        virtual void setColour (Colour newColour) = 0;
        virtual void resetToDefault() = 0;
        virtual Colour getColour() const = 0;

        void refresh();

        void mouseDown (const MouseEvent&)
        {
			CallOutBox::launchAsynchronously (new ColourSelectorComp (this, canResetToDefault),
                                              getScreenBounds(), nullptr);
        }

        void changeListenerCallback (ChangeBroadcaster* source)
        {
            const ColourSelector* const cs = (const ColourSelector*) source;

            if (cs->getCurrentColour() != getColour())
                setColour (cs->getCurrentColour());
        }

        class ColourSelectorComp   : public Component,
                                     public ButtonListener
        {
        public:
            ColourSelectorComp (ColourEditorComponent* owner_,
                                const bool canReset);

			virtual ~ColourSelectorComp();
				

            void resized()
            {
                selector.setBounds (getLocalBounds());
            }

            void buttonClicked (Button*)
            {
                owner->resetToDefault();
                owner->refresh();
                selector.setCurrentColour (owner->getColour());
            }

        private:
           
			
			Component::SafePointer<ColourEditorComponent> owner;
            ColourSelector selector;
            TextButton defaultButton;
			LookAndFeel_V3 laf;
        };

    private:

		HiColourPropertyComponent *parentColourPropertyComponent;
        Colour colour;
        bool canResetToDefault;
    };

    class ColourPropEditorComponent     : public ColourEditorComponent
    {
        HiColourPropertyComponent* const owner;

    public:
        ColourPropEditorComponent (HiColourPropertyComponent* const owner_,
                                   const bool canReset)
            : ColourEditorComponent (canReset, owner_),
              owner (owner_)
        {}

		void setColour(Colour newColour) override { owner->setColour(newColour); };

		Colour getColour() const override { return owner->getColour(); };

        void resetToDefault() override { owner->resetToDefault(); }
    };

    ScopedPointer<ColourPropEditorComponent> colourPropEditor;

	Colour actualColour;
	Identifier id;

	ScriptComponentEditPanel *panel;

};



class ScriptComponentEditPanel: public Component,
								public SafeChangeListener,
								public SliderListener
{

public:

	
	ScriptComponentEditPanel(BackendRootWindow* rootWindow);

    ~ScriptComponentEditPanel()
    {
		mc->addScriptComponentEditPanel(nullptr);

        listeners.clear();
        editedComponent = nullptr;
        panel = nullptr;
        codeDragger = nullptr;
    }
    
	SET_GENERIC_PANEL_ID("InterfacePropertyEditor");

	void addListener(ScriptComponentEditListener *listener)
	{
		listeners.addIfNotAlreadyThere(listener);
	}

	void removeListener(ScriptComponentEditListener *listener)
	{
		listeners.removeAllInstancesOf(listener);
	};

    
	void removeAllListeners()
	{
		listeners.clear();
	}
    
    void paint(Graphics &g) override
    {
        g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));
    }

	void sendPanelPropertyChangeMessage(Identifier idThatWasChanged);

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	void sliderValueChanged(Slider *) override {};

	void sliderDragEnded(Slider *s) override;

	void resized() override
	{
		panel->setBounds(0, 0, getWidth(), getHeight());
		}

	void setEditedComponent(ReferenceCountedObject* o);

	static void debugProperties(DynamicObject *properties);;

private:

	friend class CodeDragger;
	
	void fillPanel();

	void addSectionToPanel(const Array<Identifier> &idsToAdd, const String &sectionName);

	void addProperty(Array<PropertyComponent*> &arrayToAddTo, DynamicObject *obj, const Identifier &id, const StringArray &options);


	class HiFilePropertyComponent: public PropertyComponent,
								   public HiPropertyComponent,
								   public ButtonListener,
								   public ComboBoxListener
	{
	public:

		HiFilePropertyComponent(DynamicObject *properties, Identifier id, const StringArray &options, ScriptComponentEditPanel *panel);

		String getFileName() const { return currentFile; }
		
		Identifier getId() const override { return Identifier(getName()); }

		var getValueAsVar() const override { return getFileName(); }

		void refresh() override;
		
		void buttonClicked(Button *) override;
		
		void comboBoxChanged(ComboBox *) override;

	private:

		class CombinedComponent: public Component
		{
		public:
			CombinedComponent(HiFilePropertyComponent *parent);

			void resized()
			{
				button.setBounds(getWidth() - 30, 0, 30, getHeight());
				box.setBounds(0, 0, getWidth()-30, getHeight());
			}

			ComboBox box;

		private:

			TextButton button;
			

		};

		CombinedComponent component;
		StringArray pooledFiles;
		String currentFile;
		ReferenceCountedObjectPtr<DynamicObject> properties;
		Identifier id;
		ScriptComponentEditPanel *panel;
		PopupLookAndFeel plaf;
	};

	class HiSliderPropertyComponent: public SliderPropertyComponent,
									 public HiPropertyComponent
									 
	{
	public:

		HiSliderPropertyComponent(DynamicObject *properties, Identifier id, ScriptComponentEditPanel *panel);

		void setRange(double min, double max, double stepSize)
		{
			slider.setRange(min, max, stepSize);
			
			currentValue = properties->getProperty(id);
			refresh();
		}

		void setValue(double newValue) override
		{
			currentValue = newValue;
			sendSynchronousChangeMessage();
		}

		Identifier getId() const override
		{
			return Identifier(getName());
		}

		var getValueAsVar() const override
		{
			return getValue();
		}

		double getValue() const override
		{
			return currentValue;
		}

	private:

		ReferenceCountedObjectPtr<DynamicObject> properties;

		Identifier id;

		double currentValue;

		ScriptComponentEditPanel *panel;
	};

	class HiChoicePropertyComponent: public ChoicePropertyComponent,
									 public HiPropertyComponent
	{
	public:

		HiChoicePropertyComponent(DynamicObject *properties, Identifier id, const StringArray &options, ScriptComponentEditPanel *panel);

		Identifier getId() const override { return Identifier(getName()); };

		var getValueAsVar() const override { return getItemText(); };

		void setIndex (int newIndex) override;
 	
		int getIndex () const override;

		String getItemText() const;
 	
	private:

		ReferenceCountedObjectPtr<DynamicObject> properties;

		Identifier id;
		int currentIndex;

		ScriptComponentEditPanel *panel;

		PopupLookAndFeel plaf;
	};

	class HiTogglePropertyComponent: public PropertyComponent,
									 public ButtonListener,
									 public HiPropertyComponent
	{
	public:

		HiTogglePropertyComponent(DynamicObject *properties, Identifier id, ScriptComponentEditPanel* /*panel*/);

		void refresh() override
		{

		}

		Identifier getId() const override
		{
			return Identifier(getName());
		}

		var getValueAsVar() const override
		{
			return on;
		}

		void buttonClicked(Button *) override
		{
			setValue(!on);
		}

		void setValue(bool shouldBeOn, NotificationType notifyEditor=sendNotification);;

		bool getValue() const
		{
			return on;
		}

		TextButton button;
		bool on;

		PopupLookAndFeel plaf;
	};

	class HiTextPropertyComponent: public TextPropertyComponent,
								   public HiPropertyComponent
	{
	public:

		HiTextPropertyComponent(DynamicObject *properties, Identifier id, ScriptComponentEditPanel *panel, bool useMultiLine=false);

		Identifier getId() const override
		{
			return Identifier(getName());
		}

		var getValueAsVar() const override
		{
			return getValue();
		}

		void setText(const String &newText) override;

		String getText() const override;

		String getStringVersion(const var &data)
		{
			return data.toString();
		}

		var getValue() const
		{
			return (useNumberMode ? var(value) : var(currentText));
		}

		void setUseNumberMode(bool shouldUseNumberMode)
		{
			useNumberMode = shouldUseNumberMode;
		}
 	
	private:

		bool useNumberMode;
		bool isMultiLine;

		ReferenceCountedObjectPtr<DynamicObject> properties;

		Identifier id;
		
		String currentText;
		double value;

		ScriptComponentEditPanel *panel;

		PopupLookAndFeel plaf;
	};

    HiPropertyPanelLookAndFeel pplaf;
    
	Array<WeakReference<ScriptComponentEditListener>> listeners;

	ScopedPointer<PropertyPanel> panel;
	ScopedPointer<CodeDragger> codeDragger;

	ReferenceCountedObjectPtr<ReferenceCountedObject> editedComponent;


	MainController* mc;

};


#endif  // SCRIPTWATCHTABLE_H_INCLUDED
