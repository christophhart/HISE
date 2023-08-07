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

namespace hise {
using namespace juce;


struct MarkdownHelpButton::MarkdownHelp : public Component
{
	MarkdownHelp(MarkdownRenderer* renderer, int lineWidth)
	{
		setWantsKeyboardFocus(false);

		img = Image(Image::ARGB, lineWidth, (int)renderer->getHeightForWidth((float)lineWidth), true);
		Graphics g(img);

		renderer->draw(g, { 0.0f, 0.0f, (float)img.getWidth(), (float)img.getHeight() });

		setSize(img.getWidth() + 40, img.getHeight() + 40);

	}

	void mouseDown(const MouseEvent& /*e*/) override
	{
		if (auto cb = findParentComponentOfClass<CallOutBox>())
		{
			cb->dismiss();
		}
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF333333));

		g.drawImageAt(img, 20, 20);
	}

	Image img;
};

MarkdownHelpButton::MarkdownHelpButton() :
	ShapeButton("?", Colours::white.withAlpha(0.7f), Colours::white, Colours::white)
{
	setWantsKeyboardFocus(false);

	setShape(getPath(), false, true, true);

	setSize(16, 16);
	addListener(this);
}

MarkdownHelpButton::~MarkdownHelpButton()
{
	if (ownerComponent != nullptr)
		ownerComponent->removeComponentListener(this);
}

void MarkdownHelpButton::setup()
{
	parser = new MarkdownRenderer("");
	parser->setTextColour(Colours::white);
	parser->setDefaultTextSize(fontSizeToUse);
	parser->setStyleData(sd);
}

void MarkdownHelpButton::addImageProvider(MarkdownParser::ImageProvider* newImageProvider)
{
	if (parser != nullptr)
	{
		parser->setImageProvider(newImageProvider);
	}
	else
	jassertfalse; // you need to call setup before that.
}

void MarkdownHelpButton::setPopupWidth(int newPopupWidth)
{
	popupWidth = newPopupWidth;
}

void MarkdownHelpButton::setFontSize(float fontSize)
{
	fontSizeToUse = fontSize;
}


void MarkdownHelpButton::buttonClicked(Button* /*b*/)
{

	if (parser != nullptr)
	{
		if (currentPopup.getComponent())
		{
			currentPopup->dismiss();
		}
		else
		{
			auto nc = new MarkdownHelp(parser, popupWidth);

            auto window = TopLevelWindowWithOptionalOpenGL::findRoot(this);

			if (window == nullptr)
				return;

			auto lb = window->getLocalArea(this, getLocalBounds());

			if (nc->getHeight() > 700)
			{
				Viewport* viewport = new Viewport();

				viewport->setViewedComponent(nc);
				viewport->setSize(nc->getWidth() + viewport->getScrollBarThickness(), 700);
				viewport->setScrollBarsShown(true, false, true, false);

				currentPopup = &CallOutBox::launchAsynchronously(std::unique_ptr<Component>(viewport), lb, window);
                currentPopup->setAlwaysOnTop(true);
				currentPopup->setWantsKeyboardFocus(!ignoreKeyStrokes);
			}
			else
			{
				currentPopup = &CallOutBox::launchAsynchronously(std::unique_ptr<Component>(nc), lb, window);
                currentPopup->setAlwaysOnTop(true);
				currentPopup->setWantsKeyboardFocus(!ignoreKeyStrokes);
			}
		}




	}
}

void MarkdownHelpButton::attachTo(Component* componentToAttach, AttachmentType attachmentType_)
{
	if (ownerComponent != nullptr)
		ownerComponent->removeComponentListener(this);

	ownerComponent = componentToAttach;
	attachmentType = attachmentType_;

	if (ownerComponent != nullptr)
	{
		jassert(getParentComponent() == nullptr);

		if (auto parent = ownerComponent->getParentComponent())
		{
			parent->addAndMakeVisible(this);
		}
		else
		jassertfalse; // You tried to attach a help button to a component without a parent...

		setVisible(ownerComponent->isVisible());
		ownerComponent->addComponentListener(this);
		componentMovedOrResized(*ownerComponent, true, true);
	}
}

