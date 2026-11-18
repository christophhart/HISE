

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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

String getArrayTextForVar(const var& value)
{
	if (auto ar = value.getArray())
	{
		auto& arrayToStringify = *ar;
		String output;
		output << "[";
		const int maxSize = jmin<int>(4, arrayToStringify.size());

		for (int i = 0; i < maxSize - 1; i++)
		{
			output << getArrayTextForVar(arrayToStringify[i]) << ", ";
		}

		output << getArrayTextForVar(arrayToStringify[maxSize - 1]);

		if (maxSize != arrayToStringify.size())
			output << ", (...)]";
		else
			output << "]";

		return output;
	}
    else if(auto debugObject = dynamic_cast<DebugableObject*>(value.getObject()))
    {
        return debugObject->getDebugName();
    }
    
    return value.toString();
}
    
String DebugInformation::varArrayToString(const Array<var> &arrayToStringify)
{
	var ar(arrayToStringify);
	return getArrayTextForVar(ar);
}




Component* DebugInformation::createPopupComponent(const MouseEvent& e, Component* componentToNotify)
{
	if (auto c = DebugInformationBase::createPopupComponent(e, componentToNotify))
	{
		return c;
	}

	var v = getVariantCopy();

    if(BufferViewer::isArrayOrBuffer(v))
    {
		auto pc = componentToNotify->findParentComponentOfClass<PanelWithProcessorConnection>();

		if (pc == nullptr)
		{
			auto co = dynamic_cast<ControlledObject*>(componentToNotify);

			if (co == nullptr)
				co = componentToNotify->findParentComponentOfClass<ControlledObject>();

			if (co != nullptr)
			{
				if (auto activeEditor = co->getMainController()->getLastActiveEditor())
				{
					pc = activeEditor->findParentComponentOfClass<PanelWithProcessorConnection>();
				}
			}
		}

		if (pc != nullptr)
		{
			auto p = pc->getProcessor();
			auto holder = dynamic_cast<ApiProviderBase::Holder*>(p);
			jassert(holder != nullptr);

			auto display = new BufferViewer(this, holder);
			return display;
		}

		return nullptr;
	}

	if (v.isObject() || v.isArray())
	{
		return DebugableObject::Helpers::createJSONEditorForObject(e, componentToNotify, v, getTextForName());
	}

	return nullptr;
}

void DebugInformation::doubleClickCallback(const MouseEvent &e, Component* componentToNotify)
{
	PopupIncludeEditor::gotoUriLocation(nullptr, componentToNotify, getLocation());

#if 0
	auto obj = getObject();

	auto location = getLocation();

	
	

	if (auto pc = componentToNotify->findParentComponentOfClass<PanelWithProcessorConnection>())
	{
		auto p = pc->getConnectedProcessor();
		DebugableObject::Helpers::gotoLocation(p, this);
		return;
	}

	

	if (auto cso = dynamic_cast<ScriptingObject*>(obj))
	{
		auto jp = dynamic_cast<Processor*>(cso->getProcessor());

		if (auto sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(getObject()))
		{
			auto b = jp->getMainController()->getScriptComponentEditBroadcaster();
			b->setSelection(sc, sendNotification);
		}

		DebugableObject::Helpers::gotoLocation(jp, this);
	}
#endif
}

String DebugInformation::getTextForRow(Row r)
{
	switch (r)
	{
	case Row::Name: return getTextForName();
	case Row::Type: return getTextForType();
	case Row::DataType: return getTextForDataType();
	case Row::Value: return getTextForValue();
    case Row::numRows:   return "";
	}

	return "";
}

String DebugInformation::toString()
{
	String output;

	output << "Name: " << getTextForRow(Row::Name) << ", ";
	output << "Type: " << getTextForRow(Row::Type) << ", ";
	output << "DataType:" << getTextForRow(Row::DataType) << ", ";
	output << "Value: " << getTextForRow(Row::Value);

	return output;
}

