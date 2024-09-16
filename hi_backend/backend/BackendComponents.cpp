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


void MacroComponent::addSynthChainToPopup(ModulatorSynthChain *parent, PopupMenu &p, Array<MacroControlPopupData> &popupData)
{
	for(int i = 0; i < parent->getNumChildProcessors(); i++)
	{
		if(ModulatorSynthChain *msc = dynamic_cast<ModulatorSynthChain*>(parent->getChildProcessor(i)))
		{
			PopupMenu sub;

			for(int j = 0; j < HISE_NUM_MACROS; j++)
			{
				if(msc->hasActiveParameters(j))
				{
					MacroControlPopupData pData;

					pData.chain = msc;
					pData.macroIndex = j;
					pData.itemId = popupData.size() + 1;

					popupData.add(pData);

					sub.addItem(pData.itemId, msc->getMacroControlData(j)->getMacroName());
				}
			}

			if(sub.getNumItems() != 0) p.addSubMenu(msc->getId(), sub, true);

			addSynthChainToPopup(msc, p, popupData);
		}
	}
}

MacroComponent::MacroComponent(BackendRootWindow* rootWindow_) :
	Processor::OtherListener(rootWindow_->getBackendProcessor()->getMainSynthChain(), dispatch::library::ProcessorChangeEvent::Macro),
	rootWindow(rootWindow_),
	processor(rootWindow_->getBackendProcessor()),
	synthChain(processor->getMainSynthChain())
{
	setName("Macro Controls");
	
	mlaf = new MacroKnobLookAndFeel();

	for (int i = 0; i < HISE_NUM_MACROS; i++)
	{
		Slider *s = new Slider();
		s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
		s->setName(synthChain->getMacroControlData(i)->getMacroName());
		s->setRange(0.0, 127.0, 1.0);
		//s->setTextBoxStyle(Slider::TextBoxRight, true, 50, 20);
		s->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
		s->setLookAndFeel(mlaf);
		s->setValue(0.0, dontSendNotification);

		macroKnobs.add(s);
		addAndMakeVisible(s);
		s->setTextBoxIsEditable(false);
		s->addMouseListener(this, true);
		s->addListener(this);

		ShapeButton *t = new ShapeButton("", Colours::white.withAlpha(0.1f), Colours::white.withAlpha(0.7f), Colours::white);

		static const unsigned char pathData[] = { 110,109,173,14,46,68,22,127,139,67,98,173,190,50,68,22,223,148,67,177,188,50,68,34,17,164,67,173,14,46,68,34,113,173,67,108,173,102,40,68,34,193,184,67,108,177,108,23,68,22,207,150,67,108,177,20,29,68,22,127,139,67,98,161,196,33,68,34,33,130,67,173,94,
		41,68,22,31,130,67,173,14,46,68,22,127,139,67,99,109,65,105,178,67,152,161,9,68,108,77,27,167,67,152,65,32,68,108,77,91,212,67,152,153,26,68,108,173,190,34,68,36,17,196,67,108,177,196,17,68,24,31,162,67,108,65,105,178,67,152,161,9,68,99,109,164,146,17,
		68,32,11,253,67,108,164,146,17,68,148,59,50,68,108,72,37,131,67,148,59,50,68,108,72,37,131,67,40,119,196,67,108,60,147,234,67,40,119,196,67,108,152,72,5,68,40,119,164,67,108,146,74,70,67,40,119,164,67,108,146,74,70,67,148,59,66,68,108,164,146,33,68,148,
		59,66,68,108,164,146,33,68,20,9,221,67,108,164,146,17,68,32,11,253,67,99,101,0,0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));


		t->setShape(path, false, false, false);



		t->addListener(this);

		/*TextButton *t = new TextButton();
		t->setButtonText("Edit Macro " + String(i +1));
		t->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);

		t->setColour (TextButton::buttonColourId, Colour (0x884b4b4b));
		t->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
		t->setColour (TextButton::textColourOnId, Colour (0xaaffffff));
		t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));*/
		t->setTooltip("Show Edit Panel for Macro " + String(i + 1));
		t->setClickingTogglesState(true);

		editButtons.add(t);
		addAndMakeVisible(t);


		Label *l = new Label("", synthChain->getMacroControlData(i)->getMacroName());

		l->setFont(GLOBAL_BOLD_FONT());
		l->setJustificationType(Justification::centred);
		l->setEditable(false, true, false);
		l->setColour(Label::backgroundColourId, Colours::black.withAlpha(0.1f));
		l->setColour(Label::outlineColourId, Colour(0x2b000000));
		l->setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.8f));
		l->setColour(Label::ColourIds::backgroundWhenEditingColourId, Colours::white.withAlpha(0.8f));
		l->setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.4f));
		l->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
		l->addListener(this);

		macroNames.add(l);
		addAndMakeVisible(l);



	}

	otherChange(synthChain);
}

