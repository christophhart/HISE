/*
  ==============================================================================

    ScriptWatchTable.h
    Created: 2 Nov 2014 2:09:58pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef SCRIPTWATCHTABLE_H_INCLUDED
#define SCRIPTWATCHTABLE_H_INCLUDED

class ScriptComponentEditPanel;
class ScriptComponentEditListener;

class AutoPopupDebugComponent;



class ScriptingEditor;


/** A table component containing all variables and their current value for the currently debugged ScriptProcessor.
 *	@ingroup debugComponents
 */
class ScriptWatchTable      : public Component,
public TableListBoxModel,
public Timer,
public AutoPopupDebugComponent
{
public:
    
    enum ColumnId
    {
        Type = 1,
        Name,
        Value,
        numColumns
    };
    
    ScriptWatchTable(MainController *controller, BaseDebugArea *area) ;
    
    ~ScriptWatchTable()
    {
        controller = nullptr;
        
        strippedSet.clear();
        
        processor = nullptr;
        editor = nullptr;
        
        table = nullptr;
    }
    
    void timerCallback();
    
    void setScriptProcessor(ScriptProcessor *p, ScriptingEditor *editor);
    
    int getNumRows() override;
    
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;
    
    void selectedRowsChanged(int /*lastRowSelected*/);
    
    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override;
    
    String getHeadline() const;
    
    void resized() override;
    
    void refreshStrippedSet();
    
    void refreshChangedDisplay(const NamedValueSet &oldSet)
    {
        changed.setRange(0, strippedSet.size(), false);
        
        for (int i = 0; i < strippedSet.size(); i++)
        {
            var oldValue = oldSet[strippedSet.getName(i)];
            
            var newValue = strippedSet.getValueAt(i);
            
            if (!oldValue.isUndefined() && !oldValue.equals(newValue))
            {
                changed.setBit(i, true);
            }
        }
    };
    
    void fillWithGlobalVariables()
    {
        const NamedValueSet &globalSet = controller->getGlobalVariableObject()->getProperties();
        
        for (int i = 0; i < globalSet.size(); i++)
        {
            if (globalSet.getValueAt(i).isMethod()) continue;
            if (globalSet.getValueAt(i).isObject() && dynamic_cast<CreatableScriptObject*>(globalSet.getValueAt(i).getDynamicObject()) == nullptr) continue;
            
            Identifier id = Identifier("Globals:" + globalSet.getName(i).toString());
            
            strippedSet.set(id, globalSet.getValueAt(i));
        }
        
    };
    
    void fillWithLocalVariables();
    
    void mouseDoubleClick(const MouseEvent &e) override;
    
private:
    
    TableHeaderLookAndFeel laf;
    
    Font font;
    
    
    
    NamedValueSet strippedSet;
    
    BigInteger changed;
    
    MainController *controller;
    
    WeakReference<Processor> processor;
    Component::SafePointer<ScriptingEditor> editor;
    
    int numRows;            // The number of rows of data we've got
    
    ScopedPointer<TableListBox> table;     // the table component itself
    
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

	static String getTag(DynamicObject *scriptComponent, bool getEndTag);

	static String getText(DynamicObject *scriptComponent);

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
								public SliderListener,
								public AutoPopupDebugComponent
{

public:

	
	ScriptComponentEditPanel(BaseDebugArea *area);

    ~ScriptComponentEditPanel()
    {
        listeners.clear();
        editedComponent = nullptr;
        panel = nullptr;
        codeDragger = nullptr;
    }
    
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
        g.fillAll(Colour(DEBUG_AREA_BACKGROUND_COLOUR));
    }

	void sendPanelPropertyChangeMessage(Identifier idThatWasChanged);

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	void sliderValueChanged(Slider *) override {};

	void sliderDragEnded(Slider *s) override;

	void resized() override
	{
		panel->setBounds(0, 0, getWidth(), getHeight());
		}

	void setEditedComponent(DynamicObject *o);

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

	ReferenceCountedObjectPtr<DynamicObject> editedComponent;


};


#endif  // SCRIPTWATCHTABLE_H_INCLUDED