#if USE_BACKEND

struct UndoableLocationSwitch: public UndoableAction
{
    UriLocation getPosition(Processor* p)
    {
        if (auto editor = p->getMainController()->getLastActiveEditor())
        {
            if (auto editorPanel = editor->findParentComponentOfClass<CodeEditorPanel>())
            {
                mcl::TextEditor& e = dynamic_cast<mcl::FullEditor*>(editor)->editor;
                auto s = e.getTextDocument().getSelection(0).head;
                CodeDocument::Position pos(e.getDocument(), s.x, s.y);
				return e.getLanguageManager()->createUriLocation(pos);
            }
        }
        
		return {};
    }
    
    UndoableLocationSwitch(Processor* p, UriLocation location)
    {
        newProcessor = p;
        newLocation = location;
        
        if (auto editor = p->getMainController()->getLastActiveEditor())
        {
            if (auto editorPanel = editor->findParentComponentOfClass<CodeEditorPanel>())
                oldProcessor = editorPanel->getConnectedProcessor();
        }
        
        oldLocation = getPosition(oldProcessor);
    }
    
    bool perform() override
    {
        if(oldProcessor != nullptr)
            oldLocation = getPosition(oldProcessor);
        
        return gotoInternal(newProcessor.get(), newLocation);
    }
    
    bool undo() override
    {
        if(newProcessor != nullptr)
            newLocation = getPosition(newProcessor);
        
        return gotoInternal(oldProcessor.get(), oldLocation);
    }
    
    bool gotoInternal(Processor* processor, UriLocation location)
    {
		if(processor == nullptr || !location)
            return false;
        
        auto editor = processor->getMainController()->getLastActiveEditor();
		return PopupIncludeEditor::gotoUriLocation(nullptr, editor, location);
    }
    
    WeakReference<Processor> oldProcessor, newProcessor;
    UriLocation oldLocation, newLocation;
};
#endif


AttributedString DebugableObject::Helpers::getFunctionDoc(const String& docBody, const Array<Identifier>& parameters)
{
	AttributedString info;
	info.setJustification(Justification::centredLeft);

	info.append("Description: ", GLOBAL_BOLD_FONT(), Colours::black);
	info.append(docBody, GLOBAL_FONT(), Colours::black.withBrightness(0.2f));
	info.append("\nParameters: ", GLOBAL_BOLD_FONT(), Colours::black);
	for (int i = 0; i < parameters.size(); i++)
	{
		info.append(parameters[i].toString(), GLOBAL_MONOSPACE_FONT(), Colours::darkblue);
		if (i != parameters.size() - 1) info.append(", ", GLOBAL_BOLD_FONT(), Colours::black);
	}

	return info;
}

UriLocation DebugableObject::Helpers::getLocationFromProvider(Processor* p, DebugableObjectBase* obj)
{
	if (auto loc = obj->getLocation())
		return loc;

	
	if (auto asProvider = dynamic_cast<ApiProviderBase::Holder*>(p))
	{
		auto engine = asProvider->getProviderBase();

		if (auto ptr = getDebugInformation(engine, obj))
			return ptr->getLocation();
	}

	return {};
}

Component* DebugableObject::Helpers::showProcessorEditorPopup( Component* table, Processor* p)
{
#if USE_BACKEND
	if (p != nullptr)
	{
		ProcessorEditorContainer *pc = new ProcessorEditorContainer();
		pc->setRootProcessorEditor(p);
        pc->setName(p->getId());
		return pc;
	}
	else
	{
		PresetHandler::showMessageWindow("Processor does not exist", "The Processor is not existing, because it was deleted or the reference is wrong", PresetHandler::IconType::Error);
        return nullptr;
	}
#else
	ignoreUnused(table, p);
	return nullptr;
#endif
}

