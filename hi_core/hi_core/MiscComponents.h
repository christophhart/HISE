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

#ifndef MISCCOMPONENTS_H_INCLUDED
#define MISCCOMPONENTS_H_INCLUDED

namespace hise { using namespace juce;

class MouseCallbackComponent : public Component,
							   public MacroControlledObject,
							   public TouchAndHoldComponent,
						       public FileDragAndDropTarget
{
	// ================================================================================================================

public:

	enum EnterState
	{
		Nothing = 0,
		Entered,
		Exited,
		numEnterStates
	};

	enum class Action
	{
		Moved,
		Dragged,
		Clicked,
        DoubleClicked,
		MouseUp,
		Entered,
		FileMove,
		FileEnter,
		FileExit,
		FileDrop,
		Nothing
	};

	enum class CallbackLevel
	{
		NoCallbacks = 0,
		PopupMenuOnly,
		ClicksOnly,
		ClicksAndEnter,
		Drag,
		AllCallbacks
	};

	enum class FileCallbackLevel
	{
		NoCallbacks = 0,
		DropOnly,
		DropHover,
		AllCallbacks
	};

	// ================================================================================================================

	class Listener
	{
	public:

		Listener();
		virtual ~Listener();
		virtual void mouseCallback(const var &mouseInformation) = 0;

		virtual void fileDropCallback(const var& dropInformation) = 0;

	private:

		friend class WeakReference < Listener > ;
		WeakReference<Listener>::Master masterReference;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Listener)
	};

	struct RectangleConstrainer : public ComponentBoundsConstrainer
	{
		struct Listener
		{
			virtual ~Listener();
			virtual void boundsChanged(const Rectangle<int> &newBounds) = 0;

		private:

			friend class WeakReference < Listener > ;
			WeakReference<Listener>::Master masterReference;
		};

		void checkBounds(Rectangle<int> &bounds, const Rectangle<int> &, const Rectangle<int> &, bool, bool, bool, bool) override;

		void addListener(Listener *l);
		void removeListener(Listener *l);

		Rectangle<int> draggingBounds;

	private:

		Array<WeakReference<Listener>> listeners;

	};

	// ================================================================================================================

	MouseCallbackComponent();;
	virtual ~MouseCallbackComponent();;

	static StringArray getCallbackLevels(bool getFileCallbacks=false);
	static StringArray getCallbackPropertyNames();

	void setJSONPopupData(var jsonData, Rectangle<int> popupSize_);

	void setPopupMenuItems(const StringArray &newItemList);

	static PopupMenu parseFromStringArray(const StringArray& itemList, Array<int> activeIndexes, LookAndFeel* laf);

	void setActivePopupItem(int menuId);

	void setUseRightClickForPopup(bool shouldUseRightClickForPopup);
	void alignPopup(bool shouldBeAligned);

	void setDraggingEnabled(bool shouldBeEnabled);
	void setDragBounds(Rectangle<int> newDraggingBounds, RectangleConstrainer::Listener* rectangleListener);

	void addMouseCallbackListener(Listener *l);
	void removeCallbackListener(Listener *l);
	void removeAllCallbackListeners();

	static var getMouseCallbackObject(Component* c, const MouseEvent& e, CallbackLevel level, Action action, EnterState state);

	void mouseDown(const MouseEvent& event) override;

	void touchAndHold(Point<int> downPosition) override;

	void fillPopupMenu(const MouseEvent &event);

	bool isInterestedInFileDrag(const StringArray& files) override;
	void fileDragEnter(const StringArray& files, int x, int y) override;
	void fileDragMove(const StringArray& files, int x, int y) override;
	void fileDragExit(const StringArray& files) override;
	void filesDropped(const StringArray& files, int x, int y) override;

	void mouseMove(const MouseEvent& event) override;

	void mouseDrag(const MouseEvent& event) override;
	void mouseEnter(const MouseEvent &event) override;
	void mouseExit(const MouseEvent &event) override;
	void mouseUp(const MouseEvent &event) override;
    void mouseDoubleClick(const MouseEvent &event) override;

	void setEnableFileDrop(const String& newCallbackLevel, const String& allowedWildcards);



	void setAllowCallback(const String &newCallbackLevel) noexcept;
	CallbackLevel getCallbackLevel() const;

	/** overwrite this method and update the Component to display the current value of the controlled attribute. */
	virtual void updateValue(NotificationType /*sendAttributeChange=sendNotification*/);;

