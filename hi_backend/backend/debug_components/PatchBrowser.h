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
					 public Timer,
					 public MainController::ProcessorChangeHandler::Listener
{

public:

	struct MiniPeak : public Component,
					  public PooledUIUpdater::SimpleTimer,
					  public SettableTooltipClient,
					  public SafeChangeListener
	{
		enum class ProcessorType
		{
			Midi,
			Audio,
			Mod
		};

		MiniPeak(Processor* p_);;

		~MiniPeak();

		void changeListenerCallback(SafeChangeBroadcaster*) override
		{
			repaint();
		}

		void mouseDown(const MouseEvent& e) override;

		void paint(Graphics& g) override;

		float getModValue();

		int getPreferredWidth() const;

		void timerCallback() override;

		const bool isMono;
		
		float channelValues[NUM_MAX_CHANNELS];
		int numChannels;
		bool suspended = false;

		ProcessorType type;

		WeakReference<Processor> p;
	};

	// ====================================================================================================================

	void timerCallback() override
	{
		repaint();
	}

	PatchBrowser(BackendRootWindow *window);
	~PatchBrowser();

    struct Factory: public PathFactory
    {
        Path createPath(const String& url) const override;
    };
    
	SET_GENERIC_PANEL_ID("PatchBrowser");

	// ====================================================================================================================

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;;
	void itemDragEnter(const SourceDetails& dragSourceDetails) override;
	void itemDragExit(const SourceDetails& dragSourceDetails) override;
	void itemDragMove(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	void refreshBypassState();

	static void processorChanged(PatchBrowser& pb, Processor* oldProcessor, Processor* newProcessor);

	static void showProcessorInPopup(Component* c, const MouseEvent& e, Processor* p);

	void refreshPopupState();

	void mouseMove(const MouseEvent& e) override;

	void mouseExit(const MouseEvent&) override;

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

	void paintOverChildren(Graphics& g) override;

	void toggleFoldAll();

	void toggleShowChains();

	void buttonClicked(Button *b) override;

    void rebuilt() override;
    
private:

	// ====================================================================================================================

	class ModuleDragTarget : public ButtonListener,
							 public Label::Listener,
                             public SettableTooltipClient
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

		ModuleDragTarget(Processor* p);

		void buttonClicked(Button *b);

		const Processor *getProcessor() const { return p.get(); }
		Processor *getProcessor() { return p.get(); }

		DragState getDragState() const { return dragState; };
		virtual void checkDragState(const SourceDetails& dragSourceDetails);
		virtual void resetDragState();

		void handleRightClick(bool isInEditMode);

		void setDraggingOver(bool isOver);

		bool bypassed = false;

        virtual void applyLayout() = 0;
        
        ScopedPointer<HiseShapeButton> gotoWorkspace;
        
        static void setWorkspace(ModuleDragTarget& p, const Identifier& id, Processor* pr)
        {
            p.gotoWorkspace->setToggleStateAndUpdateIcon(pr == p.getProcessor());
        }
        
		void labelTextChanged(Label *l) override
		{
			if (auto p = getProcessor())
			{
				if (p->getId() != l->getText())
				{
					p->setId(l->getText(), sendNotification);
				}
			}
		}

	protected:

		Label idLabel;

		MiniPeak peak;

		WeakReference<Processor> p;

		void setDragState(DragState newState) { dragState = newState; };
		void drawDragStatus(Graphics &g, Rectangle<float> area);

		void refreshAllButtonStates();
		
		Factory f;

		HiseShapeButton closeButton;

	public:

		HiseShapeButton createButton;

	private:

		void refreshButtonState(ShapeButton *button, bool on);
		
		ScopedPointer<ShapeButton> soloButton;
		ScopedPointer<ShapeButton> hideButton;

		DragState dragState;
        
        Colour colour;
        String id;
        bool itemBypassed;
		bool isOver;
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(ModuleDragTarget);
	};

	static HiseShapeButton* skinWorkspaceButton(Processor* processor);

	// ====================================================================================================================

	class PatchCollection : public SearchableListComponent::Collection,
							public ModuleDragTarget,
							public Processor::BypassListener
	{
	public:

		PatchCollection(ModulatorSynth *synth, int hierarchy, bool showChains);

		~PatchCollection();

		void bypassStateChanged(Processor* p, bool bypassState) override
		{
			if (auto pb = findParentComponentOfClass<PatchBrowser>())
				pb->refreshBypassState();
		}

		void mouseDown(const MouseEvent& e) override;

		void refreshFoldButton();
		void buttonClicked(Button *b) override;

		void repaintChildItems()
		{
			
		}

		void paint(Graphics &g) override;
		void resized() override;

		void mouseEnter(const MouseEvent& e) override
		{
			repaint();
		}

		void mouseExit(const MouseEvent& e) override
		{
			repaint();
		}


        void applyLayout() override;
        
		float getIntendation() const { return (float)hierarchy * 20.0f; }

		Point<int> getPointForTreeGraph(bool getStartPoint) const;

		void checkDragState(const SourceDetails& dragSourceDetails);
		void resetDragState();
		void toggleShowChains();

		void setInPopup(bool isInPopup)
		{
			if (inPopup != isInPopup)
			{
				inPopup = isInPopup;
				repaint();
			}
		}

		

	private:

		Rectangle<int> iconArea;

		bool inPopup = false;
		ScopedPointer<ShapeButton> foldButton;
		int hierarchy;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchCollection)
        JUCE_DECLARE_WEAK_REFERENCEABLE(PatchCollection);
	};

	// ====================================================================================================================

	class PatchItem :  public SearchableListComponent::Item,
					   public ModuleDragTarget,
					   public Processor::BypassListener
	{
	public:

		PatchItem(Processor *p, Processor *parent_, int hierarchy_, const String &searchTerm);
		~PatchItem();

		void bypassStateChanged(Processor* p, bool bypassState) override
		{
			if (auto pb = findParentComponentOfClass<PatchBrowser>())
				pb->refreshBypassState();
		}

        void paint(Graphics& g) override;

		void mouseDown(const MouseEvent& e);

		void mouseEnter(const MouseEvent& e) override
		{
			repaint();
		}

		void mouseExit(const MouseEvent& e) override
		{
			repaint();
		}

		int getPopupHeight() const override	{ return 0; };

		void fillPopupMenu(PopupMenu &m);

		void popupCallback(int menuIndex);

        void applyLayout() override;
        
        
        
		void resized() override;

		
        
		void setInPopup(bool isInPopup)
		{
			if (inPopup != isInPopup)
			{
				inPopup = isInPopup;
				repaint();
			}
		}
        
        Rectangle<int> bypassArea;

	private:

		bool inPopup = false;
        
		
		
        WeakReference<Processor> parent;
        
        

		String lastId;

		int hierarchy;

        uint32 lastMouseDown;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchItem);
        JUCE_DECLARE_WEAK_REFERENCEABLE(PatchItem);
	};

	Component::SafePointer<BackendProcessorEditor> editor;
	Component::SafePointer<BackendRootWindow> rootWindow;

	Component::SafePointer<Component> lastTarget;

	ScopedPointer<HiseShapeButton> addButton;
	ScopedPointer<ShapeButton> foldButton;

	Array<WeakReference<Processor>> popupProcessors;

	bool foldAll;

	bool showChains = false;

	WeakReference<Processor> insertHover;

	// ====================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowser)
	JUCE_DECLARE_WEAK_REFERENCEABLE(PatchBrowser);
};