Component* DebugableObject::Helpers::createJSONEditorForObject(const MouseEvent& e, Component* table, var object, const String& id)
{
	auto cleanedObject = getCleanedObjectForJSONDisplay(object);

	JSONEditor* jsonEditor = new JSONEditor(cleanedObject);

	jsonEditor->setName((cleanedObject.isArray() ? "Show Array: " : "Show Object: ") + id);
	jsonEditor->setSize(500, 500);
	
	return jsonEditor;
}

void DebugableObject::Helpers::showJSONEditorForObject(const MouseEvent& e, Component* table, var object, const String& id)
{
#if USE_BACKEND
	auto jsonEditor = createJSONEditorForObject(e, table, object, id);
	auto e2 = e.getEventRelativeTo(table);
	GET_BACKEND_ROOT_WINDOW(table)->getRootFloatingTile()->showComponentInRootPopup(jsonEditor, table, Point<int>(table->getWidth() / 2, e2.getMouseDownY() + 5));
#else
	ignoreUnused(e, table, object, id);
#endif
}


var DebugableObject::Helpers::getCleanedObjectForJSONDisplay(const var& object)
{
	if (object.isBuffer())
	{
		return var(object.getBuffer()->toDebugString());
	}
	else if (auto obj = object.getDynamicObject())
	{
		var copy = var(new DynamicObject());

		auto source = obj->getProperties();
		auto& destination = copy.getDynamicObject()->getProperties();

		for (int i = 0; i < source.size(); i++)
		{
			destination.set(source.getName(i), getCleanedObjectForJSONDisplay(source.getValueAt(i)));
		}

		return copy;
	}
	else if (auto ar = object.getArray())
	{
		auto& source = *ar;

		Array<var> destination;

		for (const auto& v : source)
		{
			destination.add(getCleanedObjectForJSONDisplay(v));
		}

		return var(destination);
	}
	else if (auto debugObject = dynamic_cast<DebugableObjectBase*>(object.getObject()))
	{
		String valueText;
		valueText << debugObject->getDebugName() << ": " << debugObject->getDebugValue();

		return var(valueText);
	}
	else
		return object;
}

DebugInformationBase::List DebugableObject::Helpers::getDebugInformationFromString(ApiProviderBase* engine, const String& token)
{
    DebugInformationBase::List list;
    
    for (int i = 0; i < engine->getNumDebugObjects(); i++)
    {
        auto dobj = engine->getDebugInformation(i);
        auto thisList = getDebugInformationFromString(dobj, token);
        list.addArray(thisList);
    }

    StringArray textValues;
    
    for(int i = 0; i < list.size(); i++)
    {
        auto n = list[i]->getTextForName();
        
        if(n.contains(".locals") || n.contains(".args") ||
           n.contains("[") || textValues.contains(n))
            list.remove(i--);
        else
            textValues.add(n);
    }
    
    return list;
}

DebugInformationBase::List DebugableObject::Helpers::getDebugInformationFromString(DebugInformationBase::Ptr parent, const String& token)
{
    DebugInformationBase::List thisList;
    
    auto text = parent->getTextForName();
    
    if(text.startsWith(token))
        thisList.add(parent);
        
    
    if(!token.containsChar('.') && text.containsChar('.'))
    {
        // Try to resolve it "inside the namespace"...
        if(text.fromFirstOccurrenceOf(".", false, false).startsWith(token))
            thisList.add(parent);
    }
    
    for(int i = 0; i < parent->getNumChildElements(); i++)
    {
        auto childList = getDebugInformationFromString(parent->getChildElement(i), token);
        thisList.addArray(childList);
    }
    
    return thisList;
}

DebugInformation::DebugInformation(Type t): type(t)
{}

DebugInformation::~DebugInformation()
{}

