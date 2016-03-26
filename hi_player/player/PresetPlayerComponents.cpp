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

void VerticalListComponent::addComponent(ChildComponent *componentToAdd)
{
	listComponents.add(componentToAdd);

	addAndMakeVisible(componentToAdd->asComponent());

	refreshList();
}

void VerticalListComponent::refreshList()
{
	int y = 0;

	for (int i = 0; i < listComponents.size(); i++)
	{
		y += listComponents[i]->getComponentHeight();
	}

	if (y == getHeight())
	{
		resized();
	}
	else
	{
		setSize(getWidth(), y);
	}

}

void VerticalListComponent::resized()
{
	int y = 0;

	for (int i = 0; i < listComponents.size(); i++)
	{
		listComponents[i]->asComponent()->setBounds(0, y, getWidth(), listComponents[i]->getComponentHeight());

		y += listComponents[i]->asComponent()->getHeight();
	}
}

void VerticalListComponent::removeComponent(ChildComponent * presetEditor) 
{
	listComponents.removeObject(presetEditor);
}

PresetHeader::PresetHeader(PresetProcessor *pp_)
	:pp(pp_)
{
	addAndMakeVisible(foldButton = new ShapeButton("Fold", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));

	checkFoldButton();

	foldButton->addListener(this);



	addAndMakeVisible(peakMeter = new VuMeter());
	peakMeter->setType(VuMeter::StereoHorizontal);
	peakMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	peakMeter->setColour(VuMeter::ledColour, Colours::lightgrey.withAlpha(0.7f));
	peakMeter->setColour(VuMeter::outlineColour, Colour(0x45000000));
	

	addAndMakeVisible(idLabel = new Label("ID Label", TRANS("ModulatorName")));
	
	idLabel->setFont(GLOBAL_BOLD_FONT());
	idLabel->setJustificationType(Justification::centredLeft);
	idLabel->setEditable(false, false, true);
	idLabel->setColour(Label::backgroundColourId, Colour(0x00000000));
	idLabel->setColour(Label::textColourId, Colours::white.withAlpha(0.7f));
	idLabel->setColour(Label::outlineColourId, Colour(0x00ffffff));
	
	idLabel->setText(pp_->getMainSynthChain()->getId(), dontSendNotification);
	
	addAndMakeVisible(bypassButton = new ShapeButton("Bypass Instrument", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	Path bypassPath;
	bypassPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::bypassShape, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::bypassShape));
	bypassButton->setShape(bypassPath, true, true, true);
	bypassButton->setToggleState(true, dontSendNotification);

	bypassButton->addListener(this);

	addAndMakeVisible(deleteButton = new ShapeButton("Delete Instrument", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	Path deletePath;
	deletePath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	deleteButton->setShape(deletePath, true, true, true);
	deleteButton->setToggleState(true, dontSendNotification);

	deleteButton->addListener(this);

	addAndMakeVisible(volumeSlider = new Slider("Intensity Slider"));

	addAndMakeVisible(channelFilter = new ComboBox());

	pp->skin(*channelFilter);

	channelFilter->addItem("Omni", 1);
	channelFilter->setTooltip("Set the MIDI channel of the instrument");

	for (int i = 1; i <= 16; i++)
	{
		channelFilter->addItem("Ch. " + String(i), i + 1);
	}

	volumeSlider->setRange(-100.0, 0.0, 0.1);
	volumeSlider->setSkewFactorFromMidPoint(-18.0);
	volumeSlider->setTextValueSuffix(" dB");
	volumeSlider->setTooltip("Change the volume of the instrument.");
	volumeSlider->setSliderStyle(Slider::LinearBar);
	volumeSlider->setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, true, 80, 20);
	volumeSlider->setColour(Slider::backgroundColourId, Colour(0xfb282828));
	volumeSlider->setColour(Slider::thumbColourId, Colour(0xff777777));
	volumeSlider->setColour(Slider::trackColourId, Colour(0xff222222));
	volumeSlider->setColour(Slider::textBoxTextColourId, Colours::white);
	volumeSlider->setColour(Slider::textBoxOutlineColourId, Colour(0x45ffffff));
	volumeSlider->setColour(Slider::textBoxHighlightColourId, Colours::white);
	volumeSlider->setColour(Slider::ColourIds::textBoxBackgroundColourId, Colour(0xfb282828));
	volumeSlider->setColour(Label::ColourIds::outlineWhenEditingColourId, Colours::white);

	volumeSlider->setVelocityModeParameters(0.1, 1, 0.0, true);

	volumeSlider->setScrollWheelEnabled(false);
	volumeSlider->setLookAndFeel(&bpslaf);
	volumeSlider->addListener(this);
	
	pp->getMainSynthChain()->addChangeListener(this);

	changeListenerCallback(pp->getMainSynthChain());

#if JUCE_DEBUG

	startTimer(150);

#else

	startTimer(30);

#endif
}

PresetHeader::~PresetHeader()
{
	pp->getMainSynthChain()->removeChangeListener(this);

	peakMeter = nullptr;
	idLabel = nullptr;
	bypassButton = nullptr;
	foldButton = nullptr;
	deleteButton = nullptr;
	volumeSlider = nullptr;
}

void PresetHeader::buttonClicked(Button *b)
{
	if (b == bypassButton)
	{
		const bool wasBypassed = pp->getMainSynthChain()->isBypassed();

		if (wasBypassed)
		{
			volumeSlider->setEnabled(true);
			peakMeter->setEnabled(true);
			pp->getMainSynthChain()->setBypassed(false);
			idLabel->setColour(Label::textColourId, Colours::white.withAlpha(0.7f));
		}
		else
		{
			volumeSlider->setEnabled(false);
			peakMeter->setEnabled(true);
			pp->getMainSynthChain()->setBypassed(true);
			idLabel->setColour(Label::textColourId, Colours::white.withAlpha(0.3f));
		}
	}
	else if (b == deleteButton)
	{
		PresetContainerProcessorEditor *editor = findParentComponentOfClass<PresetContainerProcessorEditor>();
		if (editor != nullptr)
		{
			editor->removePresetProcessor(pp, dynamic_cast<VerticalListComponent::ChildComponent*>(getParentComponent()));
		}
	}
	else if (b == foldButton)
	{
		const bool folded = pp->getMainSynthChain()->getEditorState(Processor::Folded);

		pp->getMainSynthChain()->setEditorState(Processor::Folded, !folded);

		checkFoldButton();

		getParentAsListChild()->refreshList();

	}
}

void PresetHeader::changeListenerCallback(SafeChangeBroadcaster *)
{
	const float gain = pp->getMainSynthChain()->getAttribute(ModulatorSynth::Gain);

	volumeSlider->setValue(Decibels::gainToDecibels(gain), dontSendNotification);
}

void PresetHeader::timerCallback()
{
	peakMeter->setPeak(pp->getMainSynthChain()->getDisplayValues().outL, pp->getMainSynthChain()->getDisplayValues().outR);
}

void PresetHeader::paint(Graphics &g)
{
	laf.drawBackground(g, (float)getWidth(), (float)getHeight(), false);
}

void PresetHeader::resized()
{
	const int addCloseWidth = 16;

	const int marginX = 5;

	int x = 8;

	foldButton->setBounds(x, 8, addCloseWidth, addCloseWidth);
	
	x += addCloseWidth + marginX;

	idLabel->setBounds(x, 2, 130, getHeight() - 6);

	x += 130 + marginX;

	bypassButton->setBounds(x, 5, addCloseWidth+3, addCloseWidth+3);

	x += addCloseWidth + marginX + 5;

	peakMeter->setBounds(x, 5, 160, 20);

	x += 160 + marginX;

	volumeSlider->setBounds(x, 5, 160, 20);

	x += 160 + marginX;

	channelFilter->setBounds(x, 3, 60, 24);

	x += 60 + marginX;
	
	deleteButton->setBounds(getWidth() - 8 - addCloseWidth, 8, addCloseWidth, addCloseWidth);
	

}

void PresetHeader::checkFoldButton()
{
	Path foldPath;

	foldPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));
	
	if (!pp->getMainSynthChain()->getEditorState(Processor::Folded))
	{
		foldPath.applyTransform(AffineTransform::rotation(float_Pi * 0.5f));
		foldButton->setTooltip(TRANS("Fold this Instrument"));
	}
	else
	{
		foldButton->setTooltip(TRANS("Expand this Instrument"));
	}

	foldButton->setShape(foldPath, false, true, true);
	
}

