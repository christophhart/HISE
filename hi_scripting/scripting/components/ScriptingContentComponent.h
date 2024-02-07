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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef SCRIPTINGCONTENTCOMPONENT_H_INCLUDED
#define SCRIPTINGCONTENTCOMPONENT_H_INCLUDED

namespace hise { using namespace juce;

#define GET_SCRIPT_PROPERTY(component, id) (component->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::id))

#define GET_OBJECT_COLOUR(component, id) (Colour((uint32)(int64)GET_SCRIPT_PROPERTY(component, id)))

class VuMeter;


class ModulatorPeakMeter: public Component,
						  public SettableTooltipClient,
					  public Timer
{
public:

	ModulatorPeakMeter(Modulator *m);;

	void resized()
	{
		vuMeter->setBounds(getLocalBounds());
	}

	void setColour(Colour c)
	{
		vuMeter->setColour(VuMeter::ledColour, c);
	}

	void timerCallback() override
	{
		if(mod.get() != nullptr)
		{
			vuMeter->setPeak(mod->getDisplayValues().outL);
		}
	};
	
private:

	WeakReference<Modulator> mod;

	ScopedPointer<VuMeter> vuMeter;
};



/** A component that can be populated with GUI elements by a script. 
*	@ingroup scripting
*	
*
*
*/
class ScriptContentComponent: public ComponentWithMiddleMouseDrag,
							  public SafeChangeListener,
							  public GlobalScriptCompileListener,
							  public ScriptingApi::Content::RebuildListener,
							  public AsyncValueTreePropertyListener,
							  public Processor::DeleteListener,
							  public ScriptingApi::Content::ScreenshotListener,
							  public DragAndDropContainer,
							  public DragAndDropTarget
{
public:

	struct Updater: public Processor::OtherListener
	{
		Updater(ScriptContentComponent& parent_, Processor* p):
		  OtherListener(p, dispatch::library::ProcessorChangeEvent::Any), // TODO: check if it should be removed altogheter when the content is implemented properly
		  parent(parent_)
		{};

		ScriptContentComponent& parent;

		void otherChange(Processor* p) override
		{
			parent.updateContent();
		}
	};

	/** Creates a new Content which acts as container for all scripted elements. */
	ScriptContentComponent(ProcessorWithScriptingContent *p);;

	~ScriptContentComponent();

	/** Returns the height of the component. 
	*
	*	It does not simply call Component::getHeight(), but checks the underlying ScriptingApi::Content object 
	*	for its height property. 
	*/
	int getContentHeight() const
	{
		return contentData.get() != nullptr ? contentData->height : 0;
	};

	int getContentWidth() const
	{
		return contentData.get() != nullptr ? contentData->width : -1;
	}

	void suspendStateChanged(bool shouldBeSuspended) override
	{
		repaint();
	}

	/** returns the script name that was set with Content.setName(). */
	String getScriptName() const
	{
		return contentData != nullptr ? contentData->name : String();
	};

	/** Checks if the content is valid (recompiling the script invalidates it. */
	bool contentValid() const
	{
		return (processor != nullptr) && processor->getScriptingContent() == contentData; 
	};

	/** Returns the ScriptBaseProcessor associated with this Component. If it is not a ScriptProcessor or was deleted, it returns nullptr. */
	const JavascriptProcessor *getScriptProcessor() const
	{
		return dynamic_cast<JavascriptProcessor*>(p.get());
	}

	void processorDeleted(Processor* /*deletedProcessor*/) override;

	void updateChildEditorList(bool /*forceUpdate*/) override {};

	void paint(Graphics &g) override;

#if PERFETTO
	void paintComponentAndChildren(Graphics& g) override
	{
		dispatch::StringBuilder b;
		auto cp = g.getClipBounds();
		b << "[" << cp.getX() << ", " << cp.getY() << ", " << cp.getWidth() << ", " << cp.getHeight() << "]";
		TRACE_EVENT("component", "Render script interface", "clipBounds", DYNAMIC_STRING_BUILDER(b));
		double before = Time::getMillisecondCounterHiRes();
		Component::paintComponentAndChildren(g);
		double now = Time::getMillisecondCounterHiRes();
		auto deltaInSeconds = (now - before) * 0.001;
		auto fps = 1.0 / deltaInSeconds;

		if(fps < 800.0)
		{
			auto ct = perfetto::CounterTrack("Interface FPS Counter", "fps");
			TRACE_COUNTER("component", ct.set_is_incremental(false), fps);
		}
	}
#endif

#if 0
#define VIRTUAL_PERFETTO_OVERRIDE_2(name, t0, a0, t1, a1, label) void name(t0 a0, t1 a1) override { TRACE_EVENT("drawactions", label); Component::name(a0, a1); };

	VIRTUAL_PERFETTO_OVERRIDE_2(paintChildComponents, Graphics&, g, Rectangle<int>, clipBounds, "paint UI components");
#endif

#if 0
	void paintChildComponents(Graphics& g, Rectangle<int> clipBounds) override
	{
		TRACE_EVENT("drawactions", "paint UI components");
		Component::paintChildComponents(g, clipBounds);
	}
#endif
	
    void paintOverChildren(Graphics& g) override;
    
	void makeScreenshot(const File& target, Rectangle<float> area) override;

	void visualGuidesChanged() override;

	void prepareScreenshot() override;

	void contentWasRebuilt() override;

	ScriptingApi::Content::TextInputDataBase::Ptr currentTextBox;

    void contentRebuildStateChanged(bool rebuildState)
    {
        if(rebuildState)
        {
            deleteAllScriptComponents();
        }
        
        isRebuilding = rebuildState;
        
		WeakReference<ScriptContentComponent> tmp(this);

		auto f = [tmp]()
		{
			if (tmp != nullptr)
				tmp.get()->repaint();
		};
        
		new DelayedFunctionCaller(f, 100);
    };
    
	void scriptWasCompiled(JavascriptProcessor *p) override;

	/** Recreates all components based on the supplied Content object and restores its values. */
	void setNewContent(ScriptingApi::Content *c);

    void addMouseListenersForComponentWrappers();
	
	void deleteAllScriptComponents();;

	void refreshContentButton();

	bool keyPressed(const KeyPress &key) override;

	ScriptingApi::Content::ScriptComponent *getScriptComponentFor(Point<int> pos);

	ScriptingApi::Content::ScriptComponent* getScriptComponentFor(Component* component);

	Component* getComponentFor(ScriptingApi::Content::ScriptComponent* sc);

	void getScriptComponentsFor(Array<ScriptingApi::Content::ScriptComponent*> &arrayToFill, Point<int> pos);

	void getScriptComponentsFor(Array<ScriptingApi::Content::ScriptComponent*> &arrayToFill, const Rectangle<int> area);

	void refreshMacroIndexes();

	String getContentTooltip() const;

	/** returns the colour which can be set with ScriptingApi::Content::setColour(). */
	Colour getContentColour();

	void updateValue(int i);

	void updateValues();

	struct SimpleTraverser : public ComponentTraverser
	{
		SimpleTraverser() = default;
		
		Component* getDefaultComponent(Component* parentComponent) override;
		Component* getNextComponent(Component* current) override;
		Component* getPreviousComponent(Component* current) override;
		std::vector<Component*> getAllComponents(Component* parentComponent) override;

	};

	std::unique_ptr<ComponentTraverser> createKeyboardFocusTraverser() override
	{
		std::unique_ptr<ComponentTraverser> s;
		s.reset(new SimpleTraverser());
		return s;
	}

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	void asyncValueTreePropertyChanged(ValueTree& v, const Identifier& id) override;

	void valueTreeChildAdded(ValueTree& parent, ValueTree& child) override;

	void updateComponent(int i);

	void updateContent(ScriptingApi::Content::ScriptComponent* componentToUpdate=nullptr);

	void updateComponentPosition(ScriptCreatedComponentWrapper* wrapper);

	void updateComponentVisibility(ScriptCreatedComponentWrapper* wrapper);

	void updateComponentParent(ScriptCreatedComponentWrapper* wrapper);

	void resized();

	void setModalPopup(ScriptCreatedComponentWrapper* wrapper, bool shouldShow);

	ScriptCreatedComponentWrapper::ValuePopup::Properties* getValuePopupProperties() const
	{
		return valuePopupProperties.get();
	}

	bool onDragAction(ScriptingApi::Content::RebuildListener::DragAction a, ScriptComponent* source, var& data) override;

	void itemDropped(const SourceDetails& dragSourceDetails);

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;

	void dragOperationEnded(const DragAndDropTarget::SourceDetails& dragData);

private:

	struct ComponentDragInfo: public DrawActions::Handler::Listener,
							  public ControlledObject
	{
		ComponentDragInfo(ScriptContentComponent* parent, ScriptComponent* sc, const var& dragData);

		~ComponentDragInfo();

		ScaledImage getDragImage(bool refresh);
		
		void start(Component* sourceComponent)
		{
			source = sourceComponent;
			callRepaint();
		}

		void stop();

		bool dragTargetChanged();

		bool getCurrentComponent(bool force, var& data);

		bool isValid(bool force);

		bool stopped = false;

		void callRepaint();

		void newPaintActionsAvailable(uint64_t) override;

	private:

		ScopedPointer<Component> dummyComponent;

		var graphicsObject;

		bool validTarget = false;

		String currentDragTarget;
		ScriptComponent* currentTargetComponent = nullptr;

		Component* source = nullptr;

		ScriptContentComponent& parent;

		ScaledImage img;
		var scriptComponent;
		var dragData;
		WeakCallbackHolder paintRoutine;
		WeakCallbackHolder dragCallback;
	};

	ScopedPointer<ComponentDragInfo> currentDragInfo;

	class ModalOverlay : public Component
	{
	public:

		ModalOverlay(ScriptContentComponent& p) :
			parent(p)
		{
			setInterceptsMouseClicks(true, true);
		}

		void togglePopup(ScriptCreatedComponentWrapper* panelWrapper)
		{
			auto panel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(panelWrapper->getScriptComponent());

			if (currentPopup != panel)
			{
				showFor(panelWrapper);
			}
			else
			{
				closeModalPopup();
			}
		}

		void showFor(ScriptCreatedComponentWrapper* panelWrapper)
		{
			auto newPanel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(panelWrapper->getScriptComponent());;

			if (newPanel != currentPopup)
			{
				currentPopup = newPanel;
				currentPopup->showAsModalPopup();

				currentPopupComponent = panelWrapper->getComponent();

				setVisible(true);
				toFront(false);
				currentPopupComponent->setVisible(true);
				currentPopupComponent->toFront(false);
			}
			
		}

		void mouseDown(const MouseEvent& /*event*/) override
		{
			closeModalPopup();
		}

		void closeModalPopup()
		{
			if (currentPopup != nullptr)
			{
				currentPopup->closeAsPopup();
				setVisible(false);

				currentPopupComponent->setVisible(false);
				currentPopupComponent = nullptr;

				currentPopup = nullptr;
			}
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0x99000000));
		}

