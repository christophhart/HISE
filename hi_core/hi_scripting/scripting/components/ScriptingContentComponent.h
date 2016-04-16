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

#ifndef SCRIPTINGCONTENTCOMPONENT_H_INCLUDED
#define SCRIPTINGCONTENTCOMPONENT_H_INCLUDED

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
							  public GlobalScriptCompileListener
{
public:

	/** Creates a new Content which acts as container for all scripted elements. */
	ScriptContentComponent(ScriptBaseProcessor *p);;

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
		return contentData != nullptr ? contentData->name : String::empty;
	};

	/** Checks if the content is valid (recompiling the script invalidates it. */
	bool contentValid() const
	{
		return (processor.get() != nullptr) && processor.get()->getScriptingContent() == contentData; 
	};

	/** Returns the ScriptBaseProcessor associated with this Component. If it is not a ScriptProcessor or was deleted, it returns nullptr. */
	const ScriptProcessor *getScriptProcessor() const
	{
		return dynamic_cast<ScriptProcessor*>(processor.get());
	}

	void paint(Graphics &g) override;

	void scriptWasCompiled(ScriptProcessor *p) override;

	/** Recreates all components based on the supplied Content object and restores its values. */
	void setNewContent(ScriptingApi::Content *c);

	
	void deleteAllScriptComponents()
	{
		componentWrappers.clear();
	};

	void refreshContentButton();

	ScriptingApi::Content::ScriptComponent *getEditedComponent();

	void setEditedScriptComponent(ScriptingApi::Content::ScriptComponent *sc);

	bool keyPressed(const KeyPress &key) override;

	void paintOverChildren(Graphics &g) override;

	ScriptingApi::Content::ScriptComponent *getScriptComponentFor(Point<int> pos);

	void refreshMacroIndexes();

	String getContentTooltip() const;

	/** returns the colour which can be set with ScriptingApi::Content::setColour(). */
	Colour getContentColour();

	void updateValue(int i);

	void updateValues();

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	void updateComponent(int i);

	void updateContent(ScriptingApi::Content::ScriptComponent* componentToUpdate=nullptr);

	void resized();

private:

	friend class ScriptCreatedComponentWrapper;

	int height;

	WeakReference<ScriptingApi::Content> contentData;
	WeakReference<ScriptBaseProcessor> processor;

	int editedComponent;

	OwnedArray<ScriptCreatedComponentWrapper> componentWrappers;
};

class ModulatorSynthChainBody;
class ModulatorSynthChain;

/** A ScriptContentContainer displays all interface scripts (with Synth.addToFront(true)) with a tab bar. */
class ScriptContentContainer:  public Component,
							   public ButtonListener,
							   public GlobalScriptCompileListener
{
public:

	ScriptContentContainer(ModulatorSynthChain* chain_, ModulatorSynthChainBody *editor_=nullptr);

	~ScriptContentContainer();

	int getContentHeight() const;

	int getContentWidth() const
	{
		if(interfaces.size() != 0)
		{
			return interfaces[0]->content->getContentWidth();
		}
		else
		{
			return -1;
		}
	}

	void scriptWasCompiled(ScriptProcessor *sp) override;

	/** Refreshes the bounds of the interface contents and adds a tab bar if multiple interfaces are used. */
	void refreshContentBounds();

	void resized() override { refreshContentBounds(); };

	void paint(Graphics &g);

	/** This sets the currently displayed content. 
	*
	*	It handles the button toggle modes and sets the visibility of the desired content.
	*	The size / bounds are not changed. You can tell the method to save the current content in an EditorState by
	*	passing sendNotification as second parameter.
	*/
	void setCurrentContent(int index, NotificationType notifyEditorState);

	/** sets the content to be displayed. */
	void buttonClicked(Button *b) override;

	/** Scans all ScriptProcessors and adds their content if it doesn't already exist, and removes dangling ScriptProcessors. */
	void checkInterfaces();

	/** Restores the index from the editorstate. */
	void restoreSavedView();
	
	bool isFrontendContainer() const {return isFrontend; };

	void setIsFrontendContainer(bool shouldBeFrontendContainer)
	{
		isFrontend = shouldBeFrontendContainer;
	}

	int getNumInterfaces() const
	{
		return interfaces.size();
	}

private:

	bool processorHasContent(ScriptProcessor *sp)
	{
		for(int i = 0; i < interfaces.size(); i++)
		{
			if(interfaces[i]->content->getScriptProcessor() == sp)
			{
				return true;
			}
		}
		return false;
	}

	int getIndexForButton(Button *b)
	{
		for(int i = 0; interfaces.size(); i++)
		{
			if(interfaces[i]->button == b) return i;
		}

		return -1;
	};

	ModulatorSynthChain *chain;

	struct InterfaceScriptAndButton
	{
		InterfaceScriptAndButton(ScriptProcessor *sp, ScriptContentContainer *container);

		ScopedPointer<TextButton> button;
		ScopedPointer<ScriptContentComponent> content;
	};


#if USE_BACKEND
	Component::SafePointer<ModulatorSynthChainBody> editor;

#else
	ModulatorSynthChainBody *editor;
#endif

	Component::SafePointer<ScriptContentComponent> currentContent;

	OwnedArray<InterfaceScriptAndButton> interfaces;

	ScopedPointer<ChainBarButtonLookAndFeel> laf;



	bool isFrontend;
};


#endif  // SCRIPTINGCONTENTCOMPONENT_H_INCLUDED