void PresetHeader::sliderValueChanged(Slider *s)
{
	const float gain = Decibels::decibelsToGain((float)s->getValue());

	pp->getMainSynthChain()->setAttribute(ModulatorSynth::Gain, gain, dontSendNotification);
}

AnimatedPanelViewport::AnimatedPanelViewport() :
isShowing(false),
proportionOfAnimation(0.0f)
{
	
	addAndMakeVisible(background = new Background());
	
	background->addAndMakeVisible(refreshBrowserButton = new ShapeButton("Bypass Instrument", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	Path refreshBrowserButtonPath;
	refreshBrowserButtonPath.loadPathFromData(PlayerBinaryData::refreshIcon, sizeof(PlayerBinaryData::refreshIcon));
	refreshBrowserButton->setShape(refreshBrowserButtonPath, true, true, true);
	refreshBrowserButton->setToggleState(true, dontSendNotification);

	refreshBrowserButton->setTooltip("Refresh Instrument Database");

	refreshBrowserButton->addListener(this);

	
	addAndMakeVisible(viewport = new Viewport());

	packageList = new VerticalListComponent();

	//test = new StupidTest();

	//test->setSize(200, 1200);

	//viewport->setViewedComponent(test);

	ScopedPointer<XmlElement> xml = XmlDocument::parse(File(PresetHandler::getGlobalSampleFolder() + "/database.xml"));

	presetList = new PresetList(xml);

	viewport->setScrollBarThickness(8);

	viewport->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, Colours::white.withAlpha(0.2f));

	for (int i = 0; i < presetList->getNumPresetComponents(); i++)
	{
		VerticalListComponent::ChildComponent *c = presetList->createPresetComponentForIndex(i);

		c->asComponent()->addMouseListener(this, true);

		packageList->addComponent(c);
	}


	packageList->setSize(200 - viewport->getScrollBarThickness(), packageList->getHeight());

	packageList->refreshList();

	viewport->setViewedComponent(packageList, true);

	

	setInterceptsMouseClicks(false, true);

	
}

void AnimatedPanelViewport::setViewedComponent(Component * /*contentComponent*/)
{
	//viewport->setViewedComponent(contentComponent, false);
}

void AnimatedPanelViewport::toggleVisibility()
{
	isShowing = !isShowing;

	refreshBrowserButton->setVisible(isShowing);

	startTimer(30);
}

void AnimatedPanelViewport::timerCallback()
{
	const float animationSpeed = 1.1f;

	if (isShowing)
	{
		proportionOfAnimation = (proportionOfAnimation + 1.0f * animationSpeed) - 1.0f;;

		const int position = (int)(-proportionOfAnimation * (float)getWidth());

		showedWidth = -position;

		viewport->setTopLeftPosition(-(getWidth() - showedWidth), refreshBrowserButton->getBottom() + 3);

		background->setBounds(0, 0, showedWidth, getHeight());

		if (proportionOfAnimation >= 1.0f)
		{
			stopTimer();
			proportionOfAnimation = 1.0f;
			repaint();
		}
	}
	else
	{
		proportionOfAnimation = (proportionOfAnimation + 1.0f / animationSpeed) - 1.0f;;

		const int position = (int)(-proportionOfAnimation * (float)getWidth());

		showedWidth = -position;

		viewport->setTopLeftPosition(-(getWidth() - showedWidth), refreshBrowserButton->getBottom() + 3);
		background->setBounds(0, 0, showedWidth, getHeight());

		if (proportionOfAnimation <= 0.0f)
		{
			stopTimer();
			proportionOfAnimation = 0.0f;
			repaint();
		}
	}


}

void AnimatedPanelViewport::paint(Graphics &g)
{
	if (showedWidth > 0)
	{
		g.setColour(Colour(0xFF222222));
		g.fillRect(0, 0, showedWidth, getHeight());
	}
}

void AnimatedPanelViewport::buttonClicked(Button *b)
{
	if (b == refreshBrowserButton)
	{
		PresetList::rebuildDatabase();

		

		presetList->reloadFromXml();

		packageList->clear();

		for (int i = 0; i < presetList->getNumPresetComponents(); i++)
		{
			VerticalListComponent::ChildComponent *c = presetList->createPresetComponentForIndex(i);

			c->asComponent()->addMouseListener(this, true);

			packageList->addComponent(c);
		}

		packageList->setSize(200 - viewport->getScrollBarThickness(), packageList->getHeight());

		packageList->refreshList();
	};
}

void AnimatedPanelViewport::resized()
{
	refreshBrowserButton->setBounds(getWidth() - 24, 2, 16, 16);

	viewport->setBounds(0, refreshBrowserButton->getBottom() + 3, getWidth(), getHeight() - (refreshBrowserButton->getBottom() + 3) - 10);
	
	if (!isShowing)
	{
		viewport->setTopLeftPosition(-getWidth(), refreshBrowserButton->getBottom() + 3);
	}
}

void AnimatedPanelViewport::mouseDown(const MouseEvent &e)
{
	for (int i = 0; i < packageList->getNumListComponents(); i++)
	{
		Component *entry = packageList->getComponent(i)->asComponent();

		if (entry->isParentOf(e.eventComponent))
		{
			continue;
		}

		dynamic_cast<PresetList::Entry*>(entry)->deselectAllInstruments();
	}
}

void AnimatedPanelViewport::Background::paint(Graphics &g)
{
	g.setGradientFill(ColourGradient(Colour(0xFF444444), 0.0f, 0.0f,
		Colour(0xFF333333), 0.0f, (float)getHeight(), false));

	g.fillAll();

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.3f), 0.0f, 0.0f,
		Colours::black.withAlpha(0.0f), 4.0f, 0.0f, false));

	g.fillRect(0, 0, 10, getHeight());

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.3f), (float)getWidth(), 0.0f,
		Colours::black.withAlpha(0.0f), (float)getWidth()-4.0f, 0.0f, false));

	g.fillRect(getWidth()-10, 0, 10, getHeight());

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.3f), 0.0f, 0.0f,
		Colours::black.withAlpha(0.0f), 0.0f, 4.0f, false));

	g.fillRect(0, 0, getWidth(), 10);

	g.setColour(Colours::white.withAlpha(0.2f));

	g.drawRect(0, 0, getWidth(), getHeight(), 1);
}