String DebugInformation::getTextForDataType() const
{
	switch (type)
	{
	case Type::RegisterVariable: return "Register";
	case Type::Variables:		 return "Variables";
	case Type::Constant:		 return "Constant";
	case Type::InlineFunction:	 return "InlineFunction";
	case Type::Globals:			 return "Globals";
	case Type::Callback:		 return "Callback";
	case Type::ExternalFunction: return "ExternalFunction";
	case Type::Namespace:		 return "Namespace";
	case Type::numTypes:
	default:                     return {};
	}
}

AttributedString DebugInformation::getDescription() const
{ return AttributedString(); }

String DebugInformation::getCodeToInsert() const
{
	return getTextForName();
}

String DebugInformation::getTextForType() const
{
	return getVarType(getVariantCopy());
}

int DebugInformation::getType() const
{ return (int)type; }

String DebugInformation::getVarValue(const var& v) const
{
	if (DebugableObjectBase *d = getDebugableObject(v))
	{
		return d->getDebugValue();
	}
	else if (v.isArray())
	{
		return varArrayToString(*v.getArray());
	}
	else if (v.isBuffer())
	{
		return v.getBuffer()->toDebugString();
	}
	else return v.toString();
}

DebugableObjectBase* DebugInformation::getDebugableObject(const var& v)
{
	auto obj = v.getObject();
	return dynamic_cast<DebugableObjectBase*>(obj);
}

DynamicObjectDebugInformation::DynamicObjectDebugInformation(DynamicObject* obj_, const Identifier& id_, Type t):
	DebugInformation(t),
	obj(obj_),
	id(id_)
{}

DynamicObjectDebugInformation::~DynamicObjectDebugInformation()
{
	obj = nullptr;
}

bool DynamicObjectDebugInformation::isWatchable() const
{
	static const Array<Identifier> unwatchableIds = 
	{ 
		Identifier("Array"), 
		Identifier("String"), 
		Identifier("Buffer"),
		Identifier("Libraries")
	};

	return !unwatchableIds.contains(id);
}

String DynamicObjectDebugInformation::getTextForName() const
{ return id.toString(); }

String DynamicObjectDebugInformation::getTextForDataType() const
{ return obj != nullptr ? getVarType(obj->getProperty(id)) : "dangling"; }

String DynamicObjectDebugInformation::getTextForValue() const
{ return obj != nullptr ? getVarValue(obj->getProperty(id)) : ""; }

var DynamicObjectDebugInformation::getVariantCopy() const
{ return obj != nullptr ? obj->getProperty(id) : var(); }

DebugableObjectBase* DynamicObjectDebugInformation::getObject()
{
	auto v = getVariantCopy();

	if (auto dyn = v.getDynamicObject())
	{
		wrapper = new DynamicDebugableObjectWrapper(dyn, id, id);
		return wrapper.get();
	}

	if (auto dbo = dynamic_cast<DebugableObjectBase*>(v.getObject()))
		return dbo;

	return nullptr;
}

DebugInformationBase::Ptr DebugableObject::Helpers::getDebugInformation(ApiProviderBase* engine, DebugableObjectBase* object)
{
	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		auto dobj = engine->getDebugInformation(i);

		if (auto ptr = getDebugInformation(dobj, object))
			return ptr;
	}

	return nullptr;
}

DebugInformationBase::Ptr DebugableObject::Helpers::getDebugInformation(ApiProviderBase* engine, const var& v)
{
	if (auto obj = dynamic_cast<DebugableObjectBase*>(v.getObject()))
	{
		return getDebugInformation(engine, obj);
	}

	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		auto b = engine->getDebugInformation(i);

		if(auto dbg = dynamic_cast<DebugInformation*>(b.get()))
		{
			if(dbg->getVariantCopy() == v)
				return b;
		}
	}

	return nullptr;
}

hise::DebugInformationBase::Ptr DebugableObject::Helpers::getDebugInformation(DebugInformationBase::Ptr parent, DebugableObjectBase* object)
{
	if (parent->getObject() == object)
		return parent;

	for (int i = 0; i < parent->getNumChildElements(); i++)
	{
		if (auto c = parent->getChildElement(i))
		{
			if (auto p = getDebugInformation(c, object))
				return p;
		}
	}

	return nullptr;
}


