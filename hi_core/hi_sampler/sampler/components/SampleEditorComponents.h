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

#ifndef SAMPLEEDITORCOMPONENTS_H_INCLUDED
#define SAMPLEEDITORCOMPONENTS_H_INCLUDED

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

    SamplerSubEditor(SampleEditHandler* handler_): 
		internalChange(false),
		handler(handler_)
	{};
    
    virtual ~SamplerSubEditor() {};

	/** Call this whenever the selection changes and you want to update the other editors. */
    void selectSounds(const Array<ModulatorSamplerSound*> &selection);

	/** Overwrite this and call the method that updates the interface. */
	virtual void updateInterface() = 0;

protected:

	/** This is called whenever one of the other subeditors (or itself) called selectSounds(). 
	*
	*	Make sure you update the interface to select the new sounds here. Calls to selectSounds are legal, as they do not trigger the callback again.
	*/
    virtual void soundsSelected(const Array<ModulatorSamplerSound*> &selection) = 0;

	SampleEditHandler* handler;

private:

	

    bool internalChange;
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
	{};

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
class SampleComponent: public Timer,
					   public SafeChangeListener
{
public:

	SampleComponent(ModulatorSamplerSound *s, SamplerSoundMap *parentMap);

	~SampleComponent() 
	{ 
		if(sound != nullptr) sound->removeChangeListener(this); 
		
		masterReference.clear();
	};

	void timerCallback();

	void changeListenerCallback(SafeChangeBroadcaster *b);

	void triggerNoteOnAnimation(int velocity)
	{
		transparency = 0.3f + 0.7f * sound->getGainValueForVelocityXFade(velocity);
		startTimer(30);
	}

	Colour getColourForSound(bool wantsOutlineColour) const
	{
		if(sound.get() == nullptr) return Colours::transparentBlack;
        
		

		if (selected) return wantsOutlineColour ? Colour(SIGNAL_COLOUR) : Colour(SIGNAL_COLOUR).withBrightness(transparency).withAlpha(0.6f);

		if (sound->isMissing())
		{
			if (sound->isPurged()) return Colours::violet.withAlpha(0.3f);
			else return Colours::violet.withAlpha(0.3f);
		}
		else
		{
			if (sound->isPurged()) return Colours::brown.withAlpha(0.3f);
			else return wantsOutlineColour ? Colours::white.withAlpha(0.7f) : Colours::white.withAlpha(transparency);
		}
	}

	bool samplePathContains(Point<int> localPoint)
	{
		return bounds.contains(localPoint);
	}

    void drawSampleRectangle(Graphics &g, Rectangle<int> area);

	const ModulatorSamplerSound *getSound() const noexcept { return sound; };

	ModulatorSamplerSound *getSound() noexcept { return sound.get(); };

	void setSelected(bool shouldBeSelected)
	{
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

	void setVisible(bool shouldBeVisible) { visible = shouldBeVisible; }
	void setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; };

	bool isVisible() const { return visible; }
	bool isEnabled() const noexcept{ return enabled; };

	Rectangle<int> getBoundsInParent() { return bounds; }

	bool appliesToGroup(int groupIndexToCompare)
	{ 
		if (sound.get() != nullptr) return sound->appliesToRRGroup(groupIndexToCompare);
		else return false;
	}

private:

	friend class WeakReference < SampleComponent > ;

	WeakReference<SampleComponent>::Master masterReference;

	Rectangle<int> bounds;

	bool selected;
	bool enabled;
	bool visible;

	float transparency;

	int numOverlayerSiblings;

	Path outline;

	SamplerSoundMap *map;
	WeakReference<ModulatorSamplerSound> sound;
};

