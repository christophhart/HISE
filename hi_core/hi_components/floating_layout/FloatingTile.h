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


class FloatingTilePopup: public Component,
							public ButtonListener,
							public ComponentListener
{
public:

	enum class ColourIds
	{
		backgroundColourId = 0,
		numColourIds
	};

	FloatingTilePopup(Component* content_, Component* attachedComponent, Point<int> localPoint);

	~FloatingTilePopup();

	void buttonClicked(Button* b);
	void paint(Graphics &g);
	void resized();

	void deleteAndClose();

	void componentMovedOrResized(Component& component, bool moved, bool resized);

	void componentBeingDeleted(Component& component) override;

	Component* getComponent() { return content; };

	Point<float> arrowPosition;

	bool arrowAtBottom;
	int arrowX = -1;

	void setArrow(bool /*showError*/)
	{

	}

	void updatePosition();

private:

	Component::SafePointer<Component> attachedComponent;
	Point<int> localPointInComponent;

	ScopedPointer<Component> content;
	ScopedPointer<ShapeButton> closeButton;
};


class FloatingTileContainer;
class FloatingTileContent;

class FloatingTile : public Component
{
public:

	

	

	struct LayoutData: public ObjectWithDefaultProperties
	{
		LayoutData()
		{
			reset();
		}

		enum LayoutDataIds
		{
			ID,
			Size,
			Folded,
			Visible,
			ForceFoldButton,
			MinSize,
			numProperties
		};

		var toDynamicObject() const override;
		void fromDynamicObject(const var& objectData) override;

		int getNumDefaultableProperties() const override;
		Identifier getDefaultablePropertyId(int i) const override;
		
		bool swappingEnabled = false;
		
		var getDefaultProperty(int id) const override
		{
			switch (id)
			{
			case FloatingTile::LayoutData::LayoutData::ID:		  return var("anonymous");
			case FloatingTile::LayoutData::LayoutDataIds::Size:   return var(-0.5);
			case FloatingTile::LayoutData::LayoutDataIds::Folded: return var(0);
			case FloatingTile::LayoutData::LayoutDataIds::ForceFoldButton: return var(0);
			case FloatingTile::LayoutData::LayoutDataIds::Visible: return var(true);
			case FloatingTile::LayoutData::LayoutDataIds::MinSize: return var(-1);
			default:
				break;
			}

			jassertfalse;
			return var();
		}

		void reset()
		{
			layoutDataObject = var(new DynamicObject());

			resetObject(layoutDataObject.getDynamicObject());

			cachedValues = CachedValues();

			swappingEnabled = false;
		}

		bool isAbsolute() const
		{ 
			double currentSize = getPropertyWithDefault(layoutDataObject, LayoutDataIds::Size);
			return currentSize > 0.0; 
		}

		bool isFolded() const 
		{
			int foldState = getPropertyWithDefault(layoutDataObject, LayoutDataIds::Folded);
			return foldState > 0; 
		}

		bool canBeFolded() const 
		{
			int foldState = getPropertyWithDefault(layoutDataObject, LayoutDataIds::Folded);
			return foldState >= 0; 
		}

		void setFoldState(int newFoldState)
		{
			storePropertyInObject(layoutDataObject, LayoutDataIds::Folded, newFoldState);
			cachedValues.folded = newFoldState;
		}

		double getCurrentSize() const
		{
			double currentSize = getPropertyWithDefault(layoutDataObject, LayoutDataIds::Size);

			return currentSize;
		}

		void setCurrentSize(double newSize)
		{
			storePropertyInObject(layoutDataObject, LayoutDataIds::Size, newSize);
			cachedValues.size = newSize;
		}

		void setMinSize(int minSize)
		{
			storePropertyInObject(layoutDataObject, LayoutDataIds::MinSize, minSize);
			cachedValues.minSize = minSize;
		}

		int getMinSize() const
		{
			int minSize = getPropertyWithDefault(layoutDataObject, LayoutDataIds::MinSize);
			return minSize;
		}

		var getLayoutDataObject() const
		{
			return layoutDataObject;
		}

