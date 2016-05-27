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


// ====================================================================================================================== BETTER STUFF

BetterProcessorEditor::BetterProcessorEditor(BetterProcessorEditorContainer *rootContainer_, int intendationLevel_, Processor *p, BetterProcessorEditor *parentEditor_):
rootContainer(rootContainer_),
intendationLevel(intendationLevel_),
processor(p),
parentEditor(parentEditor_),
isPopupMode(false)
{
	processor->addChangeListener(this);

	addAndMakeVisible(header = new ProcessorEditorHeader(this));
	addAndMakeVisible(body = p->createEditor(this));
	
	addAndMakeVisible(panel = new BetterProcessorEditorPanel(this));
	addAndMakeVisible(chainBar = new ProcessorEditorChainBar(this));

    setOpaque(true);
	

	setSize(BetterProcessorEditorContainer::getWidthForIntendationLevel(intendationLevel), getActualHeight());

	header->update();
	body->updateGui();
}



void BetterProcessorEditor::changeListenerCallback(SafeChangeBroadcaster *b)
{
	// the ChangeBroadcaster must be the connected Processor!

	if (b != getProcessor())
	{
		jassertfalse;
		return;
	}

	header->update();
	
	body->updateGui();
}


bool BetterProcessorEditor::isInterestedInDragSource(const SourceDetails & dragSourceDetails)
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

void BetterProcessorEditor::itemDragEnter(const SourceDetails &dragSourceDetails)
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

void BetterProcessorEditor::itemDragExit(const SourceDetails &dragSourceDetails)
{
	ModuleBrowser::ModuleItem *dragSource = dynamic_cast<ModuleBrowser::ModuleItem*>(dragSourceDetails.sourceComponent.get());

	if (dragSource != nullptr)
	{
		dragSource->setDragState(ModuleBrowser::ModuleItem::Inactive);
	
	}

	getDragChainPanel()->setInsertPosition(-1);
}

void BetterProcessorEditor::itemDropped(const SourceDetails &dragSourceDetails)
{
	Chain *chain = getProcessorAsChain();

	BetterProcessorEditor *editorToUse = this;

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

BetterProcessorEditorPanel * BetterProcessorEditor::getDragChainPanel()
{
	if (getProcessorAsChain() != nullptr) return panel;
	else return getParentEditor()->getPanel();
}

int BetterProcessorEditor::getActualHeight() const
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

        return h + (dynamic_cast<const ScriptProcessor*>(getProcessor()) ? 0 : 6);
	}
}

void BetterProcessorEditor::resized()
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
	
}



void BetterProcessorEditor::copyAction()
{
	PresetHandler::copyProcessorToClipboard(getProcessor());
}
void BetterProcessorEditor::pasteAction()
{
	if (getProcessorAsChain() != nullptr)
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(SystemClipboard::getTextFromClipboard());

		if (xml != nullptr)
		{
			String typeName = xml->getStringAttribute("Type");

			if (typeName.isNotEmpty())
			{
				FactoryType *t = getProcessorAsChain()->getFactoryType();

				if(t->allowType(Identifier(typeName)))
				{
					Processor *c = PresetHandler::createProcessorFromClipBoard(getProcessor());

					getProcessorAsChain()->getHandler()->add(c, nullptr);

					childEditorAmountChanged();
				}
			}
		}
	}
}