/** A component which displays all loaded ModulatorSamplerSounds and allows editing of their properties. 
*	@ingroup components
*/
class SamplerSoundMap: public Component,
					   public ChangeListener,
					   public LassoSource<WeakReference<SampleComponent>>,
					   public SettableTooltipClient,
					   public Timer
{
public:
	
	/** Simple direction enum to find the next neighbour. @see selectNeighbourSample() */
	enum Neighbour
	{
		Left, ///<
		Right, ///<
		Up, ///<
		Down ///<
	};

	enum DragLimiters
	{
		NoLimit = 0,
		VelocityOnly,
		KeyOnly,
		numDragLimiters
	};

	SamplerSoundMap(ModulatorSampler *ownerSampler_);

	~SamplerSoundMap()
	{
		sampleComponents.clear();
	};

	void timerCallback() override
	{
		if (getWidth() > 0 && getHeight() > 0)
		{
			currentSnapshot = Image(Image::RGB, getWidth(), getHeight(), true);

			Graphics g2(currentSnapshot);

			drawSoundMap(g2);

			repaint();
		}

        stopTimer();
	}

	void modifierKeysChanged(const ModifierKeys &modifiers) override;

	bool keyPressed(const KeyPress &k) override;

	/** searches all sounds and selects the next neighbour. */
	void selectNeighbourSample(Neighbour n);

	void findLassoItemsInArea (Array<WeakReference<SampleComponent>> &itemsFound, const Rectangle< int > &area) override;

	void refreshSelectedSoundsFromLasso();

	SelectedItemSet<WeakReference<SampleComponent>> &getLassoSelection() override { return *selectedSounds; };

	void changeListenerCallback(ChangeBroadcaster *b) override;

	void paint(Graphics &g) override;
	void paintOverChildren(Graphics &g) override;
    
	/** returns the root notes for all files that are dragged over the component. */
	BigInteger getRootNotesForDraggedFiles() const { return draggedFileRootNotes; };

	void resized() override { updateSampleComponents(); };

	/** updates the position / size of the specified sound. */
	void updateSampleComponentWithSound(ModulatorSamplerSound *sound);

	/** updates the position / size of the sound with the supplied index. */
	void updateSampleComponent(int index);

	/** updates the position / size of all sounds. */
	void updateSampleComponents();

	void mouseDown(const MouseEvent &e) override;
	void mouseUp(const MouseEvent &e) override;
	void mouseExit(const MouseEvent &) override;
	void mouseDrag(const MouseEvent &e) override;
	void mouseMove(const MouseEvent &e) override;

	/** draws a red dot on the map if a key is pressed.
	*
	*	@param pressedKeyData the array with the velocities (-1 if the key is not pressed). @see ModulatorSampler::SamplerDisplayValues
	*/
	void setPressedKeys(const uint8 *pressedKeyData);

	/** change the selection to the supplied list of sounds. */
	void setSelectedIds(const Array<ModulatorSamplerSound*> newSelectionList);

	/** checks if the sound with the id is selected. */
	bool isSelected(int id) { return selectedIds.contains(id); };

	/** This hides all sounds that to not belong to the specified group index. If you want to display all sounds, pass -1. */
	void soloGroup(int groupIndex);

	/** updates all sounds. It deletes all SampleComponents and recreates them if new samples are detected. */
	void updateSoundData();

	void drawSampleComponentsForDragPosition(int numDraggedFiles, int x, int y);

	void clearDragPosition()
	{
		draggedFileRootNotes.clear();
		repaint();
	};

    void refreshGraphics()
    {
        if(!isTimerRunning()) startTimer(50);
    }
    
    void drawSoundMap(Graphics &g);
    
	const ModulatorSampler* getSampler() const { return ownerSampler; }

private:

	/** A POD object containing data for a dragged sound. */
	struct DragData
	{
		ModulatorSamplerSound *sound;
		int root;
		int lowKey;
		int hiKey;
		int loVel;
		int hiVel;
	};

	/** checks if the sampler contains new samples that are not displayed yet. */
	bool newSamplesDetected();

	SampleComponent* getSampleComponentAt(Point<int> point);

	void checkEventForSampleDragging(const MouseEvent &e);

	void endSampleDragging(bool copyDraggedSounds);
	
	ModulatorSampler *ownerSampler;
	SampleEditHandler* handler;

	Rectangle<int> dragArea;
	Array<DragData> dragStartData;
	bool sampleDraggingEnabled;
	BigInteger draggedFileRootNotes;
	int currentDragDeltaX;
	int currentDragDeltaY;
	int semiTonesPerNote;

	uint8 pressedKeys[128];
	int notePosition;
	int veloPosition;
	
	DragLimiters currentDragLimiter;

	Array<int> selectedIds;
	OwnedArray<SampleComponent> sampleComponents;

	Array<WeakReference<SampleComponent>> lassoSelectedComponents;

	ScopedPointer<SelectedItemSet<WeakReference<SampleComponent>>> selectedSounds;
	ScopedPointer<LassoComponent<WeakReference<SampleComponent>>> sampleLasso;

	Rectangle<int> currentLassoRectangle;

	uint32 milliSecondsSinceLastLassoCheck;
    
    Image currentSnapshot;
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
							 public SamplerSubEditor
{
public:
	SamplerSoundTable(ModulatorSampler *ownerSampler_, SampleEditHandler* handler);

	void refreshList();

    int getNumRows() override;

	bool broadcasterIsSelection(ChangeBroadcaster *b) const;

	
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;

	void selectedRowsChanged(int lastRowSelected) override;

    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override;

    void sortOrderChanged (int newSortColumnId, bool isForwards) override;

	void soundsSelected(const Array<ModulatorSamplerSound *> &selectedSounds) override;
    
    void resized() override;

	void updateInterface() override
	{
		refreshList();
	}

private:

	ModulatorSampler *ownerSampler;
	
	bool internalSelection;

    TableListBox table;     
    Font font;

	Array<ModulatorSamplerSound*> sortedSoundList;

    int numRows;

	TableHeaderLookAndFeel laf;

    //==============================================================================
    // A comparator used to sort our data when the user clicks a column header
    class DemoDataSorter
    {
    public:
		DemoDataSorter (ModulatorSamplerSound::Property propertyToSort_, bool forwards)
            : propertyToSort (propertyToSort_),
              direction (forwards ? 1 : -1)
        {}

		int compareElements (const ModulatorSamplerSound* first, const ModulatorSamplerSound* second) const
        {
			int result = first->getProperty (propertyToSort).toString()
                           .compareNatural (second->getProperty (propertyToSort).toString());

            return direction * result;
        }

    private:
		ModulatorSamplerSound::Property propertyToSort;
        int direction;
    };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerSoundTable)
};

#endif  // SAMPLEEDITORCOMPONENTS_H_INCLUDED
