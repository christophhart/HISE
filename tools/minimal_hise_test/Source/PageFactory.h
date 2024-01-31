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

    void resized() override
    {
	    if(helpButton != nullptr)
	    {
            auto b = getLocalBounds();
            b.removeFromBottom(10);
		    helpButton->setBounds(b.removeFromRight(b.getHeight()).reduced(3));
	    }
    }

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

    static String getCategoryId() { return "Layout"; }

    static String getString(const String& markdownText, Dialog& parent)
    {
        if(markdownText.contains("{{"))
        {
            String other;

	        auto it = markdownText.begin();
            auto end = markdownText.end();

            while(it < (end - 1))
            {
                auto c = *it;

                if(c == '{' && *(it+1) == '{')
                {
                    ++it;
                    ++it;
                    String variableId;

	                while(it < (end - 1))
	                {
                        c = *it;
		                if(c == '}' && *(it+1) == '}')
		                {
                            if(variableId.isNotEmpty())
                            {
	                            auto v = parent.getState().globalState[Identifier(variableId)].toString();
                                other << v;
                            }

			                ++it;
                            ++it;
                            break;
		                }
                        else
                        {
	                        variableId << c;
                        }

                        ++it;
	                }
                }
                else
                {
	                other << *it;
                    ++it;
                }
            }

            other << *it;

            return other;
        }

        return markdownText;
    }

    MarkdownText(Dialog& r, int width, const var& d);

    void createEditor(Dialog::PageInfo* rootList);

    void editModeChanged(bool isEditMode) override;

    void postInit() override;
    void paint(Graphics& g) override;
    Result checkGlobalState(var) override;

private:

    int padding = 0;
    var obj;
    MarkdownRenderer r;
};










}

}
}