struct UriLocationDebugInformation : public DebugInformationBase
{
	UriLocationDebugInformation(const UriLocation::WithExpression& e, const String& dataType) :
		location(e)
	{

	}

	void doubleClickCallback(const MouseEvent& e, Component* c) override
	{
		PopupIncludeEditor::gotoUriLocation(nullptr, c, location.second);
	}

	int getType() const override {
		return (int)UriError::ErrorType::Unknown;
	}

	String getTextForName() const override
	{
		return location.first;
	}

	String getTextForDataType() const override
	{
		return dataType;
	}

	virtual String getTextForValue() const override
	{
		return location.second.toLocationString(UriLocation::UriFormat::Compact);
	}

	virtual UriLocation getLocation() const override { return location.second; }
	virtual bool isWatchable() const override { return true; }
	virtual bool isAutocompleteable() const override { return false; }

private:

	String dataType;
	UriLocation::WithExpression location;
};

struct UriErrorDebugInformation : public UriLocationDebugInformation
{
	UriErrorDebugInformation(UriErrorLogger::SourceType s, const std::pair<UriError, int>& e) :
		UriLocationDebugInformation({ e.first.wasOk() ? "OK" : e.first.errorMessage, e.first }, ""),
		source(s),
		error(e)
	{}

	int getType() const { return (int)error.first.errorType; }

	String getTextForDataType() const override
	{
		String s;

		switch (source)
		{
		case UriErrorLogger::SourceType::None:
			s << "Unknown"; break;
		case UriErrorLogger::SourceType::Compilation:
			s << "Compilation"; break;
		case UriErrorLogger::SourceType::Runtime:
			s << "Runtime"; break;
		case UriErrorLogger::SourceType::Diagnostics:
			s << "Diagnostics"; break;
		case UriErrorLogger::SourceType::Debug:
			s << "Debugger"; break;
		}

		if (error.second > 1)
			s << " (" << String(error.second) << "x)";

		return s;
	}

	int getNumChildElements() const override
	{
		return (int)error.first.callstack.size();
	}

	Ptr getChildElement(int index) override
	{
		auto revIndex = error.first.callstack.size() - (index + 1);
		return new UriLocationDebugInformation(error.first.callstack[revIndex], "  callstack");
	}

	const UriErrorLogger::SourceType source;
	std::pair<UriError, int> error;
};

struct UriLocationTreeDebugInformation : public UriLocationDebugInformation
{
	UriLocationTreeDebugInformation(UriLocationTree::Ptr p) :
		UriLocationDebugInformation({ p->getAsString(), *p }, ""),
		node(p)
	{};

	int getType() const override
	{
		return UriError::SourceType::Include;
	}

	String getTextForDataType() const override
	{
		auto t = node->category.toString();

		if (t == "inline")
			t << " function";

		if (auto lb = getDiagnosticObject())
			t << ":" << lb->getObjectName().toString();

		auto typeInfo = node->data->getProperty("typeInfo");

		if (typeInfo.isString())
			t << ":" << typeInfo.toString();

		return t;
	}

	int getNumChildElements() const override
	{
		if (auto lb = getDiagnosticObject())
		{
			return lb->occurrences.size();
		}
		else
		{
			return node->getNumChildren();
		}
	}

	Ptr getChildElement(int index) override
	{
		if (auto lb = getDiagnosticObject())
		{
			if (isPositiveAndBelow(index, lb->occurrences.size()))
				return new UriLocationDebugInformation(lb->occurrences[index], "  reference");
		}
		else
		{
			if (auto c = node->getChild(index))
				return new UriLocationTreeDebugInformation(c);
		}

		jassertfalse;
		return nullptr;
	}

