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

namespace hise { using namespace juce;

// ====================================================================================================================== BETTER STUFF

ProcessorEditor::ProcessorEditor(ProcessorEditorContainer *rootContainer_, int intendationLevel_, Processor *p, ProcessorEditor *parentEditor_):
rootContainer(rootContainer_),
intendationLevel(intendationLevel_),
processor(p),
parentEditor(parentEditor_),
isPopupMode(false)
{
	processor->addChangeListener(this);

	addAndMakeVisible(header = new ProcessorEditorHeader(this));
 	addAndMakeVisible(body = p->createEditor(this));
	
	addAndMakeVisible(panel = new ProcessorEditorPanel(this));
	addAndMakeVisible(chainBar = new ProcessorEditorChainBar(this));

	header->addMouseListener(this, false);
	body->addMouseListener(this, false);

    //setOpaque(true);
	

	setSize(ProcessorEditorContainer::getWidthForIntendationLevel(intendationLevel), getActualHeight());

	setInterceptsMouseClicks(true, true);

	header->update(true);
	body->updateGui();
}

ProcessorEditor::~ProcessorEditor()
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

}

MarkdownLink ProcessorEditor::getLink() const
{
	return ProcessorHelpers::getMarkdownLink(getProcessor());
}

ProcessorEditorContainer* ProcessorEditor::getRootContainer()
{
	return rootContainer.getComponent();
}

void ProcessorEditor::sendResizedMessage()
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

void ProcessorEditor::setResizeFlag() noexcept
{ resizeFlag = true; }

bool ProcessorEditor::shouldBeResized() const noexcept
{ 
	return resizeFlag; 
}

void ProcessorEditor::refreshEditorSize()
{
	if (shouldBeResized())
	{
		setSize(ProcessorEditorContainer::getWidthForIntendationLevel(intendationLevel), getActualHeight());

		getRootContainer()->refreshSize(false);

		resizeFlag = false;
	}
}

void ProcessorEditor::mouseDown(const MouseEvent& event)
{
	if (event.mods.isRightButtonDown())
		CopyPasteTarget::dismissCopyAndPasteFocus();
	else
		CopyPasteTarget::grabCopyAndPasteFocus();
}

String ProcessorEditor::getObjectTypeName()
{ return getProcessor()->getId(); }

Processor* ProcessorEditor::getProcessor()
{ return processor.get(); }

const Processor* ProcessorEditor::getProcessor() const
{ return processor.get(); }

void ProcessorEditor::setIsPopup(bool shouldBePopup) noexcept
{
	isPopupMode = shouldBePopup;
	setFolded(false, sendNotification);
}

bool ProcessorEditor::isPopup() const noexcept
{ 
	return isPopupMode; 
		
}

int ProcessorEditor::getIndentationLevel() const
{ return intendationLevel; }

ProcessorEditorBody* ProcessorEditor::getBody()
{ return body; }

ProcessorEditorHeader* ProcessorEditor::getHeader()
{ return header; }

ProcessorEditorChainBar* ProcessorEditor::getChainBar()
{ return chainBar; }

ProcessorEditorPanel* ProcessorEditor::getPanel()
{ return panel; }

const ProcessorEditor* ProcessorEditor::getParentEditor() const
{ return parentEditor.getComponent(); }

ProcessorEditor* ProcessorEditor::getParentEditor()
{ return parentEditor.getComponent(); }

bool ProcessorEditor::isRootEditor() const
{ 
	if (rootContainer.getComponent() != nullptr)
	{
		return rootContainer.getComponent()->getRootEditor() == this;
	}
	return false;
}

ProcessorEditor::Iterator::Iterator(ProcessorEditor* rootEditor):
	index(0)
{
	if(rootEditor != nullptr) addChildEditors(rootEditor);
}

ProcessorEditor* ProcessorEditor::Iterator::getNextEditor()
{
	if (index < editors.size())
	{
		return editors[index++].getComponent();
	}

	return nullptr;
}


void ProcessorEditor::changeListenerCallback(SafeChangeBroadcaster *b)
{
	// the ChangeBroadcaster must be the connected Processor!

	if (b != getProcessor())
	{
		jassertfalse;
		return;
	}

	if (header != nullptr) header->update(false);
	if (body != nullptr)  body->updateGui();
}


bool ProcessorEditor::isInterestedInDragSource(const SourceDetails & dragSourceDetails)
{	
#if USE_BACKEND

	if (File::isAbsolutePath(dragSourceDetails.description.toString()))
	{
		return File(dragSourceDetails.description).getFileExtension() == ".hip";
	}

	return false;

#else

	return false;

#endif
}

void ProcessorEditor::itemDragEnter(const SourceDetails &dragSourceDetails)
{
	

	

	if (isInterestedInDragSource(dragSourceDetails))
	{

		Chain *chain = getProcessorAsChain();

		if (chain == nullptr)
		{
			chain = getParentEditor()->getProcessorAsChain();
		}

		jassert(chain != nullptr);

		int size = chain->getHandler()->getNumProcessors();
		int position = INT_MAX;

		for (int i = 0; i < size; i++)
		{
			if (chain->getHandler()->getProcessor(i) == getProcessor())
			{
				position = i;
				break;
			}
		}

		getDragChainPanel()->setInsertPosition(position);
		
		
		

	}
	else
	{
		getDragChainPanel()->setInsertPosition(-1);
		
	}
}