	/** overwrite this method and return the range that the parameter can have. */
	virtual NormalisableRange<double> getRange() const;;

	void setRange(NormalisableRange<double> &newRange);

	void setMidiLearnEnabled(bool shouldBeEnabled);

	// ================================================================================================================

private:

	FileCallbackLevel fileCallbackLevel = FileCallbackLevel::NoCallbacks;
	StringArray fileDropExtensions;

	var jsonPopupData;
	Rectangle<int> popupSize;

	Component::SafePointer<Component> currentPopup;

	bool ignoreMouseUp = false;

	NormalisableRange<double> range;

	bool midiLearnEnabled = false;

	using SubMenuList = std::tuple < String, StringArray > ;

	void sendFileMessage(Action a, const String& f, Point<int> pos);

	void sendMessage(const MouseEvent &event, Action action, EnterState state = Nothing);
	void sendToListeners(var clickInformation);

	const StringArray callbackLevels;
	CallbackLevel callbackLevel;

	StringArray itemList;
	bool useRightClickForPopup = true;
	bool popupShouldBeAligned = false;
	bool draggingEnabled = false;
	bool currentlyShowingPopup = false;

	int activePopupId = 0;

	ScopedPointer<RectangleConstrainer> constrainer;
	ComponentDragger dragger;

	Array<WeakReference<Listener>, CriticalSection> listenerList;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MouseCallbackComponent);

	// ================================================================================================================
};

struct DrawActions
{
	class PostActionBase : public ReferenceCountedObject
	{
	public:

		virtual void perform(PostGraphicsRenderer& r) = 0;
		virtual bool needsStackData() const;
	};

	class ActionBase: public ReferenceCountedObject
	{
	public:

		using Ptr = ReferenceCountedObjectPtr<ActionBase>;

		ActionBase();;
		virtual ~ActionBase();;
		virtual void perform(Graphics& g) = 0;
		virtual bool wantsCachedImage() const;;
		virtual bool wantsToDrawOnParent() const;

		virtual void setCachedImage(Image& actionImage_, Image& mainImage_);
		virtual void setScaleFactor(float sf);

	protected:

		Image actionImage;
		Image mainImage;
		
		float scaleFactor = 1.0f;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActionBase);
		JUCE_DECLARE_WEAK_REFERENCEABLE(ActionBase);
	};

	struct MarkdownAction: public ActionBase
	{
		MarkdownAction();;

		using Ptr = ReferenceCountedObjectPtr<MarkdownAction>;

		void perform(Graphics& g) override;

		CriticalSection lock;
		MarkdownRenderer renderer;
		Rectangle<float> area;
	};

	class ActionLayer : public ActionBase
	{
	public:

		using Ptr = ReferenceCountedObjectPtr<ActionLayer>;

		ActionLayer(bool drawOnParent_);;

		bool wantsCachedImage() const override;

		bool wantsToDrawOnParent() const override;;

		void setCachedImage(Image& actionImage_, Image& mainImage_) final override;

		virtual void setScaleFactor(float sf) final override;

		void perform(Graphics& g);

		void addDrawAction(ActionBase* a);

		void addPostAction(PostActionBase* a);

	protected:

		bool drawOnParent = false;

		OwnedArray<ActionBase> internalActions;
		OwnedArray<PostActionBase> postActions;
		PostGraphicsRenderer::DataStack stack;
	};

	class BlendingLayer : public ActionLayer
	{
	public:

		BlendingLayer(gin::BlendMode m, float alpha_);

		bool wantsCachedImage() const override;

		void perform(Graphics& g) override;

		float alpha;
		
		Image blendSource;
		gin::BlendMode blendMode;
	};

	struct NoiseMapManager
	{
		struct NoiseMap
		{
			NoiseMap(Rectangle<int> a, bool monochrom_);;

			const int width;
			const int height;
			Image img;
			const bool monochrom;
		};

		void drawNoiseMap(Graphics& g, Rectangle<int> area, float alpha, bool monochrom, float scale);

	private:

		NoiseMap& getNoiseMap(Rectangle<int> area, bool monochrom);

		SimpleReadWriteLock lock;
		OwnedArray<NoiseMap> maps;
	};

	struct Handler: private AsyncUpdater
	{
		struct Iterator
		{
			Iterator(Handler* handler_);

			ActionBase::Ptr getNextAction();

			bool wantsCachedImage() const;

			bool wantsToDrawOnParent() const;

			void render(Graphics& g, Component* c);

			int index = 0;
			ReferenceCountedArray<ActionBase> actionsInIterator;
			Handler* handler;
		};

		struct Listener
		{
			virtual ~Listener();;
			virtual void newPaintActionsAvailable() = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

        ~Handler();

		void beginDrawing();

		bool beginBlendLayer(const Identifier& blendMode, float alpha);

		void beginLayer(bool drawOnParent);

		ActionLayer::Ptr getCurrentLayer();

		void endLayer();

		void addDrawAction(ActionBase* newDrawAction);

		void flush();

		void logError(const String& message);

		void addDrawActionListener(Listener* l);
		void removeDrawActionListener(Listener* l);

		Rectangle<int> getScreenshotBounds(Rectangle<int> shaderBounds) const;

		void setGlobalBounds(Rectangle<int> gb, Rectangle<int> tb, float sf);

		Rectangle<int> getGlobalBounds() const;
		float getScaleFactor() const;

		std::function<void(const String& m)> errorLogger;

		bool recursion = false;

		NoiseMapManager* getNoiseMapManager();

	private:

		SharedResourcePointer<NoiseMapManager> noiseManager;

		Rectangle<int> globalBounds;
		Rectangle<int> topLevelBounds;
		float scaleFactor = 1.0f;

		void handleAsyncUpdate() override;

		Array<WeakReference<Listener>> listeners;

		SpinLock lock;

		ReferenceCountedArray<ActionLayer> layerStack;

		ReferenceCountedArray<ActionBase> nextActions;
		ReferenceCountedArray<ActionBase> currentActions;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Handler);
	};

};

