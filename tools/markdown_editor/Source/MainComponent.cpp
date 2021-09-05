/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"




//==============================================================================
MainContentComponent::MainContentComponent() :
	tdoc(doc),
	editor(tdoc),
	preview(*this),
	updater(editor, preview)
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

	

	addAndMakeVisible(editor);
	addAndMakeVisible(preview);

	editor.editor.setLanguageManager(new mcl::MarkdownLanguageManager());

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

    
    
	updater.setEnableScrollbarListening(true);

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