void MarkdownHelpButton::componentMovedOrResized(Component& c, bool cond, bool cond1)
{
	auto cBounds = c.getBoundsInParent();

	switch (attachmentType)
	{
	case Overlay:
		{
			setBounds(cBounds.withSizeKeepingCentre(16, 16));
			break;
		}
	case OverlayLeft:
		{
			auto square = cBounds.removeFromLeft(20);

			setBounds(square.withSizeKeepingCentre(16, 16));

			break;
		}
	case OverlayRight:
		{
			auto square = cBounds.removeFromRight(20);

			setBounds(square.withSizeKeepingCentre(16, 16));

			break;
		}
	case Left:
		{
			setBounds(cBounds.getX() - 20, cBounds.getY() + 2, 16, 16);
			break;
		}
	case TopRight:
		{
			Rectangle<int> r(cBounds.getRight() - 16, cBounds.getY() - 16, 16, 16);
			setBounds(r);
		}
	default:
		break;
	}
}

void MarkdownHelpButton::componentVisibilityChanged(Component& c)
{
	setVisible(c.isVisible());
}

void MarkdownHelpButton::setIgnoreKeyStrokes(bool shouldIgnoreKeyStrokes)
{
	setWantsKeyboardFocus(shouldIgnoreKeyStrokes);
	ignoreKeyStrokes = shouldIgnoreKeyStrokes;

}

MarkdownHelpButton* MarkdownHelpButton::createAndAddToComponent(Component* c, const String& s, int popupWidth)
{
	auto h = new MarkdownHelpButton();

	h->attachTo(c, MarkdownHelpButton::TopRight);
	h->setHelpText(s);
	h->setPopupWidth(popupWidth);
	return h;
}

void MarkdownHelpButton::componentBeingDeleted(Component& component)
{
	component.removeComponentListener(this);

	getParentComponent()->removeChildComponent(this);

	delete this;
}

void MarkdownHelpButton::setStyleData(const MarkdownLayout::StyleData& newStyleData)
{
	sd = newStyleData;

	if (parser != nullptr)
	{
		parser->setStyleData(sd);
		parser->parse();
	}
}


juce::Path MarkdownHelpButton::getPath()
{
	Path path;
	path.loadPathFromData(MainToolbarIcons::help, MainToolbarIcons::help_Size);

	return path;
}

#if !HISE_USE_NEW_CODE_EDITOR
void MarkdownEditor::addPopupMenuItems(PopupMenu& menuToAddTo, const MouseEvent* mouseClickEvent)
{

	menuToAddTo.addItem(AdditionalCommands::LoadFile, "Load file");
	menuToAddTo.addItem(AdditionalCommands::SaveFile, "Save file");
	menuToAddTo.addSeparator();

	CodeEditorComponent::addPopupMenuItems(menuToAddTo, mouseClickEvent);
}

void MarkdownEditor::performPopupMenuAction(int menuItemID)
{
	if (menuItemID == AdditionalCommands::LoadFile)
	{
		FileChooser fc("Load file", File(), "*.md");

		if (fc.browseForFileToOpen())
		{
			currentFile = fc.getResult();

			getDocument().replaceAllContent(currentFile.loadFileAsString());
		}

		return;
	}
	if (menuItemID == AdditionalCommands::SaveFile)
	{
		FileChooser fc("Save file", currentFile, "*.md");

		if (fc.browseForFileToSave(true))
		{
			currentFile = fc.getResult();

			currentFile.replaceWithText(getDocument().getAllContent());
		}

		return;
	}

	CodeEditorComponent::performPopupMenuAction(menuItemID);
}
#endif

