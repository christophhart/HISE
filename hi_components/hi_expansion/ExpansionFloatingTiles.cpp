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



class ExpansionPathFactory : public PathFactory
{
public:

	String getId() const override { return "Expansion Pack"; }

	Path createPath(const String& id) const override
	{
		auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

		Path p;

		LOAD_EPATH_IF_URL("filebased", ExpansionIcons::filebased);
		LOAD_EPATH_IF_URL("intermediate", ExpansionIcons::intermediate);
		LOAD_EPATH_IF_URL("encrypted", ExpansionIcons::encrypted);
		LOAD_EPATH_IF_URL("new", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);

		LOAD_EPATH_IF_URL("open", EditorIcons::openFile);
		LOAD_PATH_IF_URL("rebuild", ColumnIcons::moveIcon);


		BACKEND_ONLY(LOAD_EPATH_IF_URL("edit", OverlayIcons::penShape));
		LOAD_EPATH_IF_URL("undo", EditorIcons::undoIcon);
		LOAD_EPATH_IF_URL("redo", EditorIcons::redoIcon);
		LOAD_EPATH_IF_URL("encode", SampleMapIcons::monolith);

		return p;
	};
};



ExpansionEditBar::ExpansionEditBar(FloatingTile* parent) :
	FloatingTileContent(parent),
	factory(new ExpansionPathFactory())
{
	ExpansionPathFactory f;

	buttons.add(new HiseShapeButton("New", this, f));  buttons.getLast()->setTooltip("Create a new expansion pack folder");
	buttons.add(new HiseShapeButton("Edit", this, f)); buttons.getLast()->setTooltip("Edit the current expansion");
	buttons.add(new HiseShapeButton("Rebuild", this, f)); buttons.getLast()->setTooltip("Refresh the expansion pack data");
	buttons.add(new HiseShapeButton("Encode", this, f)); buttons.getLast()->setTooltip("Encode this expansion pack");

	addAndMakeVisible(expansionSelector = new ComboBox("Expansion Selector"));

	expansionSelector->addListener(this);
	expansionSelector->setTextWhenNothingSelected("Select Expansion");
	expansionSelector->setTextWhenNoChoicesAvailable("No Expansions available");
	
	getMainController()->skin(*expansionSelector);

	refreshExpansionList();

	auto& handler = getMainController()->getExpansionHandler();

	handler.addListener(this);
		

	for (auto b : buttons)
		addAndMakeVisible(b);
}

void ExpansionEditBar::refreshExpansionList()
{
	auto& handler = getMainController()->getExpansionHandler();

	auto list = *handler.getListOfAvailableExpansions().getArray();

	expansionSelector->clear(dontSendNotification);

	expansionSelector->addItem("No expansion", 4096);

	for (int i = 0; i < list.size(); i++)
	{
		expansionSelector->addItem(list[i].toString(), i + 1);
	}
}

ExpansionEditBar::~ExpansionEditBar()
{
	auto& handler = getMainController()->getExpansionHandler();

	handler.removeListener(this);
}

void ExpansionEditBar::resized()
{
	const int widthForIcon = getHeight();
	const int spacerWidth = 15;

	auto area = getLocalBounds().reduced(3);

	getButton("New")->setBounds(area.removeFromLeft(widthForIcon));
	
	area.removeFromLeft(spacerWidth);

	
	getButton("Encode")->setBounds(area.removeFromRight(widthForIcon));
	getButton("Edit")->setBounds(area.removeFromRight(widthForIcon));
	getButton("Rebuild")->setBounds(area.removeFromRight(widthForIcon));
	

	area.removeFromRight(spacerWidth);

	expansionSelector->setBounds(area.expanded(1));
}



struct ExpansionPopupBase : public Component,
						    public ControlledObject,
							public ExpansionHandler::Listener
{
	ExpansionPopupBase(MainController* mc) :
		Component("Edit expansion"),
		ControlledObject(mc),
		r("")
	{
		mc->getExpansionHandler().addListener(this);
	};

	~ExpansionPopupBase()
	{
		getMainController()->getExpansionHandler().removeListener(this);
	}

	virtual void initialise() = 0;

	void setMarkdownText(const String& s, int w, int h)
	{
		r.setDatabaseHolder(dynamic_cast<MarkdownDatabaseHolder*>(getMainController()));
		r.setNewText(s);
		r.setTargetComponent(this);
		r.parse();

		h += (int)r.getHeightForWidth((float)(w-20));

		setSize(w, h + 20);
	}

	void expansionPackLoaded(Expansion* currentExpansion) override
	{
		Component::SafePointer<ExpansionPopupBase> safeThis(this);
		auto f = [safeThis]()
		{
			if (safeThis.getComponent() != nullptr)
			{
				if (auto p = safeThis.getComponent()->findParentComponentOfClass<FloatingTilePopup>())
					p->deleteAndClose();
			};
		};

		MessageManager::callAsync(f);
	}

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds();
		b.removeFromTop(panelHeight).toFloat();

		r.draw(g, b.toFloat().reduced(10.0f));
	}

	void resized() override
	{
		auto b = getLocalBounds();
		b.removeFromTop(panelHeight);

		r.setChildComponentBounds(b.reduced(10));
		r.updateCreatedComponents();
	}
	
	int panelHeight = 0;
	MarkdownRenderer r;
	
	ExpansionPathFactory f;

	BlackTextButtonLookAndFeel blaf;

	
};

