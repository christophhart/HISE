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


JavascriptCodeEditor::AutoCompletePopup::AutoCompletePopup(int fontHeight_, JavascriptCodeEditor* editor_, Range<int> tokenRange_, const String &tokenText) :
	fontHeight(fontHeight_),
	editor(editor_),
	tokenRange(tokenRange_)
{
	sp = editor->scriptProcessor;



	addAndMakeVisible(listbox = new ListBox());
	addAndMakeVisible(infoBox = new InfoBox());

	const ValueTree apiTree = ValueTree(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize));

	if (tokenText.containsChar('.'))
	{
		createObjectPropertyRows(apiTree, tokenText);
	}
	else
	{
		createVariableRows();
		createApiRows(apiTree);
	}

	listbox->setModel(this);

	listbox->setRowHeight(fontHeight + 4);
	listbox->setColour(ListBox::ColourIds::backgroundColourId, Colours::transparentBlack);


	listbox->getViewport()->setScrollBarThickness(8);
	listbox->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, Colours::black.withAlpha(0.6f));
	listbox->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::trackColourId, Colours::black.withAlpha(0.4f));

	listbox->setWantsKeyboardFocus(false);
	setWantsKeyboardFocus(false);
	infoBox->setWantsKeyboardFocus(false);

	rebuildVisibleItems(tokenText);
}

JavascriptCodeEditor::AutoCompletePopup::~AutoCompletePopup()
{
	infoBox = nullptr;
	listbox = nullptr;

	allInfo.clear();
}

void JavascriptCodeEditor::AutoCompletePopup::createVariableRows()
{
	HiseJavascriptEngine *engine = sp->getScriptEngine();

	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		DebugInformation *info = engine->getDebugInformation(i);
		ScopedPointer<RowInfo> row = new RowInfo();

		row->type = info->getType();
		row->description = info->getDescription();
		row->name = info->getTextForName();
		row->typeName = info->getTextForDataType();
		row->value = info->getTextForValue();
		row->codeToInsert = info->getTextForName();

		allInfo.add(row.release());
	}
}

void JavascriptCodeEditor::AutoCompletePopup::createApiRows(const ValueTree &apiTree)
{
	for (int i = 0; i < apiTree.getNumChildren(); i++)
	{
		ValueTree classTree = apiTree.getChild(i);
		const String className = classTree.getType().toString();

		if (className != "Content" && !sp->getScriptEngine()->isApiClassRegistered(className)) continue;

		RowInfo *row = new RowInfo();
		row->codeToInsert = className;
		row->name = className;
		row->type = (int)RowInfo::Type::ApiMethod;

		allInfo.add(row);

		const ApiClass* apiClass = sp->getScriptEngine()->getApiClass(className);

		addApiMethods(classTree, Identifier(className));

		if (apiClass != nullptr) addApiConstants(apiClass, apiClass->getName());
	}
}