struct MarkdownEditorPopupComponents
{
	struct Base : public Component,
				  public ButtonListener
	{
		Base(MarkdownEditorPanel& parent_) :
			parent(parent_),
			panel(),
			createButton("Insert at position")
		{
			addAndMakeVisible(panel);
			addAndMakeVisible(createButton);
			createButton.addListener(this);

			setWantsKeyboardFocus(true);

			
		}


		bool keyPressed(const KeyPress& k) override
		{
			if (k == KeyPress::returnKey ||
				k == KeyPress::F5Key)
			{
				createButton.triggerClick();
				return true;
			}
			
			return false;
		}

		void buttonClicked(Button* ) override
		{
			auto t = getTextToInsert();

			CommonEditorFunctions::insertTextAtCaret(&parent.editor, t);

			auto tmp = &parent.editor;

			auto f = [tmp]()
			{
				tmp->grabKeyboardFocus();
			};

			MessageManager::callAsync(f);

			findParentComponentOfClass<FloatingTilePopup>()->deleteAndClose();
		}


		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF222222));
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(getBaseName(), getLocalBounds().removeFromTop(20).toFloat(), Justification::centred);
		}

		virtual String getTextToInsert() = 0;

		virtual String getBaseName() const = 0;

		void finish(int width=300)
		{
			setLookAndFeel(&laf);
			setSize(width, panel.getTotalContentHeight() + 40);
			createButton.setLookAndFeel(&alaf);
			createButton.setColour(TextButton::ColourIds::textColourOnId, Colours::white);
		}

		void resized() override
		{
			auto ar = getLocalBounds();
			createButton.setBounds(ar.removeFromBottom(20));
			ar.removeFromTop(20);

			panel.setBounds(ar);
		}

		AlertWindowLookAndFeel alaf;
		MarkdownEditorPanel& parent;
		hise::HiPropertyPanelLookAndFeel laf;

		PropertyPanel panel;
		TextButton createButton;
	};

	struct ImageCreator : public Base
	{
		struct FileDropper : public juce::PropertyComponent
		{
			FileDropper() :
				PropertyComponent("File", 32),
				fileComponent("File", {}, true, false, false, "*.png;*.PNG;*.jpg;*.JPG;*.gif;*.GIF;*.svg;*.SVG", {}, "Select image file")
			{
				addAndMakeVisible(fileComponent);
			};

			void refresh() override
			{

			}

			juce::FilenameComponent fileComponent;
		};

		struct IconSelector : public juce::PropertyComponent,
							  public Value::Listener
		{
			IconSelector(MarkdownContentProcessor& parent_) :
				PropertyComponent("Icon", 120),
				parent(parent_)
			{
				addAndMakeVisible(content);
				
			};

			void valueChanged(Value& value) override
			{
				currentFactoryId = value.toString();
				refresh();
			}

			struct Content : public Component,
							 public ComboBoxListener
			{
				Content()
				{
					addAndMakeVisible(selector);
					addAndMakeVisible(sizeSelector);

					sizeSelector.addItemList({ "32px", "48px", "64px", "full" }, 1);
					sizeSelector.setSelectedId(1, dontSendNotification);

					selector.addListener(this);
					selector.setTextWhenNothingSelected("Select a icon factory");
					selector.setTextWhenNoChoicesAvailable("Select a icon factory");
				}

				void comboBoxChanged(ComboBox* b) override
				{
					if (currentPathFactory != nullptr)
					{
						p = currentPathFactory->createPath(b->getText());
						p.scaleToFit(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(), true);
						repaint();
					}
				}

				void resized() override
				{
					pathBounds = getLocalBounds().toFloat();
					selector.setBounds(pathBounds.removeFromTop(24).toNearestInt());
					sizeSelector.setBounds(pathBounds.removeFromTop(24).toNearestInt());
					pathBounds.reduce(5.0f, 5.0f);
					
				}

				void paint(Graphics& g) override
				{
					g.fillAll(Colour(0xFF333333));

					g.setColour(Colours::white);
					g.fillPath(p);
				}

				void update()
				{
					selector.clear();

					if (currentPathFactory != nullptr)
					{
						selector.addItemList(currentPathFactory->ids, 1);
					}
				}

				Path p;
				Rectangle<float> pathBounds;

				ComboBox selector;
				ComboBox sizeSelector;
				PathFactory* currentPathFactory = nullptr;
			};

			String getPathId()
			{
				if(content.selector.getSelectedItemIndex() > 0)
					return content.selector.getText();

				return {};
			}

			void refresh() override
			{
				if (currentFactoryId.isEmpty())
					return;

				if (auto globalPath = parent.getTypedImageProvider<MarkdownParser::GlobalPathProvider>())
				{
					for (auto f : globalPath->factories->factories)
					{
						if (f->getId() == currentFactoryId)
						{
							content.currentPathFactory = f;
							break;
						}
					}
				}

				content.update();

			};

			String currentFactoryId;
			Content content;
			

			MarkdownContentProcessor& parent;
			
		};

		ImageCreator(MarkdownEditorPanel& parent) :
			Base(parent)
		{
			if (parent.updatePreview())
			{
				StringArray sa;
				Array<var> values;

				if (auto globalPath = parent.preview->getTypedImageProvider<MarkdownParser::GlobalPathProvider>())
				{
					for (auto f : globalPath->factories->factories)
					{
						sa.add(f->getId());
						values.add(f->getId());
					}
				}

				ChoicePropertyComponent* choice = new ChoicePropertyComponent(iconFactory, "Icon Factory", sa, values);

				iconSelector = new IconSelector(*parent.preview);
				iconFactory.addListener(iconSelector);
				
				dropper = new FileDropper();
				dropper->fileComponent.setDefaultBrowseTarget(parent.preview->getHolder().getDatabaseRootDirectory());

				panel.addProperties({ dropper.getComponent(), new TextPropertyComponent(customFileName, "Custom file name", 255, false),
						choice, iconSelector });
			}


			finish();
		}

		String getBaseName() const override { return "Create Image"; };

		String getTextToInsert()
		{
			if (parent.updatePreview())
			{
				auto pathId = iconSelector->getPathId();

				if (pathId.isNotEmpty())
				{
					auto size = ":" + iconSelector->content.sizeSelector.getText();

					if (size == ":full")
						size = {};

					String s;

					s << "![" << pathId << "](/images/icon_" << pathId << size << ")";
					return s;
				}

				File fToUse;

				if (auto d = dropper.getComponent())
				{
					auto f = d->fileComponent.getCurrentFile();

					auto rootFile = parent.preview->getHolder().getDatabaseRootDirectory();

					auto targetDirectory = rootFile.getChildFile("images/custom/");

					if (f.isAChildOf(targetDirectory))
						fToUse = f;
					else
					{
						auto customName = customFileName.toString();

						if (customName.isNotEmpty())
						{
							fToUse = targetDirectory.getChildFile(customName + ".s").withFileExtension(f.getFileExtension());
						}
						else
							fToUse = targetDirectory.getChildFile(f.getFileName());

						targetDirectory.createDirectory();
						f.copyFileTo(fToUse);
					}

					auto link = MarkdownLink::Helpers::getSanitizedFilename("/" + fToUse.getRelativePathFrom(rootFile));

					String s;

					s << "![" << fToUse.getFileNameWithoutExtension() << "](" << link << ") ";

					return s;
				}
			}

			return {};
		}

		Component::SafePointer<FileDropper> dropper;
		Component::SafePointer<IconSelector> iconSelector;
		Value customFileName;
		Value iconFactory;
	};

	struct LinkCreator : public Base
	{
		LinkCreator(MarkdownEditorPanel& parent_):
			Base(parent_)
		{
			if (parent.updatePreview())
			{
				linkURL = parent.preview.getComponent()->renderer.getLastLink().toString(MarkdownLink::Everything);

				auto clipboard = SystemClipboard::getTextFromClipboard();

				if (clipboard.isNotEmpty())
					linkURL = clipboard;

				auto text = CommonEditorFunctions::getCurrentSelection(&parent.editor);

				if (text.isNotEmpty())
					linkName = text;
				else
				{
					try
					{
						auto currentKeyword = parent.preview.getComponent()->renderer.getHeader().getKeywords()[0];
						linkName = currentKeyword.isEmpty() ? "Link" : currentKeyword;
					}
					catch (String&)
					{
						linkName = "Link";
					}
				}




				panel.addProperties({ new TextPropertyComponent(linkName, "Link Name", 255, false),
									  new TextPropertyComponent(linkURL, "Link URL", 1024, false) });
			}

			finish(500);
		}

		String getBaseName() const override { return "Link creator"; }

		String getTextToInsert() override
		{
			String s;
			s << "[" << linkName.toString() << "](" << linkURL.toString() << ")";
			return s;
		}

		Value linkURL;
		Value linkName;
	};

	struct TableCreator : public Base
	{
		TableCreator(MarkdownEditorPanel& parent_):
			Base(parent_)
		{
			if (parent.updatePreview())
			{
				if (auto globalPath = parent.preview->getTypedImageProvider<MarkdownParser::GlobalPathProvider>())
				{
					StringArray sa;
					Array<var> values;

					for (auto f : globalPath->factories->factories)
					{
						sa.add(f->getId());
						values.add(f->getId());
					}

					ChoicePropertyComponent* choice = new ChoicePropertyComponent(menu, "Icon table", sa, values);

					panel.addProperties({ new TextPropertyComponent(v, "Columns", 1024, true),
									  new TextPropertyComponent(numRows, "Number of rows", 2, false),
									  choice });
				}
				else
					jassertfalse;
			}

			finish();
		}
			
		
		String getBaseName() const override { return "Create table"; }

		String getTextToInsert() override
		{
			String s;
			String nl = "\n";

			if (menu.toString().isNotEmpty())
			{
				
			}
			else
			{
				auto columns = StringArray::fromLines(v.getValue().toString());
				auto rowAmount = numRows.toString().getIntValue();

				s << "|";

				for (auto r : columns)
				{
					s << " " << r.trim() << " ";
					s << "|";
				}
				s << "\n";

				s << "|";
				for (auto r : columns)
				{
					s << " --- ";
					s << "|";
				}
				s << "\n";

				for (int c = 0; c < rowAmount; c++)
				{
					s << "|";
					for (auto r : columns)
					{
						s << " cell ";
						s << "|";
					}
					s << "\n";
				}
			}

			

			return s;
		}

		Value menu;
		Value v;
		Value numRows;
	};
};

