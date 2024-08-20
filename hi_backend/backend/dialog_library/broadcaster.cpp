// Put in the header definitions of every dialog here...

#include "broadcaster_resultpage.cpp"

namespace hise {
namespace multipage {
namespace library {
using namespace juce;
Dialog* BroadcasterWizard::createDialog(State& state)
{
	DynamicObject::Ptr fullData = new DynamicObject();
	fullData->setProperty(mpid::LayoutData, JSON::parse(R"({"StyleSheet": "Dark", "Style": "#targetMetadata,\n#contextItems\n{\n\theight: 120px;\n}\n\n#content\n{\n\tpadding: 30px 80px;\n}\n\n\n\n#targetMetadata input,\n#contextItems input\n{\n\theight: 100%;\n\tfont-family: monospace;\n\tfont-size: 14px;\n\tvertical-align: top;\n\tpadding-top: 5px;\n}\n\nlabel\n{\n\tmin-width: 100px;\n}\n\n.button-selector\n{\n\tflex-wrap:wrap;\n\tgap:0px;\n}\n\n.button-selector div\n{\n\twidth: 33%;\n\tgap: 0px;\n\tpadding: 5px;\n}\n\n.button-selector label\n{\n\tdisplay: none;\n}\n\n.button-selector .toggle-button\n{\n\tbackground: #222;\n\tcolor: #ccc;\n\tborder-radius: 5px;\n\tfont-family: monospace;\n\tfont-size: 13px;\n\tmargin-right: 5px;\n}\n\n.button-selector button:checked\n{\n\tbackground: #aaa;\n\tcolor: #222;\n\tborder-radius: 5px;\n}\n\n.button-selector button::before,\n.button-selector button::after\n{\n\tdisplay: none;\n}\n\n\n", "UseViewport": true, "ConfirmClose": true, "DialogWidth": 832, "DialogHeight": 716})"));
	fullData->setProperty(mpid::Properties, JSON::parse(R"({"Header": "Broadcaster Wizard", "Subtitle": "", "Image": "", "ProjectName": "BroadcasterWizard", "Company": "", "Version": "", "BinaryName": "", "UseGlobalAppData": false, "Icon": ""})"));
	using namespace factory;
	auto mp_ = new Dialog(var(fullData.get()), state, false);
	auto& mp = *mp_;
	auto& List_0 = mp.addPage<List>({
	  { mpid::Style, "gap: 20px;" }
	});

	List_0.addChild<JavascriptFunction>({
	  { mpid::ID, "JavascriptFunctionId" }, 
	  { mpid::Code, R"(var f = document.bindCallback("checkSelection", function()
{
	return false;
});

if(f())
{
	document.navigate(3, false);
}

)" }
	});

	List_0.addChild<MarkdownText>({
	  { mpid::Text, "This wizard will take you through the steps of creating a broadcaster. You can specify every property and connection and it will create a script definition at the end of the process that you then can paste into your `onInit` callback." }
	});

	List_0.addChild<TextInput>({
	  { mpid::Text, "Broadcaster ID" }, 
	  { mpid::ID, "id" }, 
	  { mpid::EmptyText, "Enter broadcaster ID..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The ID will be used to create the script variable definition as well as act as a unique ID for every broadcaster of a single script processor. The name must be a valid HiseScript identifier." }
	});

	auto& List_4 = List_0.addChild<List>({
	  { mpid::Text, "Additional Properties" }, 
	  { mpid::Foldable, 1 }, 
	  { mpid::Folded, 1 }
	});

	List_4.addChild<TextInput>({
	  { mpid::Text, "Comment" }, 
	  { mpid::ID, "comment" }, 
	  { mpid::EmptyText, "Enter a comment..." }, 
	  { mpid::Help, "A comment that is shown in the broadcaster map and helps with navigation & code organisation" }
	});

	List_4.addChild<TextInput>({
	  { mpid::Text, "Tags" }, 
	  { mpid::ID, "tags" }, 
	  { mpid::EmptyText, "Enter tags" }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "Enter a comma separated list of strings that will be used as tags for the broadcaster. This lets you filter which broadcaster you want to show on the broadcaster map and is useful for navigating complex projects" }
	});

	List_4.addChild<ColourChooser>({
	  { mpid::Text, "Colour" }, 
	  { mpid::ID, "colour" }, 
	  { mpid::Help, "The colour of the broadcaster in the broadcaster map" }
	});

	// Custom callback for page List_0
	List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_8 = mp.addPage<List>({
	});

	List_8.addChild<MarkdownText>({
	  { mpid::Text, R"(

Please select the event type that you want to attach the broadcaster to. There are multiple event sources which can trigger a broadcaster message.

This will create a script line calling one of the `attachToXXX()` functions that hook up the broadcaster to an event source.)" }
	});

	auto& Column_10 = List_8.addChild<Column>({
	  { mpid::Width, "-0.65, -0.35" }, 
	  { mpid::Class, ".button-selector" }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "None" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "No event source. Use this option if you want to call the broadcaster manually or attach it to any other script callback slot (eg. TransportHandler callbacks)." }, 
	  { mpid::InitValue, "true" }, 
	  { mpid::UseInitValue, 1 }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "ComplexData" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, R"(An event of a complex data object (Tables, Slider Packs or AudioFiles). This can be either:

- content changes (eg. when loading in a new sample)
- a display index change (eg. if the table ruler is moved))" }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "ComponentProperties" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "Script properties of a UI component selection (eg. the `visible` property)." }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "ComponentValue" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "The value of a component. This will register the broadcaster as additional value callback that will be fired whenever the control callback will be executed." }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "ComponentVisibility" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, R"(The visibility of a UI component.

>This also takes into account the visibility of parent components so it's a more reliable way than listen to the component's `visible` property.)" }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "ContextMenu" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "Adds a popup menu when the UI component is clicked." }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "EqEvents" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "Listens to band add / delete, reset events of a parametriq EQ." }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "ModuleParameters" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "Listens to changes of a module attribute (when calling `setAttribute()`, eg. the **Reverb Width** or **Filter Frequency**" }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "MouseEvents" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "Mouse events for a UI component selection" }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "ProcessingSpecs" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "Listens to changes of the processing specifications (eg. sample rate of audio buffer size)" }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "RadioGroup" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, R"(Listens to button clicks within a given radio group ID\n> This is especially useful for implementing your page switch logic)" }
	});

