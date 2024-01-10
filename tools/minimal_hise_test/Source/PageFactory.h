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

namespace hise
{


namespace multipage {
using namespace juce;

namespace factory
{

struct Type: public Dialog::PageBase
{
	DEFAULT_PROPERTIES(MarkdownText)
    {
        return { { mpid::ID, "Type" } };
    }

    Type(Dialog& r, int width, const var& d);

    Result checkGlobalState(var globalState) override;
	void paint(Graphics& g) override;
	String typeId;
};

struct MarkdownText: public Dialog::PageBase
{
    DEFAULT_PROPERTIES(MarkdownText)
    {
        return { { mpid::Text, "### funkyNode" } };
    }
    
    MarkdownText(Dialog& r, int width, const var& d);

    void createEditorInfo(Dialog::PageInfo* rootList) override;
    
    void postInit() override;
    void paint(Graphics& g) override;
    Result checkGlobalState(var) override;

private:

    var obj;
    MarkdownRenderer r;
};

struct FileSelector: public Dialog::PageBase
{
    DEFAULT_PROPERTIES(FileSelector)
    {
        return {
            { mpid::Directory, true },
            { mpid::ID, "fileId" },
            { mpid::Wildcard, "*.*" },
            { mpid::SaveFile, true }
        };
    }
    
    void createEditorInfo(Dialog::PageInfo* rootList) override
    {
        rootList->addChild<MarkdownText>({
            { { mpid::Text, "oink? "} }
        });
    }
    
    FileSelector(Dialog& r, int width, const var& obj);
    
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








}

}
}