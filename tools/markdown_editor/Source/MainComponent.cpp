/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"



struct MarkdownLanguageManager : public mcl::LanguageManager
{
	CodeTokeniser* createCodeTokeniser() override
	{
		return new MarkdownParser::Tokeniser();
	}

	void processBookmarkTitle(juce::String& bookmarkTitle) override
	{
		bookmarkTitle.trim().removeCharacters("#`");
	}

	void addTokenProviders(mcl::TokenCollection*) override {};

	mcl::FoldableLineRange::List createLineRange(const CodeDocument& doc) override
	{
		CodeDocument::Iterator it(doc);

		int currentHeadlineLevel = 0;
		bool addAtNextChar = false;

		bool isInsideCodeBlock = false;

		struct Level
		{
			int line;
			int level;
		};

		Array<Level> levels;

		while (auto c = it.peekNextChar())
		{
			switch (c)
			{
			case '#':
			{
				if (!isInsideCodeBlock)
				{
					if (!addAtNextChar)
						currentHeadlineLevel = 0;

					currentHeadlineLevel++;
					addAtNextChar = true;
					it.skip();
					break;
				}
			}
			case '-':
			case '`':
			{
				if (it.nextChar() == c && it.nextChar() == c)
				{
					levels.add({ it.getLine(), 9000 });
					isInsideCodeBlock = !isInsideCodeBlock;
					break;
				}
			}
			default:
			{
				if (addAtNextChar)
				{
					levels.add({ it.getLine(), currentHeadlineLevel });
					addAtNextChar = false;
				}

				it.skipToEndOfLine();
				break;
			}
			}
		}

		mcl::FoldableLineRange::WeakPtr currentElement;
		mcl::FoldableLineRange::List lineRanges;

		auto getNextLineWithSameOrLowerLevel = [&](int i)
		{
			auto thisLevel = levels[i].level;

			for (int j = i + 1; j < levels.size(); j++)
			{
				if (levels[j].level <= thisLevel)
					return j;
			}

			return levels.size() - 1;
		};

		auto getCurrentLevel = [&]()
		{
			if (currentElement == nullptr)
				return -1;

			auto lineStart = currentElement->getLineRange().getStart();

			for (int i = 0; i < levels.size(); i++)
			{
				if (lineStart == levels[i].line)
					return levels[i].level;
			}

			jassertfalse;
			return -1;
		};

		isInsideCodeBlock = false;

		for (int i = 0; i < levels.size(); i++)
		{
			auto thisLevel = levels[i].level;



			if (thisLevel == 9000)
			{
				if (isInsideCodeBlock)
				{
					isInsideCodeBlock = false;
					continue;
				}

				isInsideCodeBlock = true;

				if (levels[i + 1].level == 9000)
				{
					Range<int> r(levels[i].line, levels[i + 1].line - 1);
					auto codeRange = new mcl::FoldableLineRange(doc, r);

					if (currentElement != nullptr)
					{
						currentElement->children.add(codeRange);
						codeRange->parent = currentElement;
					}
					else
					{
						// don't add this as current element as there will be no children
						lineRanges.add(codeRange);
					}
				}

				continue;
			}

			if (thisLevel >= 4)
				continue;

			auto endOfRangeIndex = getNextLineWithSameOrLowerLevel(i);

			Range<int> r(levels[i].line, levels[endOfRangeIndex].line);

			if (r.isEmpty())
				r.setEnd(doc.getNumLines());

			r.setEnd(r.getEnd() - 1);

			auto newRange = new mcl::FoldableLineRange(doc, r);



			if (currentElement == nullptr)
			{
				currentElement = newRange;
				lineRanges.add(currentElement);
			}
			else
			{
				while (getCurrentLevel() >= thisLevel)
				{
					currentElement = currentElement->parent;
				}

				if (currentElement == nullptr)
				{
					currentElement = newRange;
					lineRanges.add(currentElement);
				}
				else
				{
					currentElement->children.add(newRange);
					newRange->parent = currentElement;
					currentElement = newRange;
				}
			}
		}

		return lineRanges;
	}

