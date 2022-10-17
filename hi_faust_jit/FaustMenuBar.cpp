#include "FaustMenuBar.h"

namespace scriptnode {
namespace faust {

struct FaustGraphViewer : public Component,
						  public ControlledObject,
                          public DspNetwork::FaustManager::FaustListener,
						  public PooledUIUpdater::SimpleTimer,
						  public Thread
{
	/** Apply this scale factor to the SVG */
	static constexpr float ScaleFactor = 1.8f;
	static constexpr int HeaderHeight = 28;

	static bool callRecursive(XmlElement& xml, const std::function<bool(XmlElement&)>& f)
	{
		if (f(xml))
			return true;

		for (auto* child : xml.getChildIterator())
		{
			if (callRecursive(*child, f))
				return true;
		}

		return false;
	}

	struct Map
	{
		struct Link
		{
			File url;
			Rectangle<int> clickPosition;
		};

		File getLink(const MouseEvent& e) const
		{
			auto scaledPos = e.getPosition().transformedBy(AffineTransform::scale(ScaleFactor).followedBy(
														   AffineTransform::translation(0.0f, (float)HeaderHeight)).inverted());

			for (auto l: links)
			{
				if (l.clickPosition.contains(scaledPos))
					return l.url;
			}
			
			return {};
		}

		bool processed = false;

		void lazyProcess()
		{
			if (!processed)
			{
				auto svgDirectory = f.getParentDirectory();

				

				

				processed = true;

				// Set all text colours to bright grey
				callRecursive(*root, [](XmlElement& c)
				{
					if (c.getTagName() == "text")
					{
						auto isBoxContent = c.getStringAttribute("text-anchor") == "middle";

						c.setAttribute("fill", "#DDDDDD");
						auto hiseFontName = GLOBAL_BOLD_FONT().getTypefaceName();

						if (isBoxContent)
							hiseFontName = GLOBAL_FONT().getTypefaceName();

						c.setAttribute("font-family", hiseFontName);

						if (isBoxContent)
							c.setAttribute("font-size", 8);
					}
						

					return false;
				});

				// Set all line strokes to white
				callRecursive(*root, [](XmlElement& c)
				{
					if (c.hasAttribute("style"))
					{
						auto styleAttribute = c.getStringAttribute("style");

						auto replaceColour = [&](const String& oldColour, const Colour& nc)
						{
							String newColour = "#" + nc.toDisplayString(false);
							styleAttribute = styleAttribute.replace("fill:"+ oldColour, "fill:"+newColour);
						};

						styleAttribute = styleAttribute.replace("stroke:black", "stroke:white");
						styleAttribute = styleAttribute.replace("stroke: black", "stroke:white");

						

						// Turn on dark mode
						replaceColour("#ffffff", Colour(0xFF222222));
						replaceColour("#aaaaaa", Colour(0xFF333333));
						replaceColour("#cccccc", Colour(0xFF555555));

						// green (variables)
						//replaceColour("#47945E", Colour(0xFF776666));

						// blue (container links)
						//replaceColour("#003366", );

						// greyblue(UI elements)
						replaceColour("#477881", Colour(EFFECT_PROCESSOR_COLOUR));

						// lightblue (operators)
						//replaceColour("#4B71A1", Colour(0xFF666677));

						// orange(constants)
						replaceColour("#f44800", Colour(MIDI_PROCESSOR_COLOUR));
						
						c.setAttribute("style", styleAttribute);
					}

					return false;
				});

				callRecursive(*root, [&](XmlElement& c)
				{
					if (c.getTagName() == "a")
					{
						if (auto rectChild = c.getChildByName("rect"))
						{
							auto x = roundToInt(rectChild->getDoubleAttribute("x"));
							auto y = roundToInt(rectChild->getDoubleAttribute("y"));
							auto w = roundToInt(rectChild->getDoubleAttribute("width"));
							auto h = roundToInt(rectChild->getDoubleAttribute("height"));

							auto url = c.getStringAttribute("xlink:href");

							links.add({ svgDirectory.getChildFile(url), {x, y, w, h} });
						}
					}

					return false;
				});

				struct LinkSorter
				{
					static int compareElements(const Link& l1, const Link& l2)
					{
						if (l1.clickPosition.contains(l2.clickPosition))
							return 1;
						if (l2.clickPosition.contains(l1.clickPosition))
							return -1;

						return 0;
					}
				} sorter;

				links.sort(sorter);
			}
		}

		Map(const File& f_):
			f(f_)
		{
			root = XmlDocument::parse(f);
		}

		void lazyCreate()
		{
			lazyProcess();

			if (content == nullptr)
			{
				content = Drawable::createFromSVG(*root);
			}
		}

		Rectangle<int> getBounds()
		{
			lazyCreate();

			auto tb = content->getDrawableBounds().toNearestInt();
			return tb.transformed(AffineTransform::scale(ScaleFactor));
		}

		File f;
		Array<Link> links;

		std::unique_ptr<XmlElement> root;
		std::unique_ptr<Drawable> content;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Map);
	};

