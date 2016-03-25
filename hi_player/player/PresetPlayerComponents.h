/*
  ==============================================================================

    PresetPlayerComponents.h
    Created: 30 Jul 2015 1:18:35pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PRESETPLAYERCOMPONENTS_H_INCLUDED
#define PRESETPLAYERCOMPONENTS_H_INCLUDED

/** A simple vertical list of components which resizes their children when needed. 
*/
class VerticalListComponent : public Component
{
public:

	
	class ChildComponent
	{
	public:

		virtual int getComponentHeight() const = 0;

		virtual ~ChildComponent() {};

		Component *asComponent()
		{ 
			// You must derive the class also from Component!
			jassert (dynamic_cast<Component*>(this) != nullptr);

			return dynamic_cast<Component*>(this); 
		}

		const Component *asComponent() const 
		{
			// You must derive the class also from Component!
			jassert(dynamic_cast<const Component*>(this) != nullptr);

			return dynamic_cast<const Component*>(this); 
		}

		void refreshList();
	};

	VerticalListComponent() {};

	~VerticalListComponent()
	{
		clear();
	}

	void addComponent(ChildComponent *componentToAdd);

	void clear() { listComponents.clear(); }

	int getNumListComponents() const { return listComponents.size(); }

	ChildComponent *getComponent(int i) { return listComponents[i]; };

	void refreshList();

	void resized();
	void removeComponent(ChildComponent * presetEditor);
private:

	OwnedArray<ChildComponent> listComponents;
};


class PackageTable : public Component,
	public TableListBoxModel
{
public:

	PackageTable(ValueTree &instrumentList);

	void refreshList();

	int getNumRows() override { return instruments.getNumChildren(); };

	void paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;

	void selectedRowsChanged(int lastRowSelected) override;

	void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& e);

	void paintCell(Graphics& g, int rowNumber, int columnId,
		int width, int height, bool /*rowIsSelected*/) override;

	void resized() override;
	void deselectAll();
private:

	bool internalSelection;

	TableListBox table;

	ValueTree instruments;

	int numRows;

	TableHeaderLookAndFeel laf;
};


class PresetList
{
public:

	PresetList(XmlElement *database);

	void reloadFromXml();

	static bool databaseExists();

	static void rebuildDatabase();

	static File getDatabaseFile();

	VerticalListComponent::ChildComponent *createPresetComponentForIndex(int i);

	int getNumPresetComponents() { return presetList.getNumChildren(); };

	class Entry : public Component,
				  public VerticalListComponent::ChildComponent
	{
	public:

		Entry(ValueTree &presetData);

		~Entry()
		{
			packageTable = nullptr;
		}

		int getComponentHeight() const
		{
			if (folded)
			{
				return 38;
			}
			else
			{
				return jmin<int>(200, 50 + packageTable->getNumRows() * 18);
			}
		}

		void mouseDown(const MouseEvent &)
		{
			folded = !folded;

			refreshList();
		}

		void loadPreset(int rowNumber);

		void deselectAllInstruments()
		{
			packageTable->deselectAll();
		}

		void resized()
		{
			packageTable->setVisible(!folded);

			packageTable->setBounds(10, 40, getWidth()-20, getHeight()-47);
		}

		void paint(Graphics &g) override;

	private:

		bool folded;

		String name;
		String author;
		String version;
		String folder;

		Colour bgColour;

		ScopedPointer<PackageTable> packageTable;

		StringArray fileNames;

	};

private:

	ValueTree presetList;

};


class AnimatedPanelViewport : public Component,
							  public Timer,
							  public ButtonListener
{
public:

	AnimatedPanelViewport();

	~AnimatedPanelViewport()
	{
		viewport->setViewedComponent(nullptr, false);

		viewport = nullptr;
		

		presetList = nullptr;
	}

	void setViewedComponent(Component *contentComponent);

	void toggleVisibility();

	bool isCurrentlyShowing() const { return isShowing; };

	void timerCallback() override;

	void paint(Graphics &g) override;

	void buttonClicked(Button *b) override;

	void resized() override;

	void mouseDown(const MouseEvent &e);

private:

	class Background : public Component
	{
	public:

		Background()
		{
			setOpaque(true);

		}

		void paint(Graphics &g);

	};

	ScopedPointer<Background> background;
	ScopedPointer<Viewport> viewport;

	VerticalListComponent *packageList;

	ScopedPointer<PresetList> presetList;

	ScopedPointer<ShapeButton> refreshBrowserButton;

	bool isShowing;

	float proportionOfAnimation;

	int showedWidth;

};

class PresetHeader : public Component,
					 public Timer,
					 public ButtonListener,
					 public SafeChangeListener,
					 public Slider::Listener
{
public:


	PresetHeader(PresetProcessor *pp);

	~PresetHeader();

	void buttonClicked(Button *b) override;

	void sliderValueChanged(Slider *s) override;

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	void timerCallback() override;

	void paint(Graphics &g) override;

	void resized() override;
	
	void checkFoldButton();

private:

	VerticalListComponent::ChildComponent * getParentAsListChild()
	{
		return dynamic_cast<VerticalListComponent::ChildComponent*>(getParentComponent());

	};

	class PresetHeaderLookAndFeel : public ProcessorEditorHeaderLookAndFeel
	{
	public:

		Colour getColour(int) const override
		{
			return Colour(0xFF777777);
		}
	};

	PresetProcessor *pp;

	PresetHeaderLookAndFeel laf;
	BiPolarSliderLookAndFeel bpslaf;

	ScopedPointer<ShapeButton> foldButton;
	ScopedPointer<Label> idLabel;
	ScopedPointer<ShapeButton> bypassButton;
	ScopedPointer<ShapeButton> deleteButton;
	ScopedPointer<ComboBox> channelFilter;
	ScopedPointer<VuMeter> peakMeter;
	ScopedPointer<Slider> volumeSlider;

};



#endif  // PRESETPLAYERCOMPONENTS_H_INCLUDED
