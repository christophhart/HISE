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
	

Editor::Editor():
	CSSRootComponent(),
	doc(jdoc),
	editor(doc),
	tokenCollection(new mcl::TokenCollection(Identifier("CSS"))),
	body(ElementType::Body),
	header(Selector("#header")),
	content(Selector("#content")),
	footer(Selector("#footer")),
	cancel("Cancel"),
	prev("Previous"),
	next("Next")
{
	KeywordDataBase::printReport();

	FlexboxComponent::Helpers::writeSelectorsToProperties(next, {"#next"});
	FlexboxComponent::Helpers::writeSelectorsToProperties(cancel, {"#cancel"});
	FlexboxComponent::Helpers::writeSelectorsToProperties(prev, {"#prev"});

	body.setDefaultStyleSheet("display: flex; flex-direction: column;");
	header.setDefaultStyleSheet("width: 100%;height: 48px;");
	content.setDefaultStyleSheet("width: 100%;flex-grow: 1;display: flex;");
	footer.setDefaultStyleSheet("width: 100%; height: 48px; display:flex;");


	TopLevelWindowWithKeyMappings::loadKeyPressMap();
	setRepaintsOnMouseActivity(true);
	setSize(1600, 800);

	addAndMakeVisible(editor);
	addAndMakeVisible(list);

	addAndMakeVisible(textInput);

	
	selector.addItemList({ "**header**", "first item", "~~second item~~", "last item", "submenu::item 1", "submenu::item 2"}, 1);
	selector.setUseCustomPopup(true);
	selector.setWantsKeyboardFocus(false);
		
	editor.editor.tokenCollection = tokenCollection;
	tokenCollection->setUseBackgroundThread(false);
	editor.editor.setLanguageManager(new LanguageManager(doc));

	textInput.setRepaintsOnMouseActivity(true);	

	mcl::FullEditor::initKeyPresses(this);
		

	list.setLookAndFeel(&laf);
	laf.setTextEditorColours(list);

	list.setMultiLine(true);
	list.setReadOnly(true);
	list.setFont(GLOBAL_MONOSPACE_FONT());

	context.attachTo(*this);

	addAndMakeVisible(body);
	body.addAndMakeVisible(header);
	body.addAndMakeVisible(content);
	body.addAndMakeVisible(footer);

	footer.addAndMakeVisible(cancel);
	footer.addSpacer();
	footer.addAndMakeVisible(prev);
	footer.addAndMakeVisible(next);
	
	stateWatcher.registerComponentToUpdate(&textInput);

	auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("current.css");
	jdoc.replaceAllContent(f.loadFileAsString());
	compile();
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
	Parser p(jdoc.getAllContent());

	auto ok = p.parse();
	auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("current.css");
	f.replaceWithText(jdoc.getAllContent());
	editor.editor.clearWarningsAndErrors();
	editor.editor.setError(ok.getErrorMessage());

	for(const auto& w: p.getWarnings())
	{
		editor.editor.addWarning(w);
	}

	css = p.getCSSValues();
	css.setAnimator(&animator);



	body.setCSS(css);

	css_laf = new StyleSheetLookAndFeel(css, stateWatcher);

	cancel.setLookAndFeel(css_laf);
	prev.setLookAndFeel(css_laf);
	next.setLookAndFeel(css_laf);
	textInput.setLookAndFeel(css_laf);
	selector.setLookAndFeel(css_laf);

	cancel.setWantsKeyboardFocus(false);
	prev.setWantsKeyboardFocus(false);
	next.setWantsKeyboardFocus(false);

	list.setText(css.toString(), dontSendNotification);

	resized();
	repaint();
}

void Editor::resized()
{
	auto b = getLocalBounds();

	auto bodyArea = b.removeFromRight(b.getWidth() / 3);

	list.setBounds(b.removeFromRight(b.getWidth() / 3));
	editor.setBounds(b);

	body.setBounds(bodyArea);
	body.resized();
	
#if 0
	simple_css::Positioner pos(css, previewArea, false);

	areas[0] = pos.bodyArea;
	areas[1] = pos.removeFromTop(header);
	footer.setBounds(pos.removeFromBottom(footer.selector, 100.0f).toNearestInt());
	footer.resized();
	areas[2] = pos.removeFromTop(content);
		
	list.setBounds(b.removeFromRight(b.getWidth() / 3));

	editor.setBounds(b);
#endif
	

#if 0
	auto footerArea = areas[3];

	if(auto fss = css[footer])
	{
		footerArea = fss->getArea(footerArea, { "margin", 0});
		footerArea = fss->getArea(footerArea, { "padding", 0});

		simple_css::Positioner pos2(css, footerArea, false);

		{
			auto sel = { Selector(ElementType::Button), Selector("#cancel") };
			auto b = pos2.getLocalBoundsFromText(sel, cancel.getButtonText(), {0, 0, 100, 100 });
			cancel.setBounds(pos2.removeFromLeft(sel, b.getWidth()).toNearestInt());
		}

		{
			auto sel = { Selector(ElementType::Button), Selector("#next") };
			auto b = pos2.getLocalBoundsFromText(sel, next.getButtonText(), {0, 0, 100, 100 });
			next.setBounds(pos2.removeFromRight(sel, b.getWidth()).toNearestInt());
		}

		{
			auto sel = { Selector(ElementType::Button), Selector("#prev") };
			auto b = pos2.getLocalBoundsFromText(sel, prev.getButtonText(), {0, 0, 100, 100 });
			prev.setBounds(pos2.removeFromRight(sel, b.getWidth()).toNearestInt());
		}

#if 0
		cancel.setBounds(pos2.removeFromLeft({ Selector(ElementType::Button), Selector("#cancel") }, 100.0f).toNearestInt());
		next.setBounds(pos2.removeFromRight({ Selector(ElementType::Button), Selector("#next") }, 100.0f).toNearestInt());
		prev.setBounds(pos2.removeFromRight({ Selector(ElementType::Button), Selector("#prev") }, 100.0f).toNearestInt());
#endif

		pos2.applyMargin = false;
		textInput.setBounds(pos2.removeFromLeft(Selector(ElementType::TextInput), 180.0f).toNearestInt());

		selector.setBounds(pos2.removeFromLeft(Selector(ElementType::Selector), 180.0f).toNearestInt());
		

		if(auto c = css.getOrCreateCascadedStyleSheet({ Selector(ElementType::Button), Selector(SelectorType::ID, "next")}))
		{
			next.setMouseCursor(c->getMouseCursor());
		}
	}
#endif
		

	repaint();
}

void Editor::paint(Graphics& g)
{
	g.fillAll(Colours::black);

#if 0
	Renderer r(this, stateWatcher);
		
	if(auto ss = css[body])
	{
		auto currentState = Renderer::getPseudoClassFromComponent(this);
		stateWatcher.checkChanges(this, ss, currentState);
		r.drawBackground(g, areas[0], ss);
	}

	if(auto ss = css[content])
		r.drawBackground(g, areas[2], ss);

	if(auto ss = css[header])
		r.drawBackground(g, areas[1], ss);
	
	auto b = getLocalBounds().removeFromRight(10).toFloat();

	if(auto i = animator.items.getFirst())
	{
		g.setColour(Colours::white.withAlpha(0.4f));
		g.fillRect(b);
		b = b.removeFromBottom(i->currentProgress * b.getHeight());
		g.fillRect(b);
	}
#endif
}
}
}


