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
	return nullptr;
}

Dialog* NewProjectWizard::createDialog(State& state)
{
	return nullptr;
}

} // namespace library
} // namespace multipage
} // namespace hise