MacroComponent::~MacroComponent()
{
	processor->getMacroManager().setMacroControlLearnMode(processor->getMainSynthChain(), -1);
}

void MacroComponent::mouseDown(const MouseEvent &e)
{
	
}

void MacroComponent::buttonClicked(Button *b)
{
	for(int i = 0; i < macroKnobs.size(); i++)
	{
		macroKnobs[i]->setEnabled(!b->getToggleState());

		macroNames[i]->setColour(Label::ColourIds::outlineColourId, Colour(0x2b000000));

		editButtons[i]->setColours(Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.7f), Colours::white);
	}

	if(b->getToggleState())
	{
		dynamic_cast<ShapeButton*>(b)->setColours(Colour(SIGNAL_COLOUR), Colour(SIGNAL_COLOUR), Colour(SIGNAL_COLOUR));

		for(int i = 0; i < editButtons.size(); i++)
		{
			editButtons[i]->setToggleState(editButtons[i] == b, dontSendNotification);

			if(editButtons[i] == b)
			{
				checkActiveButtons();

				macroNames[i]->setColour(Label::ColourIds::outlineColourId, Colour(SIGNAL_COLOUR).withAlpha(0.8f));
				
				processor->getMacroManager().setMacroControlLearnMode(processor->getMainSynthChain(), i);

			}
		}
	}

	else
	{
		checkActiveButtons();

		processor->getMacroManager().setMacroControlLearnMode(processor->getMainSynthChain(), -1);
	}
};


MacroParameterTable* MacroComponent::getMainTable()
{
	auto table = FloatingTileHelpers::findTileWithId<GenericPanel<MacroParameterTable>>(rootWindow->getRootFloatingTile(), "MainMacroTable");

	if (table != nullptr)
		return table->getContentFromGenericPanel();
	else
		jassertfalse;

	return nullptr;
}

void MacroComponent::resized()
{
	if (getWidth() <= 0)
		return;

	const int macroAmount = macroKnobs.size();

	int widthPerMacroControl = 90;

	int heightPerMacroControl = 90;

	int macrosPerRow = jmax<int>(1, getWidth() / widthPerMacroControl);


	for (int i = 0; i < macroAmount; i++)
	{

		const int columnIndex = i % macrosPerRow;
		const int rowIndex = i / macrosPerRow;

		const int x = columnIndex * widthPerMacroControl;
		const int y = rowIndex * heightPerMacroControl;

		macroKnobs[i]->setBounds(x + 21, y, 48, 48);
		macroNames[i]->setBounds(x + 1, y + 54, widthPerMacroControl - 26, 20);
		editButtons[i]->setBounds(macroNames[i]->getRight() + 2, y + 54, 20, 20);
	}

	checkActiveButtons();
}

CachedViewport::CachedViewport():
isPreloading(false)
{
	addAndMakeVisible(viewport = new InternalViewport());

	setOpaque(true);

	setWantsKeyboardFocus(false);

}

bool CachedViewport::isInterestedInDragSource(const SourceDetails & dragSourceDetails)
{
    return File::isAbsolutePath(dragSourceDetails.description.toString()) && File(dragSourceDetails.description).getFileExtension() == ".hip";
}

void CachedViewport::itemDragEnter(const SourceDetails &dragSourceDetails)
{
	dragNew = isInterestedInDragSource(dragSourceDetails);

	viewport->setColour(backgroundColourId, dragNew ? Colours::green.withAlpha(0.1f) :
		Colours::lightgrey);

	repaint();
}

void CachedViewport::itemDragExit(const SourceDetails &/*dragSourceDetails*/)
{
	dragNew = false;

	viewport->setColour(backgroundColourId, dragNew ? Colours::green.withAlpha(0.1f) :
		Colours::lightgrey);
	repaint();
}

void CachedViewport::itemDropped(const SourceDetails &dragSourceDetails)
{

	if (PresetHandler::showYesNoWindow("Replace Root Container",
		"Do you want to replace the root container with the preset?"))
	{
		findParentComponentOfClass<BackendProcessorEditor>()->loadNewContainer(File(dragSourceDetails.description.toString()));


	}
	dragNew = false;

	viewport->setColour(backgroundColourId, dragNew ? Colours::green.withAlpha(0.1f) :
		Colours::lightgrey);
	repaint();
}

void CachedViewport::resized()
{
	viewport->setBounds(0, 0, getWidth(), getHeight());

}

