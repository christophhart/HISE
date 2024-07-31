// Put in the header definitions of every dialog here...

#include "broadcaster_resultpage.h"

namespace hise {
namespace multipage {
namespace library {
using namespace juce;




var BroadcasterWizard::checkSelection(const var::NativeFunctionArgs& args)
{
	if(auto fe = dynamic_cast<mcl::FullEditor*>(bpe->getBackendProcessor()->getLastActiveEditor()))
	{
		auto s = fe->editor.getTextDocument().getSelection(0);
        auto selection = fe->editor.getTextDocument().getSelectionContent(s);

        if(selection.contains("Engine.createBroadcaster"))
        {
            addListenersOnly = true;

            auto obj = selection.fromFirstOccurrenceOf("{", true, true).upToLastOccurrenceOf("}", true, true);

            auto cid = selection.upToFirstOccurrenceOf("=", false, false).trim();

            cid = cid.replace("const", "");
            cid = cid.replace("reg", "");
            cid = cid.replace("var", "");
            cid = cid.replace("global", "");
            cid = cid.trim();

            auto p = JSON::parse(obj);

            if(auto go = state.globalState.getDynamicObject())
            {
                go->setProperty("id", p["id"]);

                String args;

                if(p["args"].isArray())
                {
	                for(auto& a: *p["args"].getArray())
                        args << a.toString() << ", ";
                }

                if(cid != p["id"].toString())
                {
	                customId = cid;
                }

                go->setProperty("noneArgs", args.upToLastOccurrenceOf(", ", false, false));
            }

	        return var(true);
        }

        
	}
    
	//auto content = BackendCommandTarget::Actions::exportFileAsSnippet(bpe, false);
        
	return var(false);
}

StringArray BroadcasterWizard::getAutocompleteItems(const Identifier& textEditorId)
{
    using SourceIndex = CustomResultPage::SourceIndex;
    
    auto chain = findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor()->getMainSynthChain();
    auto sp = ProcessorHelpers::getFirstProcessorWithType<ProcessorWithScriptingContent>(chain);
    
    if(textEditorId == Identifier("moduleIds"))
    {
        auto attachType = (SourceIndex)(int)getProperty("attachType");
        
        switch(attachType)
        {
            case SourceIndex::ComplexData:
                return ProcessorHelpers::getAllIdsForType<ProcessorWithExternalData>(chain);
            case SourceIndex::EqEvents:
                return ProcessorHelpers::getAllIdsForType<CurveEq>(chain);
            case SourceIndex::ModuleParameters:
            {
                auto sa = ProcessorHelpers::getAllIdsForType<Processor>(chain);
                sa.removeDuplicates(false);
                sa.sort(true);
                return sa;
            }
			case SourceIndex::RoutingMatrix:
                return ProcessorHelpers::getAllIdsForType<RoutableProcessor>(chain);
        }
    }
    if(textEditorId == Identifier("moduleParameterIndexes"))
    {
        auto firstId = getProperty("moduleIds")[0].toString().trim();
        
        if(auto p = ProcessorHelpers::getFirstProcessorWithName(chain, firstId))
        {
            StringArray sa;
            int numParameters = p->getNumParameters();
            for(int i = 0; i < numParameters; i++)
                sa.add(p->getIdentifierForParameterIndex(i).toString());

            return sa;
        }
    }
    if(textEditorId == Identifier("componentIds") ||
       textEditorId == Identifier("targetComponentIds"))
    {
        StringArray sa;
        
        int numComponents = sp->getScriptingContent()->getNumComponents();

        for(int i = 0; i < numComponents; i++)
        {
            sa.add(sp->getScriptingContent()->getComponent(i)->getId());
        }

        return sa;
    }
    if(textEditorId == Identifier("targetModuleId"))
    {
	    auto sa = ProcessorHelpers::getAllIdsForType<Processor>(chain);
        sa.removeDuplicates(false);
        sa.sort(true);
        return sa;
    }
    if(textEditorId == Identifier("targetModuleParameter"))
    {
	    auto firstId = getProperty("targetModuleId")[0].toString().trim();
        
        if(auto p = ProcessorHelpers::getFirstProcessorWithName(chain, firstId))
        {
            StringArray sa;
            int numParameters = p->getNumParameters();
            for(int i = 0; i < numParameters; i++)
                sa.add(p->getIdentifierForParameterIndex(i).toString());

            return sa;
        }
    }
    if(textEditorId == Identifier("propertyType") ||
       textEditorId == Identifier("targetPropertyType"))
    {
        auto pToUse = textEditorId == Identifier("propertyType") ? "componentIds" : "targetComponentIds";

        auto n = getProperty(pToUse);

        StringArray sa;

        if(n.isArray())
        {
	        auto name = n[0].toString().trim();

	        if(auto sc = sp->getScriptingContent()->getComponentWithName(name.trim()))
	        {
		        auto numIds = sc->getNumIds();

	            for(int i = 0; i < numIds; i++)
		            sa.add(sc->getIdFor(i).toString());
	        }
        }

        return sa;
    }
    
    return {};
}


CustomResultPage::CustomResultPage(Dialog& r, const var& obj):
    PlaceholderContentBase(r, obj),
    textDoc(doc),
    codeEditor(textDoc)
{
    codeEditor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF222222));
    addAndMakeVisible(codeEditor);
}

var CustomResultPage::getArgs(SourceIndex source, const String& noneArgs)
{
    switch (source)
    {
    case SourceIndex::None:                 return noneArgs;
    case SourceIndex::ComplexData:          return "processor, index, value";
    case SourceIndex::ComponentProperties:  return "component, property, value";
    case SourceIndex::ComponentValue:       return "component, value";
    case SourceIndex::ComponentVisibility:  return "component, isVisible";
    case SourceIndex::ContextMenu:          return "component, index";
    case SourceIndex::EqEvents:             return "eventType, value";
    case SourceIndex::ModuleParameters:     return "processor, parameter, value";
    case SourceIndex::MouseEvents:          return "component, event";
    case SourceIndex::ProcessingSpecs:      return "sampleRate, blockSize";
    case SourceIndex::RadioGroup:           return "radioGroupIndex";
    case SourceIndex::RoutingMatrix:        return "module";
    case SourceIndex::numSourceIndexTypes: break;
    default: ;
    }

    return var();
}

String CustomResultPage::createFunctionBodyIfAnonymous(const String& functionName,
    SourceIndex sourceIndex, bool createValueFunction, const String& noneArgs)
{
    if(functionName.isNotEmpty())
        return functionName;

    String f;
    f << "function(";

    if(createValueFunction)
        f << "index, ";
    
    auto args = getArgs(sourceIndex, noneArgs).toString();

    f << args << "){";

    if(createValueFunction)
        f << "\n\treturn " << args.fromLastOccurrenceOf(", ", false, false) << ";\n";
    else
        f << "\n\t// ADD CODE HERE...\n";

    f << "}";
    return f;
}

void CustomResultPage::appendLine(String& x, const var& state, const String& suffix,
    const Array<var>& args, Array<StringProcessor> sp)
{
    x << getVariableName() << suffix << "(";

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
            else if(pr == StringProcessor::ParseArray)
                v = JSON::toString(Dialog::parseCommaList(v), true);
            else if (pr == StringProcessor::MultilineToVarArray)
            {
                auto sa = StringArray::fromLines(a.toString());
                Array<var> list;

                for(auto& s: sa)
                    list.add(var(s));
                
	            v = JSON::toString(var(list), true);
            }
            else if (pr == StringProcessor::FirstItemAsString)
                v = a[0].toString().quoted();
            else if(pr == StringProcessor::JoinToStringWithNewLines)
                v = Dialog::joinVarArrayToNewLineString(a).replace("\n", "\\n").quoted();
        }

