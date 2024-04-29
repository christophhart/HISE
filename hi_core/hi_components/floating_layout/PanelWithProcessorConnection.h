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
#ifndef PANELWITHPROCESSORCONNECTION_H_INCLUDED
#define PANELWITHPROCESSORCONNECTION_H_INCLUDED


namespace hise { using namespace juce;

class ModulatorSynthChain;


class PanelWithProcessorConnection : public FloatingTileContent,
	public Component,
	public ComboBox::Listener,
	public Processor::DeleteListener,
	public MainController::ProcessorChangeHandler::Listener
{
public:

	enum SpecialPanelIds
	{
		ProcessorId = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		Index,
		FollowWorkspace,
		numSpecialPanelIds
	};

	/** This action will be performed when a processor / index is selected. */
	class ProcessorConnection : public UndoableAction
	{
	public:

		ProcessorConnection(PanelWithProcessorConnection* panel_, Processor* newProcessor_, int newIndex_, var additionalInfo);

		/** Sets the index, the processor and refreshs the content. */
		bool perform() override;

		bool undo() override;

	private:

		Component::SafePointer<PanelWithProcessorConnection> panel;
		WeakReference<Processor> oldProcessor;
		WeakReference<Processor> newProcessor;

		int oldIndex = -1;
		int newIndex = -1;

		var additionalInfo;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorConnection)
	};

	PanelWithProcessorConnection(FloatingTile* parent);
	virtual ~PanelWithProcessorConnection();

	void paint(Graphics& g) override;

	var toDynamicObject() const override;

	void fromDynamicObject(const var& object) override;


	int getNumDefaultableProperties() const override;

	Identifier getDefaultablePropertyId(int index) const override;

	var getDefaultProperty(int index) const override;

	void incIndex(bool up);

	Processor* createDummyProcessorForDocumentation(MainController* mc);

	void moduleListChanged(Processor* b, MainController::ProcessorChangeHandler::EventType type) override;

	void resized() override;

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

    virtual void preSelectCallback(ComboBox* cb) {};
    
	void processorDeleted(Processor* /*deletedProcessor*/);

	/** Overwrite this and return the id of the processor. This is used to prevent resetting with global connector panels. */
	virtual Identifier getProcessorTypeId() const
	{
		return Identifier("unsupported");
	}
	
	bool shouldHideSelector() const;

	virtual var getAdditionalUndoInformation() const { return var(); }

	virtual void performAdditionalUndoInformation(const var& /*undoInformation*/) {};

	void refreshConnectionList();

	void refreshSelector(StringArray &items, String currentId);

    void refreshSelectorValue(Processor* p, String currentId);
    
    void refreshIndexList();

	template <class ContentType> ContentType* getContent() { return dynamic_cast<ContentType*>(content.get()); };

	template <class ContentType> const ContentType* getContent() const { return dynamic_cast<const ContentType*>(content.get()); };

	virtual void updateChildEditorList(bool /*forceUpdate*/) {}

	Processor* getProcessor() { return currentProcessor.get(); }
	const Processor* getProcessor() const { return currentProcessor.get(); }

	/** Use the connected processor for filling the index list (!= the current processor which is shown). */
	Processor* getConnectedProcessor() { return connectedProcessor.get(); }
	const Processor* getConnectedProcessor() const { return connectedProcessor.get(); }

	ModulatorSynthChain* getMainSynthChain();

	const ModulatorSynthChain* getMainSynthChain() const;

	void changeContentWithUndo(int newIndex);

	void setContentWithUndo(Processor* newProcessor, int newIndex);

    void refreshTitle();
	void refreshContent();
	void setContentForIdentifier(Identifier idToSearch);

	virtual Component* createContentComponent(int index) = 0;
	virtual void contentChanged() {};
	virtual void fillModuleList(StringArray& moduleList) = 0;
	virtual void fillIndexList(StringArray& /*indexList*/) {};
	virtual bool hasSubIndex() const { return false; }

	bool showTitleInPresentationMode() const override;
	void setCurrentProcessor(Processor* p);
	void setConnectionIndex(int newIndex);
	int getCurrentIndex() const { return currentIndex; }
	void setForceHideSelector(bool shouldHide);
	virtual bool shouldFollowNewWorkspace(Processor* p, const Identifier& id) const;

protected:

	template <class ProcessorType> void fillModuleListWithType(StringArray& moduleList)
	{
		Processor::Iterator<ProcessorType> iter(getMainSynthChain(), false);

		while (auto p = iter.getNextProcessor())
		{
			moduleList.add(dynamic_cast<Processor*>(p)->getId());
		}
	}

	const Identifier showConnectionBar;

protected:

	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override
		{
			Path p;
			LOAD_PATH_IF_URL("workspace", ColumnIcons::openWorkspaceIcon);
			return p;
		}
	} factory;

	HiseShapeButton followWorkspaceButton;

private:

	bool setContentRecursion = false;

	bool forceHideSelector = false;
	bool listInitialised = false;

	GlobalHiseLookAndFeel hlaf;

	ScopedPointer<ComboBox> connectionSelector;
	ScopedPointer<SubmenuComboBox> indexSelector;
	
	int currentIndex = -1;
	int previousIndex = -1;
	int nextIndex = -1;

	WeakReference<Processor> currentProcessor;
	WeakReference<Processor> connectedProcessor;

	ScopedPointer<Component> content;

	JUCE_DECLARE_WEAK_REFERENCEABLE(PanelWithProcessorConnection);
};

template <class ProcessorType> class GlobalConnectorPanel : public PanelWithProcessorConnection,
															public MainController::LockFreeDispatcher::PresetLoadListener
{
public:
	explicit GlobalConnectorPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{
		getMainController()->getLockFreeDispatcher().addPresetLoadListener(this);
	}

	~GlobalConnectorPanel()
	{
		getMainController()->getLockFreeDispatcher().removePresetLoadListener(this);

	}

	void newHisePresetLoaded() override
	{
		if (auto p = ProcessorHelpers::getFirstProcessorWithType<ProcessorType>(getMainController()->getMainSynthChain()))
		{
			auto idx = getCurrentIndex();
			setContentWithUndo(dynamic_cast<Processor*>(p), idx);
		}
	}

	bool shouldFollowNewWorkspace(Processor* p, const Identifier& id) const override
	{
		return followWorkspaceButton.getToggleState() && dynamic_cast<ProcessorType*>(p) != nullptr;
	}

	void setFollowWorkspace(bool enabled)
	{
		followWorkspaceButton.setToggleStateAndUpdateIcon(enabled);
	}

	Identifier getIdentifierForBaseClass() const override
	{
		return GlobalConnectorPanel<ProcessorType>::getPanelId();
	}

	static Identifier getPanelId()
	{
		String n;

		n << "GlobalConnector" << ProcessorType::getConnectorId().toString();

		return Identifier(n);
	}

	int getFixedHeight() const override { return 18; }

	Identifier getProcessorTypeId() const override
	{
		RETURN_STATIC_IDENTIFIER("Skip");
	}

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	bool hasSubIndex() const override { return false; }

	Component* createContentComponent(int /*index*/) override
	{
		return new Component();
	}

    void contentChanged() override
    {
        Identifier idToSearch = ProcessorType::getConnectorId();
        
        setContentForIdentifier(idToSearch);
    }
    
	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<ProcessorType>(moduleList);
	};

private:

};

} // namespace hise

#endif  // PANELWITHPROCESSORCONNECTION_H_INCLUDED