void JavascriptCodeEditor::AutoCompletePopup::createObjectPropertyRows(const ValueTree &apiTree, const String &tokenText)
{
	const Identifier objectId = Identifier(tokenText.upToLastOccurrenceOf(String("."), false, true));

	HiseJavascriptEngine *engine = sp->getScriptEngine();
	
	var v = engine->getScriptObject(objectId);
	const ReferenceCountedObject* o = v.getObject();

	const ApiClass* apiClass = engine->getApiClass(objectId);

	if (o != nullptr)
	{
		if (const DynamicScriptingObject* cso = dynamic_cast<const DynamicScriptingObject*>(o))
		{
			const Identifier csoName = cso->getObjectName();

			ValueTree documentedMethods = apiTree.getChildWithName(csoName);

			for (int i = 0; i < cso->getProperties().size(); i++)
			{
				const var prop = cso->getProperties().getValueAt(i);

				if (prop.isMethod())
				{
					RowInfo *info = new RowInfo();
					const Identifier id = cso->getProperties().getName(i);

					static const Identifier name("name");

					const ValueTree methodTree = documentedMethods.getChildWithProperty(name, id.toString());

					info->description = ApiHelpers::createAttributedStringFromApi(methodTree, objectId.toString(), false, Colours::black);
					info->codeToInsert = ApiHelpers::createCodeToInsert(methodTree, objectId.toString());
					info->name = info->codeToInsert;
					info->type = (int)RowInfo::Type::ApiMethod;
					allInfo.add(info);
				}
			}

			for (int i = 0; i < cso->getProperties().size(); i++)
			{
				const var prop = cso->getProperties().getValueAt(i);

				if (prop.isMethod()) continue;

				const Identifier id = cso->getProperties().getName(i);
				RowInfo *info = new RowInfo();

				info->name = objectId.toString() + "." + cso->getProperties().getName(i).toString();
				info->codeToInsert = info->name;
				info->typeName = DebugInformation::getVarType(prop);
				info->value = prop.toString();

				allInfo.add(info);
			}
		}
		else if (const ConstScriptingObject* cow = dynamic_cast<const ConstScriptingObject*>(o))
		{
			const Identifier cowName = cow->getObjectName();

			ValueTree documentedMethods = apiTree.getChildWithName(cowName);

			Array<Identifier> functionNames;
			Array<Identifier> constantNames;

			cow->getAllFunctionNames(functionNames);
			cow->getAllConstants(constantNames);

			for (int i = 0; i < functionNames.size(); i++)
			{
				RowInfo *info = new RowInfo();
				const Identifier id = functionNames[i];
				static const Identifier name("name");

				const ValueTree methodTree = documentedMethods.getChildWithProperty(name, id.toString());

				info->description = ApiHelpers::createAttributedStringFromApi(methodTree, objectId.toString(), false, Colours::black);
				info->codeToInsert = ApiHelpers::createCodeToInsert(methodTree, objectId.toString());

				if (!info->codeToInsert.contains(id.toString()))
				{


					info->codeToInsert = objectId.toString() + "." + id.toString() + "(";

					int numArgs, functionIndex;
					cow->getIndexAndNumArgsForFunction(id, functionIndex, numArgs);

					for (int j = 0; j < numArgs; j++)
					{
						info->codeToInsert << "arg" + String(j + 1);
						if (j != (numArgs - 1)) info->codeToInsert << ", ";
					}

					info->codeToInsert << ")";
				}

				info->name = info->codeToInsert;
				info->type = (int)RowInfo::Type::ApiMethod;

				jassert(info->name.isNotEmpty());

				allInfo.add(info);
			}

			for (int i = 0; i < constantNames.size(); i++)
			{
				const Identifier id = constantNames[i];
				RowInfo *info = new RowInfo();

				var value = cow->getConstantValue(cow->getConstantIndex(id));

				info->name = objectId.toString() + "." + id.toString();
				info->codeToInsert = info->name;
				info->type = (int)DebugInformation::Type::Constant;
				info->typeName = DebugInformation::getVarType(value);
				info->value = value;


				allInfo.add(info);
			}
		}
		else if (const DynamicObject* obj = dynamic_cast<const DynamicObject*>(o))
		{
			for (int i = 0; i < obj->getProperties().size(); i++)
			{
				const var prop = obj->getProperties().getValueAt(i);

				RowInfo *info = new RowInfo();

				info->name = objectId.toString() + "." + obj->getProperties().getName(i).toString();
				info->codeToInsert = info->name;
				info->typeName = DebugInformation::getVarType(prop);
				info->value = prop.toString();

				allInfo.add(info);
			}
		}
		else if (const HiseJavascriptEngine::RootObject::JavascriptNamespace* ns = dynamic_cast<const HiseJavascriptEngine::RootObject::JavascriptNamespace*>(o))
		{
			for (int i = 0; i < ns->getNumDebugObjects(); i++)
			{
				const ScopedPointer<DebugInformation> dbg = ns->createDebugInformation(i);
				RowInfo *info = new RowInfo();

				info->name = dbg->getTextForName();
				info->codeToInsert = info->name;
				info->typeName = dbg->getTextForDataType();
				info->type = dbg->getType();
				info->value = dbg->getTextForValue();

				allInfo.add(info);
			}
		}
	}
	else if (v.isString())
	{
		ValueTree stringApi = apiTree.getChildWithName("String");
		addApiMethods(stringApi, objectId);
	}
	else if (v.isArray())
	{
		ValueTree arrayApi = apiTree.getChildWithName("Array");
		addApiMethods(arrayApi, objectId);
	}
	else if (apiClass != nullptr)
	{
		ValueTree classTree = apiTree.getChildWithName(apiClass->getName());

		addApiMethods(classTree, objectId);
		addApiConstants(apiClass, objectId);
	}

	addCustomEntries(objectId, apiTree);
}

