
/*
  ==============================================================================

    PatchBrowser.h
    Created: 24 Feb 2016 8:30:25pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PATCHBROWSER_H_INCLUDED
#define PATCHBROWSER_H_INCLUDED

class BackendProcessorEditor;

/** A searchable list of the current preset. */
class PatchBrowser : public SearchableListComponent,
					 public SafeChangeListener,
					 public DragAndDropTarget,
					 public ButtonListener
{

public:

	// ====================================================================================================================

	PatchBrowser(BaseDebugArea *area, BackendProcessorEditor *editor_);
	~PatchBrowser();

	// ====================================================================================================================

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;;
	void itemDragEnter(const SourceDetails& dragSourceDetails) override;
	void itemDragExit(const SourceDetails& dragSourceDetails) override;
	void itemDragMove(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	void changeListenerCallback(SafeChangeBroadcaster *) override { rebuildModuleList(true); }

	int getNumCollectionsToCreate() const override;
	Collection *createCollection(int index) override;

	void paint(Graphics &g) override;

	void toggleFoldAll();

	void toggleShowChains();

	void buttonClicked(Button *b) override;

private:

	// ====================================================================================================================

	class ModuleDragTarget : public ButtonListener,
                             public Timer
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
			numViewSettings
		};

		ModuleDragTarget();

        void timerCallback();
        
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
        bool bypassed;
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

		void paint(Graphics &g) override;
		void resized() override;

		float getIntendation() const { return (float)hierarchy * 20.0f; }

		Point<int> getPointForTreeGraph(bool getStartPoint) const;

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
					   public ModuleDragTarget
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

	private:

		WeakReference<Processor> processor;
		WeakReference<Processor> parent;

		String lastId;

		int hierarchy;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchItem)
	};

	Component::SafePointer<BackendProcessorEditor> editor;

	Component::SafePointer<Component> lastTarget;

	ScopedPointer<ShapeButton> addButton;
	ScopedPointer<ShapeButton> foldButton;

	bool foldAll;

	bool showChains;

	// ====================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowser)
};





#endif  // PATCHBROWSER_H_INCLUDED
