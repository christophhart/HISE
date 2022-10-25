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

#ifndef SAMPLEEDITORCOMPONENTS_H_INCLUDED
#define SAMPLEEDITORCOMPONENTS_H_INCLUDED

namespace hise { using namespace juce;

class SamplerBody;
class SampleEditHandler;
class SamplerSoundWaveform;
class SamplerSoundMap;




/** A base class for all sample editing components.
*
*	It offers a synchronous callback system for selecting sounds with recursive protection.
*/
class SamplerSubEditor
{
public:

	SamplerSubEditor(SampleEditHandler* handler_);;
    
    virtual ~SamplerSubEditor() {};

	/** Overwrite this and call the method that updates the interface. */
	virtual void updateInterface() = 0;

protected:

	/** This is called whenever one of the other subeditors (or itself) called selectSounds(). 
	*
	*	Make sure you update the interface to select the new sounds here. Calls to selectSounds are legal, as they do not trigger the callback again.
	*/
	virtual void soundsSelected(int numSelected)
	{
		jassertfalse;
	}

	

	SampleEditHandler* handler;

private:

    bool internalChange;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SamplerSubEditor);
};


/** A Label component that displays a popup menu when right clicked. */
class PopupLabel: public Label
{
public:

	class TooltipPopupComponent : public PopupMenu::CustomComponent,
								  public SettableTooltipClient
	{
	public:

		TooltipPopupComponent(String text_, String tooltip_, int width_) :
			PopupMenu::CustomComponent(true),
			text(text_),
			tooltip(tooltip_),
			width(width_)
		{
			setTooltip(tooltip);
		};

		void getIdealSize(int &idealWidth, int &idealHeight) override
		{
			Font g = GLOBAL_FONT();

			idealWidth = jmax<int>(width, g.getStringWidth(text) + 5);
			idealHeight = (int)g.getHeight() + 3;
		};

		void paint(Graphics &g)
		{
			g.setFont(GLOBAL_FONT());
			
			if (!isItemHighlighted())
			{
				Colour textColour = getParentComponent()->findColour(PopupMenu::ColourIds::textColourId);
				g.setColour(textColour);
			}
			else
			{
				Colour bg = getParentComponent()->findColour(PopupMenu::ColourIds::highlightedBackgroundColourId);
				g.fillAll(bg);

				Colour textColour = getParentComponent()->findColour(PopupMenu::ColourIds::highlightedTextColourId);
				g.setColour(textColour);
				
			}

			g.drawText(text, getBounds(), Justification::centredLeft, true);



		}

	private:

		String text;
		String tooltip;
		int width;

	};

	PopupLabel(const String &name=String(), const String &initialText=String()):
		Label(name, initialText),
		currentIndex(0)
	{
		
	};

	/** Adds an option to the popup menu. */
	void addOption(const String &newOption, const String &toolTip = String()) { options.add(newOption); toolTips.add(toolTip); };

	void setTickedState(BigInteger state)
	{
		isTicked = state;
	}

	/** Clears all options. */
	void clearOptions() { options.clear(); toolTips.clear(); };

	/** Sets the label text value using the index.
	*
	*	It triggeres a labelChanged notification
	*/
	void setItemIndex(int index, NotificationType notify=sendNotification);

	/** Returns the currently selected Index. */
	int getCurrentIndex() const noexcept { return currentIndex;	};

	String getOptionDescription() const;
	

protected:

	void mouseDown(const MouseEvent &e) override;

private:

	void showPopup();
	
	

	StringArray options;
	StringArray toolTips;
	int currentIndex;

	BigInteger isTicked;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupLabel)
};




/** A simple rectangle which represents a ModulatorSamplerSound within a SamplerSoundMap.
*	@ingroup components
*/
class SampleComponent
{
public:

	SampleComponent(ModulatorSamplerSound *s, SamplerSoundMap *parentMap);

	~SampleComponent() 
	{ 
		masterReference.clear();
	};

	void setSampleIsPlayed(bool isPlayed)
	{
		played = isPlayed;
	}