void JavascriptCodeEditor::AutoCompletePopup::addCustomEntries(const Identifier &objectId, const ValueTree &apiTree)
{
	if (objectId.toString() == "g") // Special treatment for the g variable...
	{
		static const Identifier g("Graphics");

		ValueTree classTree = apiTree.getChildWithName(g);

		addApiMethods(classTree, objectId.toString());
	}
	else if (objectId.toString() == "event")
	{
		StringArray eventProperties = MouseCallbackComponent::getCallbackPropertyNames();

		for (int i = 0; i < eventProperties.size(); i++)
		{
			RowInfo *info = new RowInfo();

			info->name = objectId.toString() + "." + eventProperties[i];
			info->codeToInsert = info->name;
			info->typeName = "int";
			info->value = eventProperties[i];
			info->type = (int)DebugInformation::Type::Variables;

			allInfo.add(info);
		}
	}
}

void JavascriptCodeEditor::AutoCompletePopup::addApiConstants(const ApiClass* apiClass, const Identifier &objectId)
{
	Array<Identifier> constants;

	apiClass->getAllConstants(constants);

	for (int i = 0; i < constants.size(); i++)
	{
		const var prop = apiClass->getConstantValue(i);

		RowInfo *info = new RowInfo();

		info->name = objectId.toString() + "." + constants[i].toString();
		info->codeToInsert = info->name;
		info->typeName = DebugInformation::getVarType(prop);
		info->value = prop.toString();
		info->type = (int)DebugInformation::Type::Constant;

		allInfo.add(info);
	}
}

void JavascriptCodeEditor::AutoCompletePopup::addApiMethods(const ValueTree &classTree, const Identifier &objectId)
{


	for (int j = 0; j < classTree.getNumChildren(); j++)
	{
		ValueTree methodTree = classTree.getChild(j);

		RowInfo *row = new RowInfo();
		row->description = ApiHelpers::createAttributedStringFromApi(methodTree, objectId.toString(), false, Colours::black);
		row->codeToInsert = ApiHelpers::createCodeToInsert(methodTree, objectId.toString());
		row->name = row->codeToInsert;
		row->type = (int)RowInfo::Type::ApiMethod;

		allInfo.add(row);
	}
}

KeyboardFocusTraverser* JavascriptCodeEditor::AutoCompletePopup::createFocusTraverser()
{
	return new AllToTheEditorTraverser(editor);
}

int JavascriptCodeEditor::AutoCompletePopup::getNumRows()
{
	return visibleInfo.size();
}

void JavascriptCodeEditor::AutoCompletePopup::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	RowInfo *info = visibleInfo[rowNumber];

	if (info == nullptr) return;

	Colour c = (rowIsSelected ? Colour(0xff333333) : Colours::transparentBlack);

	g.setColour(c);
	g.fillAll();

	g.setColour(Colours::black.withAlpha(0.1f));
	g.drawHorizontalLine(0, 0.0f, (float)width);

	if (rowIsSelected)
	{
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawHorizontalLine(0, 0.0f, (float)width);
		g.setColour(Colours::black.withAlpha(0.1f));
		g.drawHorizontalLine(height - 1, 0.0f, (float)width);
	}


	char ch;
	Colour colour;

	ApiHelpers::getColourAndCharForType(info->type, ch, colour);

	g.setColour(colour);
	g.fillRoundedRectangle(1.0f, 1.0f, height - 2.0f, height - 2.0f, 4.0f);



	g.setColour(rowIsSelected ? Colours::white : Colours::black.withAlpha(0.7f));
	g.setFont(GLOBAL_MONOSPACE_FONT().withHeight((float)fontHeight));

	const String name = info->name;

	g.drawText(name, height + 2, 1, width - height - 4, height - 2, Justification::centredLeft);
}

void JavascriptCodeEditor::AutoCompletePopup::listBoxItemClicked(int row, const MouseEvent &)
{
	selectRowInfo(row);
}

void JavascriptCodeEditor::AutoCompletePopup::listBoxItemDoubleClicked(int row, const MouseEvent &)
{
	editor->closeAutoCompleteNew(visibleInfo[row]->name);
}

