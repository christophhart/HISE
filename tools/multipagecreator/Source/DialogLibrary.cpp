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

#include "DialogLibrary.h"

namespace hise {
namespace multipage {
namespace library {
using namespace juce;

BroadcasterWizard::CustomResultPage::CustomResultPage(Dialog& r, int width, const var& obj):
	PageBase(r, width, obj),
	textDoc(doc),
	codeEditor(textDoc)
{
	addAndMakeVisible(codeEditor);
	codeEditor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF222222));
	setSize(width, 300);
}

var BroadcasterWizard::CustomResultPage::getArgs(SourceIndex source)
{
	switch (source)
	{
	case SourceIndex::None: return "";
	case SourceIndex::ComplexData: return "processor, index, value";
	case SourceIndex::ComponentProperties: return "component, property, value";
	case SourceIndex::ComponentVisibility: return "component, isVisible";
	case SourceIndex::ContextMenu: return "component, index";
	case SourceIndex::EqEvents: return "eventType, value";
	case SourceIndex::ModuleParameters: return "processor, parameter, value";
	case SourceIndex::MouseEvents: return "component, event";
	case SourceIndex::ProcessingSpecs: return "sampleRate, blockSize";
	case SourceIndex::RadioGroup: return "radioGroupIndex";
	case SourceIndex::RoutingMatrix: return "module";
	case SourceIndex::numSourceIndexTypes: break;
	default: ;
	}

	return var();
}

String BroadcasterWizard::CustomResultPage::createFunctionBodyIfAnonymous(const String& functionName,
	SourceIndex sourceIndex, bool createValueFunction)
{
	if(functionName.isNotEmpty())
		return functionName;

	String f;
	f << "function(";

	if(createValueFunction)
		f << "index, ";

	auto args = getArgs(sourceIndex).toString();

	f << args << "){";

	if(createValueFunction)
		f << "\n\treturn " << args.fromLastOccurrenceOf(", ", false, false) << ";\n";

	f << "}";
	return f;
}

void BroadcasterWizard::CustomResultPage::appendLine(String& x, const var& state, const String& suffix,
	const Array<var>& args, Array<StringProcessor> sp)
{
	x << state["id"].toString() << suffix << "(";

	int idx = 0;

	for(auto& a: args)
	{
		auto v = JSON::toString(a, true);

		if(isPositiveAndBelow(idx, sp.size()))
		{
			auto pr = sp[idx];

			if(pr == StringProcessor::ParseInt)
				v = String((int)a);
			else if(pr == StringProcessor::Unquote)
				v = a.toString().unquoted();
			else if(pr == StringProcessor::JoinToStringWithNewLines)
				v = Dialog::joinVarArrayToNewLineString(a).replace("\n", "\\n").quoted();
		}

		x << v;

		if(++idx != args.size())
			x << ", ";
	}

	x << ");\n";
}

String BroadcasterWizard::CustomResultPage::getTargetLine(TargetIndex target, const var& state)
{
	String x;

	auto createValueFunction = target >= TargetIndex::ComponentProperty;

	auto functionBody = createFunctionBodyIfAnonymous(state["targetFunctionId"].toString(), (SourceIndex)(int)state["attachType"], createValueFunction);

	auto thisTarget = state["thisTarget"].toString();
	        
	if(thisTarget.isEmpty())
		thisTarget = "0";
	        
	switch(target)
	{
	case TargetIndex::None:
		break;
	case TargetIndex::Callback:
		appendLine(x, state, ".addListener",
		           { thisTarget,               state["targetMetadata"].toString(), functionBody},
		           { StringProcessor::Unquote, StringProcessor::None,              StringProcessor::Unquote });
		break;
	case TargetIndex::CallbackDelayed:
		appendLine(x, state, ".addDelayedListener",
		           { state["targetDelay"],      thisTarget,               state["targetMetadata"], functionBody},
		           { StringProcessor::ParseInt, StringProcessor::Unquote, StringProcessor::None,   StringProcessor::Unquote });
		break;
	case TargetIndex::ComponentProperty:
		appendLine(x, state, ".addComponentPropertyListener", 
		           { state["targetComponentIds"], state["targetPropertyType"], state["targetMetadata"], functionBody},
		           { StringProcessor::None,       StringProcessor::None,       StringProcessor::None,   StringProcessor::Unquote});
		break;
	case TargetIndex::ComponentRefresh:
		//asd.addComponentRefreshListener("Knob1", "repaint", "data");

		appendLine(x, state, ".addComponentRefreshListener",
		           { state["targetComponentIds"], state["targetRefreshType"], state["targetMetadata"] },
		           {});
		break;
	case TargetIndex::ComponentValue:
		appendLine(x, state, ".addComponentValueListener", 
		           { state["targetComponentIds"], state["targetMetadata"], functionBody},
		           { StringProcessor::None,       StringProcessor::None,   StringProcessor::Unquote});
		break;
	case TargetIndex::numTargetIndexTypes: break;
	default: ;
	}
	        
	return x;
}

String BroadcasterWizard::CustomResultPage::getAttachLine(SourceIndex source, const var& state)
{
	String x;
	        
	switch(source)
	{
	case SourceIndex::None: break;
	case SourceIndex::ComplexData:
		appendLine(x, state, ".attachToComplexData", { var(state["complexDataType"].toString() + "." + state["complexEventType"].toString()), 
			           state["moduleIds"],
			           state["complexSlotIndex"],
			           state["attachMetadata"]
		           });
		break;
	case SourceIndex::ComponentProperties:
		appendLine(x, state, ".attachToComponentProperties", { state["componentIds"],
			           state["propertyType"],
			           state["attachMetadata"]
		           });
		break;
	case SourceIndex::ComponentVisibility:
		appendLine(x, state, ".attachToComponentProperties", { state["componentIds"],
			           state["attachMetadata"]
		           });
		break;
	case SourceIndex::ContextMenu:
		appendLine(x, state, ".attachToContextMenu", { state["componentIds"],
			           state["contextStateFunctionId"],
			           state["contextItems"],
			           state["attachMetadata"],
			           state["contextLeftClick"],
		           },
		           {
			           StringProcessor::None,
			           StringProcessor::Unquote,
			           StringProcessor::JoinToStringWithNewLines,
			           StringProcessor::None,
			           StringProcessor::ParseInt
		           });
		break;
	case SourceIndex::EqEvents:
		appendLine(x, state, ".attachToEqEvents", {
			           state["moduleIds"],
			           state["eqEventTypes"],
			           state["attachMetadata"]
		           });
		break;
	case SourceIndex::ModuleParameters: 
		appendLine(x, state, ".attachToModuleParameter", {
			           state["moduleIds"],
			           state["moduleParameterIndexes"],
			           state["attachMetadata"]
		           });
		break;
	case SourceIndex::MouseEvents: 
		appendLine(x, state, ".attachToComponentMouseEvents", {
			           state["componentIds"],
			           state["mouseCallbackType"],
			           state["attachMetadata"]
		           });
		break;
	case SourceIndex::ProcessingSpecs:
		appendLine(x, state, ".attachToProcessingSpecs", {
			           state["attachMetadata"]
		           });
		break;
	case SourceIndex::RadioGroup: 
		appendLine(x, state, ".attachToRadioGroup", {
			           state["radioGroupId"],
			           state["attachMetadata"]
		           },
		           {
			           StringProcessor::ParseInt,
			           StringProcessor::None
		           });
		break;;
	case SourceIndex::RoutingMatrix: break;
	case SourceIndex::numSourceIndexTypes: break;
	default: ;
	}

	return x;
}

void BroadcasterWizard::CustomResultPage::postInit()
{
	auto gs = Dialog::getGlobalState(*this, {}, var());

	String b;

	String nl = "\n";

	b << "// Broadcaster definition" << nl;
	b << "const var " << Dialog::getGlobalState(*this, "id", var()).toString() << " = Engine.createBroadcaster({" << nl;

	auto sourceIndex = (SourceIndex)(int)gs["attachType"];

	        

	b << "  " << String("id").quoted() << ": " << gs["id"].toString().quoted();
	b << ",\n  " << String("args").quoted() << ": " << JSON::toString(Dialog::parseCommaList(getArgs(sourceIndex)), true);

	if(gs["tags"].toString().isNotEmpty())
		b << ",\n  " << String("tags").quoted() << ": " << JSON::toString(gs["tags"], true);

	if(gs["comment"].toString().isNotEmpty())
		b << ",\n  " << String("comment").quoted() << ": " << gs["tags"].toString();

	if((int)gs["colour"])
		b << ",\n  " << String("colour").quoted() << ": " << gs["colour"].toString();
	        
	b << nl << "});" << nl << nl;

	if(sourceIndex != SourceIndex::None)
	{
		b << "// attach to event Type" << nl;
		b << getAttachLine(sourceIndex, gs);
	}

	auto targetIndex = (TargetIndex)(int)gs["targetType"];

	if(targetIndex != TargetIndex::None)
	{
		b << nl << "// attach first listener" << nl;
		b << getTargetLine(targetIndex, gs);
	}

	doc.replaceAllContent(b);
}

void BroadcasterWizard::CustomResultPage::resized()
{
	codeEditor.setBounds(getLocalBounds());
}

Result BroadcasterWizard::CustomResultPage::checkGlobalState(var globalState)
{
	SystemClipboard::copyTextToClipboard(doc.getAllContent());
	        
	return Result::ok();
}

BroadcasterWizard::BroadcasterWizard():
  HardcodedDialogWithState()
{
	setSize(860, 720);
}

BroadcasterWizard::~BroadcasterWizard()
{
	    
}

Dialog* BroadcasterWizard::createDialog(State& state)
{
	auto mp_ = new Dialog({}, state, false);
	auto& mp = *mp_;
	mp.setProperty(mpid::Header, "Broadcaster Wizard");
	mp.setProperty(mpid::Subtitle, "");
	auto& List_0 = mp.addPage<factory::List>({
	  { mpid::Padding, "30" }, 
	  { mpid::Foldable, 0 }, 
	  { mpid::Folded, 0 }
	});

	auto& MarkdownText_1 = List_0.addChild<factory::MarkdownText>({
	  { mpid::Text, "This wizard will take you through the steps of creating a broadcaster. You can specify every property and connection and it will create a script definition at the end of the process that you then can paste into your `onInit` callback." }, 
	  { mpid::Padding, "0" }
	});

	auto& id_2 = List_0.addChild<factory::TextInput>({
	  { mpid::Text, "Broadcaster ID" }, 
	  { mpid::ID, "id" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter broadcaster ID..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The ID will be used to create the script variable definition as well as act as a unique ID for every broadcaster of a single script processor. The name must be a valid HiseScript identifier." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_3 = List_0.addChild<factory::List>({
	  { mpid::Text, "Additional Properties" }, 
	  { mpid::Padding, "20" }, 
	  { mpid::Foldable, 1 }, 
	  { mpid::Folded, 1 }
	});

	auto& comment_4 = List_3.addChild<factory::TextInput>({
	  { mpid::Text, "Comment" }, 
	  { mpid::ID, "comment" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter a comment..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "A comment that is shown in the broadcaster map and helps with navigation & code organisation" }, 
	  { mpid::Multiline, 0 }
	});

	auto& tags_5 = List_3.addChild<factory::TextInput>({
	  { mpid::Text, "Tags" }, 
	  { mpid::ID, "tags" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter tags" }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "Enter a comma separated list of strings that will be used as tags for the broadcaster. This lets you filter which broadcaster you want to show on the broadcaster map and is useful for navigating complex projects" }
	});

	auto& colour_6 = List_3.addChild<factory::ColourChooser>({
	  { mpid::Text, "Colour" }, 
	  { mpid::ID, "colour" }, 
	  { mpid::LabelPosition, "Left" }, 
	  { mpid::Help, "The colour of the broadcaster in the broadcaster map" }
	});

	// Custom callback for page List_0
	List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	auto& List_7 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& Column_8 = List_7.addChild<factory::Column>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::Width, "-0.65, -0.35" }, 
	  { mpid::Padding, "40" }
	});