    virtual void faustFileSelected(const File& f) override {};
    virtual Result compileFaustCode(const File& f) override { return Result::ok(); }
    
    virtual void faustCodeCompiled(const File& f, const Result& compileResult) override
    {
        if(faustFile == f)
        {
            setCurrentlyViewedContent(nullptr);
            setSize(600, 300);
            stopThread(1000);
            startThread(5);
            repaint();
        }
    }
    
	struct Breadcrumb: public Component
	{
		Breadcrumb(const File& f_):
			f(f_)
		{
			font = GLOBAL_BOLD_FONT();
			setSize(font.getStringWidth(getText()) + 30, HeaderHeight);
			setRepaintsOnMouseActivity(true);
		}

		Font font;

		String getText() const { return f.getFileNameWithoutExtension().upToFirstOccurrenceOf("-", false, false);  }

		void mouseDown(const MouseEvent& e)
		{
			auto parent = findParentComponentOfClass<FaustGraphViewer>();

			for (auto map : parent->directoryContent)
			{
				if (map->f == f)
				{
					MessageManager::callAsync([parent, map]()
					{
						parent->setCurrentlyViewedContent(map);
					});
					
					return;
				}
			}
		}

		void paint(Graphics& g) override
		{
			float alpha = 0.6f;

			if (isMouseOver())
				alpha += 0.2f;

			if (isMouseButtonDown())
				alpha += 0.2f;

			g.setColour(Colours::white.withAlpha(0.05f));

			auto tb = getLocalBounds().toFloat().reduced(6.0f);

			g.fillRoundedRectangle(tb, tb.getHeight() * 0.5f);

			g.setColour(Colours::white.withAlpha(alpha));

			String t;

			t << "> " << getText();

			g.setFont(font);

			

			g.drawText(t, tb, Justification::centred);
		}

		File f;
	};

	OwnedArray<Map> directoryContent;

	Map* currentlyViewedContent = nullptr;

	void createSvgFiles(const File& sourceFile)
	{
		// Create the SVG file
		auto fileAsString = sourceFile.getFullPathName().toStdString();

		StringArray stringArgs;

		Array<char*> args;

		//stringArgs.add("-O"); stringArgs.add(sourceFile.getParentDirectory().getFullPathName());
		//stringArgs.add("-o"); stringArgs.add(sourceFile.withFileExtension("svg").getFullPathName());
		stringArgs.add("-svg");

		stringArgs.add("-O"); stringArgs.add(sourceFile.getParentDirectory().getFullPathName());
		stringArgs.add("-o"); stringArgs.add(sourceFile.withFileExtension("cpp").getFileName());

		{
			// Add include directories
			auto faustPath = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Compiler::FaustPath).toString();

			stringArgs.add("-I"); stringArgs.add(sourceFile.getParentDirectory().getFullPathName());
			stringArgs.add("-I"); stringArgs.add(File(faustPath).getChildFile("share").getChildFile("faust").getFullPathName());
		}

		for (auto& a : stringArgs)
		{
			DBG(a);
			args.add(a.getCharPointer().getAddress());
		}
		
		std::string error_msg;

		::faust::generateAuxFilesFromFile(fileAsString, args.size(), (const char**)args.begin(), error_msg);
	}