        x << v;

        if(++idx != args.size())
            x << ", ";
    }

    x << ");\n";
}

String CustomResultPage::getTargetLine(TargetIndex target, const var& state)
{
    String x;

    auto createValueFunction = target >= TargetIndex::ComponentProperty;

    auto noneArgs = state["noneArgs"].toString();
    
    auto functionBody = createFunctionBodyIfAnonymous(state["targetFunctionId"].toString(), (SourceIndex)(int)state["attachType"], createValueFunction, noneArgs);

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
    case TargetIndex::ModuleParameter:
        appendLine(x, state, ".addModuleParameterSyncer",
                   { state["targetModuleId"], state["targetModuleParameter"], state["targetMetadata"] },
                   { StringProcessor::FirstItemAsString, StringProcessor::FirstItemAsString, StringProcessor::None });
    case TargetIndex::numTargetIndexTypes: break;
    default: ;
    }
            
    return x;
}

String CustomResultPage::getAttachLine(SourceIndex source, const var& state)
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
    case SourceIndex::ComponentValue:
        appendLine(x, state, ".attachToComponentValue", { state["componentIds"], state["attachMetadata"]});
        break;
    case SourceIndex::ComponentVisibility:
        appendLine(x, state, ".attachToComponentVisibility", { state["componentIds"],
                       state["attachMetadata"]
                   });
        break;
    case SourceIndex::ContextMenu:

        x << "inline function " << state["contextStateFunctionId"].toString() << "(type, index)";
        x << R"(
{
	if(type == "enabled")
	{
		return true; // return the enabled state based on the index";
	}
    if(type == "active")
    {
		return false; // return the tick state
    }

    return true;
}
)";

        appendLine(x, state, ".attachToContextMenu", { state["componentIds"],
                       state["contextStateFunctionId"],
                       state["contextItems"],
                       state["attachMetadata"],
                       state["contextLeftClick"],
                   },
                   {
                       StringProcessor::None,
                       StringProcessor::Unquote,
                       StringProcessor::MultilineToVarArray,
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

String CustomResultPage::getVariableName() const
{
    auto br = findParentComponentOfClass<BroadcasterWizard>();
    
    auto cid = Dialog::getGlobalState(*const_cast<CustomResultPage*>(this), "id", var()).toString();
    
    if(br->customId.isNotEmpty())
        cid = br->customId;

    return cid;
}

void CustomResultPage::postInit()
{
    gs = Dialog::getGlobalState(*this, {}, var());
    String b;

    auto listenersOnly = findParentComponentOfClass<BroadcasterWizard>()->addListenersOnly;

    String nl = "\n";

    if(!listenersOnly)
    {
	    b << "// Broadcaster definition" << nl;
	    b << "const var " << getVariableName() << " = Engine.createBroadcaster({" << nl;

	    auto sourceIndex = (SourceIndex)(int)gs["attachType"];
	    auto noneArgs = gs["noneArgs"].toString();

	    b << "  " << String("id").quoted() << ": " << gs["id"].toString().quoted();
	    b << ",\n  " << String("args").quoted() << ": " << JSON::toString(Dialog::parseCommaList(getArgs(sourceIndex, noneArgs)), true);

	    if(gs["tags"].toString().isNotEmpty())
	        b << ",\n  " << String("tags").quoted() << ": " << JSON::toString(gs["tags"], true);

	    if(gs["comment"].toString().isNotEmpty())
	        b << ",\n  " << String("comment").quoted() << ": " << gs["comment"].toString().quoted();

	    if((int)gs["colour"])
	        b << ",\n  " << String("colour").quoted() << ": " << gs["colour"].toString();
	            
	    b << nl << "});" << nl << nl;

	    if(sourceIndex != SourceIndex::None)
	    {
	        b << "// attach to event Type" << nl;
	        b << getAttachLine(sourceIndex, gs);
	    }
    }
    
    auto targetIndex = (TargetIndex)(int)gs["targetType"];

    if(targetIndex != TargetIndex::None)
    {
        if(listenersOnly)
        {
	        b << nl << "// attach additional listener" << nl;
        }
        else
        {
	        b << nl << "// attach first listener" << nl;
        }
        
        b << getTargetLine(targetIndex, gs);
    }

    doc.replaceAllContent(b);
}

void CustomResultPage::resized()
{
    codeEditor.setBounds(getLocalBounds());
}

Result CustomResultPage::checkGlobalState(var globalState)
{
    SystemClipboard::copyTextToClipboard(doc.getAllContent());
            
    return Result::ok();
}


} // namespace library
} // namespace multipage
} // namespace hise