Path MarkdownEditorPanel::Factory::createPath(const String& id) const
{
	Path p;

	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

	LOAD_EPATH_IF_URL("live-preview", EditorIcons::swapIcon);
	LOAD_EPATH_IF_URL("new-file", EditorIcons::newFile);
	LOAD_EPATH_IF_URL("open-file", EditorIcons::openFile);
	LOAD_EPATH_IF_URL("save-file", EditorIcons::saveFile);
	LOAD_EPATH_IF_URL("create-link", EditorIcons::urlIcon);
	LOAD_EPATH_IF_URL("create-image", EditorIcons::imageIcon);
	LOAD_EPATH_IF_URL("create-table", EditorIcons::tableIcon);

#if USE_BACKEND
	LOAD_EPATH_IF_URL("show-settings", BackendBinaryData::ToolbarIcons::settings);
#endif

	return p;
}

MarkdownEditorPanel::MarkdownEditorPanel(FloatingTile* root) :
	FloatingTileContent(root),
	tdoc(doc),

#if HISE_USE_NEW_CODE_EDITOR
	editor(tdoc),
#else
	editor(doc, &tokeniser),
#endif
	livePreview("Live Preview", this, f),
	newButton("New File", this, f),
	openButton("Open File", this, f),
	saveButton("Save File", this, f),
	urlButton("Create Link", this, f),
	imageButton("Create image", this, f),
	tableButton("Create Table", this, f),
	settingsButton("Show Settings", this, f)
{
	setLookAndFeel(&laf);

	livePreview.setToggleModeWithColourChange(true);
	livePreview.setToggleState(false, sendNotification);

	addAndMakeVisible(editor);
	addAndMakeVisible(livePreview);
	addAndMakeVisible(newButton);
	addAndMakeVisible(openButton);
	addAndMakeVisible(saveButton);
	addAndMakeVisible(urlButton);
	addAndMakeVisible(imageButton);
	addAndMakeVisible(tableButton);
	addAndMakeVisible(settingsButton);

	livePreview.setTooltip("Enable live preview of the editor's content");
	newButton.setTooltip("Create new file");
	openButton.setTooltip("Open a file");
	saveButton.setTooltip("Save a file");
	urlButton.setTooltip("Create a link");
	imageButton.setTooltip("Create a image link");
	tableButton.setTooltip("Create a table");
	settingsButton.setTooltip("Show settings");

#if HISE_USE_NEW_CODE_EDITOR
    editor.editor.setLanguageManager(new mcl::MarkdownLanguageManager());
#endif

}