	void run()
	{
		auto svgDirectoryName = faustFile.getFileNameWithoutExtension() + "-svg";
		svgDirectory = faustFile.getParentDirectory().getChildFile(svgDirectoryName);

        if (svgDirectory.isDirectory())
        {
            auto ok = svgDirectory.deleteRecursively();

            jassert(ok);
        }
        
		setStatus("Creating SVG files...");

        directoryContent.clear();
        
		createSvgFiles(faustFile);

		if (threadShouldExit())
			return;

		Map* firstMap = nullptr;

		auto list = svgDirectory.findChildFiles(File::findFiles, false, "*.svg");

		int index = 0;

		for (auto f : list)
		{
			if (threadShouldExit())
				return;

			auto progress = (double)(index++) / (double)list.size();

			String message;

			message << "parsing SVG files. ";
			message << String(roundToInt(progress * 100)) << "%";

			setStatus(message);

			directoryContent.add(new Map(f));

			if (f.getFileNameWithoutExtension() == "process")
				firstMap = directoryContent.getLast();
		}

		MessageManager::callAsync([this, firstMap]()
		{
			setCurrentlyViewedContent(firstMap);
		});
	}

	void setStatus(const String& message)
	{
		lastMessage= message;
	}

	void timerCallback() override
	{
		if (lastMessage != statusMessage)
		{
			statusMessage = lastMessage;
			repaint();
		}
	}

	String lastMessage;
	String statusMessage;

	FaustGraphViewer(DspNetwork* n, const File& faustFile_):
		ControlledObject(n->getScriptProcessor()->getMainController_()),
		SimpleTimer(n->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
		Thread("SVG Creator"),
        network(n),
		faustFile(faustFile_)
	{
        network->faustManager.addFaustListener(this);
        
		setSize(600, 300);
		startThread(5);
	}
    
    WeakReference<DspNetwork> network;
    
	~FaustGraphViewer()
	{
        network->faustManager.removeFaustListener(this);
        
		stopThread(3000);

		if (svgDirectory.isDirectory())
		{
			auto ok = svgDirectory.deleteRecursively();

			jassert(ok);
		}
	}

	void mouseDown(const MouseEvent& e)
	{
		if (currentlyViewedContent == nullptr)
			return;

		currentHoverLink = currentlyViewedContent->getLink(e);
		

		if (currentHoverLink.existsAsFile())
		{
			for (auto m : directoryContent)
			{
				if (m->f == currentHoverLink)
				{
					setCurrentlyViewedContent(m);
				}
			}
		}
	}

	void mouseExit(const MouseEvent& e)
	{
		validHoverLink = false;
		currentHoverLink = File();
		repaint();
	}

	void mouseMove(const MouseEvent& e)
	{
		if (currentlyViewedContent != nullptr)
		{
			auto prevLink = currentHoverLink;

			currentHoverLink = currentlyViewedContent->getLink(e);
			validHoverLink = currentHoverLink.existsAsFile();

			for (auto l : currentlyViewedContent->links)
			{
				if (l.url == currentHoverLink)
				{
					auto b1 = l.clickPosition.transformed(AffineTransform::scale(ScaleFactor));
					auto b2 = getLocalBounds();

					isZoomOutLink = hmath::abs(b1.getWidth() - b2.getWidth()) < 5;
				}
			}

			setMouseCursor(currentHoverLink.existsAsFile() ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);

			if (currentHoverLink != prevLink)
				repaint();
		}
	}

	void setCurrentlyViewedContent(Map* m)
	{
		if (currentlyViewedContent = m)
		{
            for (auto br : breadcrumbs)
			{
				if (br->f == m->f)
				{
					breadcrumbs.removeRange(breadcrumbs.indexOf(br), breadcrumbs.size());
					break;
				}
			}

			breadcrumbs.add(new Breadcrumb(m->f));
			addAndMakeVisible(breadcrumbs.getLast());

			auto b = m->getBounds();

			setName("Graph preview for " + m->f.getFileNameWithoutExtension().upToFirstOccurrenceOf("-", false, false));
			setSize(b.getWidth(), b.getHeight() + HeaderHeight);
			
			currentHoverLink = File();
			validHoverLink = false;

			repaint();
			resized();
		}
        else
        {
            breadcrumbs.clear();
        }
	}

	void resized() override
	{
		auto b = getLocalBounds().removeFromTop(HeaderHeight);

		for (auto br : breadcrumbs)
		{
			br->setBounds(b.removeFromLeft(br->getWidth()));
		}
	}

	void paint(Graphics& g) override
	{
		if (currentlyViewedContent != nullptr)
		{
			currentlyViewedContent->content->draw(g, 1.0f, AffineTransform::scale(ScaleFactor).followedBy(
														   AffineTransform::translation(0.0f, (float)HeaderHeight)));
		}
		else
		{
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white.withAlpha(0.5f));
			g.drawText(statusMessage, getLocalBounds().toFloat(), Justification::centred);
		}

		if (validHoverLink)
		{
			auto b = getLocalBounds().toFloat();
			
			auto f = GLOBAL_BOLD_FONT();

			String text;

			if (!isZoomOutLink)
				text << "=> ";
			
			text << currentHoverLink.getFileNameWithoutExtension().upToFirstOccurrenceOf("-", false, false);

			if (isZoomOutLink)
				text << " <=";

			auto ta = b.removeFromBottom(20).removeFromRight(f.getStringWidth(text) + 20);

			g.setColour(Colours::white.withAlpha(0.7f));
			g.fillRect(ta);
			g.setColour(Colours::black.withAlpha(0.7f));
			g.drawRect(ta, 1.0f);
			g.setFont(f);
			g.drawText(text, ta, Justification::centred);
		}
	}

