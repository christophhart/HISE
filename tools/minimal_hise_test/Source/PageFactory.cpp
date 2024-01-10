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

#include "PageFactory.h"

namespace hise
{



namespace PageFactory
{

using namespace juce;

MarkdownText::MarkdownText(MultiPageDialog& r, int width, const var& d):
	PageBase(r, width, d),
	r(d[MultiPageIds::Text].toString()),
	obj(d)
{
	setSize(width, 0);
}

void MarkdownText::postInit()
{
	auto sd = findParentComponentOfClass<MultiPageDialog>()->getStyleData();

	sd.fromDynamicObject(obj, [](const String& name){ return Font(name, 13.0f, Font::plain); });

	r.setStyleData(sd);
	r.parse();

	auto width = getWidth();

	auto h = roundToInt(r.getHeightForWidth(width - 2 * padding));
	setSize(width, h + 2 * padding);
}

void MarkdownText::paint(Graphics& g)
{
	g.fillAll(r.getStyleData().backgroundColour);
	r.draw(g, getLocalBounds().toFloat().reduced(padding));
}

void MarkdownText::createEditorInfo(MultiPageDialog::PageInfo* rootList)
{
    (*rootList)[MultiPageIds::Padding] = 10;
    
    rootList->addChild<MarkdownText>({
        { { MultiPageIds::Text, "**MarkdownText**"} }
    });
    rootList->addChild<TextInput>({
        { MultiPageIds::ID, "Text" },
        { MultiPageIds::Text, "Text" }
    });
}


Result MarkdownText::checkGlobalState(var)
{
	return Result::ok();
}

FileSelector::FileSelector(MultiPageDialog& r, int width, const var& obj):
	PageBase(r, width, obj),
	fileSelector(createFileComponent(obj)),
	fileId(obj["ID"].toString())
{
	isDirectory = obj[MultiPageIds::Directory];
	addAndMakeVisible(fileSelector);
        
	fileSelector->setBrowseButtonText("Browse");
	hise::GlobalHiseLookAndFeel::setDefaultColours(*fileSelector);
        
	setSize(width, 32);
}

FilenameComponent* FileSelector::createFileComponent(const var& obj)
{
	bool isDirectory = obj[MultiPageIds::Directory];
	auto name = obj[MultiPageIds::Text].toString();
	if(name.isEmpty())
		name = isDirectory ? "Directory" : "File";
	auto wildcard = obj[MultiPageIds::Wildcard].toString();
	auto save = (bool)obj[MultiPageIds::SaveFile];
        
	return new FilenameComponent(name, File(), true, isDirectory, save, wildcard, "", "");
}

void FileSelector::postInit()
{
	auto v = getValueFromGlobalState();

	fileSelector->setCurrentFile(getInitialFile(v), false, dontSendNotification);
}

Result FileSelector::checkGlobalState(var globalState)
{
	auto f = fileSelector->getCurrentFile();
        
	if(f != File() && !f.isRoot() && (f.isDirectory() || f.existsAsFile()))
	{
        writeState(f.getFullPathName());

		return Result::ok();
	}
        
	String message;
	message << "You need to select a ";
	if(isDirectory)
		message << "directory";
	else
		message << "file";
        
	return Result::fail(message);
}

File FileSelector::getInitialFile(const var& path)
{
	if(path.isString())
		return File(path);
	if(path.isInt() || path.isInt64())
	{
		auto specialLocation = (File::SpecialLocationType)(int)path;
		return File::getSpecialLocation(specialLocation);
	}
        
	return File();
}

void FileSelector::resized()
{
	fileSelector->setBounds(getLocalBounds());
}

Tickbox::Tickbox(MultiPageDialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, new ToggleButton())
{
	if(obj.hasProperty(MultiPageIds::Required))
	{
		required = true;
		requiredOption = obj[MultiPageIds::Required];
	}
}

void Tickbox::buttonClicked(Button* b)
{
    for(auto tb: groupedButtons)
        tb->setToggleState(b == tb, dontSendNotification);
    
    
    
    //checkGlobalState(MultiPageDialog::getGlobalState(*this, id, var()));
}

void Tickbox::postInit()
{
    auto& button = this->getComponent<ToggleButton>();
    
    Component* root = getParentComponent();
    
    while(dynamic_cast<PageBase*>(root) != nullptr &&
          dynamic_cast<Builder*>(root->getParentComponent()) == nullptr)
    {
        root = root->getParentComponent();
    }
    
    Component::callRecursive<Tickbox>(root, [&](Tickbox* tb)
    {
        if(tb->id == id)
            groupedButtons.add(&tb->getComponent<ToggleButton>());
        
        return false;
    });
    
    if(groupedButtons.size() > 1)
    {
        thisRadioIndex = groupedButtons.indexOf(&button);
        auto radioIndex = (int)getValueFromGlobalState(-1);
        
        int idx = 0;
        
        for(auto b: groupedButtons)
        {
            b->addListener(this);
            b->setToggleState(idx++ == radioIndex, dontSendNotification);
        }
    }
    else
    {
        groupedButtons.clear();
        
        button.setToggleState(getValueFromGlobalState(false), dontSendNotification);
    }
    
	button.setColour(ToggleButton::ColourIds::tickColourId, MultiPageDialog::getDefaultFont(*this).second);
    

}

Result Tickbox::checkGlobalState(var globalState)
{
    auto& button = getComponent<ToggleButton>();
    
	if(required)
	{
        if(thisRadioIndex == -1 && button.getToggleState() != required)
		{
			return Result::fail("You need to tick " + label);
		}
        else
        {
            auto somethingPressed = false;
            
            for(auto tb: groupedButtons)
                somethingPressed |= tb->getToggleState();
            
            if(!somethingPressed)
                return Result::fail("You need to select one option of " + id.toString());
        }
	}

    if(thisRadioIndex == -1)
        writeState(button.getToggleState());
    else if(button.getToggleState())
        writeState(thisRadioIndex);
    
	return Result::ok();
}

TextInput::TextInput(MultiPageDialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, new TextEditor())
{
    auto& editor = getComponent<TextEditor>();
	GlobalHiseLookAndFeel::setTextEditorColours(editor);

    setWantsKeyboardFocus(true);

    editor.setSelectAllWhenFocused(false);
    editor.setIgnoreUpDownKeysWhenSingleLine(true);
    editor.setTabKeyUsedAsCharacter(false);
    
    editor.addListener(this);
	required = obj[MultiPageIds::Required];
}

void TextInput::postInit()
{
    auto& editor = getComponent<TextEditor>();
    editor.setFont(MultiPageDialog::getDefaultFont(*this).first);
    editor.setIndents(4, 8);
	editor.setText(getValueFromGlobalState(""));
    
    if(auto m = findParentComponentOfClass<MultiPageDialog>())
    {
        auto c = m->getStyleData().headlineColour;
        editor.setColour(TextEditor::ColourIds::focusedOutlineColourId, c);
        editor.setColour(Label::ColourIds::outlineWhenEditingColourId, c);
        editor.setColour(TextEditor::ColourIds::highlightColourId, c);
    }
}

Result TextInput::checkGlobalState(var globalState)
{
    auto& editor = getComponent<TextEditor>();
    
	if(required && editor.getText().isEmpty())
	{
        return Result::fail(id + " must not be empty");
	}
		
    writeState(editor.getText());
    
	return Result::ok();
}

Container::Container(MultiPageDialog& r, int width, const var& obj):
	PageBase(r, width, obj)
{
	auto l = obj["Children"];
        
	if(l.isArray())
	{
		for(auto& r: *l.getArray())
			addChild(width, r);
	}
}

void Container::postInit()
{
	for(const auto& sp: staticPages)
	{
		childItems.add(sp->create(rootDialog, getWidth()));
		addAndMakeVisible(childItems.getLast());
	}

	for(auto c: childItems)
    {
        c->setStateObject(stateObject);
        c->postInit();
    }

	calculateSize();
}

Result Container::checkGlobalState(var globalState)
{
	for(auto c: childItems)
	{
		auto ok = c->check(globalState);
            
		if(!ok.wasOk())
			return ok;
	}
        
	return Result::ok();
}

void Container::addChild(MultiPageDialog::PageInfo::Ptr info)
{
	staticPages.add(info);
}

void Container::addChild(int width, const var& r)
{
	if(auto pi = factory.create(r))
	{
		childItems.add(pi->create(rootDialog, width));
		addAndMakeVisible(childItems.getLast());
	}
}

void List::calculateSize()
{
	int h = foldable ? (titleHeight + padding) : 0;

    for(auto c: childItems)
        c->setVisible(!folded);
    
    if(!folded)
    {
        for(auto& c: childItems)
            h += c->getHeight() + padding;
    }
    
    

	setSize(getWidth(), h);
}

List::List(MultiPageDialog& r, int width, const var& obj):
	Container(r, width, obj)
{
    foldable = obj[MultiPageIds::Foldable];
    folded = obj[MultiPageIds::Folded];
    title = obj[MultiPageIds::Text];
	setSize(width, 0);
}

void List::createEditorInfo(MultiPageDialog::PageInfo* info)
{
    auto& xxx = *info;

    auto& tt = xxx.addChild<MarkdownText>({
        { MultiPageIds::Text, "List" }
    });
    
    auto& prop = xxx.addChild<List>();
    
    xxx[MultiPageIds::Text] = "List";
    
    prop[MultiPageIds::Folded] = false;
    prop[MultiPageIds::Foldable] = true;
    prop[MultiPageIds::Padding] = 10;
    prop[MultiPageIds::Text] = "Properties";
    
    auto& listId = prop.addChild<TextInput>({
        { MultiPageIds::ID, "ID" },
        { MultiPageIds::Text, "ID" },
        { MultiPageIds::Help, "The ID of the list. This will be used for identification in some logic cases" }
    });
    
    auto& textId = prop.addChild<TextInput>({
        { MultiPageIds::ID, "Text" },
        { MultiPageIds::Text, "Text" },
        { MultiPageIds::Help, "The title text that is shown at the header bar." }
    });

    auto& padId = prop.addChild<TextInput>({
        { MultiPageIds::ID, "Padding" },
        { MultiPageIds::Text, "Padding" },
        { MultiPageIds::Help, "The spacing between child elements in pixel." }
    });

    auto& foldId1 = prop.addChild<Tickbox>({
        { MultiPageIds::ID, "Foldable" },
        { MultiPageIds::Text, "Foldable" },
        { MultiPageIds::Help, "If ticked, then this list will show a clickable header that can be folded" }
    });
    
    auto& foldId2 = prop.addChild<Tickbox>({
        { MultiPageIds::ID, "Folded" },
        { MultiPageIds::Text, "Folded" },
        { MultiPageIds::Help, "If ticked, then this list will folded as default state" }
    });
    
    auto& ff = xxx.addChild<Builder>({
        { MultiPageIds::Text, "Children" }
    });
    
    auto& typeOptions = ff.addChild<Branch>();
    
    MultiPageDialog::Factory f;
    
    bool addSelf = false;
    
    for(auto type: f.getIdList())
    {
        if(type == "List")
        {
            addSelf = true;
        }
        else
        {
            auto& itemBuilder =  typeOptions.addChild<List>();
            itemBuilder[MultiPageIds::Text] = type;

            auto obj = new DynamicObject();
            obj->setProperty(MultiPageIds::ID, type + "Id");
            obj->setProperty(MultiPageIds::Type, type);
            
            MultiPageDialog::PageInfo::Ptr c = f.create(obj);
            ScopedPointer<MultiPageDialog::PageBase> c2 = c->create(rootDialog, 0);
            c2->createEditorInfo(&itemBuilder);
        }
    }
    
    if(addSelf)
        typeOptions.childItems.add(info->clone());
}

void List::resized()
{
	auto b = getLocalBounds();

	if(b.isEmpty())
		return;

    if(foldable)
        b.removeFromTop(24 + padding);
    
    if(!folded)
    {
        for(auto c: childItems)
        {
            c->setBounds(b.removeFromTop(c->getHeight()));
            b.removeFromTop(padding);
        }
    }
}

Column::Column(MultiPageDialog& r, int width, const var& obj):
	Container(r, width, obj)
{
	padding = (int)obj[MultiPageIds::Padding];

	
    auto widthList = obj[MultiPageIds::Width];
    
    if(childItems.size() > 0)
    {
        auto equidistance = -1.0 / childItems.size();
        
        for(int i = 0; i < childItems.size(); i++)
        {
            auto v = widthList[i];
            
            if(v.isUndefined() || v.isVoid())
                widthInfo.add(equidistance);
            else
                widthInfo.add((double)v);
        }
    }

	setSize(width, 0);
}

void Column::calculateSize()
{
	int h = 0;

	for(auto& c: childItems)
		h = jmax(h, c->getHeight());
	        
	setSize(getWidth(), h);
}

void Column::resized()
{
	auto b = getLocalBounds();

	if(b.isEmpty())
		return;

	auto fullWidth = getWidth();
        
	for(const auto& w: widthInfo)
	{
		if(w > 0.0)
			fullWidth -= (int)w;

		fullWidth -= padding;
	}
	
	for(int i = 0; i < childItems.size(); i++)
	{
		auto w = widthInfo[i];

		if(w < 0.0)
			w = fullWidth * (-1.0) * w;

		childItems[i]->setBounds(b.removeFromLeft(roundToInt(w)));
		b.removeFromLeft(padding);
	}
}
}


}