MarkdownEditorPanel::~MarkdownEditorPanel()
{
}

bool MarkdownEditorPanel::updatePreview()
{
	if (preview.getComponent() != nullptr)
		return true;

	auto p = dynamic_cast<MarkdownPreview*>(getMainController()->getCurrentMarkdownPreview());

	if (p != nullptr)
	{
		setPreview(p);
		return true;
	}
			
	return false;
}

void MarkdownEditorPanel::buttonClicked(Button* b)
{
	if (b == &newButton)
	{
		FileChooser fc("Create new file", getRootFile(), "*.md");

		if (fc.browseForFileToSave(true))
		{
			currentFile = fc.getResult();

			auto fName = currentFile.getFileNameWithoutExtension();

			if (MarkdownLink::Helpers::getSanitizedFilename(fName) != fName)
			{
				PresetHandler::showMessageWindow("No valid URL", "You need to use a valid URL for the file name\nNo whitespace, no uppercase", PresetHandler::IconType::Error);
				return;
			}

			MarkdownHeader::createEmptyMarkdownFileWithMarkdownHeader(currentFile.getParentDirectory(), currentFile.getFileNameWithoutExtension(), "New file");

			loadFile(currentFile);

			if (updatePreview())
			{
				preview->getHolder().rebuildDatabase();
			}
		}
	}

	if (b == &openButton)
	{
		FileChooser fc("Load file", getRootFile(), "*.md");

		if (fc.browseForFileToOpen())
		{
			loadFile(fc.getResult());
		}
	}
	if (b == &saveButton)
	{
		if (currentFile.existsAsFile())
		{
			if (PresetHandler::showYesNoWindow("Overwrite file", "Do you want to overwrite " + currentFile.getFileName()))
			{
				currentFile.replaceWithText(doc.getAllContent());
			}
		}
		else
		{
			FileChooser fc("Save file", currentFile, "*.md");

			if (fc.browseForFileToSave(true))
			{
				currentFile = fc.getResult();
				currentFile.replaceWithText(doc.getAllContent());
			}
		}
	}
	if (b == &settingsButton)
	{
		auto window = new SettingWindows(dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject());
		window->setLookAndFeel(&laf);

		window->setModalBaseWindowComponent(this);
		window->activateSearchBox();
	}

	Component* c = nullptr;
	Point<int> p;

	if (b == &tableButton)
	{
		c = new MarkdownEditorPopupComponents::TableCreator(*this);
		p = tableButton.getBoundsInParent().getCentre().translated(0, 15);
	}
	if (b == &imageButton)
	{
		c = new MarkdownEditorPopupComponents::ImageCreator(*this);
		p = imageButton.getBoundsInParent().getCentre().translated(0, 15);
	}
	if (b == &urlButton)
	{
		c = new MarkdownEditorPopupComponents::LinkCreator(*this);
		p = urlButton.getBoundsInParent().getCentre().translated(0, 15);
	}

	if (c != nullptr)
	{
		getParentShell()->showComponentInRootPopup(c, this, p);
		c->grabKeyboardFocus();
	}
	
}

