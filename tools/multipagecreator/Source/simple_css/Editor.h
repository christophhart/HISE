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
namespace simple_css
{
using namespace juce;



struct Editor: public Component,
	           public CSSRootComponent,
		       public TopLevelWindowWithKeyMappings
{
	Editor();

	~Editor()
	{
		TopLevelWindowWithKeyMappings::saveKeyPressMap();
		context.detach();
	}

	File getKeyPressSettingFile() const override { return File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("something.js"); }
	bool keyPressed(const KeyPress& key) override;

	void compile();
	void resized() override;
	void paint(Graphics& g) override;

	mcl::TokenCollection::Ptr tokenCollection;
	hise::GlobalHiseLookAndFeel laf;
	//Rectangle<float> previewArea;
	
	FlexboxComponent body;
	FlexboxComponent header;
	FlexboxComponent content;
	FlexboxComponent footer;

	ScopedPointer<LookAndFeel> css_laf;
	
	TextButton cancel, prev, next;

	SubmenuComboBox selector;
	TextEditor textInput;
	
	juce::CodeDocument jdoc;
	mcl::TextDocument doc;
	mcl::FullEditor editor;
	juce::TextEditor list;
	OpenGLContext context;
};

	
} // namespace simple_css
} // namespace hise
