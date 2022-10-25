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

#ifndef PROCESSOREDITOR_H_INCLUDED
#define PROCESSOREDITOR_H_INCLUDED

namespace hise { using namespace juce;

#define SET_ATTRIBUTE_FROM_SLIDER(x) {getProcessor()->setAttribute(x, (float)sliderThatWasMoved->getValue(), dontSendNotification);}

#if USE_BACKEND
#define ADD_EDITOR_CREATOR(processorClassName, editorClassName) ProcessorEditorBody *processorClassName::createEditor(ProcessorEditor *parentEditor) { return new editorClassName(parentEditor); }
#else
#define ADD_EDITOR_CREATOR(processorClassName, editorClassName) ProcessorEditorBody *processorClassName::createEditor(ProcessorEditor *parentEditor) { ignoreUnused(parentEditor); jassertfalse; return nullptr; }
#endif



#define INTENDATION_WIDTH 10


/** The container that holds all vertically stacked ProcessorEditors. */
class ProcessorEditorContainer : public Component,
							     public SafeChangeBroadcaster,
								 public Processor::DeleteListener
{
public:

	LambdaBroadcaster<Processor*, Processor*> rootBroadcaster;

	ProcessorEditorContainer() {};

	~ProcessorEditorContainer();

	void processorDeleted(Processor* /*deletedProcessor*/) override
	{
		if (deleteCallback)
			deleteCallback();
	}

	std::function<void()> deleteCallback;

	void updateChildEditorList(bool forceUpdate) override;

	/** Call this whenever you change the size of a child component */
	void refreshSize(bool processorAmountChanged=false);

	void resized();

	void setRootProcessorEditor(Processor *p);

	void addSoloProcessor(Processor *);

	void removeSoloProcessor(Processor *p, bool removeAllChildProcessors=false);

	static int getWidthForIntendationLevel(int intentationLevel)
	{
		return CONTAINER_WIDTH - (intentationLevel * INTENDATION_WIDTH*2);
	};

	ProcessorEditor *getRootEditor() { return rootProcessorEditor; };
	void deleteProcessorEditor(const Processor * processorToBeRemoved);

	ProcessorEditor *getFirstEditorOf(const Processor *p);

	void clearSoloProcessors()
	{
		soloedProcessors.clear();
	}

private:

	ProcessorEditor* searchInternal(ProcessorEditor *editorToSearch, const Processor *p);

	ScopedPointer<ProcessorEditor> rootProcessorEditor;

	OwnedArray<ProcessorEditor> soloedProcessors;
};

class ProcessorEditor;
class ProcessorEditorHeader;
class ProcessorEditorChainBar;
class ProcessorEditorPanel;

class ProcessorEditor : public Component,
							  public SafeChangeListener,
							  public DragAndDropTarget,
							  public CopyPasteTarget,
							  public Dispatchable,
							  public ComponentWithDocumentation
{
public:

	ProcessorEditor(ProcessorEditorContainer *rootContainer, int intendationLevel, Processor *p, ProcessorEditor *parentEditor);

	virtual ~ProcessorEditor()
	{
		// The Editor must be destroyed before the Processor!
		jassert(processor != nullptr);

		if (processor != nullptr)
		{
			processor->removeChangeListener(this);
		}

		header = nullptr;
		body = nullptr;
		panel = nullptr;

	};

	static void deleteProcessorFromUI(Component* c, Processor* pToDelete);

    static void createProcessorFromPopup(Component* editorIfPossible, Processor* parentChainProcessor, Processor* insertBeforeSibling);
    
	static void showContextMenu(Component* c, Processor* p);

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;

	void itemDragEnter(const SourceDetails &dragSourceDetails) override;
		
	void itemDragExit(const SourceDetails &dragSourceDetails) override;

	void itemDropped(const SourceDetails &dragSourceDetails) override;;

	ProcessorEditorPanel *getDragChainPanel();

	MarkdownLink getLink() const override
	{
		return ProcessorHelpers::getMarkdownLink(getProcessor());
	}

	

	ProcessorEditorContainer *getRootContainer()
	{
		return rootContainer.getComponent();
	};

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	/** Resizes itself and sends a message to the root container. */
	void sendResizedMessage()
	{
		if (isPopupMode) return;

		if (header == nullptr || body == nullptr || panel == nullptr || chainBar == nullptr) return;

		const bool isPopupEditor = rootContainer.getComponent() == nullptr;

		if (!isPopupEditor)
		{
			setResizeFlag();
			getRootContainer()->refreshSize(false);
		}
		else
		{
			setSize(ProcessorEditorContainer::getWidthForIntendationLevel(0), getActualHeight());
		}
	}

	int getActualHeight() const;
	
	void setResizeFlag() noexcept { resizeFlag = true; };

	bool shouldBeResized() const noexcept 
	{ 
		return resizeFlag; 
	};

	void resized() override;

	void refreshEditorSize()
	{
		if (shouldBeResized())
		{
			setSize(ProcessorEditorContainer::getWidthForIntendationLevel(intendationLevel), getActualHeight());

			getRootContainer()->refreshSize(false);

			resizeFlag = false;
		}
	}

	void paint(Graphics &g) override;

	void paintOverChildren(Graphics& g) override;

	void mouseDown(const MouseEvent& event) override
	{
		if (event.mods.isRightButtonDown())
			CopyPasteTarget::dismissCopyAndPasteFocus();
		else
			CopyPasteTarget::grabCopyAndPasteFocus();
	}

	String getObjectTypeName() { return getProcessor()->getId(); };
	void copyAction();
	void pasteAction();

	Processor *getProcessor() { return processor.get(); };

	const Processor *getProcessor() const { return processor.get(); };

	void setIsPopup(bool shouldBePopup) noexcept
	{
		isPopupMode = shouldBePopup;
		setFolded(false, sendNotification);
	};

	bool isPopup() const noexcept 
	{ 
		return isPopupMode; 
		
	};

	int getIndentationLevel() const { return intendationLevel; };

	ProcessorEditorBody *getBody() { return body; };
	ProcessorEditorHeader *getHeader() { return header; };
	ProcessorEditorChainBar *getChainBar() { return chainBar; };
	ProcessorEditorPanel *getPanel() { return panel; };
	const ProcessorEditor *getParentEditor() const { return parentEditor.getComponent(); };
	ProcessorEditor *getParentEditor() { return parentEditor.getComponent(); };

	bool isRootEditor() const 
	{ 
		if (rootContainer.getComponent() != nullptr)
		{
			return rootContainer.getComponent()->getRootEditor() == this;
		}
		return false;
	};

	const Chain *getProcessorAsChain() const;
	Chain *getProcessorAsChain();

	void setFolded(bool shouldBeFolded, bool notifyEditor);
	void childEditorAmountChanged() const;

	class Iterator
	{
	public:

		Iterator(ProcessorEditor *rootEditor):
			index(0)
		{
			if(rootEditor != nullptr) addChildEditors(rootEditor);
		}

		ProcessorEditor *getNextEditor()
		{
			if (index < editors.size())
			{
				return editors[index++].getComponent();
			}

			return nullptr;
		}

	private:

		void addChildEditors(ProcessorEditor *editor);

		int index;
		Array<Component::SafePointer<ProcessorEditor>> editors;
	};

private:

	struct ConnectData
	{
		Rectangle<float> area;
		Colour c;
	};

	Array<ConnectData> connectPositions;

	bool isPopupMode;

	WeakReference<Processor> processor;

	ScopedPointer<ProcessorEditorHeader> header;
	ScopedPointer<ProcessorEditorChainBar> chainBar;
	ScopedPointer<ProcessorEditorBody> body;
	ScopedPointer<ProcessorEditorPanel> panel;

	Component::SafePointer<ProcessorEditorContainer> rootContainer;

	Component::SafePointer<ProcessorEditor> parentEditor;

	int intendationLevel;

	bool resizeFlag;

};




/** a base class for all components that can be put in a ProcessorEditor.
*	@ingroup dspEditor
*
*
*/
class ProcessorEditorChildComponent: public Component
{
public:

	/** Creates a new child component. */
	ProcessorEditorChildComponent(ProcessorEditor*editor);;

	virtual ~ProcessorEditorChildComponent()
	{
		masterReference.clear();
	};

	/** Returns a pointer to the processor. */
	Processor *getProcessor() { return processor.get(); };

	/** Returns a const pointer to the processor. */
	const Processor *getProcessor() const { return processor.get(); };

protected:

	/** Returns a const pointer to the editor if the component is initialized. */
	const ProcessorEditor *getEditor() const { return parentEditor.getComponent(); };

	/** Returns a pointer to the editor. */
	ProcessorEditor *getEditor() { return parentEditor.getComponent(); };

	
	
	/** Small helper function, that toggles a button and returns the new toggle value. */
	static bool toggleButton(Button *b)
	{
		bool on = b->getToggleState();
		b->setToggleState(!on, dontSendNotification);
		return on;
	};

private:
	
	friend class WeakReference<ProcessorEditorChildComponent>;
	WeakReference<ProcessorEditorChildComponent>::Master masterReference;

	Component::SafePointer<ProcessorEditor> parentEditor;

	WeakReference<Processor> processor;
};


class ProcessorEditorPanel : public ProcessorEditorChildComponent,
							 public Processor::DeleteListener
{
public:

	ProcessorEditorPanel(ProcessorEditor *parent);

	void processorDeleted(Processor* deletedProcessor) override;

	void addProcessorEditor(Processor *p);

	void removeProcessorEditor(Processor *p);

	void refreshSize();

	void resized() override;

	void paintOverChildren(Graphics& g) override;

	int getHeightOfAllEditors() const;

	ProcessorEditor *getChildEditor(int index)
	{
		return editors[index];
	}

	int getNumChildEditors() const { return editors.size(); };

	void updateChildEditorList(bool forceUpdate=false) override;

	void refreshChildProcessorVisibility();
	void setInsertPosition(int position);
private:

	
	int currentPosition;
	OwnedArray<ProcessorEditor> editors;
};


//	=============================================================================================================
/** A ProcessorEditorBody is an interface class that can be used to create a custom body for a certain processor. 
*	@ingroup dspEditor
*
*	If a ProcessorEditor wants to use a body, it must pass a subclass object of this in its constructor.
*/
class ProcessorEditorBody: public ProcessorEditorChildComponent
{
public:

	ProcessorEditorBody(ProcessorEditor *parentEditor):
		ProcessorEditorChildComponent(parentEditor)
	{};

	virtual ~ProcessorEditorBody() {};

	/** Overwrite this and update all gui elements. 
	*
	*	This is called asynchronously whenever the processor's setOutputValue() or setAttribute() are called.
	*/
	virtual void updateGui() = 0;
	
	/** Call this whenever you want to resize the editor from eg. a button press. 
	*
	*	It checks if the Body should be displayed.
	*/
	void refreshBodySize()
	{ 
		ProcessorEditor *parent = dynamic_cast<ProcessorEditor*>(getParentComponent());

		if (parent != nullptr)
		{
			parent->sendResizedMessage();
		}
	};

	/** Overwrite this and return the height of the body. 
	*
	*	This value can change, but whenever you change it, call refreshBodySize() to let the editor know.
	*/	
	virtual int getBodyHeight() const = 0;

};

/** A empty ProcessorEditorBody that is used by a ProcessorEditor if no ProcessorEditorBody is supplied. */
class EmptyProcessorEditorBody: public ProcessorEditorBody
{
public:

	EmptyProcessorEditorBody(ProcessorEditor *parent):
		ProcessorEditorBody(parent)
	{}

	/** does nothing. */
	void updateGui() override {};

	/** returns 0. */
	int getBodyHeight() const override {return 0;};

};

} // namespace hise

#endif  // PROCESSOREDITOR_H_INCLUDED