	Colour getColourForSound(bool wantsOutlineColour) const;

    SamplerTools::Mode getModeForSample() const;
    
	bool samplePathContains(Point<int> localPoint) const;

    void drawSampleRectangle(Graphics &g, Rectangle<int> area);

	const ModulatorSamplerSound *getSound() const noexcept { return sound.get(); };

	ModulatorSamplerSound *getSound() noexcept { return sound.get(); };

	void setSelected(bool shouldBeSelected, bool isDragSelection=false)
	{
		if (isDragSelection)
			dragSelection = shouldBeSelected;
		else
			selected = shouldBeSelected;
	}

	bool isSelected() const { return selected; }

	void setNumOverlayingSiblings(int numOverlayerSiblings_)
	{
		numOverlayerSiblings = numOverlayerSiblings_;
	}
	bool needsToBeDrawn();

	void setSampleBounds(int x, int y, int width, int height)
	{
		bounds = Rectangle<int>(x, y, width, height);
	}

    void setToolMode(SamplerTools::Mode m)
    {
        toolMode = m;
    }
    
	void setVisible(bool shouldBeVisible) { visible = shouldBeVisible; }
	void setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; };

	bool isVisible() const { return visible; }
	bool isEnabled() const noexcept{ return enabled; };

	Rectangle<int> getBoundsInParent() { return bounds; }

	bool appliesToGroup(const Array<int>& groupIndexToCompare) const;

	void checkSelected(ModulatorSamplerSound::Ptr currentSelection)
	{
		isMainSelection = currentSelection == sound;
	}

private:

    SamplerTools::Mode toolMode = SamplerTools::Mode::Nothing;
	bool isMainSelection = false;

	friend class WeakReference < SampleComponent > ;

	WeakReference<SampleComponent>::Master masterReference;

	Rectangle<int> bounds;

	bool selected;
	bool dragSelection = false;
	bool enabled;
	bool visible;
	bool played = false;

	float transparency;

	int numOverlayerSiblings;

	Path outline;

	SamplerSoundMap *map;
	ModulatorSamplerSound::Ptr sound;
};