struct ExpansionEditPopup : public ExpansionPopupBase
{
	ExpansionEditPopup(MainController* mc) :
		ExpansionPopupBase(mc),
		unlockButton("Unlock")
	{};

	void initialise() override
	{
		int h = 0;

		auto mc = getMainController();

		if (auto e = mc->getExpansionHandler().getCurrentExpansion())
		{
			setName("Edit " + e->getProperty(ExpansionIds::Name));

			eType = e->getExpansionType();

			if (eType == Expansion::FileBased)
			{
				Array<PropertyComponent*> props;

				auto p = e->getPropertyValueTree();

				for (int i = 0; i < p.getNumProperties(); i++)
				{
					auto id = p.getPropertyName(i);
					auto tp = new TextPropertyComponent(p.getPropertyAsValue(id, getMainController()->getControlUndoManager()), id.toString(), 100, false);
					tp->setLookAndFeel(&plaf);

					h += tp->getPreferredHeight();

					props.add(tp);
				}

				panel.addProperties(props);
				addAndMakeVisible(panel);

				panelHeight = h;

			}
			else
			{
				addAndMakeVisible(unlockButton);
				unlockButton.setLookAndFeel(&blaf);

				unlockButton.onClick = [mc, e, this]()
				{
					if (PresetHandler::showYesNoWindow("Unlock this expansion", "Do you want to delete the intermediate / encrypted file and revert to a file-based expansion for editing?", PresetHandler::IconType::Question))
					{
						auto f = Expansion::Helpers::getExpansionInfoFile(e->getRootFolder(), e->getExpansionType());

						if (!f.hasFileExtension(".xml"))
						{
							f.deleteFile();
							mc->getExpansionHandler().forceReinitialisation();
						}
					}
				};

				h = 80;
				panelHeight = 80;
			}

			String s;

			s << "### Expansion Content\n";
			s << "| Type | Items | Size |\n";
			s << "| ===== | == | == |\n";

			auto addRow = [&s, e](FileHandlerBase::SubDirectories type, bool lookForFiles)
			{
				s << "| **" << FileHandlerBase::getIdentifier(type).removeCharacters("/") << "** | ";

				int64 fileSize = 0;

				if (lookForFiles)
				{
					auto d = e->getSubDirectory(type);
					jassert(d.isDirectory());

					auto wc = FileHandlerBase::getWildcardForFiles(type);
					auto list = d.findChildFiles(File::findFiles, true, wc);

					for (auto l : list)
						fileSize += l.getSize();

					s << list.size() << " | ";
				}
				else
				{
					auto poolToUse = e->pool->getPoolBase(type);

					int embedded = poolToUse->getDataProvider()->getListOfAllEmbeddedReferences().size();
					int loaded = poolToUse->getNumLoadedFiles();

					fileSize = (int64)poolToUse->getDataProvider()->getSizeOfEmbeddedReferences();

					s << jmax(embedded, loaded) << " | ";
				}

				s << "`" << String((double)fileSize / 1024.0 / 1024.0, 1) << " MB` |\n";
			};

			addRow(FileHandlerBase::AdditionalSourceCode, eType == Expansion::FileBased);
			addRow(FileHandlerBase::AudioFiles, eType == Expansion::FileBased);
			addRow(FileHandlerBase::SampleMaps, false);
			addRow(FileHandlerBase::Images, eType == Expansion::FileBased);
			addRow(FileHandlerBase::MidiFiles, false);

			setMarkdownText(s, 350, h);
		}
	}

	void resized() override
	{
		ExpansionPopupBase::resized();

		auto top = getLocalBounds().removeFromTop(panelHeight);

		if (unlockButton.isVisible())
			unlockButton.setBounds(top.reduced(10).removeFromRight(80).withSizeKeepingCentre(80, 30));
		else
			panel.setBounds(top);
	}

	void paint(Graphics& g) override
	{
		ExpansionPopupBase::paint(g);

		auto top = getLocalBounds().toFloat().removeFromTop((float)panelHeight);

		String eName;
		switch (eType)
		{
		case Expansion::FileBased: eName = "File based"; break;
		case Expansion::Intermediate: eName = "Intermediate"; break;
		case Expansion::Encrypted: eName = "Encrypted"; break;
        default: jassertfalse; break;
		}

		auto p = f.createPath(eName);

		g.setColour(Colours::white.withAlpha(0.8f));
		g.setFont(GLOBAL_BOLD_FONT());

		f.scalePath(p, top.withSizeKeepingCentre(30, 30));
		g.fillPath(p);
		g.drawText(eName, top, Justification::centredBottom);
	}

