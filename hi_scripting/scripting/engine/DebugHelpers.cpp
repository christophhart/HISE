

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

String DebugInformation::varArrayToString(const Array<var> &arrayToStringify)
{
	String output;
	output << "[";
	const int maxSize = jmin<int>(4, arrayToStringify.size());

	for (int i = 0; i < maxSize - 1; i++)
		output << arrayToStringify[i].toString() << ", ";

	output << arrayToStringify[maxSize - 1].toString();

	if (maxSize != arrayToStringify.size())
		output << ", (...)]";
	else
		output << "]";

	return output;
}

StringArray DebugInformation::createTextArray()
{
	StringArray sa;
	
	sa.add(getTextForType());
	sa.add(getTextForDataType());
	sa.add(getTextForName());
	sa.add(getTextForValue());
	
	return sa;
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


void DebugableObject::Helpers::gotoLocation(Processor* processor, DebugInformation* info)
{
	gotoLocationInternal(processor, info->location);
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
#endif
}

void DebugableObject::Helpers::showJSONEditorForObject(const MouseEvent& e, Component* table, var object, const String& id)
{
#if USE_BACKEND
	JSONEditor* jsonEditor = new JSONEditor(object);

	jsonEditor->setName((object.isArray() ? "Show Array: " : "Show Object: ") + id);
	jsonEditor->setSize(500, 500);

	GET_BACKEND_ROOT_WINDOW(table)->getRootFloatingTile()->showComponentInRootPopup(jsonEditor, table, Point<int>(table->getWidth() / 2, e.getMouseDownY() + 40));
#endif
}

DebugInformation* DebugableObject::Helpers::getDebugInformation(HiseJavascriptEngine* engine, DebugableObject* object)
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

DebugInformation* DebugableObject::Helpers::getDebugInformation(HiseJavascriptEngine* engine, const var& v)
{
	if (auto obj = dynamic_cast<DebugableObject*>(v.getObject()))
	{
		return getDebugInformation(engine, obj);
	}

	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		if (engine->getDebugInformation(i)->getVariantCopy() == v)
		{
			return engine->getDebugInformation(i);
		}
	}

	return nullptr;
}