		Identifier getID() const
		{
			String id = getPropertyWithDefault(layoutDataObject, LayoutDataIds::ID);

			if (id.isNotEmpty())
				return Identifier(id);

			static const Identifier an("anonymous");

			return an;
		}

		void setId(const String& id)
		{
			storePropertyInObject(layoutDataObject, LayoutDataIds::ID, id);
			cachedValues.id = id;
		}

		void setVisible(bool shouldBeVisible)
		{
			storePropertyInObject(layoutDataObject, LayoutDataIds::Visible, shouldBeVisible);
			cachedValues.visible = shouldBeVisible;
		}

		bool isVisible() const
		{
			return getPropertyWithDefault(layoutDataObject, LayoutDataIds::Visible);
		}

		void setForceFoldButton(bool shouldBeShown)
		{
			storePropertyInObject(layoutDataObject, LayoutDataIds::ForceFoldButton, shouldBeShown);
			cachedValues.forceFoldButton = shouldBeShown;
		}

		bool mustShowFoldButton() const
		{
			return getPropertyWithDefault(layoutDataObject, LayoutDataIds::ForceFoldButton);
		}

	private:

		struct CachedValues
		{
			int folded = 0;
			double size = -0.5;
			int minSize = -1;
			bool visible = true;
			bool forceFoldButton = false;

			String id = "anonymous";
		};

		// Only for debugging...
		CachedValues cachedValues; 

		var layoutDataObject;

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
			{
				if(p != root)
					panels.add(m);
			}

			if (auto c = dynamic_cast<FloatingTileContainer*>(p->getCurrentFloatingPanel()))
			{
				for (int i = 0; i < c->getNumComponents(); i++)
					addToList(c->getComponent(i));
			}
		}

		FloatingTile* root;
		Array<ContentType*> panels;
		int index = 0;
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
        
        void mouseEnter(const MouseEvent& m) override
        {
            auto ft = dynamic_cast<FloatingTile*>(getParentComponent());
            
            ft->deleteHover = true;
            ft->repaint();
            
            ShapeButton::mouseEnter(m);
        }
        
        void mouseExit(const MouseEvent& m) override
        {
            auto ft = dynamic_cast<FloatingTile*>(getParentComponent());
            
            ft->deleteHover = false;
            ft->repaint();
            
            ShapeButton::mouseExit(m);
        }
        
        
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

	struct ResizeButton : public ShapeButton, public ButtonListener
	{
		ResizeButton();
		void buttonClicked(Button* b);
	};

	struct PopupListener
	{
		virtual ~PopupListener()
		{
			masterReference.clear();
		}

		virtual void popupChanged(Component* newComponent) = 0;

	private:

		friend class WeakReference<PopupListener>;

		WeakReference<PopupListener>::Master masterReference;
	};

	FloatingTile(FloatingTileContainer* parent, var data=var());

	

	~FloatingTile()
	{
		currentPopup = nullptr;

		content = nullptr;
		foldButton = nullptr;
		moveButton = nullptr;
		resizeButton = nullptr;
		closeButton = nullptr;
	}

	void setContent(const var& data);
	void setNewContent(const Identifier& newId);

	bool isEmpty() const;
	bool showTitle() const;

	Rectangle<int> getContentBounds();

	bool isFolded() const;
	void setFolded(bool shouldBeFolded);
	void refreshFoldButton();
	void setCanBeFolded(bool shouldBeFoldable);
	bool canBeFolded() const;

	void refreshPinButton();
	void toggleAbsoluteSize();

	const BackendRootWindow* getRootWindow() const;
	BackendRootWindow* getRootWindow();

	FloatingTile* getRootComponent();

	const FloatingTile* getRootComponent() const;

	void clear();
	void refreshRootLayout();
	
	void setLayoutModeEnabled(bool shouldBeEnabled);
	bool isLayoutModeEnabled() const;;
	bool canDoLayoutMode() const;

	ParentType getParentType() const;

	const FloatingTileContainer* getParentContainer() const { return parentContainer; }
	FloatingTileContainer* getParentContainer() { return parentContainer; }

