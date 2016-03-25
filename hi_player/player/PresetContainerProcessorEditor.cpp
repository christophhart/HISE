


PresetContainerProcessorEditor::PresetContainerProcessorEditor(PresetContainerProcessor *ppc_) :
AudioProcessorEditor(ppc_),
ppc(ppc_)
{

#if JUCE_DEBUG && INCLUDE_COMPONENT_DEBUGGER

	debugger = new jcf::ComponentDebugger(this);

#endif

	setLookAndFeel(&laf);

	addAndMakeVisible(browserButton = new ShapeButton("Browse", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));

	Path browsePath;

	browsePath.loadPathFromData(PlayerBinaryData::hamburgerIcon, sizeof(PlayerBinaryData::hamburgerIcon));
	

	browserButton->setShape(browsePath, false, true, false);
	browserButton->setTooltip(TRANS("Expand this Instrument"));

	browserButton->addListener(this);

	addAndMakeVisible(tooltipBar = new TooltipBar());


	tooltipBar->setColour(TooltipBar::ColourIds::backgroundColour, Colours::black.withAlpha(0.07f));
	tooltipBar->setColour(TooltipBar::ColourIds::iconColour, Colours::white.withAlpha(0.7f));
	tooltipBar->setColour(TooltipBar::ColourIds::textColour, Colours::white.withAlpha(0.7f));

	addAndMakeVisible(keyboard = new CustomKeyboard(ppc->keyboardState));

	addAndMakeVisible(viewport = new Viewport());

	//viewport->setColour(CachedViewport::backgroundColourId, Colour(0xFFDDDDDD));

	verticalList = new VerticalListComponent();

	viewport->setViewedComponent(verticalList, false);

	viewport->setScrollBarThickness(9);

	viewport->getVerticalScrollBar()->setVisible(true);

	refreshPresetList();


	constrainer = new ComponentBoundsConstrainer();

	constrainer->setMinimumHeight(200);

	addAndMakeVisible(borderDragger = new ResizableBorderComponent(this, constrainer));

	BorderSize<int> borderSize;

	borderSize.setTop(0);
	borderSize.setLeft(0);
	borderSize.setRight(0);
	borderSize.setBottom(7);

	borderDragger->setBorderThickness(borderSize);

	addAndMakeVisible(presetBrowser = new AnimatedPanelViewport());

	browserList = new VerticalListComponent();

	presetBrowser->setViewedComponent(browserList);


	setSize(700 + 2 * viewport->getScrollBarThickness(), 700);

}

bool PresetContainerProcessorEditor::isInterestedInFileDrag(const StringArray &files)
{
	if (files.size() != 1) return false;
	else if (File(files[0]).getFileExtension() == ".hip") return true;
	else return false;
}

void PresetContainerProcessorEditor::filesDropped(const StringArray &files, int, int)
{
	addNewPreset(File(files[0]));
}

void PresetContainerProcessorEditor::refreshPresetList(bool forceRefresh)
{
	const int numPresetsInProcessor = ppc->patches.size();

	const int numEditors = verticalList->getNumListComponents();

	if (forceRefresh || numEditors != numPresetsInProcessor)
	{
		verticalList->clear();

		for (int i = 0; i < numPresetsInProcessor; i++)
		{
			PresetProcessorEditor *editor = dynamic_cast<PresetProcessorEditor*>(ppc->patches[i]->createEditor());

			editor->addMouseListener(this, true);

			verticalList->addComponent(editor);
		}
	}
}

void PresetContainerProcessorEditor::paint(Graphics &g)
{
	g.setColour(Colours::lightgrey);
	g.fillAll();

	Rectangle<int> area = viewport->getBounds();

	g.setGradientFill(ColourGradient(Colour(0xFFAAAAAA), 0.0f, 0.0f,
		Colour(0xFF88888), 0.0f, (float)getHeight(), false));

	//g.fillRect(area);
}

void PresetContainerProcessorEditor::resized()
{
	verticalList->setBounds(0, 0, 700, verticalList->getHeight());

	const int xOffset = viewport->getScrollBarThickness();

	browserButton->setBounds(xOffset, 5, 16, 16);

	
	tooltipBar->setBounds(browserButton->getRight() + 5, 3, getWidth() - xOffset - browserButton->getRight() - 5, 20);

	viewport->setBounds(xOffset, 30, getWidth() - xOffset, getHeight() - 72 - 30);

	keyboard->setBounds(xOffset, getHeight() - 72, getWidth() - 2*xOffset, 72);

	presetBrowser->setBounds(xOffset, 30, 200, getHeight() - 72 - 30);

	borderDragger->setBounds(getBounds());
}

void PresetContainerProcessorEditor::removePresetProcessor(PresetProcessor * pp, VerticalListComponent::ChildComponent *presetEditor)
{
	verticalList->removeComponent(presetEditor);
	
	ppc->removePreset(pp);

	refreshPresetList(true);
}

void PresetContainerProcessorEditor::buttonClicked(Button *b)
{
	if (b == browserButton)
	{
		toggleBrowserWindow();
	}
}

void PresetContainerProcessorEditor::toggleBrowserWindow()
{
	presetBrowser->toggleVisibility();
}

void PresetContainerProcessorEditor::addNewPreset(const File &f)
{
	ppc->addNewPreset(f);

	refreshPresetList();
}

void PresetContainerProcessorEditor::mouseDown(const MouseEvent &)
{
	if(presetBrowser->isCurrentlyShowing()) toggleBrowserWindow();
}