	auto& MarkdownText_9 = Column_8.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Event Source Type\n\nPlease select the event type that you want to attach the broadcaster to. There are multiple event sources which can trigger a broadcaster message.\n\n> You can specify the exact source in the next step.\n\nThis will create a script line calling one of the `attachToXXX()` functions that hook up the broadcaster to an event source." }, 
	  { mpid::Padding, 0 }
	});

	auto& List_10 = Column_8.addChild<factory::List>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::Padding, "3" }, 
	  { mpid::Foldable, 0 }, 
	  { mpid::Folded, 0 }
	});

	auto& attachType_11 = List_10.addChild<factory::Button>({
	  { mpid::Text, "None" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### None\n\nNo event source. Use this option if you want to call the broadcaster manually or attach it to any other script callback slot (eg. TransportHandler callbacks)." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_12 = List_10.addChild<factory::Button>({
	  { mpid::Text, "ComplexData" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### ComplexData\n\nAn event of a complex data object (Tables, Slider Packs or AudioFiles). This can be either:\n\n- content changes (eg. when loading in a new sample)\n- a display index change (eg. if the table ruler is moved)" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_13 = List_10.addChild<factory::Button>({
	  { mpid::Text, "ComponentProperties" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### ComponentProperties\n\nScript properties of a UI component selection (eg. the `visible` property)." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_14 = List_10.addChild<factory::Button>({
	  { mpid::Text, "ComponentVisibility" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### ComponentVisibility\n\nThe visibility of a UI component.\n\n>This also takes into account the visibility of parent components so it's a more reliable way than listen to the component's `visible` property." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_15 = List_10.addChild<factory::Button>({
	  { mpid::Text, "ContextMenu" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### Context Menu\n\nAdds a popup menu when the UI component is clicked." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_16 = List_10.addChild<factory::Button>({
	  { mpid::Text, "EqEvents" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### EQ Events\n\nListens to band add / delete, reset events of a parametriq EQ." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_17 = List_10.addChild<factory::Button>({
	  { mpid::Text, "ModuleParameters" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### Module Parameter\n\nListens to changes of a module attribute (when calling `setAttribute()`, eg. the **Reverb Width** or **Filter Frequency**" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_18 = List_10.addChild<factory::Button>({
	  { mpid::Text, "MouseEvents" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "## Mouse Events\n\nMouse events for a UI component selection" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_19 = List_10.addChild<factory::Button>({
	  { mpid::Text, "ProcessingSpecs" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### Processing Specs\n\nListens to changes of the processing specifications (eg. sample rate of audio buffer size)" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_20 = List_10.addChild<factory::Button>({
	  { mpid::Text, "RadioGroup" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### Radio Group\n\nListens to button clicks within a given radio group ID\n> This is especially useful for implementing your page switch logic" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachType_21 = List_10.addChild<factory::Button>({
	  { mpid::Text, "RoutingMatrix" }, 
	  { mpid::ID, "attachType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "### Routing Matrix\n\nListens to changes of the routing matrix (the channel routing configuration) of a module." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	// Custom callback for page List_7
	List_7.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	auto& List_22 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& attachType_23 = List_22.addChild<factory::Branch>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::ID, "attachType" }
	});

	auto& List_24 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& Skip_25 = List_24.addChild<factory::Skip>({
	  { mpid::Text, "LabelText" }
	});

	auto& List_26 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_27 = List_26.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Complex Type\n\nAttaching a broadcaster to a complex data object lets you listen to table edit changes or playback position updates of one or multiple data sources. Please fill in the information below to proceed to the next step." }, 
	  { mpid::Padding, 0 }
	});

	auto& complexDataType_28 = List_26.addChild<factory::Choice>({
	  { mpid::Text, "DataType" }, 
	  { mpid::ID, "complexDataType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Custom, 0 }, 
	  { mpid::ValueMode, "Text" }, 
	  { mpid::Items, "Table\nSliderPack\nAudioFile" }, 
	  { mpid::Help, "The data type that you want to listen to" }
	});

	auto& complexEventType_29 = List_26.addChild<factory::Choice>({
	  { mpid::Text, "Event Type" }, 
	  { mpid::ID, "complexEventType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Custom, 0 }, 
	  { mpid::ValueMode, "Text" }, 
	  { mpid::Help, "The event type you want to listen to.\n\n-**Content** events will be triggered whenever the data changes (so eg. loading a new sample or editing a table will trigger this event).\n-**DisplayIndex** events will occur whenever the read position changes (so the playback position in the audio file or the table ruler in the table)." }, 
	  { mpid::Items, "Content\nDisplayIndex" }
	});

	auto& moduleIds_30 = List_26.addChild<factory::TextInput>({
	  { mpid::Text, "Module IDs" }, 
	  { mpid::ID, "moduleIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter module IDs as shown in the Patch Browser..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }, 
	  { mpid::Multiline, 0 }
	});

	auto& complexSlotIndex_31 = List_26.addChild<factory::TextInput>({
	  { mpid::Text, "Slot Index" }, 
	  { mpid::ID, "complexSlotIndex" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter Slot Index (zero based)" }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The slot index of the complex data object that you want to listen to.\n\n> Some modules have multiple complex data objects (eg. the table envelope has two tables for the attack and release phase so if you want to listen to the release table, you need to pass in `1` here." }
	});

	auto& attachMetadata_32 = List_26.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_33 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_34 = List_33.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Component Properties\n\nAttaches the broadcaster to changes of the script properties like `enabled`, `text`, etc.\n\n> If you want to listen to visibility changes, take a look at the **ComponentVisibility** attachment mode, which also takes the visibility of parent components into account." }, 
	  { mpid::Padding, 0 }
	});

	auto& componentIds_35 = List_33.addChild<factory::TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Multiline, 0 }
	});

	auto& propertyType_36 = List_33.addChild<factory::TextInput>({
	  { mpid::Text, "Property" }, 
	  { mpid::ID, "propertyType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter script property (text, enabled, etc)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "The property you want to listen to. You can also use a comma-separated list for multiple properties" }
	});

	auto& attachMetadata_37 = List_33.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_38 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_39 = List_38.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Component Visibility\n\nThis mode attaches the broadcaster to whether the component is actually shown on the interface, which also takes into account the parent visibility and whether its bounds are within the parent's dimension" }, 
	  { mpid::Padding, 0 }
	});

	auto& componentIds_40 = List_38.addChild<factory::TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "A comma-separated list of all components that you want to listen to." }, 
	  { mpid::Multiline, 0 }
	});

	auto& attachMetadata_41 = List_38.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_42 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_43 = List_42.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Context Menu\n\nThis attach mode will show a context menu when you click on the registered UI components and allow you to perform additional actions." }, 
	  { mpid::Padding, 0 }
	});

	auto& componentIds_44 = List_42.addChild<factory::TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "A comma-separated list of all components that you want to listen to." }, 
	  { mpid::Multiline, 0 }
	});

	auto& contextStateFunctionId_45 = List_42.addChild<factory::TextInput>({
	  { mpid::Text, "State Function" }, 
	  { mpid::ID, "contextStateFunctionId" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter state function id..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "A state function that will be used to query the enable and ticked state of the context menu. This will be called whenever it needs to refresh the context menu state so you need to supply a function that takes two parameters, `isEnabled` and `index` and return the appropriate boolean value." }
	});

	auto& contextItems_46 = List_42.addChild<factory::TextInput>({
	  { mpid::Text, "Item List" }, 
	  { mpid::ID, "contextItems" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter items..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "This will define the items that are shown in the list (one item per new line). You can use the markdown-like syntax to create sub menus, separators and headers known from the other Context menu builders in HISE (ScriptPanel, SubmenuCombobox)..." }, 
	  { mpid::Multiline, 1 }
	});

	auto& contextLeftClick_47 = List_42.addChild<factory::Button>({
	  { mpid::Text, "Trigger on Left Click" }, 
	  { mpid::ID, "contextLeftClick" }, 
	  { mpid::LabelPosition, "Left" }, 
	  { mpid::Help, "If this is true, the context menu will be shown when you click on the UI element with the left mouse button, otherwise it will require a right click to show up." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& attachMetadata_48 = List_42.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_49 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_50 = List_49.addChild<factory::MarkdownText>({
	  { mpid::Text, "### EQ Events\n\nThis attachment mode will register one or more Parametriq EQ modules to the broadcaster and will send events when EQ bands are added / removed.\n\n> This does not cover the band parameter changes, as this can be queried with the usual module parameter attachment mode." }, 
	  { mpid::Padding, 0 }
	});

	auto& moduleIds_51 = List_49.addChild<factory::TextInput>({
	  { mpid::Text, "Module IDs" }, 
	  { mpid::ID, "moduleIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter module IDs as shown in the Patch Browser..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }, 
	  { mpid::Multiline, 0 }
	});

	auto& eqEventTypes_52 = List_49.addChild<factory::TextInput>({
	  { mpid::Text, "Event Types" }, 
	  { mpid::ID, "eqEventTypes" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter event types (BandAdded, BandRemoved, etc)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Items, "BandAdded\nBandRemoved\nBandSelected\nFFTEnabled" }, 
	  { mpid::Help, "Set the event type that the broadcaster should react too. This can be multiple items from this list:\n\n- `BandAdded`\n- `BandRemoved`\n- `BandSelected`\n- `FFTEnabled`" }, 
	  { mpid::Multiline, 0 }
	});

	auto& attachMetadata_53 = List_49.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_54 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_55 = List_54.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Module Parameters\n\nAttaches the broadcaster to attribute changes of one or more modules." }, 
	  { mpid::Padding, 0 }
	});

	auto& moduleIds_56 = List_54.addChild<factory::TextInput>({
	  { mpid::Text, "Module IDs" }, 
	  { mpid::ID, "moduleIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter module IDs as shown in the Patch Browser..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }, 
	  { mpid::Multiline, 0 }
	});

	auto& moduleParameterIndexes_57 = List_54.addChild<factory::TextInput>({
	  { mpid::Text, "Parameters" }, 
	  { mpid::ID, "moduleParameterIndexes" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter parameters..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "The parameters that you want to listen to. This can be either the actual parameter names or the indexes of the parameters" }, 
	  { mpid::Multiline, 0 }
	});

	auto& attachMetadata_58 = List_54.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_59 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_60 = List_59.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Mouse Events\n\nThis attachment mode will cause the broadcaster to fire at certain mouse callback events (just like the ScriptPanel's `mouseCallback`)." }, 
	  { mpid::Padding, 0 }
	});

	auto& componentIds_61 = List_59.addChild<factory::TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "componentIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "A comma-separated list of all components that you want to listen to." }, 
	  { mpid::Multiline, 0 }
	});

	auto& mouseCallbackType_62 = List_59.addChild<factory::Choice>({
	  { mpid::Text, "Callback Type" }, 
	  { mpid::ID, "mouseCallbackType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Custom, 0 }, 
	  { mpid::ValueMode, "Text" }, 
	  { mpid::Help, "The callback level that determines when the broadcaster will send a message" }, 
	  { mpid::Items, "No Callbacks\nContext Menu\nClicks Only\nClicks & Hover\nClicks, Hover & Dragging\nAll Callbacks" }
	});

	auto& attachMetadata_63 = List_59.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_64 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_65 = List_64.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Processing specs\n\nThis will attach the broadcaster to changes of the audio processing specs (sample rate, buffer size, etc)." }, 
	  { mpid::Padding, 0 }
	});

	auto& attachMetadata_66 = List_64.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_67 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_68 = List_67.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Radio Group\n\nThis will attach the broadcaster to a radio group (a selection of multiple buttons that are mutually exclusive).\n\n> This is a great way of handling page logic by attaching a broadcaster to a button group and then use its callback to show and hide pages within an array." }, 
	  { mpid::Padding, 0 }
	});

	auto& radioGroupId_69 = List_67.addChild<factory::TextInput>({
	  { mpid::Text, "Radio Group" }, 
	  { mpid::ID, "radioGroupId" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter radio group ID..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The radio group id as it was set as `radioGroup` script property to the buttons that you want to listen to." }, 
	  { mpid::Multiline, 0 }
	});

	auto& attachMetadata_70 = List_67.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_71 = attachType_23.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_72 = List_71.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Routing Matrix\n\nThis attaches the broadcaster to changes in a channel routing of one or more modules (either channel resizes or routing changes including send channel assignments)." }, 
	  { mpid::Padding, 0 }
	});

	auto& moduleIds_73 = List_71.addChild<factory::TextInput>({
	  { mpid::Text, "Module IDs" }, 
	  { mpid::ID, "moduleIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter module IDs as shown in the Patch Browser..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }, 
	  { mpid::Multiline, 0 }
	});

	auto& attachMetadata_74 = List_71.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "attachMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	// Custom callback for page List_22
	List_22.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	auto& List_75 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& Column_76 = List_75.addChild<factory::Column>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::Width, "-0.65, -0.35" }, 
	  { mpid::Padding, "30" }
	});

	auto& MarkdownText_77 = Column_76.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Target type\n\nNow you can add a target listener which will receive the events caused by the event source specified on the previous page.\n\n> With this dialog you can only create a single listener, but a broadcaster is of course capable of sending messages to multiple listeners." }, 
	  { mpid::Padding, 0 }
	});

	auto& List_78 = Column_76.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& targetType_79 = List_78.addChild<factory::Button>({
	  { mpid::Text, "None" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "Skip the creation of a target callback." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& targetType_80 = List_78.addChild<factory::Button>({
	  { mpid::Text, "Callback" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "A script function that will be executed immediately when the event occurs." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& targetType_81 = List_78.addChild<factory::Button>({
	  { mpid::Text, "Callback (Delayed)" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "A script function that will be executed with a given delay after the event occurred." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& targetType_82 = List_78.addChild<factory::Button>({
	  { mpid::Text, "Component Property" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "This will change a component property based on a customizeable function that must calculate and return the value" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& targetType_83 = List_78.addChild<factory::Button>({
	  { mpid::Text, "Component Refresh" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "Simply sends out a component refresh method (eg. `repaint()`) to its registered components." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& targetType_84 = List_78.addChild<factory::Button>({
	  { mpid::Text, "Component Value" }, 
	  { mpid::ID, "targetType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "This sets the value of a component" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	// Custom callback for page List_75
	List_75.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	auto& List_85 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& targetType_86 = List_85.addChild<factory::Branch>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::ID, "targetType" }
	});

	auto& List_87 = targetType_86.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& Skip_88 = List_87.addChild<factory::Skip>({
	  { mpid::Text, "LabelText" }
	});

	auto& List_89 = targetType_86.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_90 = List_89.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Callback\n\nThis will call a function with a customizeable `this` object." }, 
	  { mpid::Padding, 0 }
	});

	auto& thisTarget_91 = List_89.addChild<factory::TextInput>({
	  { mpid::Text, "This Object" }, 
	  { mpid::ID, "thisTarget" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter this object..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "You can specify any HiseScript expression that will be used as `this` object in the function." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetFunctionId_92 = List_89.addChild<factory::TextInput>({
	  { mpid::Text, "Function" }, 
	  { mpid::ID, "targetFunctionId" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter function ID..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "This will use the given function as an ID. Leave this empty in order to create an inplace anonymous function." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetMetadata_93 = List_89.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_94 = targetType_86.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_95 = List_94.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Delayed Callback\n\nThis will call a script function with a delay." }, 
	  { mpid::Padding, 0 }
	});

	auto& thisTarget_96 = List_94.addChild<factory::TextInput>({
	  { mpid::Text, "This Object" }, 
	  { mpid::ID, "thisTarget" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter this object..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "You can specify any HiseScript expression that will be used as `this` object in the function." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetFunctionId_97 = List_94.addChild<factory::TextInput>({
	  { mpid::Text, "Function" }, 
	  { mpid::ID, "targetFunctionId" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter function ID..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "This will use the given function as an ID. Leave this empty in order to create an inplace anonymous function." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetDelay_98 = List_94.addChild<factory::TextInput>({
	  { mpid::Text, "Delay time" }, 
	  { mpid::ID, "targetDelay" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter delay time (ms)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The delay time in milliseconds" }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetMetadata_99 = List_94.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata (optional)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_100 = targetType_86.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_101 = List_100.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Component Property\n\nThis will set one or more component properties of one or more components to a given value. You can supply a custom function that will calculate a value for each target, otherwise the value of the broadcaster is used.\n\n> This is eg. useful if you just want to sync (forward) some property changes to other components." }, 
	  { mpid::Padding, 0 }
	});

	auto& targetComponentIds_102 = List_100.addChild<factory::TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "targetComponentIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetPropertyType_103 = List_100.addChild<factory::TextInput>({
	  { mpid::Text, "Property" }, 
	  { mpid::ID, "targetPropertyType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter script property (text, enabled, etc)..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "The property you want to listen to. You can also use a comma-separated list for multiple properties" }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetFunctionId_104 = List_100.addChild<factory::TextInput>({
	  { mpid::Text, "Function" }, 
	  { mpid::ID, "targetFunctionId" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter function ID..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "This will use the given function as an ID. Leave this empty in order to create an inplace anonymous function." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetMetadata_105 = List_100.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_106 = targetType_86.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_107 = List_106.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Component Refresh\n\nThis will send out an update message to the specified components" }, 
	  { mpid::Padding, 0 }
	});

	auto& targetComponentIds_108 = List_106.addChild<factory::TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "targetComponentIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetRefreshType_109 = List_106.addChild<factory::Choice>({
	  { mpid::Text, "Refresh Type" }, 
	  { mpid::ID, "targetRefreshType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Custom, 0 }, 
	  { mpid::ValueMode, "Text" }, 
	  { mpid::Help, "The type of refresh message that should be sent:\n\n- `repaint`: causes a component repaint message (and if the component is a panel, it will also call its paint routine)\n- `changed`: causes a value change callback that will fire the component's `setValue()` callback\n- `updateValueFromProcessorConnection`: updates the component from the current processor's value (if it is connected via `processorId` and `parameterId`).\n- `loseFocus`: will lose the focus of the keyboard (if the component has currently the keyboard focus).\n- `resetValueToDefault`: will reset the component to its default (as defined by the `defaultValue` property). Basically the same as a double click." }, 
	  { mpid::Items, "repaint\nchanged\nupdateValueFromProcessorConnection\nloseFocus\nresetValueToDefault" }
	});

	auto& targetMetadata_110 = List_106.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	auto& List_111 = targetType_86.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& MarkdownText_112 = List_111.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Component Value\n\nThis will cause a value change alongside with a control callback for the given components. The value that is send to the components can be customized with a function, otherwise it will use the broadcaster's value (if applicable)." }, 
	  { mpid::Padding, 0 }
	});

	auto& targetComponentIds_113 = List_111.addChild<factory::TextInput>({
	  { mpid::Text, "Component IDs" }, 
	  { mpid::ID, "targetComponentIds" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter component IDs as shown in the Component List..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 1 }, 
	  { mpid::Help, "A comma-separated list of all component IDs that you want to listen to." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetFunctionId_114 = List_111.addChild<factory::TextInput>({
	  { mpid::Text, "Function" }, 
	  { mpid::ID, "targetFunctionId" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter function ID..." }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "This will use the given function as an ID. Leave this empty in order to create an inplace anonymous function." }, 
	  { mpid::Multiline, 0 }
	});

	auto& targetMetadata_115 = List_111.addChild<factory::TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "targetMetadata" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::EmptyText, "Enter metadata..." }, 
	  { mpid::Required, 1 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Help, "The metadata that will be shown on the broadcaster map." }, 
	  { mpid::Multiline, 0 }
	});

	// Custom callback for page List_85
	List_85.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	auto& List_116 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& MarkdownText_117 = List_116.addChild<factory::MarkdownText>({
	  { mpid::Text, "Press Finished in order to copy the code below to the clipboard (you can make some manual adjustments before)." }, 
	  { mpid::Padding, 0 }
	});


    List_116.addChild<CustomResultPage>();
    
    // Custom callback for page List_116
    List_116.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){
        return Result::ok();
    });

    return mp_;
}

Dialog* NewProjectWizard::createDialog(State& state)
{
	// BEGIN AUTOGENERATED CODE
	using namespace multipage;
	using namespace factory;
	auto mp_ = new Dialog({}, state, false);
	auto& mp = *mp_;
	mp.setProperty(mpid::Header, "New Project Wizard");
	mp.setProperty(mpid::Subtitle, "");
	auto& List_0 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& MarkdownText_1 = List_0.addChild<factory::MarkdownText>({
	  { mpid::Text, "Welcome to the HISE new project wizard. This will guide you through the process of setting up a HISE project folder in order to start the development of your next HISE project.  \n\nFirst you need to select a folder where you want to store the project. Just select a preexistent empty folder or create a new folder using the file browser below.\n\n> Make sure that the folder is actually empty. If you've checked out a Git repo it will ignore the git files." }, 
	  { mpid::Padding, 0 }
	});

	auto& rootDirectory_2 = List_0.addChild<factory::FileSelector>({
	  { mpid::Text, "Project Directory:" }, 
	  { mpid::ID, "rootDirectory" }, 
	  { mpid::LabelPosition, "Above" }, 
	  { mpid::Help, "The directory that will be used for the HISE project assets. It will create subfolders for scripts, images, etc.\n\n> It's highly recommended to use a version control system like Git for the HISE project folder in order to track changes during your development" }, 
	  { mpid::Directory, 1 }, 
	  { mpid::SaveFile, 0 }
	});

	// Custom callback for page List_0
	List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	auto& List_3 = mp.addPage<factory::List>({
	  { mpid::Padding, "20" }, 
	  { mpid::Foldable, 0 }, 
	  { mpid::Folded, 0 }
	});

	auto& MarkdownText_4 = List_3.addChild<factory::MarkdownText>({
	  { mpid::Text, "Now please specify whether you want to start with a blank project, or start out with a template / import a project" }, 
	  { mpid::Padding, 0 }
	});

	auto& projectType_5 = List_3.addChild<factory::Button>({
	  { mpid::Text, "Empty Project" }, 
	  { mpid::ID, "projectType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "Create a empty folder" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& projectType_6 = List_3.addChild<factory::Button>({
	  { mpid::Text, "Import from HXI" }, 
	  { mpid::ID, "projectType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "Import a HISE project from a HXI file.\n\n> A HXI file is a compressed file format which contains all data for a given expansion and can be extracted back into a HISE project for editing / modifying." }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& projectType_7 = List_3.addChild<factory::Button>({
	  { mpid::Text, "Download Rhapsody template" }, 
	  { mpid::ID, "projectType" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "Downloads the RHAPSODY Player expansion template that gives you a head start if you want to develop a RHAPSODY Player plugin" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	// Custom callback for page List_3
	List_3.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	auto& List_8 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& projectType_9 = List_8.addChild<factory::Branch>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::ID, "projectType" }
	});

	auto& List_10 = projectType_9.addChild<factory::List>({
	  { mpid::Text, "LabelText" }
	});

	auto& Skip_11 = List_10.addChild<factory::Skip>({
	  { mpid::Text, "LabelText" }
	});

	auto& List_12 = projectType_9.addChild<factory::List>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::Padding, "30" }, 
	  { mpid::Foldable, 0 }, 
	  { mpid::Folded, 0 }
	});

	auto& MarkdownText_13 = List_12.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Import HXI File\n\nPlease select the HXI file that you want to import.\n\n> The HXI file must be encoded with the dummy password `1234` in order to be imported into HISE!" }, 
	  { mpid::Padding, 0 }
	});

	auto& hxiDirectory_14 = List_12.addChild<factory::FileSelector>({
	  { mpid::Text, "HXI File" }, 
	  { mpid::ID, "hxiDirectory" }, 
	  { mpid::LabelPosition, "Above" }, 
	  { mpid::Directory, 0 }, 
	  { mpid::SaveFile, 0 }
	});

	auto& hxiImportTask_15 = List_12.addChild<factory::LambdaTask>({
	  { mpid::Text, "Progress" }, 
	  { mpid::ID, "hxiImportTask" }, 
	  { mpid::CallOnNext, 1 }, 
	  { mpid::LabelPosition, "Above" }, 
	  { mpid::Function, "NewProjectWizard::importHXIFile" }
	});

	// lambda task for hxiImportTask_15
	hxiImportTask_15.setLambdaAction(state, BIND_MEMBER_FUNCTION_2(NewProjectWizard::importHXIFile));

	auto& List_16 = projectType_9.addChild<factory::List>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::Padding, "30" }, 
	  { mpid::Foldable, 0 }, 
	  { mpid::Folded, 0 }
	});

	auto& MarkdownText_17 = List_16.addChild<factory::MarkdownText>({
	  { mpid::Text, "### Rhapsody Player template\n\nThe template is being downloaded from the server location and extracted" }, 
	  { mpid::Padding, 0 }
	});

	auto& downloadTemplateTask_18 = List_16.addChild<factory::LambdaTask>({
	  { mpid::Text, "Download template" }, 
	  { mpid::ID, "downloadTemplateTask" }, 
	  { mpid::CallOnNext, 0 }, 
	  { mpid::LabelPosition, "Above" }, 
	  { mpid::Function, "NewProjectWizard::downloadTemplate" }
	});

	// lambda task for downloadTemplateTask_18
	downloadTemplateTask_18.setLambdaAction(state, BIND_MEMBER_FUNCTION_2(NewProjectWizard::downloadTemplate));

	auto& extractTemplateTask_19 = List_16.addChild<factory::LambdaTask>({
	  { mpid::Text, "Extract Template" }, 
	  { mpid::ID, "extractTemplateTask" }, 
	  { mpid::CallOnNext, 0 }, 
	  { mpid::LabelPosition, "Above" }, 
	  { mpid::Function, "NewProjectWizard::importHXIFile" }
	});

	// lambda task for extractTemplateTask_19
	extractTemplateTask_19.setLambdaAction(state, BIND_MEMBER_FUNCTION_2(NewProjectWizard::importHXIFile));

	// Custom callback for page List_8
	List_8.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	auto& List_20 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& MarkdownText_21 = List_20.addChild<factory::MarkdownText>({
	  { mpid::Text, "Press finish in order to close that wizard and enjoy your new project!" }, 
	  { mpid::Padding, 0 }
	});

	auto& openHiseForum_22 = List_20.addChild<factory::Launch>({
	  { mpid::Text, "https://forum.hise.dev" }, 
	  { mpid::ID, "openHiseForum" }, 
	  { mpid::CallOnNext, 1 }
	});

	auto& openHiseForum_23 = List_20.addChild<factory::Button>({
	  { mpid::Text, "Goto HISE Forum" }, 
	  { mpid::ID, "openHiseForum" }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Help, "Opens a webbrowser and goes to the HISE forum where you can start hammering your questions into the keyboard!" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	// Custom callback for page List_20
	List_20.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});

	return mp_;
	// END AUTOGENERATED CODE 

}

} // namespace library
} // namespace multipage
} // namespace hise