void MarkdownEditorPanel::setPreview(MarkdownPreview* p)
{
	if (p != nullptr)
	{
		preview = p;
		syncer = new mcl::MarkdownPreviewSyncer(editor, *p);
		syncer->setEnableScrollbarListening(true);
	}
			
}

bool MarkdownEditorPanel::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::F5Key)
	{
		if(syncer != nullptr)
			syncer->startTimer(500);
            
		return true;
	}
	if ((key.getKeyCode() == 's' ||
		key.getKeyCode() == 'S') && key.getModifiers().isCommandDown())
	{
		saveButton.triggerClick();
		return true;
	}

	return false;
}

void MarkdownEditorPanel::loadText(const String& s)
{
	currentFile = File();
	doc.replaceAllContent(s);
	setCustomTitle("Editor");
	getParentShell()->refreshRootLayout();
}

void MarkdownEditorPanel::loadFile(File file)
{
	currentFile = file;
	doc.replaceAllContent(currentFile.loadFileAsString());

	

	setCustomTitle("Editor - " + currentFile.getFileName());
	getParentShell()->refreshRootLayout();
}

juce::File MarkdownEditorPanel::getRootFile()
{
	return getMainController()->getProjectDocHolder()->getDatabaseRootDirectory();
}

void MarkdownEditorPanel::resized()
{
	auto area = getParentShell()->getContentBounds();

	auto top = area.removeFromTop(46);

	int button_margin = 10;

	livePreview.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));

	top.removeFromLeft(20);

	newButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));
	openButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));
	saveButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));

	top.removeFromLeft(20);

	urlButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));
	imageButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));
	tableButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));

	settingsButton.setBounds(top.removeFromRight(top.getHeight()).reduced(button_margin));

	int w = jmin(area.getWidth(), 2000);

	editor.setBounds(area.withSizeKeepingCentre(w, area.getHeight()));
}

}