	private:

		WeakReference<ScriptingApi::Content::ScriptPanel> currentPopup;
		Component::SafePointer<Component> currentPopupComponent;

		ScriptContentComponent& parent;
	};

	struct ContentRebuildNotifier : private AsyncUpdater
	{
		ContentRebuildNotifier(ScriptContentComponent& parent_) :
			parent(parent_)
		{};

		void notify(ScriptingApi::Content* newContent)
		{
			content = newContent;

			if (MessageManager::getInstance()->isThisTheMessageThread())
				handleAsyncUpdate();
			else
				triggerAsyncUpdate();
		}

	private:

		void handleAsyncUpdate() override
		{
			if (content != nullptr)
				parent.setNewContent(content);
		}

		WeakReference<ScriptingApi::Content> content;

		ScriptContentComponent& parent;
	};

	Updater updater;

	ModalOverlay modalOverlay;
	ContentRebuildNotifier contentRebuildNotifier;

    bool isRebuilding = false;

	friend class ScriptCreatedComponentWrapper;

	int height;

	WeakReference<ScriptingApi::Content> contentData;
	ProcessorWithScriptingContent* processor;
	WeakReference<Processor> p;

	OwnedArray<ScriptCreatedComponentWrapper> componentWrappers;
	ScriptCreatedComponentWrapper::ValuePopup::Properties::Ptr valuePopupProperties;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptContentComponent);
};