/** A component which displays all loaded ModulatorSamplerSounds and allows editing of their properties. 
*	@ingroup components
*/
class SamplerSoundMap: public Component,
					   public LassoSource<ModulatorSamplerSound::Ptr>,
					   public SettableTooltipClient,
					   public MainController::SampleManager::PreloadListener,
					   public SampleMap::Listener,
					   public PooledUIUpdater::SimpleTimer
{
public:
	
	/** Simple direction enum to find the next neighbour. @see selectNeighbourSample() */
	enum Neighbour
	{
		Left, ///<
		Right, ///<
		Up, ///<
		Down, ///<
		numNeighbours
	};

	enum DragLimiters
	{
		NoLimit = 0,
		VelocityOnly,
		KeyOnly,
		numDragLimiters
	};

	SamplerSoundMap(ModulatorSampler *ownerSampler_);

	~SamplerSoundMap();;

	void timerCallback() override
	{
		if (pCounter++ % 10 == 0)
			repaintIfIdle();
	}
	
	static void keyChanged(SamplerSoundMap& map, int noteNumber, int velocity);

	void sampleMapWasChanged(PoolReference newSampleMap) override
	{
		oldReference = newSampleMap;

		propertyUpdater = new valuetree::RecursivePropertyListener();

		propertyUpdater->setCallback(ownerSampler->getSampleMap()->getValueTree(),
			SampleIds::Helpers::getMapIds(), valuetree::AsyncMode::Asynchronously,
			BIND_MEMBER_FUNCTION_2(SamplerSoundMap::updateSamplesFromValueTree));

		updateSampleComponents();
	}

	void updateSamplesFromValueTree(ValueTree v, Identifier id)
	{
		for (int i = 0; i < sampleComponents.size(); i++)
		{
			if (auto sound = sampleComponents[i]->getSound())
			{
				if (sound->getData() == v)
				{
					updateSampleComponent(i, sendNotificationSync);
					break;
				}
			}
		}
	}

    static void updateToolMode(SamplerSoundMap& map, SamplerTools::Mode newMode)
    {
        for(auto s: map.sampleComponents)
            s->setToolMode(newMode);
        
        map.repaint();
    }
    
	void sampleAmountChanged() override;

	static void selectionChanged(SamplerSoundMap& map, int numSelected);

	void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue) override;


	void preloadStateChanged(bool isPreloading) override;

	void modifierKeysChanged(const ModifierKeys &modifiers) override;

	bool keyPressed(const KeyPress &k) override;

	/** searches all sounds and selects the next neighbour. */
	void findLassoItemsInArea (Array<ModulatorSamplerSound::Ptr> &itemsFound, const Rectangle< int > &area) override;

	SelectedItemSet<ModulatorSamplerSound::Ptr> &getLassoSelection() override;;

	static void setDisplayedSound(SamplerSoundMap& map, ModulatorSamplerSound::Ptr sound, int);

	void paint(Graphics &g) override;
	void paintOverChildren(Graphics &g) override;
    
	/** returns the root notes for all files that are dragged over the component. */
	BigInteger getRootNotesForDraggedFiles() const { return draggedFileRootNotes; };

	void resized() override 
	{ 
		updateSampleComponents(); 
	};

	/** updates the position / size of the specified sound. */
	void updateSampleComponentWithSound(ModulatorSamplerSound *sound);

	/** updates the position / size of the sound with the supplied index. */
	void updateSampleComponent(int index, NotificationType s);

	/** updates the position / size of all sounds. */
	void updateSampleComponents();

	void mouseDown(const MouseEvent &e) override;
	void mouseUp(const MouseEvent &e) override;
	void mouseExit(const MouseEvent &) override;
	void mouseDrag(const MouseEvent &e) override;
	void mouseMove(const MouseEvent &e) override;

	
	/** This hides all sounds that to not belong to the specified group index. If you want to display all sounds, pass -1. */
	void soloGroup(BigInteger groupIndex);

	/** updates all sounds. It deletes all SampleComponents and recreates them if new samples are detected. */
	void updateSoundData();

	void drawSampleComponentsForDragPosition(int numDraggedFiles, int x, int y);

	void drawSampleMapForDragPosition();

	void clearDragPosition()
	{
		draggedFileRootNotes.clear();
		repaintIfIdle();
	};

    void refreshGraphics()
    {
		repaintIfIdle();
    }
    
    void drawSoundMap(Graphics &g);
    
	const ModulatorSampler* getSampler() const { return ownerSampler; }

	void repaintIfIdle()
	{
		if (!skipRepaint)
			repaint();
	}

private:

	struct RepaintSkipper
	{
		RepaintSkipper(SamplerSoundMap& parent_):
			parent(parent_),
			prevValue(parent_.skipRepaint)
		{
			parent.skipRepaint = true;
		}

		~RepaintSkipper()
		{
			parent.skipRepaint = prevValue;
			parent.repaintIfIdle();
		}

		SamplerSoundMap& parent;
		const bool prevValue;
	};

	bool skipRepaint = false;

	int pCounter = 0;

	BigInteger currentSoloGroup;

	ModulatorSamplerSound::WeakPtr selectedSound;

	PoolReference oldReference;

	bool isPreloading = false;

	/** A POD object containing data for a dragged sound. */
	struct DragData
	{
		ModulatorSamplerSound::Ptr sound;
		StreamingHelpers::BasicMappingData data;
	};

	/** checks if the sampler contains new samples that are not displayed yet. */
	bool newSamplesDetected();

	bool shouldDragSamples(const MouseEvent& e) const;

	SampleComponent* getSampleComponentAt(Point<int> point) const;

	void createDragData(const MouseEvent& e);

	void endSampleDragging(bool copyDraggedSounds);
	
	ModulatorSampler *ownerSampler;
	SampleEditHandler* handler;

	SelectedItemSet<ModulatorSamplerSound::Ptr> dragSet;

	Rectangle<int> dragArea;
	Array<DragData> dragStartData;
	BigInteger draggedFileRootNotes;
	int currentDragDeltaX = 0;
	int currentDragDeltaY = 0;
	int semiTonesPerNote;

	bool hasDraggedSamples;

	uint8 pressedKeys[128];
	int notePosition;
	int veloPosition;
	
	DragLimiters currentDragLimiter;

	OwnedArray<SampleComponent> sampleComponents;

	ScopedPointer<LassoComponent<ModulatorSamplerSound::Ptr>> sampleLasso;

    Image currentSnapshot;

	ScopedPointer<valuetree::RecursivePropertyListener> propertyUpdater;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SamplerSoundMap);
};

