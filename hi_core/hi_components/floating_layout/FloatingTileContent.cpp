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
	setContentOwned(new FloatingTile(nullptr, var()), false);

	setResizable(true, true);
	setUsingNativeTitleBar(true);

	centreWithSize(1500, 1000);

	auto fsc = dynamic_cast<FloatingTile*>(getContentComponent());
	fsc->setLayoutModeEnabled(false);
	
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
	//ValueTree v = dynamic_cast<FloatingTile*>(getContentComponent())->getCurrentFloatingPanel()->exportAsValueTree();

	File f("D:\\testa.xml");

	//v.createXml()->writeToFile(f, "");

	delete this;
}

Identifier FloatingTileContent::getDefaultablePropertyId(int index) const
{
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::Type, "Type");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::Title, "Title");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::StyleData, "StyleData");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::LayoutData, "LayoutData");

	jassertfalse;

	return Identifier();
}

var FloatingTileContent::getDefaultProperty(int id) const
{
	auto prop = (PanelPropertyId)id;

	switch (prop)
	{
	case FloatingTileContent::Type: return var(); // This property is not defaultable
		break;
	case FloatingTileContent::Title: return "";
	case FloatingTileContent::StyleData:
	{ 
		DynamicObject::Ptr newStyleData = new DynamicObject(); 
		return var(newStyleData);	
	}
	case FloatingTileContent::LayoutData: return var(); // this property is restored explicitely
	case FloatingTileContent::numPropertyIds:
	default:
		break;
	}

	return var();
}

BackendRootWindow* FloatingTileContent::getRootWindow()
{
	return getParentShell()->getRootWindow();
}

const BackendRootWindow* FloatingTileContent::getRootWindow() const
{
	return getParentShell()->getRootWindow();
}

var FloatingTileContent::toDynamicObject() const
{
	DynamicObject::Ptr o = new DynamicObject();

	var obj(o);

	storePropertyInObject(obj, FloatingTileContent::PanelPropertyId::Type, getIdentifierForBaseClass().toString());
	storePropertyInObject(obj, FloatingTileContent::PanelPropertyId::Title, getCustomTitle(), "");
	storePropertyInObject(obj, PanelPropertyId::StyleData, var(styleData));
	storePropertyInObject(obj, PanelPropertyId::LayoutData, var(getParentShell()->getLayoutData().getLayoutDataObject()));

	if (getFixedSizeForOrientation() != 0)
		o->removeProperty("Size");

	return obj;
}


void FloatingTileContent::fromDynamicObject(const var& object)
{
	setCustomTitle(getPropertyWithDefault(object, PanelPropertyId::Title));

	styleData = getPropertyWithDefault(object, PanelPropertyId::StyleData);

	getParentShell()->setLayoutDataObject(getPropertyWithDefault(object, PanelPropertyId::LayoutData));
}

FloatingTileContent* FloatingTileContent::createPanel(const var& data, FloatingTile* parent)
{
	if (auto obj = data.getDynamicObject())
	{
		auto panelId = Identifier(obj->getProperty("Type"));

		auto p = parent->getPanelFactory()->createFromId(panelId, parent);

		jassert(p != nullptr);

		if (p != nullptr)
		{
			p->fromDynamicObject(data);
		}
			
		return p;
	}
	else
	{
		jassertfalse;

		return new EmptyComponent(parent);
	}
}


FloatingTileContent* FloatingTileContent::createNewPanel(const Identifier& id, FloatingTile* parent)
{
	return parent->getPanelFactory()->createFromId(id, parent);
}

void FloatingTileContent::setDynamicTitle(const String& newDynamicTitle)
{
	dynamicTitle = newDynamicTitle;
	getParentShell()->repaint();
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

		parent->getComponent(index)->getLayoutData().setCurrentSize(size);
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


void JSONEditor::replace()
{
	if (editedComponent.getComponent() != nullptr)
	{
		var newData;

		auto result = JSON::parse(doc->getAllContent(), newData);

		if (result.wasOk())
		{
			dynamic_cast<ObjectWithDefaultProperties*>(editedComponent.getComponent())->fromDynamicObject(newData);

			auto parent = dynamic_cast<FloatingTileContent*>(editedComponent.getComponent())->getParentShell();

			jassert(parent != nullptr);

			parent->refreshRootLayout();
			parent->refreshPinButton();
			parent->refreshFoldButton();
			parent->refreshMouseClickTarget();

			editedComponent->repaint();
		}
		else
		{
			PresetHandler::showMessageWindow("JSON Parser Error", result.getErrorMessage(), PresetHandler::IconType::Error);
		}
	}
}