void ProcessorEditor::itemDragExit(const SourceDetails &dragSourceDetails)
{
	ModuleBrowser::ModuleItem *dragSource = dynamic_cast<ModuleBrowser::ModuleItem*>(dragSourceDetails.sourceComponent.get());

	if (dragSource != nullptr)
	{
		dragSource->setDragState(ModuleBrowser::ModuleItem::Inactive);
	
	}

	getDragChainPanel()->setInsertPosition(-1);
}

void ProcessorEditor::itemDropped(const SourceDetails &dragSourceDetails)
{
	Chain *chain = getProcessorAsChain();

	ProcessorEditor *editorToUse = this;

	if (chain == nullptr)
	{
		editorToUse = getParentEditor();
		chain = editorToUse->getProcessorAsChain();

		
	}

	ModuleBrowser::ModuleItem *dragSource = dynamic_cast<ModuleBrowser::ModuleItem*>(dragSourceDetails.sourceComponent.get());

	Processor *newProcessor = nullptr;

	if (dragSource != nullptr)
	{
		String name = dragSourceDetails.description.toString().fromLastOccurrenceOf("::", false, false);
		String id = dragSourceDetails.description.toString().upToFirstOccurrenceOf("::", false, false);

		newProcessor = getProcessor()->getMainController()->createProcessor(chain->getFactoryType(), id, name);

		dragSource->setDragState(ModuleBrowser::ModuleItem::Inactive);
	}
	else
	{
		const bool isMainSynthChain = getProcessor() == getProcessor()->getMainController()->getMainSynthChain();

		Component *targetComponent = getComponentAt(dragSourceDetails.localPosition);

		const bool isHeader = (dynamic_cast<ProcessorEditorHeader*>(targetComponent) != nullptr) || (targetComponent->findParentComponentOfClass<ProcessorEditorHeader>() != nullptr);

		if (isMainSynthChain && isHeader)
		{
			if (PresetHandler::showYesNoWindow("Replace Root Container", "Do you want to replace the root container with the preset?"))
				findParentComponentOfClass<BackendProcessorEditor>()->loadNewContainer(File(dragSourceDetails.description.toString()));
			
			return;
		}
		else
		{
			newProcessor = PresetHandler::loadProcessorFromFile(File(dragSourceDetails.description.toString()), editorToUse->getProcessor());
		}
	}

	if (newProcessor != nullptr)
	{
		chain->getHandler()->add(newProcessor, editorToUse == this ? nullptr : getProcessor());

		editorToUse->changeListenerCallback(editorToUse->getProcessor());

		editorToUse->childEditorAmountChanged();

		editorToUse->getPanel()->setInsertPosition(-1);
	}
	else
	{
		PresetHandler::showMessageWindow("Error at loading preset file", "The preset can't be dropped here.", PresetHandler::IconType::Error);
	}

	
	
}

ProcessorEditorPanel * ProcessorEditor::getDragChainPanel()
{
	if (getProcessorAsChain() != nullptr) return panel;
	else return getParentEditor()->getPanel();
}

int ProcessorEditor::getActualHeight() const
{

	if (getParentEditor() != nullptr && getProcessor()->getEditorState(Processor::Folded))
	{
		return header->getHeight() + 6;
	}
	else
	{
		int h = header->getHeight() + 6;

		h += chainBar->getActualHeight();
		
		if (getProcessor()->getEditorState(Processor::BodyShown))
		{
			h += body->getBodyHeight();
		}

		panel->refreshChildProcessorVisibility();

#if HISE_IOS
		const int marginBeforePanel = 6;
#else
		const int marginBeforePanel = 0;
#endif

		h += panel->getHeightOfAllEditors() + marginBeforePanel;

        return h + (dynamic_cast<const JavascriptProcessor*>(getProcessor()) ? 0 : 6);
	}
}

void ProcessorEditor::resized()
{
	header->setBounds(0, 0, getWidth(), header->getHeight());
	chainBar->setBounds(0, header->getBottom() + 6, getWidth(), chainBar->getActualHeight());
	
#if HISE_IOS
	const int marginBeforePanel = 6;
#else
	const int marginBeforePanel = 0;
#endif

	if (isPopup())
	{
		body->setBounds(0, chainBar->getBottom(), getWidth(), getHeight() - chainBar->getBottom());
		
	}
	else
	{
		body->setBounds(0, chainBar->getBottom(), getWidth(), getProcessor()->getEditorState(Processor::BodyShown) ? body->getBodyHeight() : 0);
		panel->setBounds(INTENDATION_WIDTH, body->getBottom() + marginBeforePanel, panel->getWidth(), panel->getHeightOfAllEditors());
	}

	connectPositions.clear();

	if (auto ped = dynamic_cast<ProcessorWithExternalData*>(getProcessor()))
	{
		Component::callRecursive<ComplexDataUIBase::EditorBase>(getBody(), [this, ped](ComplexDataUIBase::EditorBase* t)
			{
				if (auto te = dynamic_cast<TableEditor*>(t))
				{
					for (int i = 0; i < ped->getNumDataObjects(ExternalData::DataType::Table); i++)
					{
						if (ped->getTable(i) == te->getEditedTable())
						{
							auto c = ped->getSharedReferenceColour(ExternalData::DataType::Table, i);
							if (!c.isTransparent())
							{
								this->connectPositions.add({ getLocalArea(te, te->getLocalBounds()).toFloat(), c });
								return false;
							}
						}
					}
				}
				if (auto sp = dynamic_cast<SliderPack*>(t))
				{
					for (int i = 0; i < ped->getNumDataObjects(ExternalData::DataType::SliderPack); i++)
					{
						if (ped->getSliderPack(i) == sp->getData())
						{
							auto c = ped->getSharedReferenceColour(ExternalData::DataType::SliderPack, i);
							if (!c.isTransparent())
							{
								this->connectPositions.add({ getLocalArea(sp, sp->getLocalBounds()).toFloat(), c });
								return false;
							}
						}
					}
				}

				if (auto af = dynamic_cast<MultiChannelAudioBufferDisplay*>(t))
				{
					for (int i = 0; i < ped->getNumDataObjects(ExternalData::DataType::AudioFile); i++)
					{
						if (ped->getAudioFile(i) == af->getBuffer())
						{
							auto c = ped->getSharedReferenceColour(ExternalData::DataType::AudioFile, i);
							if (!c.isTransparent())
							{
								this->connectPositions.add({ getLocalArea(af, af->getLocalBounds()).toFloat(), c });
								return false;
							}
						}
					}
				}

				return false;
			});
	}

}