	Expansion::ExpansionType eType;
	HiPropertyPanelLookAndFeel plaf;
	PropertyPanel panel;
	TextButton unlockButton;
};

class ExpansionHandlerPopup : public ExpansionPopupBase
{
public:

	ExpansionHandlerPopup(MainController* mc) :
		ExpansionPopupBase(mc),
		resetButton("Reset encryption"),
		refreshButton("Refresh expansions")
	{
		addAndMakeVisible(resetButton);
		resetButton.setLookAndFeel(&blaf);
		addAndMakeVisible(refreshButton);
		refreshButton.setLookAndFeel(&blaf);
	};

	void resized()
	{
		ExpansionPopupBase::resized();
		auto top = getLocalBounds().removeFromTop(50);

		refreshButton.setBounds(top.removeFromLeft(180).reduced(10));
		resetButton.setBounds(top.removeFromRight(180).reduced(10));
	}

	void initialise() override
	{
		String s;

		s << "### Global Expansion Properties\n";

		auto mc = getMainController();

		auto key = mc->getExpansionHandler().getEncryptionKey();
		if (key.isEmpty())
			key = "undefined";

		auto& h = mc->getExpansionHandler();

		s << "There are " << h.getNumExpansions() << " expansions that have been initialised successfully.  \n";

		if (auto e = h.getCurrentExpansion())
			s << "The current expansion is: " << e->getProperty(ExpansionIds::Name) << "\n";
		else
			s << "The current expansion has not been set\n";

		s << "#### Allowed expansion types\n";
		
		for (auto e : mc->getExpansionHandler().getAllowedExpansionTypes())
			s << "- **" << Expansion::Helpers::getExpansionTypeName(e) << "**\n";

		s << "#### Expansion list\n";

		s << "| Expansion | Type |\n";
		s << "| ==== | === |\n";

		for (int i = 0; i < h.getNumExpansions(); i++)
		{

			auto e = h.getExpansion(i);
			auto boldener = h.getCurrentExpansion() == e ? "**" : "";


			s << "| " << boldener << e->getProperty(ExpansionIds::Name) << boldener << " | ";

			switch (e->getExpansionType())
			{
			case Expansion::FileBased: s << "File-Based |\n"; break;
			case Expansion::Intermediate: s << "Intermediate |\n"; break;
			case Expansion::Encrypted: s << "Encrypted |\n"; break;
            default:                   jassertfalse; break;
			}
		}

		s << "\n";

		if (!h.initialisationErrors.isEmpty())
		{
			s << "##### Initialisation error details\n";
			s << "| Expansion | Error |\n";
			s << "| === | ======== |\n";

			for (auto e : h.initialisationErrors)
			{
				s << "| " << e.e->getProperty(ExpansionIds::Name) << " | " << e.r.getErrorMessage() << " |\n";
			}
		}

		s << "##### EncryptionKey\n`" << key << "`  \n";

		if (key == "undefined")
			s << "> Use `EncryptionHandler.setEncryptionKey()` in order to set a key that will be used to encrypt the expansion.\n";

		s << "##### Credentials\n";
		s << "```javascript\n";
		s << JSON::toString(mc->getExpansionHandler().getCredentials());
		s << "```\n\n";

		panelHeight = 50;
		setMarkdownText(s, 500, 50);
	}

	TextButton resetButton;
	TextButton refreshButton;
};


void ExpansionEditBar::buttonClicked(Button* b)
{
	auto& handler = getMainController()->getExpansionHandler();

	if (b->getName() == "New")
	{
		FileChooser fc("Create new Expansion", handler.getExpansionFolder(), "", true);

		if (fc.browseForDirectory())
		{
			handler.createNewExpansion(fc.getResult());
			refreshExpansionList();
		}
	}
	if (b->getName() == "Edit")
	{
		auto c = new ExpansionEditPopup(getMainController());
		c->initialise();

		findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(c, this, b->getBoundsInParent().getCentre().translated(0, 20));
	}
	if (b->getName() == "Rebuild")
	{
		auto c = new ExpansionHandlerPopup(getMainController());
		c->initialise();

		findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(c, this, b->getBoundsInParent().getCentre().translated(0, 20));
	}
	if (b->getName() == "Encode")
	{
		auto m = new ExpansionEncodingWindow(getMainController(), handler.getCurrentExpansion(), false);
		m->setModalBaseWindowComponent(this);
	}
}

void ExpansionEditBar::expansionPackLoaded(Expansion* e)
{
	if (e != nullptr)
	{
		expansionSelector->setText(e->getProperty(ExpansionIds::Name), dontSendNotification);
	}
	else
	{
		expansionSelector->setText("No expansion", dontSendNotification);
	}
}

void ExpansionEditBar::comboBoxChanged(ComboBox* /*comboBoxThatHasChanged*/)
{
	auto& handler = getMainController()->getExpansionHandler();

	if (expansionSelector->getText() == "No expansion")
	{
		handler.setCurrentExpansion("");
	}
	else
	{
		handler.setCurrentExpansion(expansionSelector->getText());
	}

	
}

}