bool JavascriptCodeEditor::AutoCompletePopup::handleEditorKeyPress(const KeyPress& k)
{
	if (k == KeyPress::upKey)
	{
		selectRowInfo(jmax<int>(0, currentlySelectedBox - 1));
		return true;
	}
	else if (k == KeyPress::downKey)
	{
		selectRowInfo(jmin<int>(getNumRows() - 1, currentlySelectedBox + 1));
		return true;
	}
	else if (k == KeyPress::returnKey)
	{
		const bool insertSomething = currentlySelectedBox >= 0;

		if (insertSomething && currentlySelectedBox < visibleInfo.size())
			editor->closeAutoCompleteNew(insertSomething ? visibleInfo[currentlySelectedBox]->name : String());
		else
			editor->closeAutoCompleteNew({});

		return insertSomething;
	}
	else if (k == KeyPress::spaceKey || k == KeyPress::tabKey || k.getTextCharacter() == ';' || k.getTextCharacter() == '(')
	{
		editor->closeAutoCompleteNew({});
		return false;
	}
	else
	{
		String selection = editor->getTextInRange(editor->getCurrentTokenRange());

		rebuildVisibleItems(selection);
		return false;
	}
}

void JavascriptCodeEditor::AutoCompletePopup::paint(Graphics& g)
{
	g.setColour(Colour(0xFFBBBBBB));
	g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 3.0f);
}

void JavascriptCodeEditor::AutoCompletePopup::resized()
{
	infoBox->setBounds(3, 3, getWidth() - 6, 3 * fontHeight - 6);
	listbox->setBounds(3, 3 * fontHeight + 3, getWidth() - 6, getHeight() - 3 * fontHeight - 6);
}

void JavascriptCodeEditor::AutoCompletePopup::selectRowInfo(int rowIndex)
{
	listbox->repaintRow(currentlySelectedBox);

	currentlySelectedBox = rowIndex;

	listbox->selectRow(currentlySelectedBox);
	listbox->repaintRow(currentlySelectedBox);
	infoBox->setInfo(visibleInfo[currentlySelectedBox]);
}

void JavascriptCodeEditor::AutoCompletePopup::rebuildVisibleItems(const String &selection)
{
	visibleInfo.clear();

	int maxNameLength = 0;

	for (int i = 0; i < allInfo.size(); i++)
	{
		if (allInfo[i]->matchesSelection(selection))
		{
			maxNameLength = jmax<int>(maxNameLength, allInfo[i]->name.length());
			visibleInfo.add(allInfo[i]);
		}
	}

	listbox->updateContent();

	const float maxWidth = 450.0f;
	const int height = jmin<int>(200, fontHeight * 3 + (visibleInfo.size()) * (fontHeight + 4));
	setSize((int)maxWidth + 6, height + 6);
}

void JavascriptCodeEditor::AutoCompletePopup::InfoBox::setInfo(RowInfo *newInfo)
{
	currentInfo = newInfo;

	if (newInfo != nullptr)
	{
		if (newInfo->description.getNumAttributes() == 0)
		{
			infoText = AttributedString();

			infoText.append("Type: ", GLOBAL_BOLD_FONT(), Colours::black);
			infoText.append(newInfo->typeName, GLOBAL_MONOSPACE_FONT(), Colours::black);
			infoText.append(" Value: ", GLOBAL_BOLD_FONT(), Colours::black);
			infoText.append(newInfo->value, GLOBAL_MONOSPACE_FONT(), Colours::black);

			infoText.setJustification(Justification::centredLeft);
		}
		else
		{
			infoText = newInfo->description;
		}

		repaint();
	}


}

void JavascriptCodeEditor::AutoCompletePopup::InfoBox::paint(Graphics &g)
{
	g.setColour(Colours::black.withAlpha(0.2f));
	g.fillAll();

	if (currentInfo != nullptr)
	{

		char c;

		Colour colour;

		ApiHelpers::getColourAndCharForType(currentInfo->type, c, colour);

		g.setColour(colour);

		const Rectangle<float> area(5.0f, (float)(getHeight() / 2 - 12), (float)24.0f, (float)24.0f);

		g.fillRoundedRectangle(area, 5.0f);
		g.setColour(Colours::black.withAlpha(0.4f));
		g.drawRoundedRectangle(area, 5.0f, 1.0f);
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white);

		String type;
		type << c;
		g.drawText(type, area, Justification::centred);

		const Rectangle<float> infoRectangle(area.getRight() + 8.0f, 2.0f, (float)getWidth() - area.getRight() - 8.0f, (float)getHeight() - 4.0f);

		infoText.draw(g, infoRectangle);
	}
}