PresetList::PresetList(XmlElement * /*database*/)
{
	reloadFromXml();
	
}

void PresetList::reloadFromXml()
{

	if (!getDatabaseFile().existsAsFile())
	{
		rebuildDatabase();

	}
	
	ScopedPointer<XmlElement> newDatabase = XmlDocument::parse(getDatabaseFile());

	jassert(newDatabase != nullptr);

	presetList = ValueTree::fromXml(*newDatabase);
}

bool PresetList::databaseExists()
{
	return true;
}

void PresetList::rebuildDatabase()
{
	File globalFolder = PresetHandler::getGlobalSampleFolder();

	DirectoryIterator iter(globalFolder, false, "*", File::findDirectories);



	ScopedPointer<XmlElement> database = new XmlElement("database");

	database->setAttribute("HINT", "This file is autogenerated and we be deleted if you rescan the database. If you want to edit a package property, directly edit \'packageName/package.xml\'.");

	while (iter.next())
	{
		File packageFile = iter.getFile().getFullPathName() + "/package.xml";

		if (packageFile.existsAsFile())
		{
			XmlElement *packageXml = XmlDocument::parse(packageFile);

			jassert(packageXml != nullptr);

			if (packageXml != nullptr)
			{
				packageXml->setAttribute("folder", iter.getFile().getFullPathName());

				database->addChildElement(packageXml);
			}
		}
	}

	database->writeToFile(getDatabaseFile(), "");
}