void ProcessorEditor::copyAction()
{
	PresetHandler::copyProcessorToClipboard(getProcessor());
}
void ProcessorEditor::pasteAction()
{
	if (getProcessorAsChain() != nullptr)
	{
		auto xml = XmlDocument::parse(SystemClipboard::getTextFromClipboard());

		if (xml != nullptr)
		{
			String typeName = xml->getStringAttribute("Type");

			if (typeName.isNotEmpty())
			{
				FactoryType *t = getProcessorAsChain()->getFactoryType();

				Component::SafePointer<ProcessorEditor> safeEditor = this;

				if(t->allowType(Identifier(typeName)))
				{
					auto f = [safeEditor](Processor* p)
					{
						Processor *c = PresetHandler::createProcessorFromClipBoard(p);

						dynamic_cast<Chain*>(p)->getHandler()->add(c, nullptr);

						PresetHandler::setUniqueIdsForProcessor(c);

						WeakReference<Processor> safeP = p;

						auto f2 = [safeEditor]()
						{
							if(safeEditor.getComponent() != nullptr)
								safeEditor.getComponent()->childEditorAmountChanged();

							BACKEND_ONLY(GET_BACKEND_ROOT_WINDOW(safeEditor.getComponent())->sendRootContainerRebuildMessage(false));
						};

						MessageManager::callAsync(f2);

						return SafeFunctionCall::OK;
					};

					getProcessor()->getMainController()->getKillStateHandler().killVoicesAndCall(getProcessor(), f, MainController::KillStateHandler::SampleLoadingThread);
				}
			}
		}
	}
}


void ProcessorEditor::paint(Graphics &g)
{
	if (getProcessor() == nullptr)
		return;

	Colour c = getProcessor()->getColour();
    const float z = (float)getIndentationLevel() * 0.04f;
    c = c.withMultipliedBrightness(1.0f + z);

	if (isSelectedForCopyAndPaste()) c = getProcessorAsChain() ? c.withMultipliedBrightness(1.05f) : c.withMultipliedBrightness(1.05f);

	if (dynamic_cast<BackendRootWindow*>(getParentComponent()) != nullptr)
	{
		// prevent screenshots from being transparent:
		g.fillAll(Colour(0xFF282828));
	}
    
    if(dynamic_cast<ModulatorSynth*>(getProcessor()) ||
       dynamic_cast<Chain*>(getProcessor()) )
    {
        g.setColour(c);
    }
    else
    {
		float alpha = 0.1f;

		if (!dynamic_cast<ProcessorEditorPanel*>(getParentComponent()))
			alpha += 0.15f;

        c = c.withAlpha(alpha);
        
		g.setColour(c);

		/*
        g.setGradientFill(ColourGradient(c.withMultipliedBrightness(1.02f),
                                         0.0f, yOffset,
                                         c.withMultipliedBrightness(0.98f).withAlpha(0.1f),
                                         0.0f, jmax(30.0f, (float)getHeight()),
                                         false));
										 */

        g.fillAll();

		auto b = getLocalBounds().removeFromTop(header->getHeight() + JUCE_LIVE_CONSTANT_OFF(5)).removeFromBottom(10).toFloat();

		g.setGradientFill(ColourGradient(c.withAlpha(0.4f),
			0.0f, b.getY(),
			Colours::transparentBlack,
				0.0f, b.getBottom(),
			false));
		
		g.fillRect(b.reduced(1.0f, 0.0f));
    }

    //g.setColour(Colours::red);
	//g.fillAll();
        
    
    //Colour lineColour = isSelectedForCopyAndPaste() ? Colours::white : Colours::black;
        
    Colour lineColour = Colour(0xFF111111);
    
    if(dynamic_cast<Chain*>(getProcessor()))
    {
        lineColour = getProcessor()->getColour().withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(0.85f));
    }
    
    if(dynamic_cast<ModulatorSynth*>(getProcessor()))
    {
        lineColour = Colour(0xff5a5959);
    }
    
    
    
    g.setColour(lineColour);
    
    if(dynamic_cast<Chain*>(getProcessor()) && !dynamic_cast<ModulatorSynth*>(getProcessor()))
    {
        g.fillRoundedRectangle(0.0f, 0.0f, 3.0f, (float)getHeight(), 1.5f);

		g.setColour(lineColour.withAlpha(0.1f));
		g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);

    }
    else
    {
        g.drawLine(0.0f, 0.f, 0.f, (float)getHeight());
        g.drawLine((float)getWidth(), 0.0f, (float)getWidth(), (float)getHeight());
           
        g.drawLine(0.0f, (float)getHeight(), (float)getWidth(), (float)getHeight());
    }
        
	
}


