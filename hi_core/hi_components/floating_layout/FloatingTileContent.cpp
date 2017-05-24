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


FloatingFlexBoxWindow::FloatingFlexBoxWindow() :
	DocumentWindow("HISE Floating Window", Colour(0xFF333333), allButtons, true)
{
	ValueTree state;


	setContentOwned(new FloatingTile(nullptr, state), false);

	setResizable(true, true);
	setUsingNativeTitleBar(true);

	centreWithSize(1500, 1000);

	auto fsc = dynamic_cast<FloatingTile*>(getContentComponent());
	fsc->setLayoutModeEnabled(false, true);
	
	FloatingInterfaceBuilder ib(fsc);

	const int root = 0;

#if 0
	ib.setNewContentType<FloatingTabComponent>(root);

	const int firstVertical = ib.addChild<VerticalTile>(root);

	const int leftColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int mainColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int rightColumn = ib.addChild<HorizontalTile>(firstVertical);

	ib.setSizes(firstVertical, { -0.5, 900.0, -0.5 }, dontSendNotification);
	ib.setAbsoluteSize(firstVertical, { false, true, false }, dontSendNotification);
	ib.setLocked(firstVertical, { false, true, false}, sendNotification);
	ib.setDeletable(root, false, { false });
	ib.setDeletable(firstVertical, false, { false, false, false });


	const int mainArea = ib.addChild<EmptyComponent>(mainColumn);
	const int keyboard = ib.addChild<MidiKeyboardPanel>(mainColumn);

	ib.setSwappable(firstVertical, false, { false, false, false });
	ib.setSwappable(mainColumn, false, { false, false });

	ib.getPanel(firstVertical)->setDeletable(false);
	ib.getContainer(firstVertical)->setAllowInserting(false);
	ib.getPanel(mainColumn)->setReadOnly(true);
	ib.getPanel(root)->setDeletable(false);

	ib.setCustomName(firstVertical, "Main Workspace", { "Left Panel", "Mid Panel", "Right Panel" });
	

	ib.getPanel(mainColumn)->getParentContainer()->refreshLayout();

	ib.finalizeAndReturnRoot(true);
#endif

	//FloatingPanelTemplates::createMainPanel(fsc);
}

void FloatingFlexBoxWindow::closeButtonPressed()
{
	ValueTree v = dynamic_cast<FloatingTile*>(getContentComponent())->getCurrentFloatingPanel()->exportAsValueTree();

	File f("D:\\testa.xml");

	//v.createXml()->writeToFile(f, "");

	delete this;
}

BackendRootWindow* FloatingTileContent::getRootWindow()
{
	return getParentShell()->getRootWindow();
}

const BackendRootWindow* FloatingTileContent::getRootWindow() const
{
	return getParentShell()->getRootWindow();
}

FloatingTileContent* FloatingTileContent::createPanel(ValueTree& state, FloatingTile* parent)
{
	auto p = parent->getPanelFactory()->createFromId(state.getType(), parent);


	jassert(p != nullptr);

	if(p != nullptr)
		p->restoreFromValueTree(state);

	return p;
}


FloatingTileContent* FloatingTileContent::createNewPanel(const Identifier& id, FloatingTile* parent)
{
	return parent->getPanelFactory()->createFromId(id, parent);
}

const BackendProcessorEditor* FloatingTileContent::getMainPanel() const
{
	return getParentShell()->findParentComponentOfClass<BackendRootWindow>()->getMainPanel();
}

BackendProcessorEditor* FloatingTileContent::getMainPanel()
{
	return getParentShell()->findParentComponentOfClass<BackendRootWindow>()->getMainPanel();
}

int FloatingTileContent::getFixedSizeForOrientation() const
{
	auto pType = getParentShell()->getParentType();

	if (pType == FloatingTile::ParentType::Horizontal)
		return getFixedWidth();
	else if (pType == FloatingTile::ParentType::Vertical)
		return getFixedHeight();
	else
		return 0;
}

#if 0

FloatingPanel* FloatingPanel::createPanel(ValueTree& state, FloatingShellComponent* parent)
{
	auto pt = getPanelTypeForId(Identifier(state.getType()));

	FloatingPanel* p = createNewPanel(pt, parent);

	if (p != nullptr)
		p->restoreFromValueTree(state);

	return p;
}

FloatingPanel* FloatingPanel::Factory::createNewPanel(PanelType pt, FloatingShellComponent* parent)
{
	FloatingPanel* p = nullptr;

	switch (pt)
	{
	case PanelType::PTHorizontalTile: p = new HorizontalTile(parent); break;
	case PanelType::PTVerticalTile:	p = new VerticalTile(parent); break;
	case PanelType::PTTabs:			p = new FloatingTabComponent(parent); break;
	case PanelType::PTNote:			p = new Note(parent); break;
	}

	if (p == nullptr)
		return new EmptyComponent(parent);

	return p;
}
#endif

