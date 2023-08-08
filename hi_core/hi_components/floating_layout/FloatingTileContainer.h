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

#ifndef FLOATINGTILECONTAINER_H_INCLUDED
#define FLOATINGTILECONTAINER_H_INCLUDED

namespace hise { using namespace juce;

class FloatingTile;

/** A floating tile container is the base class for components that can host other floating tiles in various arrangements. */
class FloatingTileContainer : public FloatingTileContent
{
public:

	enum ContainerPropertyIds
	{
		Dynamic = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		Content,
		numContainerPropertyIds
	};

	/** Creates a new floating tile container.
	*
	*	You have to supply the parent tile.
	*/
	FloatingTileContainer(FloatingTile* parent) :
		FloatingTileContent(parent)
	{};

	virtual ~FloatingTileContainer()
	{
		jassert(getNumComponents() == 0);
	};

	/** Returns the FloatingTile at the given index. */
	const FloatingTile* getComponent(int index) const;

	/** Returns the FloatingTile at the given index. */
	FloatingTile* getComponent(int index);

	/** Returns the number of floating tiles in this container. */
	int getNumComponents() const;

    int getNumVisibleComponents() const;
    
    int getNumVisibleAndResizableComponents() const;
    
	/** Deletes all floating tiles in this container. */
	void clear();
	
	/** Returns the index of the given component or -1 if its not found. */
	int getIndexOfComponent(const FloatingTile* componentToLookFor) const;

	/** Adds a floating tile to the container. 
	*
	*	This takes ownership of this floating tile and calls componentAdded() to give subclasses the possibility of 
	*	implement their arrangement.
	*
	*	It also causes the root tile to refresh its layout so that the container can be resized accordingly.
	*/
	void addFloatingTile(FloatingTile* newComponent);

	/** Removes and deletes the given FloatingTile.
	*
	*	
	*/
	void removeFloatingTile(FloatingTile* componentToRemove);

	bool shouldIntendAddButton() const;

	/** This returns the area that can be populated with content.
	*
	*	Usually this will be the full size minus the title bar if visible.
	*/

	virtual Rectangle<int> getContainerBounds() const = 0;
	
	var toDynamicObject() const override;
	void fromDynamicObject(const var& objectData) override;

	int getNumDefaultableProperties() const override;
	Identifier getDefaultablePropertyId(int i) const override;
	var getDefaultProperty(int id) const override;

	void enableSwapMode(bool shouldBeSwappable, FloatingTile* source);

	/** This will be called whenever the layout needs to be updated eg. when a new floating tile is added or the resizers were dragged. */
	virtual void refreshLayout();

	bool showTitleInPresentationMode() const override { return false; }

	void setIsDynamic(bool shouldBeDynamic)
	{
		dynamic = shouldBeDynamic;
	};

	bool isDynamic() const 
	{ 
		return dynamic; 
	}

	void notifySiblingChange();

	void moveContent(int oldIndex, int newIndex);

protected:

	virtual void componentAdded(FloatingTile* newComponent) = 0;
	
	virtual void componentRemoved(FloatingTile* deletedComponent) = 0;
	

private:

	bool dynamic = true;

	Component::SafePointer<FloatingTile> resizeSource;

	OwnedArray<FloatingTile> components;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingTileContainer)
};

/** A tab component that sits within a floating tile. */
class FloatingTabComponent : public FloatingTileContainer,
	public TabbedComponent
{
public:

	enum TabPropertyIds
	{
		CurrentTab = FloatingTileContainer::ContainerPropertyIds::numContainerPropertyIds,
		CycleKeyPress,
		numTabPropertyIds
	};

	SET_PANEL_NAME("Tabs");

	struct CloseButton : public ShapeButton, public ButtonListener
	{
		CloseButton();
		void buttonClicked(Button* b);
	};

	struct LookAndFeel : public LookAndFeel_V3
	{
		int getTabButtonBestWidth(TabBarButton &b, int tabDepth);
		void 	drawTabButton(TabBarButton &b, Graphics &g, bool isMouseOver, bool isMouseDown);
		virtual Rectangle< int > 	getTabButtonExtraComponentBounds(const TabBarButton &b, Rectangle< int > &textArea, Component &extraComp);
		virtual void 	drawTabAreaBehindFrontButton(TabbedButtonBar &b, Graphics &g, int w, int h);
	};

	FloatingTabComponent(FloatingTile* parent);

	~FloatingTabComponent();

	String getTitle() const override { return ""; };

	Rectangle<int> getContainerBounds() const override;

	void popupMenuClickOnTab(int tabIndex, const String& tabName) override;

	int getNumChildPanelsWithType(const Identifier& panelId) const;

	template <typename ContentType> int getNumChildPanelsWithType() const
	{
		static_assert(std::is_base_of<FloatingTileContent, ContentType>(), "ContentType must be derived from FloatingTileContent");
		return getNumChildPanelsWithType(ContentType::getPanelId());
	}

	void refreshLayout() override;

	void componentAdded(FloatingTile* newComponent) override;
	void componentRemoved(FloatingTile* deletedComponent) override;

	void mouseDown(const MouseEvent& event) override;

	var toDynamicObject() const override;
	void fromDynamicObject(const var& objectData) override;

    void setDisplayedFloatingTile(FloatingTile* t)
    {
        for(int i = 0; i < getNumComponents(); i++)
        {
            if(getComponent(i) == t)
            {
                setCurrentTabIndex(i);
                return;
            }
        }
    }
    
	int getNumDefaultableProperties() const override;
	Identifier getDefaultablePropertyId(int i) const override;
	var getDefaultProperty(int id) const override;

	void paint(Graphics& g) override;

	void resized();

	void addButtonClicked();

	void setAddButtonCallback(const std::function<void()>& f);

	void currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName) override;

    Identifier getCycleKeyPress() const { return cycleKeyId; }
    
    void setCycleKeyPress(const Identifier& k)
    {
        cycleKeyId = k;
    }
    