	Column_10.addChild<Button>({
	  { mpid::Text, "RoutingMatrix" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::Help, "Listens to changes of the routing matrix (the channel routing configuration) of a module." }
	});

	// Custom callback for page List_8
	List_8.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_23 = mp.addPage<List>({
	});

	auto& attachType_24 = List_23.addChild<Branch>({
	  { mpid::ID, "attachType" }
	});

	auto& List_25 = attachType_24.addChild<List>({
	});

	List_25.addChild<MarkdownText>({
	  { mpid::Text, R"(### No source

You can use a broadcaster without attaching it to a event source. In this case, please add a comma separated list of argument IDs that you want this broadcaster to use.
)" }
	});

	List_25.addChild<TextInput>({
	  { mpid::Text, "Arguments" }, 
	  { mpid::ID, "noneArgs" }, 
	  { mpid::Height, 80 }, 
	  { mpid::Help, "This is a comma separated list of arguments and will define how many data slots the broadcaster will have. This property will also define how many arguments a listener callback is supposed to have." }
	});

	auto& List_28 = attachType_24.addChild<List>({
	});

	List_28.addChild<MarkdownText>({
	  { mpid::Text, R"(### Complex Type

Attaching a broadcaster to a complex data object lets you listen to table edit changes or playback position updates of one or multiple data sources. Please fill in the information below to proceed to the next step.)" }
	});

	List_28.addChild<Choice>({
	  { mpid::Text, "DataType" }, 
	  { mpid::ID, "complexDataType" }, 
	  { mpid::ValueMode, "Text" }, 
	  { mpid::Items, R"(Table
SliderPack
AudioFile)" }, 
	  { mpid::Help, "The data type that you want to listen to" }
	});

	List_28.addChild<Choice>({
	  { mpid::Text, "Event Type" }, 
	  { mpid::ID, "complexEventType" }, 
	  { mpid::ValueMode, "Text" }, 
	  { mpid::Help, R"(The event type you want to listen to.

-**Content** events will be triggered whenever the data changes (so eg. loading a new sample or editing a table will trigger this event).
-**DisplayIndex** events will occur whenever the read position changes (so the playback position in the audio file or the table ruler in the table).)" }, 
	  { mpid::Items, R"(Content
DisplayIndex)" }
	});