struct FloatingPanelTemplates::Helpers
{
	static void addNewShellTo(FloatingTileContainer* parent)
	{
		parent->addFloatingTile(new FloatingTile(parent));
	}

	static FloatingTileContainer* getChildContainer(FloatingTile* parent)
	{
		return dynamic_cast<FloatingTileContainer*>(parent->getCurrentFloatingPanel());
	}

	template <class PanelType> static void setContent(FloatingTile* parent)
	{
		parent->setNewContent(PanelType::getPanelId());
	};

	static FloatingTile* getChildShell(FloatingTile* parent, int index)
	{
		auto c = dynamic_cast<FloatingTileContainer*>(parent->getCurrentFloatingPanel());

		jassert(c != nullptr);

		return c->getComponent(index);
	}

	static void setShellSize(FloatingTileContainer* parent, int index, double size, bool isRelative)
	{
		if (isRelative)
			size *= -1.0;

		parent->getComponent(index)->getLayoutData().currentSize = size;
	}
};


void FloatingPanelTemplates::create2x2Matrix(FloatingTile* parent)
{
	Helpers::setContent<HorizontalTile>(parent);
	Helpers::addNewShellTo(Helpers::getChildContainer(parent));

	Helpers::setContent<VerticalTile>(Helpers::getChildShell(parent, 0));
	Helpers::setContent<VerticalTile>(Helpers::getChildShell(parent, 1));

	auto v1 = Helpers::getChildContainer(Helpers::getChildShell(parent, 0));
	auto v2 = Helpers::getChildContainer(Helpers::getChildShell(parent, 1));

	Helpers::addNewShellTo(v1);
	Helpers::addNewShellTo(v2);
}

void FloatingPanelTemplates::create3Columns(FloatingTile* parent)
{
	Helpers::setContent<VerticalTile>(parent);

	auto c = Helpers::getChildContainer(parent);

	Helpers::addNewShellTo(c);
	Helpers::addNewShellTo(c);
}

void FloatingPanelTemplates::create3Rows(FloatingTile* parent)
{
	Helpers::setContent<HorizontalTile>(parent);

	auto c = Helpers::getChildContainer(parent);

	Helpers::addNewShellTo(c);
	Helpers::addNewShellTo(c);
}



void FloatingTileContent::Factory::registerAllPanelTypes()
{
#if PUT_FLOAT_IN_CODEBASE
	registerType<MainPanel>();
	registerType<MainTopBar>();
#endif
	registerType<SpacerPanel>();
	registerType<HorizontalTile>();
	registerType<VerticalTile>();
	registerType<FloatingTabComponent>();
	registerType<EmptyComponent>();
	registerType<Note>();
	registerType<MidiKeyboardPanel>();
	registerType<TableEditorPanel>();
	registerType<CodeEditorPanel>();
	registerType<SliderPackPanel>();
	registerType<ConsolePanel>();
	registerType<ApplicationCommandButtonPanel>();

}


void addCommandIcon(FloatingTile* parent, PopupMenu& m, int commandID)
{
	ApplicationCommandInfo r(commandID);

	parent->getRootWindow()->getCommandInfo(commandID, r);

	m.addItem(10000 + commandID, r.shortName);
}