void ProcessorEditor::paintOverChildren(Graphics& g)
{
	CopyPasteTarget::paintOutlineIfSelected(g);

	Random r;
	
	if (!connectPositions.isEmpty())
	{
		static const unsigned char pathData[] = { 110,109,76,183,106,66,20,174,165,65,108,244,253,60,66,20,174,165,65,98,51,179,47,66,4,86,106,65,145,237,21,66,225,122,40,65,20,174,240,65,225,122,40,65,98,4,86,154,65,225,122,40,65,225,122,40,65,4,86,154,65,225,122,40,65,20,174,240,65,98,225,122,40,65,
18,131,35,66,4,86,154,65,98,144,70,66,20,174,240,65,98,144,70,66,98,145,237,21,66,98,144,70,66,51,179,47,66,154,25,54,66,244,253,60,66,16,216,29,66,108,76,183,106,66,16,216,29,66,98,164,240,90,66,133,235,77,66,2,171,45,66,27,175,112,66,20,174,240,65,
27,175,112,66,98,45,178,87,65,27,175,112,66,0,0,0,0,143,194,58,66,0,0,0,0,20,174,240,65,98,0,0,0,0,45,178,87,65,45,178,87,65,0,0,0,0,20,174,240,65,0,0,0,0,98,2,171,45,66,0,0,0,0,164,240,90,66,61,10,11,65,76,183,106,66,20,174,165,65,99,109,90,36,155,66,
20,174,165,65,108,174,71,132,66,20,174,165,65,98,2,43,140,66,61,10,11,65,211,205,162,66,0,0,0,0,207,119,189,66,0,0,0,0,98,14,173,222,66,0,0,0,0,84,163,249,66,45,178,87,65,84,163,249,66,20,174,240,65,98,84,163,249,66,143,194,58,66,14,173,222,66,27,175,
112,66,207,119,189,66,27,175,112,66,98,211,205,162,66,27,175,112,66,2,43,140,66,133,235,77,66,174,71,132,66,16,216,29,66,108,90,36,155,66,16,216,29,66,98,186,201,161,66,154,25,54,66,139,172,174,66,98,144,70,66,207,119,189,66,98,144,70,66,98,211,13,211,
66,98,144,70,66,248,147,228,66,18,131,35,66,248,147,228,66,20,174,240,65,98,248,147,228,66,4,86,154,65,211,13,211,66,225,122,40,65,207,119,189,66,225,122,40,65,98,139,172,174,66,225,122,40,65,186,201,161,66,236,81,106,65,90,36,155,66,20,174,165,65,99,
109,133,171,190,66,63,53,205,65,98,12,2,196,66,63,53,205,65,141,87,200,66,68,139,222,65,141,87,200,66,96,229,243,65,98,141,87,200,66,190,159,4,66,12,2,196,66,193,74,13,66,133,171,190,66,193,74,13,66,108,135,22,237,65,193,74,13,66,98,106,188,215,65,193,
74,13,66,102,102,198,65,190,159,4,66,102,102,198,65,96,229,243,65,98,102,102,198,65,68,139,222,65,106,188,215,65,63,53,205,65,135,22,237,65,63,53,205,65,108,133,171,190,66,63,53,205,65,99,101,0,0 };

		Path p;
		p.loadPathFromData(pathData, sizeof(pathData));

		for (auto r : connectPositions)
		{
			PathFactory::scalePath(p, r.area.withWidth(24.0f).withHeight(24.0f).reduced(5.0f));

			g.setColour(Colours::black.withAlpha(0.9f));
			g.strokePath(p, PathStrokeType(2.0f));
			g.setColour(r.c.withAlpha(1.0f).withSaturation(0.7f).withBrightness(0.8f));
			g.fillPath(p);
		}
	}
}

const Chain * ProcessorEditor::getProcessorAsChain() const { return dynamic_cast<const Chain*>(getProcessor()); }
Chain * ProcessorEditor::getProcessorAsChain() { return dynamic_cast<Chain*>(getProcessor()); }

void ProcessorEditor::deleteProcessorFromUI(Component* c, Processor* pToDelete)
{
	auto dontAskDelete = dynamic_cast<ModulatorSynth*>(pToDelete) == nullptr && dynamic_cast<JavascriptMidiProcessor*>(pToDelete) == nullptr;

	if (dontAskDelete || PresetHandler::showYesNoWindow("Delete " + pToDelete->getId() + "?", "Do you want to delete the Synth module?"))
	{
		auto brw = GET_BACKEND_ROOT_WINDOW(c);

		auto f = [brw](Processor* p)
		{
			if (auto c = dynamic_cast<Chain*>(p->getParentProcessor(false)))
			{
				c->getHandler()->remove(p, false);
				jassert(!p->isOnAir());
			}

			brw->sendRootContainerRebuildMessage(false);

			return SafeFunctionCall::OK;
		};

		

		pToDelete->getMainController()->getGlobalAsyncModuleHandler().removeAsync(pToDelete, f);
	}
}

