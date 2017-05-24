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

#ifndef FLOATINGTILECONTAINER_H_INCLUDED
#define FLOATINGTILECONTAINER_H_INCLUDED


/** A floating tile container is the base class for components that can host other floating tiles in various arrangements. */
class FloatingTileContainer : public FloatingTileContent
{
public:

	/** Creates a new floating tile container.
	*
	*	You have to supply the parent tile.
	*/
	FloatingTileContainer(FloatingTile* parent) :
		FloatingTileContent(parent)
	{};

	virtual ~FloatingTileContainer() {};

	/** Returns the FloatingTile at the given index. */
	const FloatingTile* getComponent(int index) const;

	/** Returns the FloatingTile at the given index. */
	FloatingTile* getComponent(int index);

	/** Returns the number of floating tiles in this container. */
	int getNumComponents() const;

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
	

	void enableSwapMode(bool shouldBeSwappable, FloatingTile* source);

	/** This will be called whenever the layout needs to be updated (eg. when a new floating tile is added or the resizers were dragged. */
	virtual void refreshLayout() {};

	void setAllowInserting(bool shouldBeAllowed);

	bool isInsertingEnabled() const { return allowInserting; }

	bool showTitleInPresentationMode() const override { return false; }

protected:

	virtual void componentAdded(FloatingTile* newComponent) = 0;
	virtual void componentRemoved(FloatingTile* deletedComponent) = 0;

private:

	Component::SafePointer<FloatingTile> resizeSource;

	OwnedArray<FloatingTile> components;

	bool allowInserting = true;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingTileContainer)
};

/** A tab component that sits within a floating tile. */
class FloatingTabComponent : public FloatingTileContainer,
	public TabbedComponent,
	public ButtonListener
{
public:

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

	Rectangle<int> getContainerBounds() const override
	{
		auto localBounds = dynamic_cast<const Component*>(this)->getLocalBounds();

		return localBounds.withTrimmedTop(getTabBarDepth());
	}

	void componentAdded(FloatingTile* newComponent) override;
	void componentRemoved(FloatingTile* deletedComponent) override;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	void paint(Graphics& g) override;

	void resized();
	void buttonClicked(Button* b) override;

private:

	ScopedPointer<ShapeButton> addButton;

	LookAndFeel laf;
};

class ResizableFloatingTileContainer : public FloatingTileContainer,
								   public Component,
								   public ButtonListener
{
public:

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

	bool showTitleInPresentationMode() const override
	{
		return getCustomTitle().isNotEmpty();
	}

	Rectangle<int> getContainerBounds() const override
	{
		auto localBounds = dynamic_cast<const Component*>(this)->getLocalBounds();

		const bool isInTabs = dynamic_cast<const FloatingTabComponent*>(getParentShell()->getParentContainer());

		return getParentShell()->isLayoutModeEnabled() && isInsertingEnabled() || (!isInTabs && hasCustomTitle()) ? localBounds.withTrimmedTop(16) : localBounds;
	}

	virtual bool isVertical() const { return vertical; }

	/** Call this if you want to refresh the layout. 
	*
	*	It will recreate the resizer bars and move the panels to their position.
	**/
	void refreshLayout() override;

	int getMinimumOffset() const;
	int getMaximumOffset() const;

	/** Returns the size of the given area for the used dimension. */
	int getDimensionSize(Rectangle<int> area) const;

	/** Returns the offset of the given area for the used dimension. */
	int getDimensionOffset(Rectangle<int> area) const;

	/** Returns the end of the given area for the used dimension. */
	int getDimensionEnd(Rectangle<int> area) const;

	void buttonClicked(Button* b) override;

	void resized() override;

	void foldComponent(Component* c, bool shouldBeFolded);

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

protected:

	void componentAdded(FloatingTile* c) override;
	void componentRemoved(FloatingTile* c) override;

private:

	void performLayout(Rectangle<int> area);

	void rebuildResizers();

	void setBoundsOneDimension(Component* c, int offset, int size, Rectangle<int> area);

	BigInteger bigResizers;

	const bool vertical;
	bool animate = true;

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

#endif  // FLOATINGTILECONTAINER_H_INCLUDED
