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

#ifndef FLOATINGTILE_H_INCLUDED
#define FLOATINGTILE_H_INCLUDED



class FloatingTileContainer;
class FloatingTileContent;

class FloatingTile : public Component
{
public:

	struct LayoutData 
	{
		void updateValueTree(ValueTree & v) const;
		void restoreFromValueTree(const ValueTree &v);

		bool isLocked = false;
		bool isFolded = false;
		bool isAbsolute = false;
		double currentSize = -0.5;
		bool swappingEnabled = false;
		bool layoutModeEnabled = true;
		bool layoutModePossible = true;
		bool swappable = true;
		bool deletable = true;
		bool readOnly = false;
		bool foldable = true;

		void reset()
		{
			isLocked = false;
			isFolded = false;
			isAbsolute = false;
			currentSize = -0.5;
			swappingEnabled = false;
			layoutModeEnabled = true;
			layoutModePossible = true;
			swappable = true;
			deletable = true;
			readOnly = false;
			foldable = true;
		}
	};

	template <typename ContentType> class Iterator
	{
	public:


		Iterator(FloatingTile* root)
		{
			addToList(root);
		}

		ContentType* getNextPanel()
		{
			if (index < panels.size())
				return panels[index++];

			return nullptr;
		}

	private:


		void addToList(FloatingTile* p)
		{
			if (auto m = dynamic_cast<ContentType*>(p->getCurrentFloatingPanel()))
				panels.add(m);

			if (auto c = dynamic_cast<FloatingTileContainer*>(p->getCurrentFloatingPanel()))
			{
				for (int i = 0; i < c->getNumComponents(); i++)
					addToList(c->getComponent(i));
			}
		}

		FloatingTile* root;
		Array<ContentType*> panels;
		int index = 1;
	};

	enum class ParentType
	{
		Root,
		Horizontal,
		Vertical,
		Tabbed,
		numParentTypes
	};

	struct CloseButton : public ShapeButton, public ButtonListener
	{
		CloseButton();
		void buttonClicked(Button* b);
	};

	struct MoveButton : public ShapeButton, public ButtonListener
	{
		MoveButton();
		void buttonClicked(Button* b);
	};

	struct FoldButton : public ShapeButton, public ButtonListener
	{
		FoldButton();
		void buttonClicked(Button* b);
	};

	struct LockButton : public ShapeButton, public ButtonListener
	{
		LockButton();
		void buttonClicked(Button* b);
	};

	struct ResizeButton : public ShapeButton, public ButtonListener
	{
		ResizeButton();
		void buttonClicked(Button* b);
	};

	FloatingTile(FloatingTileContainer* parent, ValueTree state = ValueTree());;

	bool isEmpty() const;

	void setFolded(bool shouldBeFolded);

	void setCanBeFolded(bool shouldBeFoldable);

	bool canBeFolded() const;

	void setUseAbsoluteSize(bool shouldUseAbsoluteSize);
	void toggleAbsoluteSize();

	void setLockedSize(bool shouldBeLocked);

	const BackendRootWindow* getRootWindow() const;
	BackendRootWindow* getRootWindow();

	FloatingTile* getRootComponent();

	void clear();
	void refreshRootLayout();
	void setLayoutModeEnabled(bool shouldBeEnabled, bool setChildrenToSameSetting=true);

	bool isLayoutModeEnabled() const { return layoutData.layoutModeEnabled; };

	void setCanDoLayoutMode(bool shouldBeAllowed);

	bool canDoLayoutMode() const;

	void toggleLayoutModeForParentContainer();

	ParentType getParentType() const;

	const FloatingTileContainer* getParentContainer() const { return parentContainer; }
	FloatingTileContainer* getParentContainer() { return parentContainer; }

	bool hasChildren() const;

	void enableSwapMode(bool shouldBeEnabled, FloatingTile* source);
	void swapWith(FloatingTile* otherComponent);

	void bringButtonsToFront();

	void paint(Graphics& g) override;
	void paintOverChildren(Graphics& g) override;

	void mouseEnter(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;

	void resized();

	/** Returns the current size in the container. */
	double getCurrentSizeInContainer();
	
	const FloatingTileContent* getCurrentFloatingPanel() const;
	FloatingTileContent* getCurrentFloatingPanel();

	
	bool isFolded() const;
	bool isInVerticalLayout() const;

	const LayoutData& getLayoutData() const { return layoutData; }
	LayoutData& getLayoutData() { return layoutData; }

	struct LayoutHelpers
	{
	public:

		static void setContentBounds(FloatingTile* t);
		static bool showCloseButton(const FloatingTile* t);
		static bool showMoveButton(const FloatingTile* t);
		static bool showFoldButton(const FloatingTile* t);
		static bool showLockButton(const FloatingTile* t);
	};

	void setContent(ValueTree& data);

	void setNewContent(const Identifier& newId);

	void setDeletable(bool shouldBeDeletable);
	bool canBeDeleted() const;

	void setSwappable(bool shouldBeSwappable);

	void setReadOnly(bool shouldBeReadOnly);
	bool isReadOnly() const noexcept;

	FloatingTileContent::Factory* getPanelFactory() { return &panelFactory; };

	void setSelector(const FloatingTileContent* originPanel, Point<int> mousePosition);

	class Selector : public Component
	{
	public:

		Selector(Rectangle<int>& target_, Point<int> pos):
			target(target_),
			mousePosition(pos)
		{
			addAndMakeVisible(content = new Content());
		}

		~Selector()
		{

		}

		void mouseDown(const MouseEvent& event)
		{
			findParentComponentOfClass<FloatingTile>()->setSelector(nullptr, {});
		}

		void paint(Graphics& g)
		{
			g.fillAll(Colours::black.withAlpha(0.2f));

			g.setColour(Colours::white.withAlpha(0.1f));
			g.fillRect(target);
		}

		void resized()
		{
			int x = jmax<int>(mousePosition.getX() - 300, 10);
			int y = jmax<int>(mousePosition.getY() - 300, 10);

			x = jmin<int>(x, getWidth() - 610);
			y = jmin<int>(y, getHeight() - 510);

			content->setBounds(x, y, 600, 500);
		}

	private:

		class Content : public Component
		{
		public:

			void paint(Graphics& g)
			{
				g.fillAll(Colours::black.withAlpha(0.3f));
			}

			void mouseDown(const MouseEvent&)
			{
				findParentComponentOfClass<FloatingTile>()->setSelector(nullptr, {});
			}

		private:

		};
		
		Rectangle<int> target;

		ScopedPointer<Content> content;

		Point<int> mousePosition;

	};

private:

	ScopedPointer<Selector> currentSelector;

	int leftOffset = 0;
	int rightOffset = 0;

	LayoutData layoutData;

	PopupLookAndFeel plaf;

	Component::SafePointer<FloatingTile> currentSwapSource;

	ScopedPointer<ShapeButton> closeButton;
	ScopedPointer<MoveButton> moveButton;
	ScopedPointer<FoldButton> foldButton;
	ScopedPointer<LockButton> lockButton;
	ScopedPointer<ResizeButton> resizeButton;

	ScopedPointer<Component> content;

	FloatingTileContainer* parentContainer;

	FloatingTileContent::Factory panelFactory;
};

#endif  // FLOATINGTILE_H_INCLUDED