	void setupEditor(mcl::TextEditor* editor) override
	{
		editor->setCodeTokeniser(new MarkdownParser::Tokeniser());
		//editor->setEnableAutocomplete(false);
		editor->setEnableBreakpoint(false);
	}
};

//==============================================================================
MainContentComponent::MainContentComponent() :
	tdoc(doc),
	editor(tdoc),
	preview(*this),
	updater(*this)
{
#if JUCE_MAC
    MenuBarModel::setMacMainMenu(this);
#else
	addAndMakeVisible(menuBar);
	menuBar.setModel(this);
    setEnableOpenGL(this);
#endif
    
    setWantsKeyboardFocus(true);

    addKeyCommand('n', ModifierKeys::commandModifier, [&]()
    {
        applySetting(SettingIds::CurrentFile, "");
    });
    
    addKeyCommand(KeyPress::F6Key, {}, [&]()
    {
        toggleSetting(SettingIds::ShowEditor);
    });

    addKeyCommand(KeyPress::F7Key, {}, [&]()
    {
        toggleSetting(SettingIds::ShowPreview);
    });
    
	setLookAndFeel(&laf);

	doc.addListener(this);

	addAndMakeVisible(editor);
	addAndMakeVisible(preview);

	editor.editor.setLanguageManager(new MarkdownLanguageManager());

	editor.setLookAndFeel(&plaf);

	
	editor.editor.addPopupMenuFunction(
	[](mcl::TextEditor& te , PopupMenu& m, const MouseEvent& e)
	{
        m.addItem(12000, "Insert image link");
		m.addItem(13000, "Insert link");
	},
	[this](mcl::TextEditor& te, int result)
	{
		if (result == 12000)
		{
			FileChooser fc("Create image link", rootDirectory.getChildFile("images"));

			if (fc.browseForFileToOpen())
			{
				String f;
				f << "![ALT_TEXT](/" << fc.getResult().getRelativePathFrom(rootDirectory).replace("\\", "/") << ")";
				te.insert(f);
			}

			return true;
		}
		if (result == 13000)
		{
			FileChooser fc("Create link", currentFile.getParentDirectory());

			if (fc.browseForFileToOpen())
			{
                auto s = te.getTextDocument().getSelection(0);
                auto selection = te.getTextDocument().getSelectionContent(s);
                
				String f;
                f << "[";
                
                if(selection.isNotEmpty())
                    f << selection;
                else
                    f << "LinkName";
                
                f << "](/" << fc.getResult().getRelativePathFrom(rootDirectory).replace("\\", "/") << ")";
                
                auto tl = f.length();
                
				te.insert(f);

                if(selection.isEmpty())
                {
                    s = te.getTextDocument().getSelection(0);
                    s.tail.y -= (tl-1);
                    s.head.y -= (tl-9);
                    te.getTextDocument().setSelection(0, s, false);
                }
			}

			return true;
		}
		
		return false;
	}
	);

	preview.setViewOptions((int)MarkdownPreview::ViewOptions::Naked);
	preview.setStyleData(MarkdownLayout::StyleData::createDarkStyle());
	//preview.renderer.setCreateFooter(false);
	preview.setNewText("", File());
	preview.setMouseMode(MarkdownPreview::Drag);

    
    
	setEnableScrollbarListening(true);

    setSize (1280, 800);

	loadSettings();
	editor.loadSettings(getSettingsFile(true));
}

MainContentComponent::~MainContentComponent()
{
#if JUCE_MAC
    MenuBarModel::setMacMainMenu(nullptr);
#endif
    
	currentSettings.getDynamicObject()->setProperty(SettingIds::Width, getWidth());
	currentSettings.getDynamicObject()->setProperty(SettingIds::Height, getHeight());

	detachOpenGl();

	saveSettings();
}

void MainContentComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(0xFF393939));
}

void MainContentComponent::resized()
{
	auto b = getLocalBounds();

    if(menuBar.isVisible())
        menuBar.setBounds(b.removeFromTop(24));

	if(preview.isVisible())
		preview.setBounds(editor.isVisible() ? b.removeFromRight(getWidth() / 2) : b);

	if(editor.isVisible())
		editor.setBounds(b);
}

void MainContentComponent::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	synchroniseTabs(scrollBarThatHasMoved == &editor.editor.getVerticalScrollBar());
}