CachedViewport::InternalViewport::InternalViewport() :
isCurrentlyScrolling(false)
{
	setOpaque(true);

	setColour(backgroundColourId, Colours::lightgrey);

	//getVerticalScrollBar()->addMouseListener(this, true);
	//getVerticalScrollBar()->setWantsKeyboardFocus(false);
	getVerticalScrollBar().setMouseClickGrabsKeyboardFocus(false);

	setWantsKeyboardFocus(false);

}



void CachedViewport::InternalViewport::paint(Graphics &g)
{
	Colour c15 = HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright);

	g.setColour(c15);
	g.fillAll();

	Rectangle<int> area = getLocalBounds();
	area.removeFromRight(getScrollBarThickness());
	area.setHeight(area.getHeight() + 5);

	Colour c2 = Colour(0xff242424);

	g.setColour(c2);
	g.fillRect(area.toFloat());

	if (isCurrentlyScrolling)
	{

		const float scalingFactor = (float)Desktop::getInstance().getDisplays().getMainDisplay().scale;

		Rectangle<int> position = getViewArea();

		Rectangle<int> scaledPosition = Rectangle<int>((int)((float)position.getX() * scalingFactor),
			(int)((float)position.getY() * scalingFactor),
			(int)((float)position.getWidth() * scalingFactor),
			(int)((float)position.getHeight()*scalingFactor));

		Image clippedImage = cachedImage.getClippedImage(scaledPosition);

		AffineTransform backScaler = AffineTransform::scale(1.0f / scalingFactor);

		g.drawImageTransformed(clippedImage, backScaler);
	}
}

multipage::EncodedDialogBase::EncodedDialogBase(BackendRootWindow* bpe_, bool addBorder_):
	ControlledObject(bpe_->getBackendProcessor()),
	rootWindow(bpe_),
	closeButton("close", nullptr, factory),
	addBorder(addBorder_)
{
	addAndMakeVisible(closeButton);

	closeButton.onClick = [this]()
	{
		if(dialog != nullptr)
		{
			dialog->cancel();
		}
	};

	closeButton.setVisible(addBorder);
}

BreadcrumbComponent::BreadcrumbComponent(ProcessorEditorContainer* container_) :
	ControlledObject(container_->getRootEditor()->getProcessor()->getMainController()),
	container(container_)
{
	getMainController()->getProcessorChangeHandler().addProcessorChangeListener(this);
	refreshBreadcrumbs();
	container->rootBroadcaster.addListener(*this, newRoot);
}

BreadcrumbComponent::~BreadcrumbComponent()
{
	getMainController()->getProcessorChangeHandler().removeProcessorChangeListener(this);
}

void BreadcrumbComponent::moduleListChanged(Processor* /*processorThatWasChanged*/, MainController::ProcessorChangeHandler::EventType type)
{
#if USE_OLD_PROCESSOR_DISPATCH
	if (type == MainController::ProcessorChangeHandler::EventType::ProcessorRenamed)
		refreshBreadcrumbs();
#endif
#if USE_NEW_PROCESSOR_DISPATCH
	// implement me...
	jassertfalse;
#endif
}

void BreadcrumbComponent::paint(Graphics &g)
{
	for (int i = 1; i < breadcrumbs.size(); i++)
	{
		g.setColour(Colours::white.withAlpha(0.6f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(">", breadcrumbs[i]->getRight(), breadcrumbs[i]->getY(), 20, breadcrumbs[i]->getHeight(), Justification::centred, true);
	}
}

void BreadcrumbComponent::refreshBreadcrumbs()
{
	breadcrumbs.clear();

	const Processor *mainSynthChain = getMainController()->getMainSynthChain();
	const Processor *currentRoot = container->getRootEditor()->getProcessor();

	while (currentRoot != mainSynthChain)
	{
		Breadcrumb *b = new Breadcrumb(currentRoot);
		breadcrumbs.add(b);
		addAndMakeVisible(b);

		currentRoot = ProcessorHelpers::findParentProcessor(currentRoot, false);
	}

	Breadcrumb *chain = new Breadcrumb(mainSynthChain);
	breadcrumbs.add(chain);
	addAndMakeVisible(chain);

	resized();
}

void BreadcrumbComponent::resized()
{
	auto b = getLocalBounds().reduced(0, 4);

	b.removeFromLeft(5);

	for (int i = breadcrumbs.size() - 1; i >= 0; i--)
	{
		auto br = breadcrumbs[i];
		br->setBounds(b.removeFromLeft(br->getPreferredWidth()));
		b.removeFromLeft(20);
	}
		

	repaint();
}

void BreadcrumbComponent::Breadcrumb::mouseDown(const MouseEvent& )
{
	auto p = findParentComponentOfClass<BreadcrumbComponent>();
	p->container->setRootProcessorEditor(processor);
}

} // namespace hise