private:

    Identifier cycleKeyId;
    
	ScopedPointer<ShapeButton> addButton;

	PopupLookAndFeel plaf;

	LookAndFeel laf;
};

class ResizableFloatingTileContainer : public FloatingTileContainer,
								   public Component,
								   public ButtonListener
{
public:

	enum ColourIds
	{
		backgroundColourId,
		resizerColourId,
		numColourIds
	};

	class InternalResizer : public Component
	{
	public:

		InternalResizer(ResizableFloatingTileContainer* parent_, int index);

		/** If there are no movable floating tiles on one side of the resizer, this returns false. */
		bool hasSomethingToDo() const;

		int getCurrentSize() const;

		void paint(Graphics& g) override;

		void mouseDrag(const MouseEvent& event) override;
		void mouseUp(const MouseEvent& event) override;
		void mouseDown(const MouseEvent& e) override;

        bool isDragEnabled() const;
        
	private:

		int downOffset = 0;

		Array<Component::SafePointer<FloatingTile>> prevPanels;
		Array<Component::SafePointer<FloatingTile>> nextPanels;

		double totalPrevDownSize = 0.0;
		double totalNextDownSize = 0.0;

		Array<double> prevDownSizes;
		Array<double> nextDownSizes;

		Path resizeIcon;

		bool active = false;

		ResizableFloatingTileContainer* parent;

		int index;
	};

	ResizableFloatingTileContainer(FloatingTile* parent, bool isVerticalTile);

	virtual ~ResizableFloatingTileContainer();

	String getTitle() const override;

	Rectangle<int> getContainerBounds() const override;
	
	virtual bool isVertical() const { return vertical; }

	/** Call this if you want to refresh the layout. 
	*
	*	It will recreate the resizer bars and move the panels to their position.
	**/
	void refreshLayout() override;

	void paint(Graphics& g) override;

	int getMinimumOffset() const;
	int getMaximumOffset() const;

	/** Returns the size of the given area for the used dimension. */
	int getDimensionSize(Rectangle<int> area) const;

	/** Returns the offset of the given area for the used dimension. */
	int getDimensionOffset(Rectangle<int> area) const;

	/** Returns the end of the given area for the used dimension. */
	int getDimensionEnd(Rectangle<int> area) const;

	void buttonClicked(Button* b) override;

	void enableAnimationForNextLayout()
	{
#if HISE_FLOATING_TILE_ALLOW_RESIZE_ANIMATION
		animate = true;
#endif
	}

	void resized() override;

	bool isTitleBarDisplayed() const;

	void mouseDown(const MouseEvent& event) override;

protected:

	void componentAdded(FloatingTile* c) override;
	void componentRemoved(FloatingTile* c) override;

private:

	void performLayout(Rectangle<int> area);

	void rebuildResizers();

	void setBoundsOneDimension(Component* c, int offset, int size, Rectangle<int> area);

	BigInteger bigResizers;

	const bool vertical;
	bool animate = false;

	Array<double> storedSizes;
	Array<Component*> currentlyDisplayedComponents;

	OwnedArray<InternalResizer> resizers;
	ScopedPointer<ShapeButton> addButton;
};

class HorizontalTile : public ResizableFloatingTileContainer
{
public:

	SET_PANEL_NAME("HorizontalTile");

	HorizontalTile(FloatingTile* parent) : ResizableFloatingTileContainer(parent, true) {}
};

class VerticalTile : public ResizableFloatingTileContainer
{
public:

	SET_PANEL_NAME("VerticalTile");

	VerticalTile(FloatingTile* parent): ResizableFloatingTileContainer(parent, false) {}
};

} // namespace hise

#endif  // FLOATINGTILECONTAINER_H_INCLUDED
