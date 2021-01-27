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

#ifndef PATCHBROWSER_H_INCLUDED
#define PATCHBROWSER_H_INCLUDED

namespace hise { using namespace juce;

class BackendProcessorEditor;

/** A searchable list of the current preset. */
class PatchBrowser : public SearchableListComponent,
					 public DragAndDropTarget,
					 public ButtonListener,
					 public MainController::ProcessorChangeHandler::Listener
{

public:

	// ====================================================================================================================

	PatchBrowser(BackendRootWindow *window);
	~PatchBrowser();

	SET_GENERIC_PANEL_ID("PatchBrowser");

	// ====================================================================================================================

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;;
	void itemDragEnter(const SourceDetails& dragSourceDetails) override;
	void itemDragExit(const SourceDetails& dragSourceDetails) override;
	void itemDragMove(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	void moduleListChanged(Processor* /*changedProcessor*/, MainController::ProcessorChangeHandler::EventType type) override
	{
		if (type == MainController::ProcessorChangeHandler::EventType::ProcessorRenamed ||
			type == MainController::ProcessorChangeHandler::EventType::ProcessorColourChange ||
			type == MainController::ProcessorChangeHandler::EventType::ProcessorBypassed)
		{
			repaintAllItems();
		}
		else
		{
			rebuildModuleList(true);
		}
	}

	int getNumCollectionsToCreate() const override;
	Collection *createCollection(int index) override;

	void paint(Graphics &g) override;

	void toggleFoldAll();

	void toggleShowChains();

	void buttonClicked(Button *b) override;

private:

	// ====================================================================================================================

	class ModuleDragTarget : public ButtonListener
	{
	public:

		enum class DragState
		{
			Inactive = 0,
			Allowed,
			Forbidden,
			numDragStates
		};

		enum class ViewSettings
		{
			ToggleFoldAll = 1,
			ShowChains,
			Visible,
			Solo,
			Root,
			Bypassed,
			Copy,
			CreateScriptVariableDeclaration,
			PasteProcessorFromClipboard,
			numViewSettings
		};

		ModuleDragTarget();

		void buttonClicked(Button *b);

		virtual const Processor *getProcessor() const = 0;
		virtual Processor *getProcessor() = 0;

		DragState getDragState() const { return dragState; };
		virtual void checkDragState(const SourceDetails& dragSourceDetails);
		virtual void resetDragState();

		void setDraggingOver(bool isOver);

	protected:

		void setDragState(DragState newState) { dragState = newState; };
		void drawDragStatus(Graphics &g, Rectangle<float> area);

		void refreshAllButtonStates();
		

	private:

		void refreshButtonState(ShapeButton *button, bool on);
		
		ScopedPointer<ShapeButton> soloButton;
		ScopedPointer<ShapeButton> hideButton;

		

		DragState dragState;
        
        Colour colour;
        String id;
        bool itemBypassed;
		bool isOver;
	};

	// ====================================================================================================================

	class PatchCollection : public SearchableListComponent::Collection,
							public ModuleDragTarget
	{
	public:

		PatchCollection(ModulatorSynth *synth, int hierarchy, bool showChains);

		~PatchCollection();

		void mouseDoubleClick(const MouseEvent& event) override;
		
		void refreshFoldButton();
		void buttonClicked(Button *b) override;

		void repaintChildItems()
		{
			
		}

		void paint(Graphics &g) override;
		void resized() override;

		float getIntendation() const { return (float)hierarchy * 20.0f; }

		juce::Point<int> getPointForTreeGraph(bool getStartPoint) const;

		Processor *getProcessor() override { return root.get(); };
		const Processor *getProcessor() const override { return root.get(); };

		void checkDragState(const SourceDetails& dragSourceDetails);
		void resetDragState();
		void toggleShowChains();
	private:

		ScopedPointer<ShapeButton> foldButton;

		WeakReference<Processor> root;

		int hierarchy;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchCollection)
	};

	// ====================================================================================================================

	class PatchItem :  public SearchableListComponent::Item,
					   public ModuleDragTarget,
                       public Label::Listener
	{
	public:

		PatchItem(Processor *p, Processor *parent_, int hierarchy_, const String &searchTerm);
		~PatchItem();

        void paint(Graphics& g) override;

		void mouseDoubleClick(const MouseEvent& );

		int getPopupHeight() const override	{ return 0; };

		void fillPopupMenu(PopupMenu &m);

		void popupCallback(int menuIndex);

		virtual Processor *getProcessor() override { return processor.get(); };
		virtual const Processor *getProcessor() const override { return processor.get(); };

        void mouseDown(const MouseEvent &e) override
        {
            
            const bool isEditable = dynamic_cast<Chain*>(processor.get()) == nullptr ||
                                    dynamic_cast<ModulatorSynth*>(processor.get()) != nullptr;
            
            if(isEditable && e.mods.isShiftDown())
            {
                idLabel->showEditor();
            }

			Item::mouseDown(e);

        }
        
        void labelTextChanged(Label *l) override
        {
            if(processor.get() != nullptr && processor->getId() != l->getText())
            {
                processor->setId(l->getText(), sendNotification);
            }
        }
        
	private:

		WeakReference<Processor> processor;
        WeakReference<Processor> parent;
        
        ScopedPointer<Label> idLabel;

		String lastId;

		int hierarchy;

        uint32 lastMouseDown;
        
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchItem)
	};

	Component::SafePointer<BackendProcessorEditor> editor;
	Component::SafePointer<BackendRootWindow> rootWindow;

	Component::SafePointer<Component> lastTarget;

	ScopedPointer<ShapeButton> addButton;
	ScopedPointer<ShapeButton> foldButton;

	bool foldAll;

	bool showChains = false;

	// ====================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowser)
};


} // namespace hise


#endif  // PATCHBROWSER_H_INCLUDED