	List_28.addChild<TextInput>({
	  { mpid::Text, "Module IDs" }, 
	  { mpid::ID, "moduleIds" }, 
	  { mpid::EmptyText, "Enter module IDs as shown in the Patch Browser..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }, 
	  { mpid::Height, 80 }
	});

	List_28.addChild<TextInput>({
	  { mpid::Text, "Slot Index" }, 
	  { mpid::ID, "complexSlotIndex" }, 
	  { mpid::EmptyText, "Enter Slot Index (zero based)" }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, R"(The slot index of the complex data object that you want to listen to.

> Some modules have multiple complex data objects (eg. the table envelope has two tables for the attack and release phase so if you want to listen to the release table, you need to pass in `1` here.)" }, 
	  { mpid::Height, 80 }
	});

	List_28.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_35 = attachType_24.addChild<List>({
	});

	List_35.addChild<MarkdownText>({
	  { mpid::Text, R"(### Component Properties

Attaches the broadcaster to changes of the script properties like `enabled`, `text`, etc.

> If you want to listen to visibility changes, take a look at the **ComponentVisibility** attachment mode, which also takes the visibility of parent components into account.)" }
	});

	List_35.addChild<TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_35.addChild<TextInput>({
	  { mpid::Text, "Property" }, 
	  { mpid::ID, "propertyType" }, 
	  { mpid::EmptyText, "Enter script property (text, enabled, etc)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The property you want to listen to. You can also use a comma-separated list for multiple properties" }, 
	  { mpid::Height, 80 }
	});

	List_35.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_40 = attachType_24.addChild<List>({
	});

	List_40.addChild<MarkdownText>({
	  { mpid::Text, R"(### Component Value

This attachment mode will register the broadcaster to control callbacks of a Component.)" }
	});

	List_40.addChild<TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_40.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_44 = attachType_24.addChild<List>({
	});

	List_44.addChild<MarkdownText>({
	  { mpid::Text, R"(### Component Visibility

This mode attaches the broadcaster to whether the component is actually shown on the interface, which also takes into account the parent visibility and whether its bounds are within the parent's dimension)" }
	});

	List_44.addChild<TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A comma-separated list of all components that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_44.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_48 = attachType_24.addChild<List>({
	});

	List_48.addChild<MarkdownText>({
	  { mpid::Text, R"(### Context Menu

This attach mode will show a context menu when you click on the registered UI components and allow you to perform additional actions.)" }
	});

	List_48.addChild<TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A comma-separated list of all components that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_48.addChild<TextInput>({
	  { mpid::Text, "State Function" }, 
	  { mpid::ID, "contextStateFunctionId" }, 
	  { mpid::EmptyText, "Enter state function id..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "A function name that will be created by the template generator to query the active and ticked state of each context menu item. " }, 
	  { mpid::Height, 80 }
	});

	List_48.addChild<TextInput>({
	  { mpid::Text, "Item List" }, 
	  { mpid::ID, "contextItems" }, 
	  { mpid::EmptyText, "Enter items..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "This will define the items that are shown in the list (one item per new line). You can use the markdown-like syntax to create sub menus, separators and headers known from the other Context menu builders in HISE (ScriptPanel, SubmenuCombobox)..." }, 
	  { mpid::Multiline, 1 }, 
	  { mpid::Style, "height: 150px;" }, 
	  { mpid::Height, 80 }
	});

	List_48.addChild<Button>({
	  { mpid::Text, "Trigger on Left Click" }, 
	  { mpid::ID, "contextLeftClick" }, 
	  { mpid::Help, "If this is true, the context menu will be shown when you click on the UI element with the left mouse button, otherwise it will require a right click to show up." }
	});

	List_48.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }
	});

	auto& List_55 = attachType_24.addChild<List>({
	});

	List_55.addChild<MarkdownText>({
	  { mpid::Text, R"(### EQ Events

This attachment mode will register one or more Parametriq EQ modules to the broadcaster and will send events when EQ bands are added / removed.

> This does not cover the band parameter changes, as this can be queried with the usual module parameter attachment mode.)" }
	});

	List_55.addChild<TextInput>({
	  { mpid::Text, "Module IDs" }, 
	  { mpid::ID, "moduleIds" }, 
	  { mpid::EmptyText, "Enter module IDs as shown in the Patch Browser..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }, 
	  { mpid::Height, 80 }
	});

	List_55.addChild<TextInput>({
	  { mpid::Text, "Event Types" }, 
	  { mpid::ID, "eqEventTypes" }, 
	  { mpid::EmptyText, "Enter event types (BandAdded, BandRemoved, etc)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Items, R"(BandAdded
BandRemoved
BandSelected
FFTEnabled)" }, 
	  { mpid::Help, R"(Set the event type that the broadcaster should react too. This can be multiple items from this list:

- `BandAdded`
- `BandRemoved`
- `BandSelected`
- `FFTEnabled`)" }, 
	  { mpid::Height, 80 }, 
	  { mpid::ParseArray, 1 }
	});

	List_55.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }
	});