	operator bool() const { return !directoryContent.isEmpty(); }

	File faustFile;
	File svgDirectory;
	File currentHoverLink;
	bool validHoverLink = false;
	bool isZoomOutLink = false;

	OwnedArray<Breadcrumb> breadcrumbs;
};

FaustMenuBar::FaustMenuBar(faust_jit_node *n) :
	addButton("add", this, factory),
	editButton("edit", this, factory),
	reloadButton("reset", this, factory),
	svgButton("preview", this, factory),
	node(n),
    dragger(n->getScriptProcessor()->getMainController_()->getGlobalUIUpdater())
{
	// we must provide a valid faust_jit_node pointer
	jassert(n);
	setLookAndFeel(&claf);
    
    auto h = 24;
    
    if(n->isUsingNormalisation())
    {
        addAndMakeVisible(dragger);
        h += 28 + UIValues::NodeMargin;
    }
    
	setSize(256, h);
    
	addAndMakeVisible(classSelector);
	classSelector.setColour(ComboBox::ColourIds::textColourId, Colour(0xFFAAAAAA));
	classSelector.setLookAndFeel(&claf);
	classSelector.addListener(this);

	editButton.setTooltip("Edit the current Faust source file in external editor");
	addAndMakeVisible(addButton);
	addAndMakeVisible(editButton);
	addAndMakeVisible(svgButton);

	editButton.setTooltip("Show the SVG diagram in a popup");

	addAndMakeVisible(reloadButton);
	// gather existing source files
	rebuildComboBoxItems();
    
    auto& fb = n->getRootNetwork()->faustManager;
    
    editButton.setToggleModeWithColourChange(true);
    
    fb.addFaustListener(this);
    
}

FaustMenuBar::~FaustMenuBar()
{
    node->getRootNetwork()->faustManager.removeFaustListener(this);
}


void FaustMenuBar::createNewFile()
{
	auto name = PresetHandler::getCustomName(node->getClassId(), "Enter the name for the Faust file");

	if (name.isNotEmpty())
	{
		if (!faust_jit_wrapper::isValidClassId(name))
		{
			// We want a clear and modal feedback that that wasn't a good idea...
			PresetHandler::showMessageWindow("Illegal file name", "Can't add file, because its name is not a valid class identifier: " + name, PresetHandler::IconType::Error);
			return;
		}
		node->createSourceAndSetClass(name);
		rebuildComboBoxItems();
		//refreshButtonState();
	}
}

std::optional<juce::File> FaustMenuBar::promptForDestinationFile(String extension, File& previousDestFile)
{
	auto destChooser = juce::FileChooser("File with the same name already exists, select a destination file",
		previousDestFile, "*." + extension);
	if (!destChooser.browseForFileToSave(true))
		return {};
	return destChooser.getResult();
}