class AutomationDataBrowser : public SearchableListComponent,
							  public ControlledObject,
							  public ButtonListener
{
public:

	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override;
	} factory;

	using AutomationData = MainController::UserPresetHandler::CustomAutomationData;

	SET_GENERIC_PANEL_ID("AutomationDataBrowser");

	AutomationDataBrowser(BackendRootWindow* bw);

	static void updateList(AutomationDataBrowser& c, bool unused)
	{
		SafeAsyncCall::callAsyncIfNotOnMessageThread<AutomationDataBrowser>(c, [](AutomationDataBrowser& d)
		{
			d.rebuildModuleList(true);
			d.getMainController()->getUserPresetHandler().deferredAutomationListener.addListener(d, updateList, false);
		});
	}

	struct AutomationCollection : public SearchableListComponent::Collection,
								  public ControlledObject,
								  public PooledUIUpdater::SimpleTimer
	{
		struct ConnectionItem : public SearchableListComponent::Item,
								public SafeChangeListener
		{
			ConnectionItem(AutomationData::Ptr d_, AutomationData::ConnectionBase::Ptr c_);

			~ConnectionItem();

			void changeListenerCallback(SafeChangeBroadcaster* b) override
			{
				repaint();
			}

			void paint(Graphics& g) override;

			AutomationData::Ptr d;
			AutomationData::ConnectionBase::Ptr c;
		};

		void paint(Graphics& g) override;

		AutomationCollection(MainController* mc, AutomationData::Ptr data_, int index);
		
		void checkIfChanged(bool rebuildIfChanged);

		void timerCallback() override;

		const int index;
		AutomationData::Ptr data;

		bool hasMidiConnection = false;
		bool hasComponentConnection = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(AutomationCollection);
	};

	void buttonClicked(Button* b) override;

	int getNumCollectionsToCreate() const override;

	Collection* createCollection(int index) override;

	AutomationData::List filteredList;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationDataBrowser);
	JUCE_DECLARE_WEAK_REFERENCEABLE(AutomationDataBrowser);

	ScopedPointer<HiseShapeButton> midiButton, componentButton;
};


} // namespace hise


#endif  // PATCHBROWSER_H_INCLUDED
