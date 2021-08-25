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
	updater(*this)
{
	addAndMakeVisible(menuBar);
	menuBar.setModel(this);

	setEnableOpenGL(this);

	setLookAndFeel(&laf);

	doc.addListener(this);

	addAndMakeVisible(editor);
	addAndMakeVisible(preview);

	editor.setMarkdownMode();
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
				String f;
				f << "[LinkName](/" << fc.getResult().getRelativePathFrom(rootDirectory).replace("\\", "/") << ")";
				te.insert(f);

				auto s = te.getTextDocument().getSelection(0);
				s.head.y++;
				s.tail.y = s.head.y + 8;
				te.getTextDocument().setSelection(0, s, false);
			}

			return true;
		}
		
		return false;
	}
	);

	preview.setViewOptions((int)MarkdownPreview::ViewOptions::Naked);
	preview.setStyleData(MarkdownLayout::StyleData::createBrightStyle());
	//preview.renderer.setCreateFooter(false);
	preview.setNewText("", File());
	preview.setMouseMode(MarkdownPreview::Drag);

	setEnableScrollbarListening(true);

    setSize (1280, 800);

	loadSettings();
}

MainContentComponent::~MainContentComponent()
{
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
		m.addItem(1, "New File");
		m.addItem(2, "Open File");
		m.addItem(3, "Save File");

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
		m.addItem(1, "Show Editor", true, getSetting(SettingIds::ShowEditor));
		m.addItem(2, "Show Preview", true, getSetting(SettingIds::ShowPreview));
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
		case 3:
		{
			FileChooser fc("Set root directory", File());

			if (fc.browseForDirectory())
				applySetting(SettingIds::RootDirectory, fc.getResult().getFullPathName());
		}
		}
	}
}

void MainContentComponent::applySetting(const Identifier& id, var newValue)
{
	currentSettings.getDynamicObject()->setProperty(id, newValue);

	if (id == SettingIds::ShowEditor)
	{
		editor.setVisible(newValue);
		resized();
	}
	if (id == SettingIds::ShowPreview)
	{
		preview.setVisible(newValue);
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
	}
	if (id == SettingIds::RootDirectory)
	{
		rootDirectory = File(newValue.toString());

		if (rootDirectory.isDirectory())
		{
			preview.renderer.setLinkResolver(new MarkdownParser::FileLinkResolver(rootDirectory));
			preview.renderer.setImageProvider(new MarkdownParser::FileBasedImageProvider(&preview.renderer, rootDirectory));
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
	auto text = f.loadFileAsString();

	text = text.removeCharacters("\r");

	currentFile = f;
	tdoc.getCodeDocument().replaceAllContent(text);
	tdoc.getCodeDocument().clearUndoHistory();
	
	preview.setNewText(text, f, true);
	resized();
}
