/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#define S(x) String(x, 2)


#include "MainComponent.h"

#if !PUT_FLOAT_IN_CODEBASE
#include "../../hi_core/hi_components/floating_layout/FloatingLayout.cpp"
#endif

// Use this to quickly scale the window
#define SCALE_2 0



struct ValueTreeHelpers
{
	static var convertToDynamicObject(ValueTree& v)
	{
		DynamicObject::Ptr valueTreeAsObject = new DynamicObject();

		valueTreeAsObject->setProperty("Type", v.getType().toString());

		for (int i = 0; i < v.getNumProperties(); i++)
		{
			valueTreeAsObject->setProperty(v.getPropertyName(i), v.getProperty(v.getPropertyName(i)));
		}

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			var childAsObject = convertToDynamicObject(v.getChild(i));

			Identifier child = String(i);

			valueTreeAsObject->setProperty(child, childAsObject);
		}

		return valueTreeAsObject;
	}

	static ValueTree createFromDynamicObject(const var& object)
	{
		DynamicObject::Ptr obj = object.getDynamicObject();

		if (obj != nullptr)
		{
			ValueTree v(Identifier(obj->getProperty("Type")));

			auto props = obj->getProperties();

			for (int i = 0; i < props.size(); i++)
			{
				if (props.getName(i) == Identifier("Type"))
					continue;

				if (props.getValueAt(i).isObject())
				{
					ValueTree child = createFromDynamicObject(props.getValueAt(i));

					v.addChild(child, -1, nullptr);
				}
				else
				{
					v.setProperty(props.getName(i), props.getValueAt(i), nullptr);
				}
			}

			return v;
		}

		return {};
	}

};

String FloatingTile::exportAsJSON() const
{
	ValueTree v = getCurrentFloatingPanel()->exportAsValueTree();

	var vAsObject = ValueTreeHelpers::convertToDynamicObject(v);

	auto json = JSON::toString(vAsObject, false);

	return json;
}


void FloatingTile::loadFromJSON(const String& jsonData)
{
	var jsonVar;

	auto result = JSON::parse(jsonData, jsonVar);

	if (result.wasOk())
	{
		ValueTree v = ValueTreeHelpers::createFromDynamicObject(jsonVar);

		setContent(v);
	}
}


void FloatingTile::swapContainerType(const Identifier& containerId)
{
	auto v = getCurrentFloatingPanel()->exportAsValueTree();

	var vAsObject = ValueTreeHelpers::convertToDynamicObject(v);

	vAsObject.getDynamicObject()->setProperty("Type", containerId.toString());

	auto newContainer = ValueTreeHelpers::createFromDynamicObject(vAsObject);

	for (int i = 0; i < newContainer.getNumChildren(); i++)
	{
		newContainer.getChild(i).setProperty("Size", -0.5, nullptr);
	}

	setContent(newContainer);
}



struct DebugPanelLookAndFeel : public ResizableFloatingTileContainer::LookAndFeel
{
	void paintBackground(Graphics& g, ResizableFloatingTileContainer& container) override
	{
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff373737)));
		g.fillRect(container.getContainerBounds());

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff2c2c2c)));

		//g.drawVerticalLine(0, 0.0f, (float)container.getHeight());

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff5f5f5f)));

		//g.drawVerticalLine(container.getWidth() - 1, 0.0f, (float)container.getHeight());
	}
};

Component* FloatingPanelTemplates::createMainPanel(FloatingTile* rootTile)
{
	rootTile->setLayoutModeEnabled(false, true);

	FloatingInterfaceBuilder ib(rootTile);

	const int root = 0;

	ib.setNewContentType<HorizontalTile>(root);

	const int topBar = ib.addChild<MainTopBar>(root);

	ib.getContainer(root)->setIsDynamic(false);

	const int tabs = ib.addChild<FloatingTabComponent>(root);

	ib.getContainer(tabs)->setIsDynamic(true);

	ib.setSizes(root, { 32.0, -1.0 });
	
	ib.setFoldable(root, false, { false, false });

	const int firstVertical = ib.addChild<VerticalTile>(tabs);

	ib.getContainer(firstVertical)->setIsDynamic(false);
	ib.getPanel(firstVertical)->setVital(true);
	ib.getPanel(firstVertical)->getLayoutData().backgroundColour = HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright);

	const int leftColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int mainColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int rightColumn = ib.addChild<HorizontalTile>(firstVertical);

	

	ib.getContainer(leftColumn)->setIsDynamic(true);
	ib.getContainer(mainColumn)->setIsDynamic(false);
	ib.getContainer(rightColumn)->setIsDynamic(true);

	ib.getPanel(leftColumn)->getLayoutData().backgroundColour = Colour(0xFF222222);
	ib.getPanel(leftColumn)->getLayoutData().minSize = 150;
	ib.getPanel(leftColumn)->setCanBeFolded(true);
	
	ib.getPanel(mainColumn)->getLayoutData().backgroundColour = HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright);

	ib.getPanel(rightColumn)->getLayoutData().backgroundColour = Colour(0xFF222222);
	ib.getPanel(rightColumn)->getLayoutData().minSize = 150;
	ib.getPanel(rightColumn)->setCanBeFolded(true);

	ib.getPanel(root)->getLayoutData().backgroundColour = HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId);
	
	ib.setSizes(firstVertical, { -0.5, 900.0, -0.5 }, dontSendNotification);
	

	const int mainArea = ib.addChild<EmptyComponent>(mainColumn);
	const int keyboard = ib.addChild<MidiKeyboardPanel>(mainColumn);

	ib.getPanel(keyboard)->getLayoutData().backgroundColour = HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright);

	ib.setFoldable(mainColumn, false, { false, false });

	ib.setCustomName(firstVertical, "Main Workspace", { "Left Panel", "", "Right Panel" });