File PresetList::getDatabaseFile()
{
	return File(PresetHandler::getGlobalSampleFolder() + "/database.xml");
}

VerticalListComponent::ChildComponent * PresetList::createPresetComponentForIndex(int i)
{
	ValueTree v = presetList.getChild(i);

	return new Entry(v);
}

PresetList::Entry::Entry(ValueTree &presetData):
folded(true)
{
	name = presetData.getProperty("name");
	author = presetData.getProperty("author");
	version = presetData.getProperty("version");
	folder = presetData.getProperty("folder").toString();

	for (int i = 0; i < presetData.getNumChildren(); i++)
	{
		fileNames.add(presetData.getChild(i).getProperty("file", ""));
	}

	bgColour = presetData.hasProperty("bgColour") ? Colour(presetData.getProperty("bgColour").toString().getHexValue32()) : Colour(0xFF555555);

	addAndMakeVisible(packageTable = new PackageTable(presetData));
	
	setSize(200 - 8, 140);
};

void PresetList::Entry::paint(Graphics &g)
{

	g.setGradientFill(ColourGradient(bgColour.withAlpha(0.9f), 0.0f, 0.0f,
		bgColour.withAlpha(0.6f), 0.0f, (float)getHeight(), false));

	g.fillRoundedRectangle(5.0f, 2.0f, (float)getWidth() - 10.0f, (float)getHeight() - 5.0f, 2.0f);

	g.setColour(Colours::white.withAlpha(0.4f));

	g.drawRoundedRectangle(5.0f, 2.0f, (float)getWidth() - 10.0f, (float)getHeight() - 5.0f, 2.0f, 1.0f);



	g.setFont(GLOBAL_BOLD_FONT().withHeight(15.0f));

	g.setColour(Colours::white.withAlpha(0.7f));

	g.drawText(name, 12, 4, getWidth() - 24, 15, Justification::centredLeft);

	g.setFont(GLOBAL_MONOSPACE_FONT().withHeight(11.0f));

	g.drawText(version, 12, 4, getWidth() - 24, 15, Justification::centredRight);

	g.setColour(Colours::black.withAlpha(0.6f));

	g.setFont(GLOBAL_BOLD_FONT().italicised());

	g.drawText(author, 12, 18, getWidth() - 24, 15, Justification::centredLeft);


}