void ProcessorEditor::createProcessorFromPopup(Component* editorIfPossible, Processor* parentChainProcessor, Processor* insertBeforeSibling)
{
    Processor *processorToBeAdded = nullptr;
    
    auto c = dynamic_cast<Chain*>(parentChainProcessor);
    
    jassert(c != nullptr);
    FactoryType *t = c->getFactoryType();
    StringArray types;
    bool clipBoard = false;
    int result;

    // =================================================================================================================
    // Create the Popup

    {
		PopupLookAndFeel plaf;
        PopupMenu m;
        
        m.setLookAndFeel(&plaf);

        m.addSectionHeader("Create new Processor ");
        
        t->fillPopupMenu(m);

        m.addSeparator();
        m.addSectionHeader("Add from Clipboard");

        String clipBoardName = PresetHandler::getProcessorNameFromClipboard(t);

        if(clipBoardName != String())  m.addItem(CLIPBOARD_ITEM_MENU_INDEX, "Add " + clipBoardName + " from Clipboard");
        else                                m.addItem(-1, "No compatible Processor in clipboard.", false);

        clipBoard = clipBoardName != String();

        result = m.show();
    }

    // =================================================================================================================
    // Create the processor

    if(result == 0)                                    return;

    else if(result == CLIPBOARD_ITEM_MENU_INDEX && clipBoard)
        processorToBeAdded = PresetHandler::createProcessorFromClipBoard(parentChainProcessor);

    else
    {
        Identifier type = t->getTypeNameFromPopupMenuResult(result);
        String typeName = t->getNameFromPopupMenuResult(result);

        String name = typeName;
        
        if (name.isNotEmpty())
            processorToBeAdded = MainController::createProcessor(t, type, name);
        else return;
    }

    auto brw = GET_BACKEND_ROOT_WINDOW(editorIfPossible);
    auto editor = dynamic_cast<ProcessorEditor*>(editorIfPossible);
    
    auto f = [c, brw, editor, processorToBeAdded, insertBeforeSibling](Processor* p)
    {
        if (ProcessorHelpers::is<ModulatorSynth>(processorToBeAdded) && dynamic_cast<ModulatorSynthGroup*>(c) == nullptr)
            dynamic_cast<ModulatorSynth*>(processorToBeAdded)->addProcessorsWhenEmpty();

        c->getHandler()->add(processorToBeAdded, insertBeforeSibling);
        
        PresetHandler::setUniqueIdsForProcessor(processorToBeAdded);

        MessageManager::callAsync([brw, c, editor, processorToBeAdded]()
        {
            brw->sendRootContainerRebuildMessage(false);
            
            if(editor != nullptr)
            {
                editor->changeListenerCallback(editor->getProcessor());
                editor->childEditorAmountChanged();
            }
           
            brw->gotoIfWorkspace(processorToBeAdded);
            
            PresetHandler::setChanged(dynamic_cast<Processor*>(c));
        });

        return SafeFunctionCall::OK;
    };

    processorToBeAdded->getMainController()->getKillStateHandler().killVoicesAndCall(processorToBeAdded, f, MainController::KillStateHandler::SampleLoadingThread);
    
    return;
}