void BetterProcessorEditor::paint(Graphics &g)
{
	const float yOffset = (float)header->getBottom();

	Colour c = getProcessor()->getColour();

	if (isSelectedForCopyAndPaste()) c = getProcessorAsChain() ? c.withMultipliedBrightness(1.05f) : c.withMultipliedBrightness(1.05f);

    if(dynamic_cast<ModulatorSynth*>(getProcessor()))
    {
        g.setColour(c);
    }
    else
    {
        g.setGradientFill(ColourGradient(c.withMultipliedBrightness(1.05f),
                                         0.0f, yOffset,
                                         c.withMultipliedBrightness(0.95f),
                                         0.0f, jmax(30.0f, (float)getHeight()),
                                         false));

    }
    

	g.fillAll(); 
        
    Colour lineColour = isSelectedForCopyAndPaste() ? Colours::white : Colours::black;
        
    g.setColour(lineColour.withAlpha(0.25f));
        
	g.drawLine(0.0f, 0.f, 0.f, (float)getHeight());
	g.drawLine((float)getWidth(), 0.0f, (float)getWidth(), (float)getHeight());
       
    g.drawLine(0.0f, (float)getHeight(), (float)getWidth(), (float)getHeight());

    g.setGradientFill(ColourGradient(Colour(0x6e000000),
                                        0.0f, 27.0f,
                                        Colour(0x00000000),
                                        0.0f, 35.0f,
                                        false));
    g.fillRect(0, (int)yOffset, getWidth(), (int)yOffset);
}


const Chain * BetterProcessorEditor::getProcessorAsChain() const { return dynamic_cast<const Chain*>(getProcessor()); }
Chain * BetterProcessorEditor::getProcessorAsChain() { return dynamic_cast<Chain*>(getProcessor()); }

void BetterProcessorEditor::setFolded(bool shouldBeFolded, bool notifyEditor)
{
	getProcessor()->setEditorState(Processor::Folded, shouldBeFolded, notifyEditor ? sendNotification : dontSendNotification);
}

void BetterProcessorEditor::childEditorAmountChanged() const
{
	panel->updateChildEditorList();
}

void BetterProcessorEditorContainer::refreshSize(bool )
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

	setSize(getWidthForIntendationLevel(0), y);
	resized();
	
}

void BetterProcessorEditorContainer::resized()
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


void BetterProcessorEditorContainer::setRootProcessorEditor(Processor *p)
{
	addAndMakeVisible(rootProcessorEditor = new BetterProcessorEditor(this, 0, p, nullptr));
	refreshSize(false);
}

void BetterProcessorEditorContainer::addSoloProcessor(Processor *p)
{
	BetterProcessorEditor *soloEditor = new BetterProcessorEditor(this, 0, p, nullptr);

	addAndMakeVisible(soloEditor);

	soloedProcessors.add(soloEditor);

	refreshSize(false);
}

void BetterProcessorEditorContainer::removeSoloProcessor(Processor *p, bool removeAllChildProcessors/*=false*/)
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



BetterProcessorEditor * BetterProcessorEditorContainer::getFirstEditorOf(const Processor *p)
{
	return searchInternal(getRootEditor(), p);
}

BetterProcessorEditor* BetterProcessorEditorContainer::searchInternal(BetterProcessorEditor *editorToSearch, const Processor *p)
{
	if (editorToSearch->getProcessor() == p)
	{
		return editorToSearch;
	}
	for (int i = 0; i < editorToSearch->getPanel()->getNumChildEditors(); i++)
	{
		BetterProcessorEditor * editor = searchInternal(editorToSearch->getPanel()->getChildEditor(i), p);

		if (editor != nullptr) return editor;
	}

	return nullptr;
}

// ===================================================================================================== PANEL

BetterProcessorEditorPanel::BetterProcessorEditorPanel(BetterProcessorEditor *parent) :
ProcessorEditorChildComponent(parent),
currentPosition(-1)
{
	updateChildEditorList();
}

void BetterProcessorEditorPanel::addProcessorEditor(Processor *p)
{
	BetterProcessorEditor *editor = new BetterProcessorEditor(getEditor()->getRootContainer(), getEditor()->getIndentationLevel() + 1, p, getEditor());
	editors.add(editor);

	refreshSize();

	
}