class MarkdownPreviewPanel : public Component,
	public FloatingTileContent
{
public:

	SET_PANEL_NAME("MarkdownPanel");

	enum SpecialPanelIds
	{
		ShowToc = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		ShowSearch,
		ShowBack,
		BoldFontName,
		FixTocWidth,
		StartURL,
		ServerUpdateURL,
		CustomContent,
		numSpecialPanelIds
	};

	MarkdownPreviewPanel(FloatingTile* parent);

	~MarkdownPreviewPanel()
	{
		preview = nullptr;
	}

	void paint(Graphics& g) override
	{
		g.fillAll(findPanelColour(PanelColourId::bgColour));
	}

	void visibilityChanged() override
	{
		if (preview == nullptr)
			return;

		if (isVisible())
		{
			if (auto projectHolder = dynamic_cast<ProjectDocDatabaseHolder*>(preview->renderer.getHolder()))
			{
				if (URL::isProbablyAWebsiteURL(serverURL))
				{
					URL sUrl(serverURL);
					projectHolder->setProjectURL(sUrl);
				}
			}
		}
	}

	void fromDynamicObject(const var& obj) override
	{
		FloatingTileContent::fromDynamicObject(obj);

		serverURL = getPropertyWithDefault(obj, SpecialPanelIds::ServerUpdateURL);

		showSearch = getPropertyWithDefault(obj, SpecialPanelIds::ShowSearch);
		showBack = getPropertyWithDefault(obj, SpecialPanelIds::ShowBack);
		showToc = getPropertyWithDefault(obj, SpecialPanelIds::ShowToc);
		startURL = getPropertyWithDefault(obj, SpecialPanelIds::StartURL);
		customContent = getPropertyWithDefault(obj, SpecialPanelIds::CustomContent);

		boldFontName = getPropertyWithDefault(obj, SpecialPanelIds::BoldFontName).toString();

		sd.f = getFont();
		sd.fontSize = getFont().getHeight();

		if (boldFontName.isNotEmpty())
		{
			sd.useSpecialBoldFont = true;
			sd.boldFont = getMainController()->getFontFromString(boldFontName, sd.fontSize);
		}

		sd.backgroundColour = findPanelColour(PanelColourId::bgColour);
		sd.textColour = findPanelColour(PanelColourId::textColour);
		sd.headlineColour = findPanelColour(PanelColourId::itemColour1);
		sd.linkColour = findPanelColour(PanelColourId::itemColour2);
		
		fixWidth = getPropertyWithDefault(obj, SpecialPanelIds::FixTocWidth);

		initPanel();
	}

	void initPanel();

	var toDynamicObject() const override
	{
		auto obj = FloatingTileContent::toDynamicObject();

		storePropertyInObject(obj, SpecialPanelIds::ShowBack, showBack);
		storePropertyInObject(obj, SpecialPanelIds::ShowSearch, showSearch);
		storePropertyInObject(obj, SpecialPanelIds::ShowToc, showToc);
		storePropertyInObject(obj, SpecialPanelIds::BoldFontName, boldFontName);
		storePropertyInObject(obj, SpecialPanelIds::FixTocWidth, fixWidth);
		storePropertyInObject(obj, SpecialPanelIds::StartURL, startURL);
		storePropertyInObject(obj, SpecialPanelIds::ServerUpdateURL, serverURL);
		storePropertyInObject(obj, SpecialPanelIds::CustomContent, customContent);


		return obj;
	}

	int getNumDefaultableProperties() const override
	{
		return SpecialPanelIds::numSpecialPanelIds;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < (int)FloatingTileContent::PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowToc, "ShowToc");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowSearch, "ShowSearch");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowBack, "ShowBack");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::BoldFontName, "BoldFontName");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::FixTocWidth, "FixTocWidth");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::StartURL, "StartURL");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ServerUpdateURL, "ServerUpdateURL");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::CustomContent, "CustomContent");

		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < (int)FloatingTileContent::PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowToc, true);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowSearch, true);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowBack, true);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::BoldFontName, "");
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::FixTocWidth, -1);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::StartURL, "/");
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ServerUpdateURL, "");
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::CustomContent, "");

		jassertfalse;
		return{};
	}


	void resized() override
	{
		if(preview != nullptr)
			preview->setBounds(getParentShell()->getContentBounds());
	}

	bool showSearch = true;
	bool showBack = true;
	bool showToc = true;
	int fixWidth = -1;
	String boldFontName;

	MarkdownLayout::StyleData sd;
	String contentFile;
	String startURL;
	String serverURL;
	String customContent;
	int options;

	ScopedPointer<HiseMarkdownPreview> preview;
};



} // namespace hise
#endif  // SCRIPTINGCONTENTCOMPONENT_H_INCLUDED