#if PUT_FLOAT_IN_CODEBASE
	ib.setNewContentType<MainPanel>(mainArea);
#endif

	ib.finalizeAndReturnRoot(true);

	return dynamic_cast<Component*>(ib.getPanel(mainArea)->getCurrentFloatingPanel());
}

void EmptyComponent::resized()
{
#if 0
	bool useRowLayout = getWidth() > getHeight();

	bool smallMode = false;

	bool isRow = flexBox.flexDirection == FlexBox::Direction::row;


	if (isRow && getWidth() < 200)
		smallMode = true;

	if (!isRow && getHeight() < 200)
		smallMode = true;

	flexBox.flexWrap = smallMode ? FlexBox::Wrap::noWrap : FlexBox::Wrap::wrap;


	int maxHeight = getHeight() - 40;

	if (useRowLayout)
	{
		flexBox.flexDirection = FlexBox::Direction::row;

		int width = (getWidth()-80) / flexBox.items.size();

		for (auto& item : flexBox.items)
		{
			item.width = width;
			item.maxWidth = width;
			item.minWidth = width;

			item.margin = 0;

			item.height = maxHeight;
			item.maxHeight = maxHeight;
			item.minHeight = maxHeight;
		}

	}
	else
	{
		flexBox.flexDirection = FlexBox::Direction::column;

		for (auto& item : flexBox.items)
		{
			
			item.minWidth = getWidth() - 40;
			item.width = getWidth() - 40;
			item.maxWidth = getWidth() - 40;

			int h = (maxHeight-30) / flexBox.items.size();

			item.height = h;

			item.minHeight = h;
			item.maxHeight = h;

		}
	}

	

	

	flexBox.performLayout(getLocalBounds().withTrimmedTop(16));
#endif
}


void EmptyComponent::Category::resized()
{
	//flexBox.flexDirection = getWidth() > 800 ? FlexBox::Direction::row : FlexBox::Direction::column;

	flexBox.alignContent = FlexBox::AlignContent::flexStart;
	flexBox.justifyContent = FlexBox::JustifyContent::center;


	bool smallMode = false;
	
	if (getWidth() < 200)
		smallMode = true;

	


	int maxWidth = getWidth() - 20;
	int maxHeight = getHeight() - 50;

	int s = 72;

	for (auto& item : flexBox.items)
	{
		item.height = s;
		item.maxHeight = s;
		item.minHeight = s;

		item.margin = 5;

		item.width = s;
		item.maxWidth = s;
		item.minWidth = s;
	}

	flexBox.justifyContent = FlexBox::JustifyContent::center;

	if (smallMode)
	{
		flexBox.flexWrap = FlexBox::Wrap::noWrap;
		flexBox.flexDirection = FlexBox::Direction::column;
		flexBox.alignContent = FlexBox::AlignContent::center;
		flexBox.justifyContent = FlexBox::JustifyContent::flexStart;
		
	}
	else
	{
		flexBox.flexWrap = FlexBox::Wrap::wrap;
		flexBox.flexDirection = FlexBox::Direction::row;
		flexBox.alignContent = FlexBox::AlignContent::flexStart;
		flexBox.justifyContent = FlexBox::JustifyContent::spaceAround;

		int numItems = flexBox.items.size();

		int rows = (int)sqrt((double)numItems);

		int columns = numItems / rows + 1;
	}

	

	flexBox.performLayout(getLocalBounds().withTrimmedTop(20));
}


void EmptyComponent::addCategory()
{
	flexBox.items.add(FlexItem().withMargin(30));

	auto& flexItem = flexBox.items.getReference(flexBox.items.size() - 1);

	flexItem.minWidth = 80;
	flexItem.maxWidth = 4000;
	flexItem.width = 400;

	flexItem.minHeight = 120;
	flexItem.maxHeight = 120;
	

	auto c = new Category(flexItem);
	categories.add(c);
	flexItem.associatedComponent = c;
	addAndMakeVisible(c);
}





//==============================================================================
MainContentComponent::MainContentComponent(const String &commandLine)
{
#if !PUT_FLOAT_IN_CODEBASE
	addAndMakeVisible(root = new FloatingTile(nullptr));

	root->setNewContent(HorizontalTile::getPanelId());

	setSize(1200, 800);
#else
	standaloneProcessor = new StandaloneProcessor();

	addAndMakeVisible(editor = standaloneProcessor->createEditor());

	setSize(editor->getWidth(), editor->getHeight());

	handleCommandLineArguments(commandLine);
#endif
}

MainContentComponent::~MainContentComponent()
{
	
	root = nullptr;

	//open.detach();
	editor = nullptr;

	

	standaloneProcessor = nullptr;
}

void MainContentComponent::paint (Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
}

void MainContentComponent::resized()
{
#if !PUT_FLOAT_IN_CODEBASE
	root->setBounds(getLocalBounds());
#else
#if SCALE_2
	editor->setSize(getWidth()*2, getHeight()*2);
#else
    editor->setSize(getWidth(), getHeight());
#endif
#endif

}