void PresetList::Entry::loadPreset(int rowNumber)
{
	File presetFile = File(folder + "/" + fileNames[rowNumber]);

	if (presetFile.exists())
	{
		findParentComponentOfClass<PresetContainerProcessorEditor>()->addNewPreset(presetFile);
	}
}

PackageTable::PackageTable(ValueTree &instrumentList):
instruments(instrumentList)
{
	// Create our table component and add it to this component..
	addAndMakeVisible(table);
	table.setModel(this);

	// give it a border
	table.setColour(ListBox::outlineColourId, Colours::grey);
	table.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

	table.setOutlineThickness(0);

	table.getViewport()->setScrollBarThickness(8);

	table.setLookAndFeel(&laf);

	table.getHeader().addColumn("File", 1, 172-9, 172-9, 172-9, TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);
	
	table.setHeaderHeight(0);

	table.setMultipleSelectionEnabled(false);

	table.updateContent();

	table.getViewport()->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, Colours::white.withAlpha(0.2f));

	table.setRowHeight(18);


}

void PackageTable::paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
	const float h = (float)table.getRowHeight()-1.0f;
	const float w = (float)getWidth();

	if (rowNumber % 2) g.fillAll(Colours::white.withAlpha(0.05f));
	else
	{
		g.fillAll(Colours::white.withAlpha(0.15f));
	}

	if (rowIsSelected)
	{

		g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.5f), 0.0f, 0.0f,
			Colours::white.withAlpha(0.6f), 0.0f, h, false));

		g.fillAll();

		g.setColour(Colours::white.withAlpha(0.3f));

		g.drawHorizontalLine(0, 0.0f, w);

		g.drawLine(0.0f, 0.0f, 0.0f, h);

		g.setColour(Colours::black.withAlpha(0.3f));

		g.drawHorizontalLine((int)h, 0.0f, w);

		g.drawLine(w, 1.0f, w, h);
		

	}
		
}

void PackageTable::selectedRowsChanged(int /*lastRowSelected*/)
{
	
}

void PackageTable::cellDoubleClicked(int rowNumber, int /*columnId*/, const MouseEvent& )
{
	dynamic_cast<PresetList::Entry*>(getParentComponent())->loadPreset(rowNumber);
}

void PackageTable::paintCell(Graphics& g, int rowNumber, int /*columnId*/, int width, int height, bool rowIsSelected)
{
	g.setColour(rowIsSelected ? Colours::black : Colours::white.withAlpha(.8f));
	g.setFont(GLOBAL_BOLD_FONT());

	String	text = instruments.getChild(rowNumber).getProperty("name");

	g.drawText(text, 2, 0, width - 4, height, Justification::centred, true);

}

void PackageTable::resized()
{
	table.setBounds(getLocalBounds());
}

void PackageTable::deselectAll()
{
	table.deselectAllRows();
}

void VerticalListComponent::ChildComponent::refreshList()
{
	VerticalListComponent *parent = dynamic_cast<VerticalListComponent*>(asComponent()->getParentComponent());

	if (parent != nullptr)
	{
		parent->refreshList();
	}
}
