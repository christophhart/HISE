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
							   public TouchAndHoldComponent
{
	// ================================================================================================================

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
		Nothing
	};

public:

	enum class CallbackLevel
	{
		NoCallbacks = 0,
		PopupMenuOnly,
		ClicksOnly,
		ClicksAndEnter,
		Drag,
		AllCallbacks
	};

	// ================================================================================================================

	class Listener
	{
	public:

		Listener() {}
		virtual ~Listener() { masterReference.clear(); }
		virtual void mouseCallback(const var &mouseInformation) = 0;

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

	static StringArray getCallbackLevels();
	static StringArray getCallbackPropertyNames();

	void setJSONPopupData(var jsonData, Rectangle<int> popupSize_) 
	{ 
		jsonPopupData = jsonData; 
		popupSize = popupSize_;
	}

	void setPopupMenuItems(const StringArray &newItemList);

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

	void mouseDown(const MouseEvent& event) override;

	void touchAndHold(Point<int> downPosition) override;

	void fillPopupMenu(const MouseEvent &event);

	void mouseMove(const MouseEvent& event) override;

	void mouseDrag(const MouseEvent& event) override;
	void mouseEnter(const MouseEvent &event) override;
	void mouseExit(const MouseEvent &event) override;
	void mouseUp(const MouseEvent &event) override;
    void mouseDoubleClick(const MouseEvent &event) override;

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

	var jsonPopupData;
	Rectangle<int> popupSize;

	Component::SafePointer<Component> currentPopup;

	bool ignoreMouseUp = false;

	NormalisableRange<double> range;

	bool midiLearnEnabled = false;

	using SubMenuList = std::tuple < String, StringArray > ;

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

		ActionBase() {};
		virtual ~ActionBase() {};
		virtual void perform(Graphics& g) = 0;
		virtual bool wantsCachedImage() const { return false; };
		virtual bool wantsToDrawOnParent() const { return false; }

		void setCachedImage(Image& img) { cachedImage = img; }

	protected:

		Image cachedImage;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActionBase);
		JUCE_DECLARE_WEAK_REFERENCEABLE(ActionBase);
	};

	class ActionLayer : public ActionBase
	{
	public:

		using Ptr = ReferenceCountedObjectPtr<ActionLayer>;

		ActionLayer(bool drawOnParent_) :
			ActionBase(),
			drawOnParent(drawOnParent_)
		{};

		bool wantsCachedImage() const override { return postActions.size() > 0; }

		bool wantsToDrawOnParent() const override { return drawOnParent; };

		void perform(Graphics& g)
		{
			for (auto action : internalActions)
				action->perform(g);

			if (postActions.size() > 0)
			{
				PostGraphicsRenderer r(stack, cachedImage);
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

	private:

		bool drawOnParent = false;

		OwnedArray<ActionBase> internalActions;
		OwnedArray<PostActionBase> postActions;
		PostGraphicsRenderer::DataStack stack;
	};

	struct Handler: private AsyncUpdater
	{
		struct Iterator
		{
			Iterator(Handler* handler)
			{
				if(handler != nullptr)
					actionsInIterator.addArray(handler->nextActions);
			}

			ActionBase* getNextAction()
			{
				if (index < actionsInIterator.size())
					return actionsInIterator[index++];

				return nullptr;
			}

			bool wantsCachedImage() const
			{
				for (auto action : actionsInIterator)
					if (action->wantsCachedImage())
						return true;

				return false;
			}

			int index = 0;
			ReferenceCountedArray<ActionBase> actionsInIterator;
		};

		struct Listener
		{
			virtual ~Listener() {};
			virtual void newPaintActionsAvailable() = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

		void beginDrawing()
		{
			currentActions.clear();
		}

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
			{
				layerStack.getLast()->addDrawAction(newDrawAction);
			}
			else
			{
				currentActions.add(newDrawAction);
			}
		}

		void flush()
		{
			nextActions.swapWith(currentActions);
			currentActions.clear();
			layerStack.clear();
			triggerAsyncUpdate();
		}

		void addDrawActionListener(Listener* l) { listeners.addIfNotAlreadyThere(l); }
		void removeDrawActionListener(Listener* l) { listeners.removeAllInstancesOf(l); }

	private:

		void handleAsyncUpdate() override
		{
			for (auto l : listeners)
			{
				if (l != nullptr)
					l->newPaintActionsAvailable();
			}
		}

		Array<WeakReference<Listener>> listeners;

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
					public DrawActions::Handler::Listener
{
public:

	// ================================================================================================================

	BorderPanel(DrawActions::Handler* drawHandler);
	~BorderPanel();

	void newPaintActionsAvailable() override { repaint(); }

	void paint(Graphics &g);
	Colour c1, c2, borderColour;

	void resized() override
	{
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

	bool recursion = false;

	float borderRadius;
	float borderSize;
	Image image;
	bool isUsingCustomImage = false;

	bool isPopupPanel = false;

	ImageButton closeButton;

	WeakReference<DrawActions::Handler> drawHandler;

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

	// ================================================================================================================
};

} // namespace hise

#endif  // MISCCOMPONENTS_H_INCLUDED