juce::StringArray MainContentComponent::getMenuBarNames()
{
	return { "File", "Settings" };
}

juce::PopupMenu MainContentComponent::getMenuForIndex(int topLevelMenuIndex, const String& menuName)
{
	PopupMenu m;
	m.setLookAndFeel(&plaf);
	
	if (topLevelMenuIndex == 0)
	{
        m.addItem(1, "New File (Cmd+N)");
		m.addItem(2, "Open File (Cmd+O)");
		m.addItem(3, "Save File (Cmd+S)");

		PopupMenu recentFiles;

		auto v = getSetting(SettingIds::FileList);

		if (auto ar = v.getArray())
		{
			int index = 0;

			for (auto e : *ar)
				recentFiles.addItem(9000 + index++, e.toString());
		}

		m.addSubMenu("Open recent file", recentFiles);

		m.addSeparator();
		m.addItem(4, "Quit");
	}
	if (topLevelMenuIndex == 1)
	{
		m.addItem(1, "Show Editor (F6)", true, getSetting(SettingIds::ShowEditor));
		m.addItem(2, "Show Preview (F7)", true, getSetting(SettingIds::ShowPreview));
        m.addItem(4, "Dark Preview", true, getSetting(SettingIds::DarkPreview));
		m.addSeparator();
		m.addItem(3, "Set root directory");
	}

	return m;
}

void MainContentComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
	if (topLevelMenuIndex == 0)
	{
		switch (menuItemID)
		{
		case 1: applySetting(SettingIds::CurrentFile, ""); break;
		case 2:
		{
			FileChooser fc("Load File", rootDirectory, "*.md");
			if (fc.browseForFileToOpen())
			{
				applySetting(SettingIds::CurrentFile, fc.getResult().getFullPathName());
			}
			break;
		}
		case 3:
			saveIfChanged(true);
			break;
		case 4:
			
			JUCEApplication::quit();
			break;
		default:
			menuItemID -= 9000;

			auto v = getSetting(SettingIds::FileList)[menuItemID].toString();

			if (v.isNotEmpty())
				applySetting(SettingIds::CurrentFile, v);

			break;
		}
	}
	if (topLevelMenuIndex == 1)
	{
		switch (menuItemID)
		{
		case 1: toggleSetting(SettingIds::ShowEditor); break;
		case 2: toggleSetting(SettingIds::ShowPreview); break;
        case 4: toggleSetting(SettingIds::DarkPreview); break;
		case 3:
		{
			FileChooser fc("Set root directory", File());

			if (fc.browseForDirectory())
				applySetting(SettingIds::RootDirectory, fc.getResult().getFullPathName());
		}
		}
	}
}

void MainContentComponent::saveIfChanged(bool silent)
{
	if (!currentFile.existsAsFile())
	{
		if (silent)
		{
			FileChooser fc("Save file", rootDirectory, "*.md");

			if (fc.browseForFileToSave(true))
			{
				auto f = fc.getResult();
				f.replaceWithText(doc.getAllContent().withCleanedLineEndings());
				applySetting(SettingIds::CurrentFile, f.getFullPathName());
			}
		}

		return;
	}
		

	auto fileContent = currentFile.loadFileAsString().withCleanedLineEndings();
	auto currentContent = doc.getAllContent().withCleanedLineEndings();

	auto hasChanged = fileContent.hashCode64() != currentContent.hashCode64();

	if (hasChanged && (silent || AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, "Overwrite file", "Do you want to overwrite the file " + currentFile.getFullPathName())))
	{
		currentFile.replaceWithText(currentContent);
	}
}

