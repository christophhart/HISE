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

#ifndef FLOATINGTILE_H_INCLUDED
#define FLOATINGTILE_H_INCLUDED

namespace hise { using namespace juce;

class FloatingTilePopup: public Component,
							public ButtonListener,
							public ComponentListener
{
public:

	enum class RectangleType
	{
		FullBounds,
		BoxPath,
		Title,
		CloseButton,
		ContentBounds
	};

	Rectangle<int> getRectangle(RectangleType t) const;

	enum class ColourIds
	{
		backgroundColourId = 0,
		numColourIds
	};

	FloatingTilePopup(Component* content_, Component* attachedComponent, Point<int> localPoint);

	~FloatingTilePopup();

	void buttonClicked(Button* b);
	void paint(Graphics &g);

	bool hasTitle() const;
	bool keyPressed(const KeyPress& k) override;
	void resized();
	void deleteAndClose();
	void componentMovedOrResized(Component& component, bool moved, bool resized);
	void componentBeingDeleted(Component& component) override;
	Component* getComponent() { return content; };

	Point<float> arrowPosition;
	bool arrowAtBottom;
	int arrowX = -1;

	void setArrow(bool /*showError*/) {}
	void updatePosition();
	Component* getAttachedComponent() const { return attachedComponent.getComponent(); }
	void mouseDrag(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDown(const MouseEvent& e) override;
	void rebuildBoxPath();
	void addFixComponent(Component* c);
    std::function<void(bool)> onDetach;
	bool skipToggle = false;
	Component* getTrueContent();

	template <typename ComponentType> ComponentType* getContent()
	{
		if(auto c = dynamic_cast<ComponentType*>(getTrueContent()))
			return c;

		return nullptr;
	}

private:

	bool dragging = false;
	ComponentDragger dragger;

	juce::ComponentBoundsConstrainer constrainer;

	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override;
	} factory;

	struct CloseButton : public Button
	{
		CloseButton() :
			Button("closeButton")
		{};

		void paintButton(Graphics& g, bool over, bool down) override;
		void resized() override;

		Path p;
	};
    
    PostGraphicsRenderer::DataStack stack;
    
    Path boxPath;
	Image shadowImage;
    
	Component::SafePointer<Component> attachedComponent;
	Point<int> localPointInComponent;

	ScopedPointer<Component> content;
	ScopedPointer<CloseButton> closeButton;
    
public:

	HiseShapeButton moveButton;
};


class BackendRootWindow;
class FloatingTile;

class ComponentWithBackendConnection
{
public:

	virtual ~ComponentWithBackendConnection() {};
	virtual BackendRootWindow* getBackendRootWindow() = 0;
	virtual const BackendRootWindow* getBackendRootWindow() const = 0;
	virtual FloatingTile* getRootFloatingTile() = 0;
};

#if HISE_HEADLESS
class DummyBackendComponent
{
public:
	BackendRootWindow * getBackendRootWindow() { return nullptr; }
	const BackendRootWindow* getBackendRootWindow() const { return nullptr; }
	FloatingTile* getRootFloatingTile() { return nullptr; }
};
#endif

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
            ForceShowTitle,
			MinSize,
			FocusKeyPress,
			FoldKeyPress,
			numProperties
		};

		var toDynamicObject() const override;
		void fromDynamicObject(const var& objectData) override;
		int getNumDefaultableProperties() const override;
		Identifier getDefaultablePropertyId(int i) const override;
		var getDefaultProperty(int id) const override;
		void reset();
		bool isAbsolute() const;
		bool isFolded() const;
		bool canBeFolded() const;
		int getForceTitleState() const;
		void setFoldState(int newFoldState);
		void setKeyPress(bool isFocus, const Identifier& shortcutId);

		bool swappingEnabled = false;

		KeyPress getFoldKeyPress(Component* c) const;
		KeyPress getFocusKeyPress(Component* c) const;

		double getCurrentSize() const;
		void setForceShowTitle(int shouldForceTitle);
		void setCurrentSize(double newSize);
		void setMinSize(int minSize);
		int getMinSize() const;
		var getLayoutDataObject() const;
		Identifier getID() const;
		void setId(const String& id);
		void setVisible(bool shouldBeVisible);
		bool isVisible() const;
		void setForceFoldButton(bool shouldBeShown);
		bool mustShowFoldButton() const;

	private:

		struct CachedValues
		{
			int folded = 0;
			double size = -0.5;
			int minSize = -1;
			bool visible = true;
			bool forceFoldButton = false;
            int forceShowTitle = 0;

			String id = "anonymous";
		};

		// Only for debugging...
		CachedValues cachedValues; 

		var layoutDataObject;

		KeyPress focusShortcut;

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
        
        void mouseEnter(const MouseEvent& m) override;
		void mouseExit(const MouseEvent& m) override;
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

	FloatingTile(MainController* mc, FloatingTileContainer* parent, var data=var());
	~FloatingTile() override;

	void forEachDetachedPopup(const std::function<void(FloatingTilePopup* p)>& f);

	void setContent(NamedValueSet&& data);

	void setContent(const var& data);
	void setNewContent(const Identifier& newId);
	void setNewContent(Component* newContent);

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

	const BackendRootWindow* getBackendRootWindow() const ;
	BackendRootWindow* getBackendRootWindow();

	FloatingTile* getRootFloatingTile();

	const FloatingTile* getRootFloatingTile() const;

	void clear();
	void refreshRootLayout();
	
	void setLayoutModeEnabled(bool shouldBeEnabled);
	bool isLayoutModeEnabled() const;;
	bool canDoLayoutMode() const;

    void ensureVisibility();
    
	ParentType getParentType() const;

	const FloatingTileContainer* getParentContainer() const { return parentContainer; }
	FloatingTileContainer* getParentContainer() { return parentContainer; }

	void setAllowChildComponentCreation(bool shouldCreateChildComponents);
	bool shouldCreateChildComponents() const;

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

	void setOverlayComponent(Component* newOverlayComponent, int fadeTime);

	void resized();

	/** Returns the current size in the container. */
	double getCurrentSizeInContainer();
	
	bool keyPressed(const KeyPress& k) override;

	const FloatingTileContent* getCurrentFloatingPanel() const;
	FloatingTileContent* getCurrentFloatingPanel();
	
	bool isInVerticalLayout() const;

	const LayoutData& getLayoutData() const { return layoutData; }
	LayoutData& getLayoutData() { return layoutData; }

	Path getIcon() const;

	void setCustomIcon(int newIconId)
	{
		iconId = newIconId;
		getParentContainer()->siblingAmountChanged();
	}

	void setLayoutDataObject(const var& newLayoutData)
	{
		layoutData.fromDynamicObject(newLayoutData);
	}

	String exportAsJSON() const;
	void loadFromJSON(const String& jsonData);

	void editJSON();

	FloatingTilePopup* showComponentInRootPopup(Component* newComponent, Component* attachedComponent, Point<int> localPoint, bool wrapInViewport=false, bool maximiseViewport=false);

	FloatingTilePopup* showComponentAsDetachedPopup(Component* newComponent, Component* attachedComponent, Point<int> localPoint, bool wrapInViewport = false);

	Component* wrapInViewport(Component* c, bool shouldBeMaximised);

	bool isRootPopupShown() const;

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

	FloatingTile* toggleFold();

	void setCloseTogglesVisibility(bool shouldToggleVisibility);
	void setForceShowTitle(bool shouldShowTitle);

	FloatingTileContent::Factory* getPanelFactory() { return &panelFactory; };

	const FloatingTileContent::Factory* getPanelFactory() const { return &panelFactory; };

	void addPopupListener(PopupListener* listener);
	void removePopupListener(PopupListener* listener);

	MainController* getMainController() { return mc; }

	const MainController* getMainController() const { return mc; }

	/** Pass in a lambda and it will call it on every child that matches. If you return true, it will abort the execution. */
	template <typename ContentType> bool forEach(const std::function<bool(ContentType*)>& f)
	{
		if (auto typed = dynamic_cast<ContentType*>(getCurrentFloatingPanel()))
		{
			if (f(typed))
				return true;
		}

		if (auto c = dynamic_cast<FloatingTileContainer*>(getCurrentFloatingPanel()))
		{
			for (int i = 0; i < c->getNumComponents(); i++)
			{
				if (c->getComponent(i)->forEach(f))
					return true;
			}
		}

		return false;
	}

	template <typename ContentType> ContentType* findFirstChildWithType()
	{
		ContentType* result = nullptr;
		ContentType** resultPointer = &result;

		forEach<ContentType>([resultPointer](ContentType* t)
		{
			*resultPointer = t;
			return true;
		});

		return result;
	}
    
    template <typename ContentType> ContentType* findParentTileWithType()
    {
        if(auto e = dynamic_cast<ContentType*>(getCurrentFloatingPanel()))
            return e;
        
        if (getParentType() == ParentType::Root)
            return nullptr;

        auto parent = getParentContainer()->getParentShell();
        
        return parent->findParentTileWithType<ContentType>();
    }

	void setIsFloatingTileOnInterface()
	{
		interfaceFloatingTile = true;
	}

	bool isOnInterface() const { return interfaceFloatingTile; }
	void removePopup(FloatingTilePopup* p);
	void detachCurrentPopupAsync();
	void toggleDetachPopup(FloatingTilePopup* p);