void FaustMenuBar::importFile(String extension)
{
	auto chooser = juce::FileChooser("Faust file to import into the project",
		node->getFaustRootFile(), "*." + extension);
	if (!chooser.browseForFileToOpen())
		return;

	File sourceFile = chooser.getResult();

	if (!sourceFile.existsAsFile())
		return;

	File destFile = node->getFaustRootFile().getChildFile(sourceFile.getFileNameWithoutExtension() + "." + extension);

	if (destFile.exists()) {
		auto maybeNewDestFile = promptForDestinationFile(extension, destFile);
		if (!maybeNewDestFile.has_value())
			return;
		destFile = *maybeNewDestFile;
	}

	auto classId = destFile.getFileNameWithoutExtension();
	if (extension == "dsp" && !faust_jit_wrapper::isValidClassId(classId)) {
		DBG("Can't import file, because its name is not a valid class identifier: " + classId);
		return;
	}

	DBG("Copying: \"" + sourceFile.getFullPathName() + "\" -> \"" + destFile.getFullPathName() + "\"");
	if (!sourceFile.copyFileTo(destFile))
		return;

	// If it's a faust source file, load it
	if (extension == "dsp")
		node->setClass(destFile.getFileNameWithoutExtension());
}

void FaustMenuBar::renameFile()
{
	auto classId = node->getClassId();
	auto faustDir = node->getFaustRootFile();
	auto currentFile = faustDir.getChildFile(classId + ".dsp");

	auto maybeNewDestFile = promptForDestinationFile("dsp", currentFile);
	if (!maybeNewDestFile.has_value())
		return;
	auto destFile = *maybeNewDestFile;

	// TODO race conditions?

	node->removeClassId(classId);
	if (!currentFile.moveFileTo(destFile))
	{
		node->logError("Could not move file to new location: " +
			currentFile.getFullPathName() + " --> " +
			destFile.getFullPathName());
		return;
	}

	node->setClass(destFile.getFileNameWithoutExtension());
}


void FaustMenuBar::paintOverChildren(Graphics& g)
{
	if (compilePending)
	{
		g.fillAll(Colour(0xCC353535));
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Recompiling Faust code...", getLocalBounds().toFloat(), Justification::verticallyCentred);
	}
}

void FaustMenuBar::executeMenuAction(int option)
{
	switch (option) {
	case NEW_FILE:
		createNewFile();
		break;
	case IMPORT_FILE:
		importFile("dsp");
		break;
	case IMPORT_LIB:
		importFile("lib");
		break;
    case REBUILD_PARAMETERS:
    {
        StringArray automatedProperties;
        
        for(auto p: node->getParameterTree())
        {
            if(p[PropertyIds::Automated])
                automatedProperties.add(p[PropertyIds::ID].toString());
        }
        
        node->getParameterTree().removeAllChildren(node->getUndoManager());
        buttonClicked(&reloadButton);
        
        for(auto s: automatedProperties)
        {
            auto newP = node->getParameterTree().getChildWithProperty(PropertyIds::ID, s);
            
            if(newP.isValid())
                newP.setProperty(PropertyIds::Automated, true, node->getUndoManager());
        }
        
        break;
    }
        
	case RENAME_FILE:
		renameFile();
        break;
	case REMOVE_FILE:
		removeFile();
        break;

		// add code for more functions here
	default:
		std::cerr << "FaustMenuBar: Unknown MenuOption: " + option << std::endl;
	}
}

void FaustMenuBar::rebuildComboBoxItems()
{
	// DBG("FaustMenuBar rebuilt");
	classSelector.clear(dontSendNotification);
	classSelector.addItemList(node->getAvailableClassIds(), 1);

	// if (auto w = source->getWorkbench())
	//     classSelector.setText(w->getInstanceId().toString(), dontSendNotification);
	classSelector.setText(node->getClassId());
}