void MainContentComponent::applySetting(const Identifier& id, var newValue)
{
	currentSettings.getDynamicObject()->setProperty(id, newValue);

    MenuBarModel::menuItemsChanged();
    
	if (id == SettingIds::ShowEditor)
	{
        int lineToShow = -1;
        
        if(newValue)
            lineToShow = preview.renderer.getLineNumberForY(preview.viewport.getViewPositionY());
        
        MarkdownRenderer::ScopedScrollDisabler sds(preview.renderer);
        
        editor.setVisible(newValue);
        resized();
        
        if(lineToShow != -1)
            editor.editor.setFirstLineOnScreen(lineToShow);
	}
	if (id == SettingIds::ShowPreview)
	{
        auto firstLine = editor.editor.getFirstLineOnScreen();
		preview.setVisible(newValue);
        editor.editor.setFirstLineOnScreen(firstLine + 2);
		resized();
	}
	if (id == SettingIds::CurrentFile)
	{
		File f(newValue.toString());

		if (f.existsAsFile())
		{
			auto fl = getSetting(SettingIds::FileList);

			if (fl.isArray())
			{
				fl.getArray()->removeAllInstancesOf(newValue);
				fl.getArray()->insert(0, newValue);
			}
		}

		setFile(f);
		editor.grabKeyboardFocusAsync();
	}
    if(id == SettingIds::DarkPreview)
    {
        bool isDarkPreview = (bool)newValue;
        
        auto s = isDarkPreview ? MarkdownLayout::StyleData::createDarkStyle():
                                 MarkdownLayout::StyleData::createBrightStyle();
        
        s.fontSize = preview.renderer.getStyleData().fontSize;
        
        preview.setStyleData(s);
        
        MarkdownRenderer::ScopedScrollDisabler sds(preview.renderer);
        preview.setNewText(doc.getAllContent(), currentFile, false);
        preview.repaint();
    }
	if (id == SettingIds::RootDirectory)
	{
		rootDirectory = File(newValue.toString());

		if (rootDirectory.isDirectory())
		{
			preview.renderer.setLinkResolver(new MarkdownParser::FileLinkResolver(rootDirectory));
			preview.renderer.setImageProvider(new MarkdownParser::FileBasedImageProvider(&preview.renderer, rootDirectory));
            
            auto tempDir = getSettingsFile(true).getSiblingFile("imagecache");
            tempDir.createDirectory();
            
            preview.renderer.setImageProvider(new MarkdownParser::URLImageProvider(tempDir, &preview.renderer));
		}
	}
	if (id == SettingIds::Width)
	{
		auto thisWidth = jmin(Desktop::getInstance().getDisplays().getMainDisplay().userArea.getWidth(), (int)newValue);
		setSize(thisWidth, getHeight());
	}
	if (id == SettingIds::Height)
	{
		auto thisHeight = jmin(Desktop::getInstance().getDisplays().getMainDisplay().userArea.getHeight(), (int)newValue);
		setSize(getWidth(), thisHeight);
	}
}

void MainContentComponent::toggleSetting(const Identifier& id)
{
	applySetting(id, !getSetting(id));
}

void MainContentComponent::setEnableScrollbarListening(bool shouldListenToScrollBars)
{
	if (shouldListenToScrollBars)
	{
		preview.viewport.getVerticalScrollBar().addListener(this);
		editor.editor.getVerticalScrollBar().addListener(this);
	}
	else
	{
		preview.viewport.getVerticalScrollBar().removeListener(this);
		editor.editor.getVerticalScrollBar().removeListener(this);
	}
}

void MainContentComponent::synchroniseTabs(bool editorIsSource)
{
	if (recursiveScrollProtector)
		return;
    
    if(!editor.isVisible() || !preview.isVisible())
        return;

	ScopedValueSetter<bool> svs(recursiveScrollProtector, true);

	auto ps = &preview.viewport.getVerticalScrollBar();
	auto es = &editor.editor.getVerticalScrollBar();

	if (!editorIsSource)
	{
		auto yPos = (float)preview.viewport.getViewPositionY();
		auto lineNumber = preview.renderer.getLineNumberForY(yPos);

		editor.editor.setFirstLineOnScreen(lineNumber);
	}
	else
	{
		auto currentLine = editor.editor.getFirstLineOnScreen();
		auto yPos = preview.renderer.getYForLineNumber(currentLine);
		preview.viewport.setViewPosition(0, yPos);
	}
}

void MainContentComponent::setFile(const File& f)
{
	saveIfChanged();

	auto text = f.loadFileAsString().withCleanedLineEndings();

	currentFile = f;
	tdoc.getCodeDocument().replaceAllContent(text);
	tdoc.getCodeDocument().clearUndoHistory();
	
	preview.setNewText(text, f, true);
	resized();
}
