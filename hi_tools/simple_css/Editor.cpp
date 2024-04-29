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

namespace hise
{
namespace simple_css
{
	

Editor::Editor(Component* target_, const CompileCallback& f_):
	doc(jdoc),
	editor(doc),
	f(f_),
	target(target_),
	tokenCollection(new mcl::TokenCollection(Identifier("CSS")))
{
	TopLevelWindowWithKeyMappings::loadKeyPressMap();
	setRepaintsOnMouseActivity(true);
	setSize(1600, 800);

	setOpaque(true);

	addAndMakeVisible(editor);
	addAndMakeVisible(list);

	editor.editor.tokenCollection = tokenCollection;
	tokenCollection->setUseBackgroundThread(false);
	editor.editor.setLanguageManager(new LanguageManager(doc));

	mcl::FullEditor::initKeyPresses(this);

	list.setLookAndFeel(&laf);
	laf.setTextEditorColours(list);

	list.setMultiLine(true);
	list.setReadOnly(true);
	list.setFont(GLOBAL_MONOSPACE_FONT());

	auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("current.css");
	jdoc.replaceAllContent(f.loadFileAsString());
	compile();
}

void Editor::userTriedToCloseWindow()
{

	MessageManager::callAsync([this]()
	{
		delete this;
	});
}

void Editor::showEditor(Component* t, const CompileCallback& f)
{
	auto e = new Editor(t, f);
	e->setVisible(true);

	int flags = 0;

	flags |= ComponentPeer::StyleFlags::windowAppearsOnTaskbar;
	flags |= ComponentPeer::StyleFlags::windowHasCloseButton;
	flags |= ComponentPeer::StyleFlags::windowHasDropShadow;
	flags |= ComponentPeer::StyleFlags::windowHasMaximiseButton;
	flags |= ComponentPeer::StyleFlags::windowHasTitleBar;
	flags |= ComponentPeer::StyleFlags::windowIsResizable;

	e->setName("Live CSS Editor");
	e->addToDesktop(flags, nullptr);
	e->centreWithSize(900, 600);

}

bool Editor::keyPressed(const KeyPress& key)
{
	if(key == KeyPress::F5Key)
	{
		compile();
		return true;
	}

	return false;
}

void Editor::compile()
{
	if(target.getComponent() == nullptr)
		userTriedToCloseWindow();

	Parser p(jdoc.getAllContent());

	auto ok = p.parse();
	auto tf = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("current.css");
	tf.replaceWithText(jdoc.getAllContent());
	editor.editor.clearWarningsAndErrors();
	editor.editor.setError(ok.getErrorMessage());

	for(const auto& w: p.getWarnings())
	{
		editor.editor.addWarning(w);
	}

	auto css = p.getCSSValues();

	if(f)
		f(css);

	list.setText(css.toString(), dontSendNotification);
	repaint();
}

void Editor::resized()
{
	auto b = getLocalBounds();
	list.setBounds(b.removeFromRight(b.getWidth() / 4));
	editor.setBounds(b);
}

void Editor::paint(Graphics& g)
{
	g.fillAll(Colours::black);
}
}
}