void ProcessorEditor::showContextMenu(Component* c, Processor* p)
{
	enum
	{
		Copy = 2,
		InsertBefore,
		CloseAllChains,
		ViewXml,
		CheckForDuplicate,
		CreateGenericScriptReference,
		CreateTableProcessorScriptReference,
		CreateAudioSampleProcessorScriptReference,
		CreateSamplerScriptReference,
		CreateMidiPlayerScriptReference,
		CreateSliderPackProcessorReference,
		CreateSlotFXReference,
		CreateRoutingMatrixReference,
		ReplaceWithClipboardContent,
		SaveAllSamplesToGlobalFolder,
		OpenAllScriptsInPopup,
		OpenInterfaceInPopup,
		ConnectToScriptFile,
		ReloadFromExternalScript,
		DisconnectFromScriptFile,
		SaveCurrentInterfaceState,
		numMenuItems
	};

	PopupLookAndFeel plaf;
	PopupMenu m;
	m.setLookAndFeel(&plaf);

	const bool isMainSynthChain = p->getMainController()->getMainSynthChain() == p;

	m.addSectionHeader("Copy Tools");

	m.addItem(Copy, "Copy " + p->getId() + " to Clipboard");
	m.addItem(ViewXml, "Show XML data");

	if ((dynamic_cast<Chain*>(p) == nullptr || dynamic_cast<ModulatorSynth*>(p)) && !isMainSynthChain)
	{
		m.addItem(InsertBefore, "Add Processor before this module", true);
	}

	m.addSeparator();

	m.addItem(CreateGenericScriptReference, "Create generic script reference");

	if (dynamic_cast<ModulatorSampler*>(p) != nullptr)
		m.addItem(CreateSamplerScriptReference, "Create typed Sampler script reference");
	else if (dynamic_cast<MidiPlayer*>(p) != nullptr)
		m.addItem(CreateMidiPlayerScriptReference, "Create typed MIDI Player script reference");
	else if (dynamic_cast<HotswappableProcessor*>(p) != nullptr)
		m.addItem(CreateSlotFXReference, "Create typed SlotFX script reference");

    if(auto ed = dynamic_cast<ExternalDataHolder*>(p))
    {
        m.addItem(CreateTableProcessorScriptReference,
                  "Create typed Table script reference",
                  ed->getNumDataObjects(ExternalData::DataType::Table));
        m.addItem(CreateAudioSampleProcessorScriptReference,
                  "Create typed Audio sample script reference",
                  ed->getNumDataObjects(ExternalData::DataType::AudioFile));
        m.addItem(CreateSliderPackProcessorReference,
                  "Create typed Slider Pack script reference",
                  ed->getNumDataObjects(ExternalData::DataType::SliderPack));
    }
    
	m.addItem(CreateRoutingMatrixReference, "Create typed Routing matrix script reference", dynamic_cast<RoutableProcessor*>(p) != nullptr);

	m.addSeparator();

	if (isMainSynthChain)
	{
		m.addSeparator();
		m.addSectionHeader("Root Container Tools");
		m.addItem(CheckForDuplicate, "Check children for duplicate IDs");
	}

	else if (auto sp = dynamic_cast<JavascriptProcessor*>(p))
	{
		m.addSeparator();
		m.addSectionHeader("Script Processor Tools");
		
		m.addItem(ConnectToScriptFile, "Connect to external script", true, sp->isConnectedToExternalFile());
		m.addItem(ReloadFromExternalScript, "Reload external script", sp->isConnectedToExternalFile(), false);
		m.addItem(DisconnectFromScriptFile, "Disconnect from external script", sp->isConnectedToExternalFile(), false);
	}

	int result = m.show();

	if (result == 0) return;

	if (result == Copy) PresetHandler::copyProcessorToClipboard(p);

	else if (result == InsertBefore)
	{
		createProcessorFromPopup(c, p->getParentProcessor(false), p);
	}
	else if (result == CheckForDuplicate)
	{
		PresetHandler::checkProcessorIdsForDuplicates(p, false);
	}
	else if (result == CreateGenericScriptReference)
		ProcessorHelpers::getScriptVariableDeclaration(p);
	else if (result == CreateAudioSampleProcessorScriptReference)
		ProcessorHelpers::getTypedScriptVariableDeclaration(p, "AudioSampleProcessor");
	else if (result == CreateMidiPlayerScriptReference)
		ProcessorHelpers::getTypedScriptVariableDeclaration(p, "MidiPlayer");
	else if (result == CreateRoutingMatrixReference)
		ProcessorHelpers::getTypedScriptVariableDeclaration(p, "RoutingMatrix");
	else if (result == CreateSamplerScriptReference)
		ProcessorHelpers::getTypedScriptVariableDeclaration(p, "Sampler");
	else if (result == CreateSlotFXReference)
		ProcessorHelpers::getTypedScriptVariableDeclaration(p, "SlotFX");
	else if (result == CreateTableProcessorScriptReference)
		ProcessorHelpers::getTypedScriptVariableDeclaration(p, "TableProcessor");
	else if (result == CreateSliderPackProcessorReference)
		ProcessorHelpers::getTypedScriptVariableDeclaration(p, "SliderPackProcessor");
	else if (result == ConnectToScriptFile)
	{
		FileChooser fc("Select external script", GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Scripts));

		if (fc.browseForFileToOpen())
		{
			File scriptFile = fc.getResult();

			const String scriptReference = GET_PROJECT_HANDLER(p).getFileReference(scriptFile.getFullPathName(), ProjectHandler::SubDirectories::Scripts);

			dynamic_cast<JavascriptProcessor*>(p)->setConnectedFile(scriptReference);
		}
	}
	else if (result == ReloadFromExternalScript)
	{
		dynamic_cast<JavascriptProcessor*>(p)->reloadFromFile();
	}
	if (result == ViewXml)
	{
		auto root = GET_BACKEND_ROOT_WINDOW(c)->getRootFloatingTile();

		auto xml = p->exportAsValueTree().createXml();

		auto nc = new mcl::XmlEditor(File(), xml->createDocument(""));
		nc->setName(p->getId() + " XML Data");
		root->showComponentInRootPopup(nc, c, {0, 0});
	}
	else if (result == DisconnectFromScriptFile)
	{
		if (PresetHandler::showYesNoWindow("Disconnect from script file", "Do you want to disconnect the script from the connected file?\nAny changes you make here won't be saved in the file"))
		{
			dynamic_cast<JavascriptProcessor*>(p)->disconnectFromFile();
		}
	}
	else
	{
		File f = PresetHandler::getPresetFileFromMenu(result - PRESET_MENU_ITEM_DELTA, p);

		if (!f.existsAsFile()) return;

		FileInputStream fis(f);

		ValueTree testTree = ValueTree::readFromStream(fis);

		if (testTree.isValid() && testTree.getProperty("Type") == "SynthChain")
		{
			c->findParentComponentOfClass<BackendProcessorEditor>()->loadNewContainer(testTree);
		}
		else
		{
			PresetHandler::showMessageWindow("Invalid Preset", "The selected Preset file was not a container", PresetHandler::IconType::Error);
		}
	}
}

void ProcessorEditor::setFolded(bool shouldBeFolded, bool notifyEditor)
{
	getProcessor()->setEditorState(Processor::Folded, shouldBeFolded, notifyEditor ? sendNotification : dontSendNotification);
}

void ProcessorEditor::childEditorAmountChanged() const
{
	panel->updateChildEditorList();
}

ProcessorEditorContainer::ProcessorEditorContainer()
{}

void ProcessorEditorContainer::processorDeleted(Processor*)
{
	if (deleteCallback)
		deleteCallback();
}

int ProcessorEditorContainer::getWidthForIntendationLevel(int intentationLevel)
{
	return CONTAINER_WIDTH - (intentationLevel * INTENDATION_WIDTH*2);
}

ProcessorEditor* ProcessorEditorContainer::getRootEditor()
{ return rootProcessorEditor; }

void ProcessorEditorContainer::clearSoloProcessors()
{
	soloedProcessors.clear();
}