	bool hasChildren() const;

	void enableSwapMode(bool shouldBeEnabled, FloatingTile* source);
	void swapWith(FloatingTile* otherComponent);

	void swapContainerType(const Identifier& containerId);

	void bringButtonsToFront();

	void paint(Graphics& g) override;
	void paintOverChildren(Graphics& g) override;

	void refreshMouseClickTarget();

	void mouseEnter(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;

	void resized();

	/** Returns the current size in the container. */
	double getCurrentSizeInContainer();
	
	const FloatingTileContent* getCurrentFloatingPanel() const;
	FloatingTileContent* getCurrentFloatingPanel();
	
	bool isInVerticalLayout() const;

	const LayoutData& getLayoutData() const { return layoutData; }
	LayoutData& getLayoutData() { return layoutData; }

	Path getIcon() const;

	void setLayoutDataObject(const var& newLayoutData)
	{
		layoutData.fromDynamicObject(newLayoutData);
	}

	String exportAsJSON() const;
	void loadFromJSON(const String& jsonData);

	void editJSON();

	FloatingTilePopup* showComponentInRootPopup(Component* newComponent, Component* attachedComponent, Point<int> localPoint);

	struct LayoutHelpers
	{
	public:

		static void setContentBounds(FloatingTile* t);
		static bool showCloseButton(const FloatingTile* t);
		static bool showMoveButton(const FloatingTile* t);
		static bool showPinButton(const FloatingTile* t);
		static bool showFoldButton(const FloatingTile* t);
	};

	void setVital(bool shouldBeVital) { vital = shouldBeVital; }
	bool isVital() const { return vital; }
	bool canBeDeleted() const;
	bool isSwappable() const;

	void setCloseTogglesVisibility(bool shouldToggleVisibility)
	{
		closeTogglesVisibility = shouldToggleVisibility;
	}

	FloatingTileContent::Factory* getPanelFactory() { return &panelFactory; };

	const FloatingTileContent::Factory* getPanelFactory() const { return &panelFactory; };

	void addPopupListener(PopupListener* listener)
	{
		jassert(getParentType() == ParentType::Root);

		listeners.addIfNotAlreadyThere(listener);
	}

	void removePopupListener(PopupListener* listener)
	{
		jassert(getParentType() == ParentType::Root);

		listeners.removeAllInstancesOf(listener);
	}

private:

	

	class TilePopupLookAndFeel : public PopupLookAndFeel
	{
		void getIdealPopupMenuItemSize(const String &text, bool isSeparator, int standardMenuItemHeight, int &idealWidth, int &idealHeight) override;

		void drawPopupMenuSectionHeader(Graphics& g, const Rectangle<int>& area, const String& sectionName);

	};

	void refreshFixedSizeForNewContent();

	bool vital = false;

	bool closeTogglesVisibility = false;

    bool deleteHover = false;
    
	bool layoutModeEnabled = false;

	int leftOffsetForTitleText = 0;
	int rightOffsetForTitleText = 0;

	LayoutData layoutData;

	CodeDocument currentJSON;

	TilePopupLookAndFeel plaf;

	Component::SafePointer<FloatingTile> currentSwapSource;

	ScopedPointer<ShapeButton> closeButton;
	ScopedPointer<MoveButton> moveButton;
	ScopedPointer<FoldButton> foldButton;
	ScopedPointer<ResizeButton> resizeButton;

	ScopedPointer<Component> content;

	ScopedPointer<FloatingTilePopup> currentPopup;


	Array<WeakReference<PopupListener>> listeners;

	FloatingTileContainer* parentContainer;
	FloatingTileContent::Factory panelFactory;
};


struct FloatingTileHelpers
{
	template <class ContentType> static ContentType* findTileWithId(FloatingTile* root, const Identifier& id)
	{
		FloatingTile::Iterator<ContentType> iter(root);

		while (auto t = iter.getNextPanel())
		{
			if (t->getParentShell()->getLayoutData().getID() == id)
				return t;
		}

		return nullptr;
	}

};

#endif  // FLOATINGTILE_H_INCLUDED
