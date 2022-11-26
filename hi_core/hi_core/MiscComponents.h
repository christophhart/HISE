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

		Listener() {}
		virtual ~Listener() { masterReference.clear(); }
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
			virtual ~Listener() { masterReference.clear(); }
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
	virtual ~MouseCallbackComponent() {};

	static StringArray getCallbackLevels(bool getFileCallbacks=false);
	static StringArray getCallbackPropertyNames();

	void setJSONPopupData(var jsonData, Rectangle<int> popupSize_) 
	{ 
		jsonPopupData = jsonData; 
		popupSize = popupSize_;
	}

	void setPopupMenuItems(const StringArray &newItemList);

	static PopupMenu parseFromStringArray(const StringArray& itemList, Array<int> activeIndexes, LookAndFeel* laf);

	void setActivePopupItem(int menuId)
	{
		activePopupId = menuId;
	}

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
	virtual void updateValue(NotificationType /*sendAttributeChange=sendNotification*/)
	{
		repaint();
	};

	/** overwrite this method and return the range that the parameter can have. */
	virtual NormalisableRange<double> getRange() const
	{
		return range;
	};

	void setRange(NormalisableRange<double> &newRange)
	{
		range = newRange;
	}

	void setMidiLearnEnabled(bool shouldBeEnabled)
	{
		midiLearnEnabled = shouldBeEnabled;
	}

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
		virtual bool needsStackData() const { return false; }
	};

	class ActionBase: public ReferenceCountedObject
	{
	public:

		using Ptr = ReferenceCountedObjectPtr<ActionBase>;

		ActionBase() {};
		virtual ~ActionBase() {};
		virtual void perform(Graphics& g) = 0;
		virtual bool wantsCachedImage() const { return false; };
		virtual bool wantsToDrawOnParent() const { return false; }

		virtual void setCachedImage(Image& actionImage_, Image& mainImage_) { actionImage = actionImage_; mainImage = mainImage_; }
		virtual void setScaleFactor(float sf) { scaleFactor = sf; }

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
		MarkdownAction() :
			renderer("")
		{};

		using Ptr = ReferenceCountedObjectPtr<MarkdownAction>;

		void perform(Graphics& g) override
		{
			ScopedLock sl(lock);
			renderer.draw(g, area);
		}

		CriticalSection lock;
		MarkdownRenderer renderer;
		Rectangle<float> area;
	};

	class ActionLayer : public ActionBase
	{
	public:

		using Ptr = ReferenceCountedObjectPtr<ActionLayer>;

		ActionLayer(bool drawOnParent_) :
			ActionBase(),
			drawOnParent(drawOnParent_)
		{};

		bool wantsCachedImage() const override 
		{ 
			if(postActions.size() > 0)
				return true;

			for (auto a : internalActions)
			{
				if (a->wantsCachedImage())
					return true;
			}

			return false;
		}

		bool wantsToDrawOnParent() const override { return drawOnParent; };

		void setCachedImage(Image& actionImage_, Image& mainImage_) final override
		{ 
			ActionBase::setCachedImage(actionImage_, mainImage_);

			// do not propagate the main image
			for (auto a : internalActions)
				a->setCachedImage(actionImage_, actionImage_);
		}

		virtual void setScaleFactor(float sf) final override
		{ 
			ActionBase::setScaleFactor(sf);

			for (auto a : internalActions)
				a->setScaleFactor(sf);
		}

		void perform(Graphics& g)
		{
			for (auto action : internalActions)
				action->perform(g);
			
			if (postActions.size() > 0)
			{
				PostGraphicsRenderer r(stack, actionImage, scaleFactor);
				int numDataRequired = 0;

				for (auto p : postActions)
				{
					if (p->needsStackData())
						numDataRequired++;
				}
				
				r.reserveStackOperations(numDataRequired);

				for (auto p : postActions)
					p->perform(r);
			}
		}

		void addDrawAction(ActionBase* a)
		{
			internalActions.add(a);
		}

		void addPostAction(PostActionBase* a)
		{
			postActions.add(a);
		}

	protected:

		bool drawOnParent = false;

		OwnedArray<ActionBase> internalActions;
		OwnedArray<PostActionBase> postActions;
		PostGraphicsRenderer::DataStack stack;
	};

	class BlendingLayer : public ActionLayer
	{
	public:

		BlendingLayer(gin::BlendMode m, float alpha_) :
			ActionLayer(true),
			blendMode(m),
			alpha(alpha_)
		{

		}

		bool wantsCachedImage() const override { return true; }

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

		void drawNoiseMap(Graphics& g, Rectangle<int> area, float alpha, bool monochrom, float scale)
		{
			auto originalArea = area;

			if(scale != 1.0f)
				area = area.transformed(AffineTransform::scale(scale));

			const auto& m = getNoiseMap(area, monochrom);

			g.setColour(Colours::black.withAlpha(alpha));

			if (scale != 1.0f)
				g.drawImageWithin(m.img, originalArea.getX(), originalArea.getY(), originalArea.getWidth(), originalArea.getHeight(), RectanglePlacement::stretchToFit);
			else
				g.drawImageAt(m.img, area.getX(), area.getY());
		}

	private:

		NoiseMap& getNoiseMap(Rectangle<int> area, bool monochrom)
		{
			for (auto m : maps)
			{

				if (area.getWidth() == m->width &&
					area.getHeight() == m->height &&
					monochrom == m->monochrom)
				{
					return *m;
				}
			}

			maps.add(new NoiseMap(area, monochrom));

			return *maps.getLast();
		}

		SimpleReadWriteLock lock;
		OwnedArray<NoiseMap> maps;
	};

	struct Handler: private AsyncUpdater
	{
		struct Iterator
		{
			Iterator(Handler* handler_):
				handler(handler_)
			{
				if (handler != nullptr)
				{
					actionsInIterator.ensureStorageAllocated(handler->nextActions.size());
					SpinLock::ScopedLockType sl(handler->lock);

					actionsInIterator.addArray(handler->nextActions);
				}
			}

			ActionBase::Ptr getNextAction()
			{
				if (index < actionsInIterator.size())
					return actionsInIterator[index++];

				return nullptr;
			}

			bool wantsCachedImage() const
			{
				for (auto action : actionsInIterator)
					if (action != nullptr && action->wantsCachedImage())
						return true;

				return false;
			}

			bool wantsToDrawOnParent() const
			{
				for (auto action : actionsInIterator)
					if (action != nullptr && action->wantsToDrawOnParent())
						return true;

				return false;
			}

			void render(Graphics& g, Component* c);

			int index = 0;
			ReferenceCountedArray<ActionBase> actionsInIterator;
			Handler* handler;
		};

		struct Listener
		{
			virtual ~Listener() {};
			virtual void newPaintActionsAvailable() = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

        ~Handler()
        {
            cancelPendingUpdate();
        }
        
		void beginDrawing()
		{
			currentActions.clear();
		}

		bool beginBlendLayer(const Identifier& blendMode, float alpha);

		void beginLayer(bool drawOnParent)
		{
			auto newLayer = new ActionLayer(drawOnParent);

			addDrawAction(newLayer);
			layerStack.insert(-1, newLayer);
		}

		ActionLayer::Ptr getCurrentLayer()
		{
			return layerStack.getLast();
		}

		void endLayer()
		{
			layerStack.removeLast();
		}

		void addDrawAction(ActionBase* newDrawAction)
		{
			if (layerStack.getLast() != nullptr)
				layerStack.getLast()->addDrawAction(newDrawAction);
			else
				currentActions.add(newDrawAction);
		}

		void flush()
		{
			{
				SpinLock::ScopedLockType sl(lock);

				nextActions.swapWith(currentActions);
				currentActions.clear();
				layerStack.clear();
			}

			triggerAsyncUpdate();
		}

		void logError(const String& message)
		{
			if (errorLogger)
				errorLogger(message);
		}

		void addDrawActionListener(Listener* l) { listeners.addIfNotAlreadyThere(l); }
		void removeDrawActionListener(Listener* l) { listeners.removeAllInstancesOf(l); }

		Rectangle<int> getScreenshotBounds(Rectangle<int> shaderBounds) const;

		void setGlobalBounds(Rectangle<int> gb, Rectangle<int> tb, float sf)
		{
			globalBounds = gb;
			topLevelBounds = tb;
			scaleFactor = sf;
		}

		Rectangle<int> getGlobalBounds() const { return globalBounds; }
		float getScaleFactor() const { return scaleFactor; }

		std::function<void(const String& m)> errorLogger;

		bool recursion = false;

		NoiseMapManager* getNoiseMapManager() { return &noiseManager.getObject(); }

	private:

		SharedResourcePointer<NoiseMapManager> noiseManager;

		Rectangle<int> globalBounds;
		Rectangle<int> topLevelBounds;
		float scaleFactor = 1.0f;

		void handleAsyncUpdate() override
		{
			for (auto l : listeners)
			{
				if (l != nullptr)
					l->newPaintActionsAvailable();
			}
		}

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

	void newOpenGLContextCreated() override
	{
	}

	void renderOpenGL() override
	{
		
	}

	void openGLContextClosing() override
	{
	}

	void newPaintActionsAvailable() override { repaint(); }

	void paint(Graphics &g);
	Colour c1, c2, borderColour;

	void registerToTopLevelComponent()
	{
#if 0
		if (srs == nullptr)
		{
			if (auto tc = findParentComponentOfClass<TopLevelWindowWithOptionalOpenGL>())
				srs = new TopLevelWindowWithOptionalOpenGL::ScopedRegisterState(*tc, this);
		}
#endif
	}

	void resized() override
	{
		registerToTopLevelComponent();

		if (isPopupPanel)
		{
			closeButton.setBounds(getWidth() - 24, 0, 24, 24);
		}
		else
			closeButton.setVisible(false);
		
	}

	void buttonClicked(Button* b) override;

    void changeListenerCallback(SafeChangeBroadcaster* b);
    
#if HISE_INCLUDE_RLOTTIE
	void setAnimation(RLottieAnimation::Ptr newAnimation)
	{
		animation = newAnimation;
	}

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
