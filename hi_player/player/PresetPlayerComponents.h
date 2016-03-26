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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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
