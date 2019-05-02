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
class ScriptContentComponent: public Component,
							  public SafeChangeListener,
							  public GlobalScriptCompileListener,
							  public ScriptingApi::Content::RebuildListener,
							  public AsyncValueTreePropertyListener,
							  public Processor::DeleteListener							
{
public:

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

    void paintOverChildren(Graphics& g) override
    {
        if(isRebuilding)
        {
            g.fillAll(Colours::black.withAlpha(0.8f));
            g.setColour(Colours::white);
            g.setFont(GLOBAL_BOLD_FONT());
            g.drawText("Rebuilding...", 0, 0, getWidth(), getHeight(), Justification::centred, false);
        }
    }
    
	void contentWasRebuilt() override;

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

private:

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

	struct ProjectLinkResolver : public ControlledObject,
		public MarkdownParser::LinkResolver
	{
		ProjectLinkResolver(MainController* mc) :
			ControlledObject(mc),
			LinkResolver()
		{};

		String getContent(const MarkdownLink& url) override
		{
#if USE_BACKEND

			ignoreUnused(url);
			jassertfalse; // Do this correctly soon...

			return {};
#else
			return getMainController()->getEmbeddedMarkdownContent("{PROJECT_FOLDER}" + url.toString(MarkdownLink::Everything));
#endif
		}

		MarkdownParser::ResolveType getPriority() const override 
		{ 
#if USE_BACKEND
			return MarkdownParser::ResolveType::FileBased;
#else
			return MarkdownParser::ResolveType::Cached; 
#endif
		}

		LinkResolver* clone(MarkdownParser* ) const override { return new ProjectLinkResolver(const_cast<MainController*>(getMainController())); };
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("ProjectLinkResolver"); };
	};

	struct PooledImageProvider : public ControlledObject,
		public MarkdownParser::ImageProvider
	{
		PooledImageProvider(MainController* mc, MarkdownParser* parent) :
			ControlledObject(mc),
			ImageProvider(parent)
		{

		}

		Image getImage(const MarkdownLink& url, float width) override
		{
#if USE_BACKEND
			PoolReference ref(getMainController(), "{PROJECT_FOLDER}" + url.toString(MarkdownLink::Everything), FileHandlerBase::Images);

			if (!ref.isValid())
				return {};

			auto f = getMainController()->getCurrentImagePool()->loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
#else
			PoolReference ref(getMainController()->getCurrentImagePool(), "{PROJECT_FOLDER}" + url.toString(MarkdownLink::UrlWithoutAnchor), FileHandlerBase::Images);

			auto f = getMainController()->getCurrentImagePool()->loadFromReference(ref, PoolHelpers::LoadAndCacheWeak);
#endif

			if (auto d = f.getData())
				return resizeImageToFit(*d, width);

			return {};
		}


		MarkdownParser::ResolveType getPriority() const override
		{
#if USE_BACKEND
			return MarkdownParser::ResolveType::FileBased;
#else
			return MarkdownParser::ResolveType::Cached;
#endif
		}

		ImageProvider* clone(MarkdownParser* newParent) const override { return new PooledImageProvider(const_cast<MainController*>(getMainController()), newParent); };
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("PooledImageProvider"); };
	};

	SET_PANEL_NAME("MarkdownPanel");

	enum SpecialPanelIds
	{
		ContentFile = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
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

	void fromDynamicObject(const var& object) override;

	var toDynamicObject() const override
	{
		auto v = FloatingTileContent::toDynamicObject();

		v.getDynamicObject()->setProperty("ContentFile", contentFile);

		return v;
	}

	void parseContent();

	int getNumDefaultableProperties() const override
	{
		return SpecialPanelIds::numSpecialPanelIds;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < (int)FloatingTileContent::PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ContentFile, "ContentFile");

		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < (int)FloatingTileContent::PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ContentFile, "");

		jassertfalse;
		return{};
	}


	void resized() override
	{
		preview->setBounds(getParentShell()->getContentBounds());
		parseContent();
	}

	ScopedPointer<MarkdownPreview> preview;
	String contentFile;
};



} // namespace hise
#endif  // SCRIPTINGCONTENTCOMPONENT_H_INCLUDED