ProcessorEditorContainer::~ProcessorEditorContainer()
{
	Processor* oldRoot = nullptr;
	if (rootProcessorEditor != nullptr)
		oldRoot = rootProcessorEditor->getProcessor();

	rootBroadcaster.sendMessage(sendNotificationSync, oldRoot, nullptr);
}

void ProcessorEditorContainer::updateChildEditorList(bool forceUpdate)
{
	rootProcessorEditor->getPanel()->updateChildEditorList(forceUpdate);
	refreshSize();
}

void ProcessorEditorContainer::refreshSize(bool )
{
	int y = 0;

	if (rootProcessorEditor != nullptr)
	{
		y += rootProcessorEditor->getActualHeight();	
	}

	for (int i = 0; i < soloedProcessors.size(); i++)
	{
		y += soloedProcessors[i]->getActualHeight();
	}

	Component::callRecursive<Component>(this, [](Component* c)
	{
		if (auto te = dynamic_cast<TableEditor*>(c))
			te->setScrollWheelEnabled(false);

		if (auto s = dynamic_cast<HiSlider*>(c))
			s->setScrollWheelEnabled(false);

		return false;
	});

	setSize(getWidthForIntendationLevel(0), y);
	resized();
	
}

void ProcessorEditorContainer::resized()
{
	int y = 0;

	if (rootProcessorEditor != nullptr)
	{
		rootProcessorEditor->setBounds(0, 0, getWidthForIntendationLevel(0), rootProcessorEditor->getActualHeight());

		y += rootProcessorEditor->getActualHeight();


	}

	for (int i = 0; i < soloedProcessors.size(); i++)
	{
		soloedProcessors[i]->setBounds(0, y, getWidthForIntendationLevel(0), soloedProcessors[i]->getActualHeight());

		y += soloedProcessors[i]->getActualHeight();
	}

	sendChangeMessage();
}


void ProcessorEditorContainer::setRootProcessorEditor(Processor *p)
{
	Processor* oldRoot = nullptr;

	if (rootProcessorEditor != nullptr)
		oldRoot = rootProcessorEditor->getProcessor();

	addAndMakeVisible(rootProcessorEditor = new ProcessorEditor(this, 0, p, nullptr));

	p->addDeleteListener(this);

	refreshSize(false);

	rootBroadcaster.sendMessage(sendNotificationAsync, oldRoot, p);
}

void ProcessorEditorContainer::addSoloProcessor(Processor *p)
{
	ProcessorEditor *soloEditor = new ProcessorEditor(this, 0, p, nullptr);

	addAndMakeVisible(soloEditor);

	soloedProcessors.add(soloEditor);

	refreshSize(false);
}

void ProcessorEditorContainer::removeSoloProcessor(Processor *p, bool removeAllChildProcessors/*=false*/)
{
	for (int i = 0; i < soloedProcessors.size(); i++)
	{
		if (soloedProcessors[i]->getProcessor() == p)
		{
			soloedProcessors.remove(i);
			break;
		}
	}

	if (removeAllChildProcessors)
	{
		for (int i = 0; i < p->getNumChildProcessors(); i++)
		{
			removeSoloProcessor(p->getChildProcessor(i), true);
		}
	}

	//refreshSize(false);

	

}



ProcessorEditor * ProcessorEditorContainer::getFirstEditorOf(const Processor *p)
{
	return searchInternal(getRootEditor(), p);
}

ProcessorEditor* ProcessorEditorContainer::searchInternal(ProcessorEditor *editorToSearch, const Processor *p)
{
	if (editorToSearch->getProcessor() == p)
	{
		return editorToSearch;
	}
	for (int i = 0; i < editorToSearch->getPanel()->getNumChildEditors(); i++)
	{
		ProcessorEditor * editor = searchInternal(editorToSearch->getPanel()->getChildEditor(i), p);

		if (editor != nullptr) return editor;
	}

	return nullptr;
}

// ===================================================================================================== PANEL

ProcessorEditorPanel::ProcessorEditorPanel(ProcessorEditor *parent) :
ProcessorEditorChildComponent(parent),
currentPosition(-1)
{
	updateChildEditorList();
}

void ProcessorEditorPanel::processorDeleted(Processor* deletedProcessor)
{
	MessageManagerLock lock;

	removeProcessorEditor(deletedProcessor);

	deletedProcessor->removeDeleteListener(this);
}

void ProcessorEditorPanel::addProcessorEditor(Processor *p)
{
	MessageManagerLock lock;

	p->addDeleteListener(this);

	ProcessorEditor *editor = new ProcessorEditor(getEditor()->getRootContainer(), getEditor()->getIndentationLevel() + 1, p, getEditor());
	editors.add(editor);

	refreshSize();

	
}

void ProcessorEditorPanel::removeProcessorEditor(Processor *p)
{
	if (getEditor()->getRootContainer() != nullptr)
	{
		getEditor()->getRootContainer()->sendChangeMessage();
	}

	for (int i = 0; i < editors.size(); i++)
	{
		if (editors[i]->getProcessor() == p)
		{
			editors.remove(i, true);
			//getEditor()->getProcessorAsChain()->getHandler()->remove(p);
			break;
		}
	}

	

	getEditor()->getHeader()->enableChainHeader();

	refreshSize();

	GET_BACKEND_ROOT_WINDOW(this)->sendRootContainerRebuildMessage(false);
}