	auto& List_60 = attachType_24.addChild<List>({
	});

	List_60.addChild<MarkdownText>({
	  { mpid::Text, R"(### Module Parameters

Attaches the broadcaster to attribute changes of one or more modules.)" }
	});

	List_60.addChild<TextInput>({
	  { mpid::Text, "Module IDs" }, 
	  { mpid::ID, "moduleIds" }, 
	  { mpid::EmptyText, "Enter module IDs as shown in the Patch Browser..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }, 
	  { mpid::Height, 80 }
	});

	List_60.addChild<TextInput>({
	  { mpid::Text, "Parameters" }, 
	  { mpid::ID, "moduleParameterIndexes" }, 
	  { mpid::EmptyText, "Enter parameters..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The parameters that you want to listen to. This can be either the actual parameter names or the indexes of the parameters" }, 
	  { mpid::Height, 80 }
	});

	List_60.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_65 = attachType_24.addChild<List>({
	});

	List_65.addChild<MarkdownText>({
	  { mpid::Text, R"(### Mouse Events

This attachment mode will cause the broadcaster to fire at certain mouse callback events (just like the ScriptPanel's `mouseCallback`).)" }
	});

	List_65.addChild<TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A comma-separated list of all components that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_65.addChild<Choice>({
	  { mpid::Text, "Callback Type" }, 
	  { mpid::ID, "mouseCallbackType" }, 
	  { mpid::ValueMode, "Text" }, 
	  { mpid::Help, "The callback level that determines when the broadcaster will send a message" }, 
	  { mpid::Items, R"(No Callbacks
Context Menu
Clicks Only
Clicks & Hover
Clicks, Hover & Dragging
All Callbacks)" }
	});

	List_65.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }
	});

	auto& List_70 = attachType_24.addChild<List>({
	});

	List_70.addChild<MarkdownText>({
	  { mpid::Text, R"(### Processing specs

This will attach the broadcaster to changes of the audio processing specs (sample rate, buffer size, etc).)" }
	});

