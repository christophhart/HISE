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

namespace hise {
namespace multipage {
namespace factory {
using namespace juce;


Type::Type(Dialog& r, int width, const var& d):
	PageBase(r, width, d)
{
	auto visible = true;

	if(d.hasProperty(mpid::Visible))
		visible = (bool)d[mpid::Visible];

	setSize(width, visible ? 42 : 0);
	typeId = d[mpid::Type].toString();
}

Result Type::checkGlobalState(var globalState)
{
	if(typeId.isNotEmpty())
	{
		writeState(typeId);
		return Result::ok();
	}
	else
	{
		return Result::fail("Must define Type property");
	}
}

void Type::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	b.removeFromBottom(10);
	g.setColour(Colours::black.withAlpha(0.1f));
	g.fillRect(b);

	auto f = Dialog::getDefaultFont(*this);

	g.setColour(f.second);
	g.setFont(f.first);

	String m;
	m << "Type: " << typeId;

	g.drawText(m, b, Justification::centred);
}

MarkdownText::MarkdownText(Dialog& r, int width, const var& d):
	PageBase(r, width, d),
	r(d[mpid::Text].toString()),
	obj(d)
{
	setSize(width, 0);
}

void MarkdownText::postInit()
{
	auto sd = findParentComponentOfClass<Dialog>()->getStyleData();

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

void MarkdownText::createEditorInfo(Dialog::PageInfo* rootList)
{
    (*rootList)[mpid::Padding] = 10;
    
    rootList->addChild<Type>({
        { mpid::ID, "Type"},
        { mpid::Type, "MarkdownText"}
    });
    rootList->addChild<TextInput>({
        { mpid::ID, "Text" },
        { mpid::Text, "Text" },
        { mpid::Required, true },
        { mpid::Multiline, true },
        { mpid::Value, "Some markdown **text**." }
    });
}


Result MarkdownText::checkGlobalState(var)
{
	return Result::ok();
}

FileSelector::FileSelector(Dialog& r, int width, const var& obj):
	PageBase(r, width, obj),
	fileSelector(createFileComponent(obj)),
	fileId(obj["ID"].toString())
{
	isDirectory = obj[mpid::Directory];
	addAndMakeVisible(fileSelector);
        
	fileSelector->setBrowseButtonText("Browse");
	hise::GlobalHiseLookAndFeel::setDefaultColours(*fileSelector);
        
	setSize(width, 32);
}

FilenameComponent* FileSelector::createFileComponent(const var& obj)
{
	bool isDirectory = obj[mpid::Directory];
	auto name = obj[mpid::Text].toString();
	if(name.isEmpty())
		name = isDirectory ? "Directory" : "File";
	auto wildcard = obj[mpid::Wildcard].toString();
	auto save = (bool)obj[mpid::SaveFile];
        
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



} // factory
} // multipage
} // hise