	void doubleClickCallback(const MouseEvent& e, Component* toNotify) override
	{
		auto occurrences = JavascriptLocationTreeHelpers::getOccurrences(*node);

		if (occurrences.size() == 1)
		{
			PopupIncludeEditor::gotoUriLocation(nullptr, toNotify, occurrences[0].second);
		}
		else
		{
			PopupLookAndFeel plaf;
			PopupMenu m;
			m.setLookAndFeel(&plaf);
			m.addSectionHeader("Goto call site");
			
			int idx = 1;

			for (const auto& c : occurrences)
			{
				String s;
				s << c.second.toLocationString(UriLocation::UriFormat::Compact) << ": ";
				s << c.first;
				m.addItem(idx++, s);
			}

			if (auto r = m.show())
			{
				auto l = occurrences[r - 1].second;
				PopupIncludeEditor::gotoUriLocation(nullptr, toNotify, l);
			}
		}
	}

	LightweightDiagnostics* getDiagnosticObject() const
	{
		return dynamic_cast<LightweightDiagnostics*>(node->data->getProperty("typeInfo").getObject());
	}

	UriLocationTree::Ptr node;
};

void JavascriptProcessor::ErrorProvider::getColourAndLetterForType(int type, Colour& colour, char& letter)
{
	auto s = (UriError::ErrorType)type;

	switch (s)
	{
	case UriError::ErrorType::OK:
		colour = Colour(HISE_OK_COLOUR);
		letter = 'C';
		break;
	case UriError::ErrorType::Unknown:
		colour = Colours::grey;
		letter = 'S';
		break;
	case UriError::ErrorType::Error:
		colour = Colour(HISE_ERROR_COLOUR);
		letter = 'E';
		break;
	case UriError::ErrorType::Warning:
		colour = Colour(HISE_WARNING_COLOUR);
		letter = 'W';
		break;
	case UriError::ErrorType::Info:
		colour = Colour(HISE_OK_COLOUR);
		letter = 'I';
		break;
	case UriError::ErrorType::Hint:
		colour = Colours::slateblue;
		letter = 'H';
		break;
	case UriError::ErrorType::Breakpoint:
	case UriError::ErrorType::Blink:
	case UriError::ErrorType::BreakBlink:
		colour = Colour(HISE_BREAKPOINT_COLOUR);
		letter = 'B';
		break;
	}

	colour = colour.withMultipliedBrightness(0.5f);
}

int JavascriptProcessor::ErrorProvider::getNumDebugObjects() const
{
	int num = 0;

	if (currentBreakpoint.second != 0)
		num++;

	num += currentBlinkEvents.size();

	if (currentIncludeTree != nullptr)
		num++;

	if (lastTree != nullptr)
		num++;

	num += getNumErrors(UriError::SourceType::All);

	return num;
}

DebugInformationBase::Ptr JavascriptProcessor::ErrorProvider::getDebugInformation(int index)
{
	if (currentBreakpoint.second != 0)
	{
		if (index == 0)
			return new UriErrorDebugInformation(UriErrorLogger::SourceType::Debug, currentBreakpoint);

		index--;
	}

	if (isPositiveAndBelow(index, currentBlinkEvents.size()))
	{
		return new UriErrorDebugInformation(UriErrorLogger::SourceType::Debug, currentBlinkEvents[index]);
	}
	else
		index -= currentBlinkEvents.size();

	if (currentIncludeTree != nullptr)
	{
		if (index == 0)
			return new UriLocationTreeDebugInformation(currentIncludeTree);

		index--;
	}

	if (lastTree != nullptr)
	{
		if (index == 0)
			return new UriLocationTreeDebugInformation(lastTree);

		index--;
	}

	auto list = getErrorList(SourceType::All);

	if (isPositiveAndBelow(index, list.size()))
	{
		auto item = list[index];
		return new UriErrorDebugInformation(item.first.sourceType, item);
	}

	return nullptr;
}

} // namespace hise
