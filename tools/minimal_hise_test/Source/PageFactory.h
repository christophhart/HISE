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

#pragma once

#include <JuceHeader.h>
#include "MultiPageDialog.h"

namespace hise
{
using namespace juce;

namespace PageFactory
{

struct MarkdownText: public MultiPageDialog::PageBase
{
    SN_NODE_ID("Markdown");

    MarkdownText(MultiPageDialog& r, int width, const var& d);

    void postInit() override;

    void paint(Graphics& g) override;
    Result checkGlobalState(var) override;

private:

    var obj;

    MarkdownRenderer r;
};

struct FileSelector: public MultiPageDialog::PageBase
{
    SN_NODE_ID("FileSelector");

    FileSelector(MultiPageDialog& r, int width, const var& obj);

    void postInit() override;
    Result checkGlobalState(var globalState) override;

    static FilenameComponent* createFileComponent(const var& obj);
    static File getInitialFile(const var& path);

    void resized() override;

private:
    
    bool isDirectory = false;
    ScopedPointer<juce::FilenameComponent> fileSelector;
    Identifier fileId;
};

struct Tickbox: public MultiPageDialog::PageBase
{
    SN_NODE_ID("Toggle");

    Tickbox(MultiPageDialog& r, int width, const var& obj);;

    void postInit() override;
    void paint(Graphics& g) override;
    void resized() override;

    Result checkGlobalState(var globalState) override;

private:

    bool required = false;
    bool requiredOption = false;
    
    String label;
    ToggleButton button;
};

struct TextInput: public MultiPageDialog::PageBase
{
    SN_NODE_ID("Textinput");

    TextInput(MultiPageDialog& r, int width, const var& obj);;

    void postInit() override;
    void paint(Graphics& g) override;
    void resized() override;

    Result checkGlobalState(var globalState) override;

    TextEditor editor;

private:

    bool error = false;

    bool required = false;
    String label;
};

struct Container: public MultiPageDialog::PageBase
{
    Container(MultiPageDialog& r, int width, const var& obj);;
    virtual ~Container() {};

    void postInit() override;
    Result checkGlobalState(var globalState) override;

    virtual void calculateSize() = 0;

    void addChild(MultiPageDialog::PageInfo::Ptr info);

protected:

    OwnedArray<PageBase> childItems;
    MultiPageDialog::PageInfo::List staticPages;

private:

    void addChild(int width, const var& r);
    MultiPageDialog::Factory factory;
};

struct List: public Container
{
    SN_NODE_ID("List");

    void calculateSize() override;

    List(MultiPageDialog& r, int width, const var& obj);
    void resized() override;
    
};

struct Column: public Container
{
	SN_NODE_ID("Column");

    Column(MultiPageDialog& r, int width, const var& obj);

    void calculateSize() override;
	void resized() override;

    void postInit() override
    {
	    for(auto sp: staticPages)
            widthInfo.add((*sp)[MultiPageIds::Width]);

        Container::postInit();
    }

	Array<double> widthInfo;
};


}

}