void FaustMenuBar::resized()
{
	// DBG("FaustMenuBar resized");
	auto b = getLocalBounds().reduced(0, 1);
	

    if(dragger.isVisible())
    {
        dragger.setBounds(b.removeFromBottom(28));
        b.removeFromBottom(UIValues::NodeMargin);
    }
    
    auto h = b.getHeight();
    
	addButton.setBounds(b.removeFromLeft(h - 4));
	classSelector.setBounds(b.removeFromLeft(100));
	b.removeFromLeft(3);
	editButton.setBounds(b.removeFromRight(h).reduced(2));
	svgButton.setBounds(b.removeFromRight(h).reduced(2));
	reloadButton.setBounds(b.removeFromRight(h + editButton.getWidth() / 2).reduced(2));

	b.removeFromLeft(10);
}

void FaustMenuBar::buttonClicked(Button* b)
{
	if (b == &addButton) {

		juce::PopupMenu m;
		m.setLookAndFeel(&claf);
		for (int o = MENU_OPTION_FIRST; o < MENU_OPTION_LAST; o++) {
			m.addItem(o, getTextForMenuOptionId(o), true);
		}

		int menu_selection = (MenuOption)m.show();
		if (menu_selection > 0)
			executeMenuAction(menu_selection);
	}
	else if (b == &svgButton)
	{
		auto fc = findParentComponentOfClass<FloatingTile>();

		auto sourceFile = node->getFaustFile(node->getClassId());

		fc->showComponentInRootPopup(new FaustGraphViewer(node->getRootNetwork(), sourceFile), b, { 7, 20 }, true, true);
	}
	else if (b == &editButton) {
		DBG("Edit button pressed");
		auto sourceFile = node->getFaustFile(node->getClassId());
		auto sourceFilePath = sourceFile.getFullPathName();
        
        auto p = dynamic_cast<Processor*>(node->getScriptProcessor());
        
        auto openInEditor = GET_HISE_SETTING(p, HiseSettings::Compiler::FaustExternalEditor);
        
        if(openInEditor)
        {
            juce::Process::openDocument(sourceFilePath, "");
        }
        else
        {
            auto& faustManager = node->getRootNetwork()->faustManager;
            faustManager.setSelectedFaustFile(sourceFile, sendNotificationSync);
        }
	}
	else if (b == &reloadButton) {
		DBG("Reload button pressed");
        
        auto& faustManager = node->getRootNetwork()->faustManager;
        auto sourceFile = node->getFaustFile(node->getClassId());
        
        faustManager.sendCompileMessage(sourceFile, sendNotificationSync);
	}
}

void FaustMenuBar::comboBoxChanged(ComboBox *comboBoxThatHasChanged)
{
	auto name = comboBoxThatHasChanged->getText();
	DBG("Combobox changed, new text: " + name);
	node->setClass(name);
}

void FaustMenuBar::faustFileSelected(const File& f)
{
    auto sourceFile = node->getFaustFile(node->getClassId());
    
    editButton.setToggleStateAndUpdateIcon(matchesFile(f));
}

void FaustMenuBar::preCompileFaustCode(const File& f)
{
	if (matchesFile(f))
	{
		compilePending = true;
		SafeAsyncCall::repaint(this);
	}
}

Result FaustMenuBar::compileFaustCode(const File& f)
{
    // Nothing to do here
    return Result::ok();
}

void FaustMenuBar::faustCodeCompiled(const File& f, const Result& compileResult)
{
	if (matchesFile(f))
	{
		compilePending = false;
		repaint();
	}
	
    ignoreUnused(f, compileResult);
}

juce::Path FaustMenuBar::Factory::createPath(const String& url) const
{
	// TODO: Faust Logo
	if (url == "snex")
	{
		snex::ui::SnexPathFactory f;
		return f.createPath(url);
	}

	Path p;

	LOAD_PATH_IF_URL("new", ColumnIcons::threeDots);
	LOAD_PATH_IF_URL("edit", ColumnIcons::openWorkspaceIcon);
	LOAD_PATH_IF_URL("compile", EditorIcons::compileIcon);
	LOAD_PATH_IF_URL("reset", EditorIcons::swapIcon);
	LOAD_PATH_IF_URL("add", ColumnIcons::threeDots);
	LOAD_PATH_IF_URL("preview", BackendBinaryData::ToolbarIcons::viewPanel);

	return p;
}

}
}