void BetterProcessorEditorPanel::removeProcessorEditor(Processor *p)
{
	if (getEditor()->getRootContainer() != nullptr)
	{
		getEditor()->getRootContainer()->removeSoloProcessor(p, true);
		getEditor()->getRootContainer()->sendChangeMessage();
	}

	for (int i = 0; i < editors.size(); i++)
	{
		if (editors[i]->getProcessor() == p)
		{
			editors.remove(i, true);
			getEditor()->getProcessorAsChain()->getHandler()->remove(p);
			break;
		}
	}

	

	getEditor()->getHeader()->enableChainHeader();

	refreshSize();

	findParentComponentOfClass<BackendProcessorEditor>()->rebuildModuleList(false);

	

}

int BetterProcessorEditorPanel::getHeightOfAllEditors() const
{
	int y = 0;

	for (int i = 0; i < editors.size(); i++)
	{
		// This must be set correctly with refreshChildProcessorVisiblity()!
		//jassert(editors[i]->getProcessor()->getEditorState(Processor::EditorState::Visible) == editors[i]->isVisible());

		if (!editors[i]->getProcessor()->getEditorState(Processor::EditorState::Visible)) continue;

#if HISE_IOS
		y += editors[i]->getActualHeight() + 10;
#else
		y += editors[i]->getActualHeight() + 3;
#endif
	}

	return y;
}

void BetterProcessorEditorPanel::refreshChildProcessorVisibility()
{
	Processor *p = getEditor()->getProcessor();

	if (p->getNumChildProcessors() != editors.size()) return;

	for (int i = 0; i < p->getNumChildProcessors(); i++)
	{
		const bool childIsVisible = p->getChildProcessor(i)->getEditorState(Processor::EditorState::Visible);

		editors[i]->setVisible(childIsVisible);
	}
}

void BetterProcessorEditorPanel::setInsertPosition(int position)
{
	currentPosition = position;
	
	repaint();
}

void BetterProcessorEditorPanel::updateChildEditorList()
{
	if (editors.size() == getProcessor()->getNumChildProcessors())
	{
		getEditor()->getHeader()->enableChainHeader();
		return;
	}

	editors.clear();

	for (int i = 0; i < getProcessor()->getNumChildProcessors(); i++)
	{
		if (i >= editors.size())
		{
			BetterProcessorEditor *editor = new BetterProcessorEditor(getEditor()->getRootContainer(), getEditor()->getIndentationLevel() + 1, getProcessor()->getChildProcessor(i), getEditor());

			addAndMakeVisible(editor);

			editors.add(editor);

		}
	}

	getEditor()->getHeader()->enableChainHeader();


	refreshSize();

	if (getEditor()->getRootContainer() != nullptr)
	{
		getEditor()->getRootContainer()->sendChangeMessage();
	}
}

void BetterProcessorEditorPanel::refreshSize()
{
	refreshChildProcessorVisibility();

	setSize(BetterProcessorEditorContainer::getWidthForIntendationLevel(getEditor()->getIndentationLevel()),
		getHeightOfAllEditors());

	getEditor()->sendResizedMessage();
}


void BetterProcessorEditorPanel::resized()
{
	if (getHeightOfAllEditors() == 0) return;

	int y = 0;

	for (int i = 0; i < editors.size(); i++)
	{
		if (!editors[i]->getProcessor()->getEditorState(Processor::EditorState::Visible)) continue;

		editors[i]->setBounds(0, y, BetterProcessorEditorContainer::getWidthForIntendationLevel(editors[i]->getIndentationLevel()) - 4, editors[i]->getActualHeight());

#if HISE_IOS
		y += editors[i]->getActualHeight() + 10;
#else
		y += editors[i]->getActualHeight() + 3;
#endif
	}

}

void BetterProcessorEditorPanel::paintOverChildren(Graphics& g)
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

ProcessorEditorChildComponent::ProcessorEditorChildComponent(BetterProcessorEditor*editor) :
parentEditor(editor),
processor(editor->getProcessor())
{

}