class BorderPanel : public MouseCallbackComponent,
                    public SafeChangeListener,
					public SettableTooltipClient,
				    public ButtonListener,
					public OpenGLRenderer,
					public DrawActions::Handler::Listener
{
public:

	// ================================================================================================================

	BorderPanel(DrawActions::Handler* drawHandler);
	~BorderPanel();

	void newOpenGLContextCreated() override;

	void renderOpenGL() override;

	void openGLContextClosing() override;

	void newPaintActionsAvailable() override;

	void paint(Graphics &g);
	Colour c1, c2, borderColour;

	void registerToTopLevelComponent();

	void resized() override;

	void buttonClicked(Button* b) override;

    void changeListenerCallback(SafeChangeBroadcaster* b);
    
#if HISE_INCLUDE_RLOTTIE
	void setAnimation(RLottieAnimation::Ptr newAnimation);

	RLottieAnimation::Ptr animation;
#endif

	// ================================================================================================================

	ScopedPointer<TopLevelWindowWithOptionalOpenGL::ScopedRegisterState> srs;

	bool recursion = false;

	float borderRadius;
	float borderSize;
	Image image;
	bool isUsingCustomImage = false;

	bool isPopupPanel = false;

	ImageButton closeButton;

	WeakReference<DrawActions::Handler> drawHandler;

	JUCE_DECLARE_WEAK_REFERENCEABLE(BorderPanel);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BorderPanel);
	
	// ================================================================================================================
};


class ImageComponentWithMouseCallback : public MouseCallbackComponent
{
public:

	// ================================================================================================================

	ImageComponentWithMouseCallback();

	void paint(Graphics &g) override;;

	void setImage(const Image &newImage);;
	void setAlpha(float newAlpha);;
	void setOffset(int newOffset);;
	void setScale(double newScale);;

private:

	AffineTransform scaler;
	Image image;
	float alpha;
	int offset;
	double scale;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageComponentWithMouseCallback);

	// ================================================================================================================
};


class MultilineLabel : public Label
{
public:

	// ================================================================================================================

	MultilineLabel(const String &name);;
	~MultilineLabel() {};

	void setMultiline(bool shouldBeMultiline);;
	TextEditor *createEditorComponent() override;

	void setUsePasswordCharacter(bool shouldUsePassword)
	{
		usePasswordChar = shouldUsePassword;
		repaint();
	}

	void paint(Graphics& g) override;

private:

	bool usePasswordChar = false;
	bool multiline;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultilineLabel);
    JUCE_DECLARE_WEAK_REFERENCEABLE(MultilineLabel);

	// ================================================================================================================
};

} // namespace hise

#endif  // MISCCOMPONENTS_H_INCLUDED