private:

	bool interfaceFloatingTile = false;

	MainController* mc;

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
	bool allowChildComponentCreation = true;
	int iconId = -1;
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
	OwnedArray<FloatingTilePopup> detachedPopups;
	ScopedPointer<FloatingTilePopup> currentPopup;
	Array<WeakReference<PopupListener>> listeners;
	FloatingTileContainer* parentContainer;
	FloatingTileContent::Factory panelFactory;
	ScopedPointer<Component> overlayComponent;
    ZoomableViewport::Laf slaf;
};

#if USE_BACKEND

class FloatingTileDocumentWindow : public DocumentWindow,
								   public ComponentWithBackendConnection,
								   public TopLevelWindowWithOptionalOpenGL,
                                   public TopLevelWindowWithKeyMappings,
							       public ModalBaseWindow
{
public:

	FloatingTileDocumentWindow(BackendRootWindow* parentRoot);

	~FloatingTileDocumentWindow();

	void resized() override
	{
		getContentComponent()->setBounds(getLocalBounds());
	}

	void closeButtonPressed() override;

	bool keyPressed(const KeyPress& key) override;

	BackendRootWindow* getBackendRootWindow() override { return parent; };

	const BackendRootWindow* getBackendRootWindow() const override { return parent; };

	FloatingTile* getRootFloatingTile() override;;

	virtual const MainController* getMainControllerToUse() const;
	virtual MainController* getMainControllerToUse();

    void initialiseAllKeyPresses() override;
    
    
    File getKeyPressSettingFile() const override
    {
        return ProjectHandler::getAppDataDirectory(nullptr).getChildFile("KeyPressMapping.xml");
    }
    
private:

	BackendRootWindow* parent;
	
};

#endif

struct FloatingTileHelpers
{
	static const Identifier getTileID(FloatingTile* parent);
	template <class ContentType> static ContentType* findTileWithId(FloatingTile* root, const Identifier& id)
	{
		FloatingTile::Iterator<ContentType> iter(root);

		while (auto t = iter.getNextPanel())
		{
			auto tid = getTileID(t->getParentShell());
			if (tid == id || id.isNull())
				return t;
		}

		return nullptr;
	}
};

} // namespace hise

#endif  // FLOATINGTILE_H_INCLUDED
