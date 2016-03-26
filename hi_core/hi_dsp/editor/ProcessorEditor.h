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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef PROCESSOREDITOR_H_INCLUDED
#define PROCESSOREDITOR_H_INCLUDED


#define SET_ATTRIBUTE_FROM_SLIDER(x) {getProcessor()->setAttribute(x, (float)sliderThatWasMoved->getValue(), dontSendNotification);}



#define CONTAINER_WIDTH 900 - 32
#define INTENDATION_WIDTH 10


class MidiKeyboardFocusTraverser : public KeyboardFocusTraverser
{
	Component *getDefaultComponent(Component *parentComponent) override;
};

class ComponentWithMidiKeyboardTraverser : public Component
{
public:

	KeyboardFocusTraverser *createFocusTraverser() override { return new MidiKeyboardFocusTraverser(); };
};

/** The container that holds all vertically stacked ProcessorEditors. */
class BetterProcessorEditorContainer : public ComponentWithMidiKeyboardTraverser,
									   public SafeChangeBroadcaster
{
public:

	BetterProcessorEditorContainer() {};

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

	BetterProcessorEditor *getRootEditor() { return rootProcessorEditor; };
	void deleteProcessorEditor(const Processor * processorToBeRemoved);

	BetterProcessorEditor *getFirstEditorOf(const Processor *p);

	void clearSoloProcessors()
	{
		soloedProcessors.clear();
	}

private:

	BetterProcessorEditor* searchInternal(BetterProcessorEditor *editorToSearch, const Processor *p);

	ScopedPointer<BetterProcessorEditor> rootProcessorEditor;

	OwnedArray<BetterProcessorEditor> soloedProcessors;
};

class BetterProcessorEditor;
class ProcessorEditorHeader;
class ProcessorEditorChainBar;
class BetterProcessorEditorPanel;

class BetterProcessorEditor : public ComponentWithMidiKeyboardTraverser,
							  public SafeChangeListener,
							  public DragAndDropTarget,

							  public CopyPasteTarget
{
public:

	BetterProcessorEditor(BetterProcessorEditorContainer *rootContainer, int intendationLevel, Processor *p, BetterProcessorEditor *parentEditor);

	virtual ~BetterProcessorEditor()
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

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;

	void itemDragEnter(const SourceDetails &dragSourceDetails) override;
		
	void itemDragExit(const SourceDetails &dragSourceDetails) override;

	void itemDropped(const SourceDetails &dragSourceDetails) override;;

	BetterProcessorEditorPanel *getDragChainPanel();

	BetterProcessorEditorContainer *getRootContainer()
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
			setSize(BetterProcessorEditorContainer::getWidthForIntendationLevel(0), getActualHeight());
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
			setSize(BetterProcessorEditorContainer::getWidthForIntendationLevel(intendationLevel), getActualHeight());

			getRootContainer()->refreshSize(false);

			resizeFlag = false;
		}
	}

	void paint(Graphics &g) override;

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
	BetterProcessorEditorPanel *getPanel() { return panel; };
	const BetterProcessorEditor *getParentEditor() const { return parentEditor.getComponent(); };
	BetterProcessorEditor *getParentEditor() { return parentEditor.getComponent(); };

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
private:

	bool isPopupMode;

	WeakReference<Processor> processor;

	ScopedPointer<ProcessorEditorHeader> header;
	ScopedPointer<ProcessorEditorChainBar> chainBar;
	ScopedPointer<ProcessorEditorBody> body;
	ScopedPointer<BetterProcessorEditorPanel> panel;

	Component::SafePointer<BetterProcessorEditorContainer> rootContainer;

	Component::SafePointer<BetterProcessorEditor> parentEditor;

	int intendationLevel;

	bool resizeFlag;

};




/** a base class for all components that can be put in a ProcessorEditor.
*	@ingroup dspEditor
*
*
*/
class ProcessorEditorChildComponent: public ComponentWithMidiKeyboardTraverser
{
public:

	/** Creates a new child component. */
	ProcessorEditorChildComponent(BetterProcessorEditor*editor);;

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
	const BetterProcessorEditor *getEditor() const { return parentEditor.getComponent(); };

	/** Returns a pointer to the editor. */
	BetterProcessorEditor *getEditor() { return parentEditor.getComponent(); };

	
	
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

	Component::SafePointer<BetterProcessorEditor> parentEditor;

	WeakReference<Processor> processor;
};


class BetterProcessorEditorPanel : public ProcessorEditorChildComponent
{
public:

	BetterProcessorEditorPanel(BetterProcessorEditor *parent);

	void addProcessorEditor(Processor *p);

	void removeProcessorEditor(Processor *p);

	void refreshSize();

	void resized() override;

	void paintOverChildren(Graphics& g) override;

	int getHeightOfAllEditors() const;

	BetterProcessorEditor *getChildEditor(int index)
	{
		return editors[index];
	}

	int getNumChildEditors() const { return editors.size(); };

	void updateChildEditorList();

	void refreshChildProcessorVisibility();
	void setInsertPosition(int position);
private:

	
	int currentPosition;
	OwnedArray<BetterProcessorEditor> editors;
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

	ProcessorEditorBody(BetterProcessorEditor *parentEditor):
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
		BetterProcessorEditor *parentEditor = dynamic_cast<BetterProcessorEditor*>(getParentComponent());

		if (parentEditor != nullptr)
		{
			parentEditor->sendResizedMessage();
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

	EmptyProcessorEditorBody(BetterProcessorEditor *parent):
		ProcessorEditorBody(parent)
	{}

	/** does nothing. */
	void updateGui() override {};

	/** returns 0. */
	int getBodyHeight() const override {return 0;};

};

#endif  // PROCESSOREDITOR_H_INCLUDED