int ProcessorEditorPanel::getHeightOfAllEditors() const
{
	int y = 0;

	for (int i = 0; i < editors.size(); i++)
	{
		// This must be set correctly with refreshChildProcessorVisiblity()!
		//jassert(editors[i]->getProcessor()->getEditorState(Processor::EditorState::Visible) == editors[i]->isVisible());

        if(editors[i]->getProcessor() == nullptr)
        {
            jassertfalse;
            continue;
        }
        
		if (!editors[i]->getProcessor()->getEditorState(Processor::EditorState::Visible)) continue;

#if HISE_IOS
		y += editors[i]->getActualHeight() + 10;
#else
		y += editors[i]->getActualHeight() + 3;
#endif
	}

	return y;
}

ProcessorEditor* ProcessorEditorPanel::getChildEditor(int index)
{
	return editors[index];
}

int ProcessorEditorPanel::getNumChildEditors() const
{ return editors.size(); }

void ProcessorEditorPanel::refreshChildProcessorVisibility()
{
	Processor *p = getEditor()->getProcessor();

	if (p->getNumChildProcessors() != editors.size()) return;

	for (int i = 0; i < p->getNumChildProcessors(); i++)
	{
		const bool childIsVisible = p->getChildProcessor(i)->getEditorState(Processor::EditorState::Visible);

		editors[i]->setVisible(childIsVisible);
	}
}

void ProcessorEditorPanel::setInsertPosition(int position)
{
	currentPosition = position;
	
	repaint();
}

ProcessorEditorBody::ProcessorEditorBody(ProcessorEditor* parentEditor):
	ProcessorEditorChildComponent(parentEditor)
{}

ProcessorEditorBody::~ProcessorEditorBody()
{}

void ProcessorEditorBody::refreshBodySize()
{ 
	ProcessorEditor *parent = dynamic_cast<ProcessorEditor*>(getParentComponent());

	if (parent != nullptr)
	{
		parent->sendResizedMessage();
	}
}

void ProcessorEditorPanel::updateChildEditorList(bool forceUpdate)
{
	if (!forceUpdate && editors.size() == getProcessor()->getNumChildProcessors())
	{
		getEditor()->getHeader()->enableChainHeader();
		return;
	}

	editors.clear();

	for (int i = 0; i < getProcessor()->getNumChildProcessors(); i++)
	{
		if (i >= editors.size())
		{
			ProcessorEditor *editor = new ProcessorEditor(getEditor()->getRootContainer(), getEditor()->getIndentationLevel() + 1, getProcessor()->getChildProcessor(i), getEditor());

			addAndMakeVisible(editor);

			editors.add(editor);
			getProcessor()->getChildProcessor(i)->addDeleteListener(this);


		}
	}

	getEditor()->getHeader()->enableChainHeader();


	refreshSize();

	resized();

	if (getEditor()->getRootContainer() != nullptr)
	{
		getEditor()->getRootContainer()->sendChangeMessage();
	}
}

void ProcessorEditorPanel::refreshSize()
{
	refreshChildProcessorVisibility();

	setSize(ProcessorEditorContainer::getWidthForIntendationLevel(getEditor()->getIndentationLevel()),
		getHeightOfAllEditors());

	getEditor()->sendResizedMessage();
}


void ProcessorEditorPanel::resized()
{
	if (getHeightOfAllEditors() == 0) return;

	int y = 0;

	for (int i = 0; i < editors.size(); i++)
	{
		if (!editors[i]->getProcessor()->getEditorState(Processor::EditorState::Visible)) continue;

		editors[i]->setBounds(0, y, ProcessorEditorContainer::getWidthForIntendationLevel(editors[i]->getIndentationLevel()) - 4, editors[i]->getActualHeight());

#if HISE_IOS
		y += editors[i]->getActualHeight() + 10;
#else
		y += editors[i]->getActualHeight() + 3;
#endif
	}

}

void ProcessorEditorPanel::paintOverChildren(Graphics& g)
{
	if (currentPosition != -1)
	{
		int y = 0;

		if (currentPosition == INT_MAX)
		{
			y = editors.getLast()->getBottom();
		}
		else
		{
			const int offset = getProcessor()->getNumInternalChains();

			y = editors[currentPosition + offset]->getY();
		}

		g.setColour(Colours::red);

		g.drawLine(0.0f, (float)y, (float)getWidth(), (float)y, 3.0f);
	}
}

ProcessorEditorChildComponent::ProcessorEditorChildComponent(ProcessorEditor*editor) :
parentEditor(editor),
processor(editor->getProcessor())
{

}

ProcessorEditorChildComponent::~ProcessorEditorChildComponent()
{
	masterReference.clear();
}

Processor* ProcessorEditorChildComponent::getProcessor()
{ return processor.get(); }

const Processor* ProcessorEditorChildComponent::getProcessor() const
{ return processor.get(); }

const ProcessorEditor* ProcessorEditorChildComponent::getEditor() const
{ return parentEditor.getComponent(); }

ProcessorEditor* ProcessorEditorChildComponent::getEditor()
{ return parentEditor.getComponent(); }

bool ProcessorEditorChildComponent::toggleButton(Button* b)
{
	bool on = b->getToggleState();
	b->setToggleState(!on, dontSendNotification);
	return on;
}

void ProcessorEditor::Iterator::addChildEditors(ProcessorEditor *editor)
{
	editors.add(editor);

	for (int i = 0; i < editor->getPanel()->getNumChildEditors(); i++)
	{
		addChildEditors(editor->getPanel()->getChildEditor(i));
	}
}

} // namespace hise