void FloatingTileContent::Factory::handlePopupMenu(PopupMenu& m, FloatingTile* parent)
{
	if (parent->isReadOnly())
		return;

	enum class PopupMenuOptions
	{
		Cancel = 0,
		Empty,
		Spacer,
		BigResizer,
		HorizontalTile,
		VerticalTile,
		Tabs,
		Matrix2x2,
		ThreeColumns,
		ThreeRows,
		Note,
		MidiKeyboard,
		ScriptEditor,
		TablePanel,
		SliderPackPanel,
		Console,
		toggleLayoutMode,
		toggleGlobalLayoutMode,
		MenuCommandOffset = 10000,
		numOptions
	};

	if (parent->canBeDeleted())
	{
		if (parent->isLayoutModeEnabled())
		{
			m.addSectionHeader("Layout Elements");
			m.addItem((int)PopupMenuOptions::Spacer, "Spacer");
			m.addItem((int)PopupMenuOptions::BigResizer, "Big Resizer");
			m.addItem((int)PopupMenuOptions::HorizontalTile, "Horizontal Tile");
			m.addItem((int)PopupMenuOptions::VerticalTile, "Vertical Tile");
			m.addItem((int)PopupMenuOptions::Tabs, "Tabs");
			m.addItem((int)PopupMenuOptions::Matrix2x2, "2x2 Matrix");
			m.addItem((int)PopupMenuOptions::ThreeColumns, "3 Columns");
			m.addItem((int)PopupMenuOptions::ThreeRows, "3 Rows");
		}

		m.addSectionHeader("Misc Tools");
		m.addItem((int)PopupMenuOptions::Note, "Note");
		m.addItem((int)PopupMenuOptions::MidiKeyboard, "Virtual Keyboard");
		m.addItem((int)PopupMenuOptions::TablePanel, "Table Editor");
		m.addItem((int)PopupMenuOptions::SliderPackPanel, "Array Editor");
		m.addItem((int)PopupMenuOptions::ScriptEditor, "Script Editor");
		m.addItem((int)PopupMenuOptions::Console, "Console");
		m.addSeparator();

		PopupMenu icons;

		addCommandIcon(parent, icons, BackendCommandTarget::MainToolbarCommands::MenuNewFile);
		addCommandIcon(parent, icons, BackendCommandTarget::MainToolbarCommands::MenuOpenFile);
		addCommandIcon(parent, icons, BackendCommandTarget::MainToolbarCommands::MenuSaveFile);
		addCommandIcon(parent, icons, BackendCommandTarget::MainToolbarCommands::MenuEditUndo);
		addCommandIcon(parent, icons, BackendCommandTarget::MainToolbarCommands::MenuEditRedo);
		addCommandIcon(parent, icons, BackendCommandTarget::MainToolbarCommands::Settings);
		
		m.addSubMenu("Icons", icons);

	}

	m.addItem((int)PopupMenuOptions::toggleLayoutMode, "Toggle Layout Mode for Parent", true, parent->isLayoutModeEnabled());
	m.addItem((int)PopupMenuOptions::toggleGlobalLayoutMode, "Toggle Global Layout Mode", true, parent->getRootComponent()->isLayoutModeEnabled());



	const int result = m.show();

	if (result > (int)PopupMenuOptions::MenuCommandOffset)
	{
		int c = result - (int)PopupMenuOptions::MenuCommandOffset;
		parent->setNewContent("Icon");
		return;
	}

	PopupMenuOptions resultOption = (PopupMenuOptions)result;



	switch (resultOption)
	{
	case PopupMenuOptions::Cancel:				return;
	case PopupMenuOptions::Empty:				parent->setNewContent(GET_PANEL_NAME(EmptyComponent)); break;
	case PopupMenuOptions::Spacer:				parent->setNewContent(GET_PANEL_NAME(SpacerPanel)); break;
	case PopupMenuOptions::HorizontalTile:		parent->setNewContent(GET_PANEL_NAME(HorizontalTile)); break;
	case PopupMenuOptions::VerticalTile:		parent->setNewContent(GET_PANEL_NAME(VerticalTile)); break;
	case PopupMenuOptions::Tabs:				parent->setNewContent(GET_PANEL_NAME(FloatingTabComponent)); break;
	case PopupMenuOptions::Matrix2x2:			FloatingPanelTemplates::create2x2Matrix(parent); break;
	case PopupMenuOptions::ThreeColumns:		FloatingPanelTemplates::create3Columns(parent); break;
	case PopupMenuOptions::ThreeRows:			FloatingPanelTemplates::create3Rows(parent); break;
	case PopupMenuOptions::Note:				parent->setNewContent(GET_PANEL_NAME(Note)); break;
	case PopupMenuOptions::MidiKeyboard:		parent->setNewContent(GET_PANEL_NAME(MidiKeyboardPanel)); break;
	case PopupMenuOptions::TablePanel:			parent->setNewContent(GET_PANEL_NAME(TableEditorPanel)); break;
	case PopupMenuOptions::ScriptEditor:		parent->setNewContent(GET_PANEL_NAME(CodeEditorPanel)); break;
	case PopupMenuOptions::SliderPackPanel:		parent->setNewContent(GET_PANEL_NAME(SliderPackPanel)); break;
	case PopupMenuOptions::Console:				parent->setNewContent(GET_PANEL_NAME(ConsolePanel)); break;
	case PopupMenuOptions::toggleLayoutMode:    parent->toggleLayoutModeForParentContainer(); break;
	case PopupMenuOptions::toggleGlobalLayoutMode:    parent->getRootComponent()->toggleLayoutModeForParentContainer(); break;
	case PopupMenuOptions::numOptions:
	default:
		jassertfalse;
		break;
	}
}