/** A wrapper class around a SamplerSoundMap which adds a keyboard that can be clicked to trigger the note. 
*	@ingroup components
*/
class MapWithKeyboard: public Component
{
public:

	MapWithKeyboard(ModulatorSampler *ownerSampler);

	void paint(Graphics &g) override;
	void resized() override;
	void mouseDown(const MouseEvent &e) override;
	void mouseUp(const MouseEvent&e) override;

	ScopedPointer<SamplerSoundMap> map;
	BigInteger selectedRootNotes;

private:

	int lastNoteNumber;
	ModulatorSampler *sampler;
};


/** A table component containing all samples of a ModulatorSampler.
*	@ingroup components
*/
class SamplerSoundTable    : public Component,
                             public TableListBoxModel,
							 public SamplerSubEditor,
							 public MainController::SampleManager::PreloadListener
{
public:
	SamplerSoundTable(ModulatorSampler *ownerSampler_, SampleEditHandler* handler);
	~SamplerSoundTable();
	void refreshList();

    int getNumRows() override;

	bool broadcasterIsSelection(ChangeBroadcaster *b) const;


	void preloadStateChanged(bool isPreloading) override;
	
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;

	void selectedRowsChanged(int lastRowSelected) override;

    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override;

    void sortOrderChanged (int newSortColumnId, bool isForwards) override;

	void soundsSelected(int numSelected) override;
    
    void resized() override;

	void updateInterface() override
	{
		if(!isPreloading)
			refreshList();
	}

	void refreshPropertyForRow(int index, const Identifier& id);

private:

	bool isDefaultOrder = false;

	friend class SamplerTable;

	bool isPreloading = false;

	ModulatorSampler *ownerSampler;
	
	bool internalSelection;

    TableListBox table;     
    Font font;

	SampleSelection sortedSoundList;

    int numRows;

	TableHeaderLookAndFeel laf;

	Array<Identifier> columnIds;

    //==============================================================================
    // A comparator used to sort our data when the user clicks a column header
    class DemoDataSorter
    {
    public:
		DemoDataSorter (const Identifier& propertyToSort_, bool forwards)
            : propertyToSort (propertyToSort_),
              direction (forwards ? 1 : -1),
			  isStringProperty(propertyToSort_ == SampleIds::FileName)
        {}

		int compareElements (ModulatorSamplerSound::Ptr first, ModulatorSamplerSound::Ptr second) const
        {
			if (first == nullptr || second == nullptr)
				return direction;

			if (isStringProperty)
			{
				int result = first->getSampleProperty(propertyToSort).toString()
					.compareNatural(second->getSampleProperty(propertyToSort).toString());

				return direction * result;
			}
			else
			{
				auto v1 = (int)first->getSampleProperty(propertyToSort);
				auto v2 = (int)second->getSampleProperty(propertyToSort);

				int result = 0;

				if (v1 < v2)
					result = -1;
				else if (v1 > v2)
					result = 1;

				return direction * result;
			}
        }

    private:

		const bool isStringProperty;
		Identifier propertyToSort;
        int direction;
    };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerSoundTable)
};

} // namespace hise

#endif  // SAMPLEEDITORCOMPONENTS_H_INCLUDED