	List_70.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }
	});

	auto& List_73 = attachType_24.addChild<List>({
	});

	List_73.addChild<MarkdownText>({
	  { mpid::Text, R"(### Radio Group

This will attach the broadcaster to a radio group (a selection of multiple buttons that are mutually exclusive).

> This is a great way of handling page logic by attaching a broadcaster to a button group and then use its callback to show and hide pages within an array.)" }
	});

	List_73.addChild<TextInput>({
	  { mpid::Text, "Radio Group" }, 
	  { mpid::ID, "radioGroupId" }, 
	  { mpid::EmptyText, "Enter radio group ID..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The radio group id as it was set as `radioGroup` script property to the buttons that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_73.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }
	});

	auto& List_77 = attachType_24.addChild<List>({
	});

	List_77.addChild<MarkdownText>({
	  { mpid::Text, R"(### Routing Matrix

This attaches the broadcaster to changes in a channel routing of one or more modules (either channel resizes or routing changes including send channel assignments).)" }
	});

	List_77.addChild<TextInput>({
	  { mpid::Text, "Module IDs" }, 
	  { mpid::ID, "moduleIds" }, 
	  { mpid::EmptyText, "Enter module IDs as shown in the Patch Browser..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }, 
	  { mpid::Height, 80 }
	});

	List_77.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	// Custom callback for page List_23
	List_23.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_81 = mp.addPage<List>({
	});

	List_81.addChild<MarkdownText>({
	  { mpid::Text, R"(### Target type

Now you can add a target listener which will receive the events caused by the event source specified on the previous page.

)" }
	});

	auto& Column_83 = List_81.addChild<Column>({
	  { mpid::Width, "-0.65, -0.35" }, 
	  { mpid::Class, ".button-selector" }, 
	  { mpid::Style, "margin-top: 10px;" }
	});

	Column_83.addChild<Button>({
	  { mpid::Text, "None" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::Help, "Skip the creation of a target callback." }
	});

	Column_83.addChild<Button>({
	  { mpid::Text, "Callback" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::Help, "A script function that will be executed immediately when the event occurs." }
	});

	Column_83.addChild<Button>({
	  { mpid::Text, "Callback (Delayed)" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::Help, "A script function that will be executed with a given delay after the event occurred." }
	});

	Column_83.addChild<Button>({
	  { mpid::Text, "Component Property" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::Help, "This will change a component property based on a customizeable function that must calculate and return the value" }
	});

	Column_83.addChild<Button>({
	  { mpid::Text, "Component Refresh" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::Help, "Simply sends out a component refresh method (eg. `repaint()`) to its registered components." }
	});

	Column_83.addChild<Button>({
	  { mpid::Text, "Component Value" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::Help, "This sets the value of a component" }
	});

	Column_83.addChild<Button>({
	  { mpid::Text, "Module Parameter" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::Help, "This will sync the given module parameter to the last argument of the broadcaster." }
	});

	Column_83.addChild<List>({
	});

	// Custom callback for page List_81
	List_81.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_92 = mp.addPage<List>({
	});

	auto& targetType_93 = List_92.addChild<Branch>({
	  { mpid::ID, "targetType" }
	});

	auto& List_94 = targetType_93.addChild<List>({
	});

	List_94.addChild<Skip>({
	});

	auto& List_96 = targetType_93.addChild<List>({
	});

	List_96.addChild<MarkdownText>({
	  { mpid::Text, R"(### Callback

This will call a function with a customizeable `this` object.)" }
	});

	List_96.addChild<TextInput>({
	  { mpid::Text, "This Object" }, 
	  { mpid::ID, "thisTarget" }, 
	  { mpid::EmptyText, "Enter this object..." }, 
	  { mpid::Help, "You can specify any HiseScript expression that will be used as `this` object in the function." }, 
	  { mpid::Height, 80 }
	});

	List_96.addChild<TextInput>({
	  { mpid::Text, "Function" }, 
	  { mpid::ID, "targetFunctionId" }, 
	  { mpid::EmptyText, "Enter function ID..." }, 
	  { mpid::Help, "This will use the given function as an ID. Leave this empty in order to create an inplace anonymous function." }, 
	  { mpid::Height, 80 }
	});

	List_96.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (required)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_101 = targetType_93.addChild<List>({
	});

	List_101.addChild<MarkdownText>({
	  { mpid::Text, R"(### Delayed Callback

This will call a script function with a delay.)" }
	});

	List_101.addChild<TextInput>({
	  { mpid::Text, "This Object" }, 
	  { mpid::ID, "thisTarget" }, 
	  { mpid::EmptyText, "Enter this object..." }, 
	  { mpid::Help, "You can specify any HiseScript expression that will be used as `this` object in the function." }, 
	  { mpid::Height, 80 }
	});

	List_101.addChild<TextInput>({
	  { mpid::Text, "Function" }, 
	  { mpid::ID, "targetFunctionId" }, 
	  { mpid::EmptyText, "Enter function ID..." }, 
	  { mpid::Help, "This will use the given function as an ID. Leave this empty in order to create an inplace anonymous function." }
	});

	List_101.addChild<TextInput>({
	  { mpid::Text, "Delay time" }, 
	  { mpid::ID, "targetDelay" }, 
	  { mpid::EmptyText, "Enter delay time (ms)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The delay time in milliseconds" }, 
	  { mpid::Height, 80 }
	});

	List_101.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (required)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_107 = targetType_93.addChild<List>({
	});

	List_107.addChild<MarkdownText>({
	  { mpid::Text, R"(### Component Property

This will set one or more component properties of one or more components to a given value. You can supply a custom function that will calculate a value for each target, otherwise the value of the broadcaster is used.

> This is eg. useful if you just want to sync (forward) some property changes to other components.)" }
	});

	List_107.addChild<TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "targetComponentIds" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_107.addChild<TextInput>({
	  { mpid::Text, "Property" }, 
	  { mpid::ID, "targetPropertyType" }, 
	  { mpid::EmptyText, "Enter script property (text, enabled, etc)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The property you want to listen to. You can also use a comma-separated list for multiple properties" }, 
	  { mpid::Height, 80 }
	});

	List_107.addChild<TextInput>({
	  { mpid::Text, "Function" }, 
	  { mpid::ID, "targetFunctionId" }, 
	  { mpid::EmptyText, "Enter function ID..." }, 
	  { mpid::Help, "This will use the given function as an ID. Leave this empty in order to create an inplace anonymous function." }, 
	  { mpid::Height, 80 }
	});

	List_107.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (required)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_113 = targetType_93.addChild<List>({
	});

	List_113.addChild<MarkdownText>({
	  { mpid::Text, R"(### Component Refresh

This will send out an update message to the specified components)" }
	});

	List_113.addChild<TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "targetComponentIds" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_113.addChild<Choice>({
	  { mpid::Text, "Refresh Type" }, 
	  { mpid::ID, "targetRefreshType" }, 
	  { mpid::ValueMode, "Text" }, 
	  { mpid::Help, R"(The type of refresh message that should be sent:

- `repaint`: causes a component repaint message (and if the component is a panel, it will also call its paint routine)
- `changed`: causes a value change callback that will fire the component's `setValue()` callback
- `updateValueFromProcessorConnection`: updates the component from the current processor's value (if it is connected via `processorId` and `parameterId`).
- `loseFocus`: will lose the focus of the keyboard (if the component has currently the keyboard focus).
- `resetValueToDefault`: will reset the component to its default (as defined by the `defaultValue` property). Basically the same as a double click.)" }, 
	  { mpid::Items, R"(repaint
changed
updateValueFromProcessorConnection
loseFocus
resetValueToDefault)" }
	});

	List_113.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (required)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_118 = targetType_93.addChild<List>({
	});

	List_118.addChild<MarkdownText>({
	  { mpid::Text, R"(### Component Value

This will cause a value change alongside with a control callback for the given components. The value that is send to the components can be customized with a function, otherwise it will use the broadcaster's value (if applicable).)" }
	});

	List_118.addChild<TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "targetComponentIds" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Height, 80 }
	});

	List_118.addChild<TextInput>({
	  { mpid::Text, "Function" }, 
	  { mpid::ID, "targetFunctionId" }, 
	  { mpid::EmptyText, "Enter function ID..." }, 
	  { mpid::Help, "This will use the given function as an ID. Leave this empty in order to create an inplace anonymous function." }, 
	  { mpid::Height, 80 }
	});

	List_118.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (required)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	auto& List_123 = targetType_93.addChild<List>({
	});

	List_123.addChild<MarkdownText>({
	  { mpid::Text, R"(### Module Parameter

Attaching this as listener will send the last argument of the broadcaster as parameter change to the given module.)" }
	});

	List_123.addChild<TextInput>({
	  { mpid::Text, "Module ID" }, 
	  { mpid::ID, "targetModuleId" }, 
	  { mpid::EmptyText, "Enter module ID" }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "The ID of the module that you want to send the parameter change to." }, 
	  { mpid::Height, 80 }
	});

	List_123.addChild<TextInput>({
	  { mpid::Text, "Parameter" }, 
	  { mpid::ID, "targetModuleParameter" }, 
	  { mpid::EmptyText, "Enter parameter ID" }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Items, "{DYNAMIC}" }, 
	  { mpid::Help, "A single parameter (either an integer or the parameter ID) that should be set for the given module" }, 
	  { mpid::Height, 80 }
	});

	List_123.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::EmptyText, "Enter metadata (required)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Height, 80 }
	});

	// Custom callback for page List_92
	List_92.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_128 = mp.addPage<List>({
	});

	List_128.addChild<MarkdownText>({
	  { mpid::Text, "Press Finished in order to copy the code below to the clipboard (you can make some manual adjustments before)." }
	});

	List_128.addChild<Placeholder<CustomResultPage>>({
	  { mpid::Style, "height: 350px;" }
	});

	// Custom callback for page List_128
	List_128.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	return mp_;
}
} // namespace library
} // namespace multipage
} // namespace hise