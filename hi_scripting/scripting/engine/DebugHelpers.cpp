

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


void DebugInformation::rightClickCallback(const MouseEvent& e, Component* componentToNotify)
{
	var v = getVariantCopy();


	if (v.isBuffer())
	{
#if USE_BACKEND
		auto display = new HiseAudioThumbnail();
		display->setBuffer(v);

		display->setName("Buffer Viewer");
		display->setSize(500, 100);
		display->setShouldScaleVertically(true);

		auto e2 = e.getEventRelativeTo(componentToNotify);

		GET_BACKEND_ROOT_WINDOW(componentToNotify)->getRootFloatingTile()->showComponentInRootPopup(display, componentToNotify, Point<int>(componentToNotify->getWidth() / 2, e2.getMouseDownY() + 10));
#endif
		return;
	}

	if (v.isObject() || v.isArray())
	{
		DebugableObject::Helpers::showJSONEditorForObject(e, componentToNotify, v, getTextForName());
	}
}

void DebugInformation::doubleClickCallback(const MouseEvent &e, Component* componentToNotify)
{
	if (auto cso = dynamic_cast<ScriptingObject*>(getObject()))
	{
		auto jp = dynamic_cast<Processor*>(cso->getProcessor());

		if (auto sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(getObject()))
		{
			auto b = jp->getMainController()->getScriptComponentEditBroadcaster();
			b->setSelection(sc, sendNotification);
		}

		DebugableObject::Helpers::gotoLocation(jp, this);
	}
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


void gotoLocationInternal(Processor* processor, DebugableObject::Location location)
{
#if USE_BACKEND
	auto editor = dynamic_cast<JavascriptCodeEditor*>(processor->getMainController()->getLastActiveEditor());

	if (editor == nullptr)
		return;

	if (auto editorPanel = editor->findParentComponentOfClass<CodeEditorPanel>())
	{
		editorPanel->gotoLocation(processor, location.fileName, location.charNumber);
	}
	else if (location.fileName.isNotEmpty())
	{
		auto jsp = dynamic_cast<JavascriptProcessor*>(processor);

		File f(location.fileName);

		jsp->showPopupForFile(f, location.charNumber);
	}
	else if (auto scriptEditor = editor->findParentComponentOfClass<ScriptingEditor>())
	{
		scriptEditor->showOnInitCallback();
		scriptEditor->gotoChar(location.charNumber);
	}
#else
	ignoreUnused(processor, location);
#endif
}

void DebugableObject::Helpers::gotoLocation(Component* ed, JavascriptProcessor* sp, const Location& location)
{
#if USE_BACKEND
	auto handler = dynamic_cast<ScriptEditHandler*>(ed);

	if (sp == nullptr && handler != nullptr)
	{
		sp = handler->getScriptEditHandlerProcessor();
	}

	if (sp == nullptr)
	{
		// You have to somehow manage to pass the processor here...
		jassertfalse;
		return;
	}

	gotoLocationInternal(dynamic_cast<Processor*>(sp), location);

#else
	ignoreUnused(ed, sp, location);
#endif
}


void DebugableObject::Helpers::gotoLocation(Processor* processor, DebugInformationBase* info)
{
	gotoLocationInternal(processor, info->getLocation());
}



void DebugableObject::Helpers::showProcessorEditorPopup(const MouseEvent& e, Component* table, Processor* p)
{
#if USE_BACKEND
	if (p != nullptr)
	{
		ProcessorEditorContainer *pc = new ProcessorEditorContainer();
		pc->setRootProcessorEditor(p);

		GET_BACKEND_ROOT_WINDOW(table)->getRootFloatingTile()->showComponentInRootPopup(pc, table, Point<int>(table->getWidth() / 2, e.getMouseDownY() + 40));
	}
	else
	{
		PresetHandler::showMessageWindow("Processor does not exist", "The Processor is not existing, because it was deleted or the reference is wrong", PresetHandler::IconType::Error);
	}
#else
	ignoreUnused(e, table, p);
#endif
}

void DebugableObject::Helpers::showJSONEditorForObject(const MouseEvent& e, Component* table, var object, const String& id)
{
#if USE_BACKEND

	auto cleanedObject = getCleanedObjectForJSONDisplay(object);

	JSONEditor* jsonEditor = new JSONEditor(cleanedObject);

	jsonEditor->setName((cleanedObject.isArray() ? "Show Array: " : "Show Object: ") + id);
	jsonEditor->setSize(500, 500);
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

DebugInformationBase* DebugableObject::Helpers::getDebugInformation(ApiProviderBase* engine, DebugableObjectBase* object)
{
	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		if (engine->getDebugInformation(i)->getObject() == object)
		{
			return engine->getDebugInformation(i);
		}
	}

	return nullptr;
}

DebugInformationBase* DebugableObject::Helpers::getDebugInformation(ApiProviderBase* engine, const var& v)
{
	if (auto obj = dynamic_cast<DebugableObjectBase*>(v.getObject()))
	{
		return getDebugInformation(engine, obj);
	}

	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		if(auto dbg = dynamic_cast<DebugInformation*>(engine->getDebugInformation(i)))
		{
			if(dbg->getVariantCopy() == v)
				return dbg;
		}
	}

	return nullptr;
}

} // namespace hise
