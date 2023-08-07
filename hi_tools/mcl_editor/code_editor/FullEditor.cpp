/** ============================================================================
 *
 * TextEditor.cpp
 *
 * Copyright (C) Jonathan Zrake
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


namespace mcl
{

using namespace juce;

namespace NavigatorIcons
{
	static const unsigned char goggles[] = { 110,109,125,127,210,66,252,41,205,67,98,72,97,72,66,213,40,203,67,176,114,148,64,221,52,181,67,195,245,168,62,223,207,152,67,98,82,184,206,191,217,254,139,67,207,247,163,64,33,176,127,67,238,124,144,65,100,155,107,67,108,96,229,210,66,190,31,112,66,98,
193,138,226,66,131,192,222,65,117,19,13,67,178,157,55,64,190,223,47,67,213,120,105,62,98,178,29,91,67,248,83,67,192,137,113,128,67,197,32,235,65,129,21,130,67,156,68,145,66,98,66,96,130,67,102,166,160,66,61,26,130,67,158,175,175,66,129,85,129,67,205,
12,190,66,108,49,8,145,67,205,12,190,66,98,117,67,144,67,158,175,175,66,113,253,143,67,102,166,160,66,49,72,144,67,156,68,145,66,98,41,236,145,67,197,32,235,65,217,206,164,67,248,83,67,192,211,109,186,67,213,120,105,62,98,248,211,203,67,178,157,55,64,
2,187,217,67,131,192,222,65,90,164,221,67,190,31,112,66,108,242,170,4,68,100,155,107,67,98,233,230,7,68,33,176,127,67,53,150,9,68,217,254,139,67,186,25,9,68,223,207,152,67,98,127,10,8,68,205,188,180,67,98,16,250,67,184,110,202,67,193,26,223,67,74,12,
205,67,108,193,26,223,67,100,155,190,67,98,106,156,242,67,20,14,188,67,106,44,1,68,172,28,172,67,94,242,1,68,252,185,151,67,98,63,213,2,68,178,93,128,67,182,35,244,67,74,236,87,67,109,199,220,67,197,96,84,67,98,68,107,197,67,129,213,80,67,182,3,177,67,
18,227,115,67,244,61,175,67,178,77,145,67,98,49,120,173,67,252,169,168,67,250,254,190,67,137,17,189,67,68,91,214,67,76,215,190,67,98,168,70,217,67,0,16,191,67,37,38,220,67,133,251,190,67,12,242,222,67,164,160,190,67,108,12,242,222,67,66,16,205,67,98,
207,199,219,67,139,92,205,67,41,140,216,67,162,101,205,67,96,69,213,67,4,38,205,67,98,55,57,188,67,125,63,203,67,66,48,168,67,168,86,185,67,0,112,162,67,168,54,162,67,108,238,28,162,67,57,84,162,67,108,84,227,158,67,119,30,141,67,108,150,179,158,67,119,
30,141,67,98,41,12,153,67,152,254,146,67,106,28,145,67,16,168,150,67,84,83,136,67,199,171,150,67,108,84,83,136,67,172,44,135,67,98,211,237,144,67,96,37,135,67,207,231,151,67,233,38,128,67,207,231,151,67,63,21,111,67,98,207,231,151,67,182,211,93,67,135,
230,144,67,104,209,79,67,195,69,136,67,104,209,79,67,98,186,73,127,67,104,209,79,67,109,71,113,67,182,211,93,67,109,71,113,67,63,21,111,67,98,109,71,113,67,111,34,128,67,139,44,127,67,20,30,135,67,160,42,136,67,139,44,135,67,108,160,42,136,67,166,171,
150,67,98,178,29,128,67,221,164,150,67,223,143,113,67,244,141,147,67,180,136,102,67,236,129,142,67,108,137,129,96,67,57,84,162,67,108,166,219,95,67,168,54,162,67,98,35,91,84,67,168,86,185,67,246,72,44,67,125,63,203,67,72,97,244,66,4,38,205,67,98,102,
38,233,66,139,92,205,67,74,12,222,66,178,93,205,67,78,34,211,66,238,44,205,67,108,78,34,211,66,43,199,190,67,98,10,151,220,66,33,0,191,67,0,64,230,66,201,6,191,67,186,9,240,66,76,215,190,67,98,113,189,38,67,137,17,189,67,2,203,73,67,252,169,168,67,125,
63,70,67,178,77,145,67,98,57,180,66,67,18,227,115,67,221,228,25,67,129,213,80,67,23,89,214,66,197,96,84,67,98,223,207,113,66,74,236,87,67,51,51,203,65,178,93,128,67,92,143,231,65,252,185,151,67,98,164,112,0,66,162,149,172,67,82,248,129,66,193,202,188,
67,125,127,210,66,51,195,190,67,108,125,127,210,66,252,41,205,67,99,101,0,0 };

	static const unsigned char toc[] = { 110,109,203,129,171,67,18,131,233,67,108,215,99,150,66,18,131,233,67,98,102,230,143,66,61,90,233,67,113,125,137,66,96,5,233,67,227,37,132,66,229,32,232,67,98,47,93,130,66,156,212,231,67,123,212,128,66,176,114,231,67,16,88,126,66,166,27,231,67,108,88,
57,52,61,70,86,199,67,108,92,143,130,62,35,59,199,67,108,0,0,0,0,35,59,199,67,108,0,0,0,0,78,98,251,65,108,133,43,174,66,78,98,251,65,108,133,43,174,66,229,208,250,65,98,131,64,174,66,131,192,238,65,195,53,174,66,57,180,235,65,186,137,174,66,45,178,223,
65,98,141,151,177,66,166,155,94,65,2,171,201,66,164,112,221,63,16,24,230,66,127,106,60,62,98,147,24,233,66,49,8,172,60,35,219,233,66,158,239,39,61,59,223,236,66,0,0,0,0,108,211,141,100,67,0,0,0,0,98,184,30,101,67,143,194,117,60,92,175,101,67,182,243,
253,60,66,64,102,67,127,106,60,61,98,72,193,103,67,49,8,44,62,209,34,104,67,86,14,45,62,66,160,105,67,188,116,211,62,98,6,1,119,67,180,200,38,64,80,29,129,67,168,198,99,65,106,220,129,67,45,178,223,65,98,72,241,129,67,57,180,235,65,184,238,129,67,131,
192,238,65,215,243,129,67,229,208,250,65,108,215,243,129,67,207,247,250,65,108,211,141,100,67,207,247,250,65,108,211,141,100,67,229,208,250,65,108,59,223,236,66,229,208,250,65,108,59,223,236,66,14,237,131,67,108,231,187,0,67,14,237,131,67,108,205,44,
47,67,41,252,72,67,108,113,157,93,67,14,237,131,67,108,211,141,100,67,14,237,131,67,108,211,141,100,67,106,188,252,65,108,174,7,130,67,106,188,252,65,108,174,7,130,67,78,98,251,65,108,123,36,142,67,78,98,251,65,108,145,93,142,67,229,208,247,65,108,49,
136,142,67,225,122,250,65,108,49,136,142,67,244,125,155,66,108,37,134,142,67,63,117,155,66,108,37,134,142,67,35,59,199,67,108,111,18,57,66,35,59,199,67,108,113,253,163,66,49,24,217,67,108,106,76,163,67,49,24,217,67,108,106,76,163,67,86,142,238,66,108,
156,164,142,67,27,239,155,66,108,156,164,142,67,137,65,252,65,108,190,79,177,67,238,188,201,66,98,147,88,178,67,174,199,206,66,152,62,179,67,125,63,212,66,217,142,179,67,205,140,218,66,98,154,169,179,67,109,167,220,66,186,169,179,67,236,209,222,66,76,
183,179,67,188,244,224,66,108,76,183,179,67,178,77,225,67,98,248,147,179,67,168,134,226,67,184,110,179,67,248,195,227,67,43,231,178,67,113,221,228,67,98,23,25,178,67,55,137,230,67,109,183,176,67,207,231,231,67,137,17,175,67,242,178,232,67,98,188,148,
173,67,94,106,233,67,244,29,173,67,188,84,233,67,203,129,171,67,18,131,233,67,99,101,0,0 };
}

juce::Path FullEditor::Factory::createPath(const String& url) const
{
	Path p;
	LOAD_PATH_IF_URL("goggles", NavigatorIcons::goggles);
	LOAD_PATH_IF_URL("toc", NavigatorIcons::toc);
	return p;
}

FullEditor::FullEditor(TextDocument& d) :
	editor(d),
	codeMap(d),
	foldMap(d),
	mapButton("goggles", this, factory),
	foldButton("toc", this, factory)
{
	addAndMakeVisible(editor);
	addAndMakeVisible(foldMap);
	addAndMakeVisible(codeMap);

	codeMap.setSize(200, 100);

	auto initButton = [this](HiseShapeButton& b)
	{
		addAndMakeVisible(b);
		b.offColour = Colours::white.withAlpha(0.3f);
		b.setToggleModeWithColourChange(true);
		b.refreshButtonColours();
	};

	initButton(mapButton);
	initButton(foldButton);

	codeMap.transformToUse = editor.transform;
}

void FullEditor::TemplateProvider::addTokens(TokenCollection::List& tokens)
{
	for (int i = 0; i < templateExpressions.size(); i++)
	{

	}
}

void FullEditor::addAutocompleteTemplate(const String& templateExpression, const String& classId)
{

}

void FullEditor::loadSettings(const File& sFile)
{
	settingFile = sFile;

	auto s = JSON::parse(settingFile);

	editor.setLineBreakEnabled(s.getProperty(TextEditorSettings::LineBreaks, true));
	mapWidth = s.getProperty(TextEditorSettings::MapWidth, 150);
		
	mapButton.setToggleStateAndUpdateIcon(s.getProperty(TextEditorSettings::EnableMap, false));

	resized();

	codeMap.allowHover = s.getProperty(TextEditorSettings::EnableHover, true);
	editor.showAutocompleteAfterDelay = s.getProperty(TextEditorSettings::AutoAutocomplete, true);
}

void FullEditor::saveSetting(Component* c, const Identifier& id, const var& newValue)
{
	auto pe = c->findParentComponentOfClass<FullEditor>();
		
	auto s = JSON::parse(pe->settingFile);

	if (s.getDynamicObject() == nullptr)
		s = var(new DynamicObject());

	s.getDynamicObject()->setProperty(id, newValue);
	pe->settingFile.replaceWithText(JSON::toString(s));

	if (id == TextEditorSettings::MapWidth)
	{
		pe->mapWidth = (int)newValue;
		pe->resized();
	}
	if (id == TextEditorSettings::EnableHover)
	{
		pe->codeMap.allowHover = (bool)newValue;
	}
	if (id == TextEditorSettings::AutoAutocomplete)
	{
		pe->editor.showAutocompleteAfterDelay = (bool)newValue;
	}
	if (id == TextEditorSettings::LineBreaks)
	{
		pe->editor.setLineBreakEnabled((bool)newValue);
	}
	if (id == TextEditorSettings::EnableMap)
	{
		pe->mapButton.setToggleStateAndUpdateIcon((bool)newValue);
		pe->resized();
	}
}

void FullEditor::setReadOnly(bool shouldBeReadOnly)
{
	editor.setReadOnly(shouldBeReadOnly);
}

bool FullEditor::injectBreakpointCode(String& s)
{
	return editor.gutter.injectBreakPoints(s);
}

void FullEditor::setColourScheme(const juce::CodeEditorComponent::ColourScheme& s)
{
	editor.colourScheme = s;
}

void FullEditor::setCurrentBreakline(int n)
{
	editor.gutter.setCurrentBreakline(n);
}

void FullEditor::sendBlinkMessage(int n)
{
	editor.gutter.sendBlinkMessage(n);
}

void FullEditor::addBreakpointListener(GutterComponent::BreakpointListener* l)
{
	editor.gutter.addBreakpointListener(l);
}

void FullEditor::removeBreakpointListener(GutterComponent::BreakpointListener* l)
{
	editor.gutter.removeBreakpointListener(l);
}

void FullEditor::enableBreakpoints(bool shouldBeEnabled)
{
	editor.gutter.setBreakpointsEnabled(shouldBeEnabled);
}

void FullEditor::initKeyPresses(Component* root)
{
	String category = "Code Editor";

	TopLevelWindowWithKeyMappings::addShortcut(root, category, TextEditorShortcuts::show_fold_map, "Show Editor Fold Map", 
											   KeyPress('r', ModifierKeys::commandModifier, 0));

	TopLevelWindowWithKeyMappings::addShortcut(root, category, TextEditorShortcuts::show_full_search, "Find all occurrences",
		KeyPress('f', ModifierKeys::shiftModifier | ModifierKeys::commandModifier, 'F'));

	mcl::TextEditor::initKeyPresses(root);
}

bool FullEditor::keyPressed(const KeyPress& k)
{
	if (TopLevelWindowWithKeyMappings::matches(this, k, TextEditorShortcuts::show_fold_map))
	{
		ScopedValueSetter<bool> svs(overlayFoldMap, true);
		foldButton.triggerClick(sendNotificationSync);
		return true;
	}

	return false;
}

void FullEditor::buttonClicked(Button* b)
{
	if (b == &foldButton && !overlayFoldMap)
		mapButton.setToggleStateAndUpdateIcon(false);
	
	if (b == &mapButton)
	{
		FullEditor::saveSetting(&mapButton, TextEditorSettings::EnableMap, b->getToggleState());
		foldButton.setToggleStateAndUpdateIcon(false);
	}
		

	resized();

	if (foldButton.getToggleState())
		foldMap.grabKeyboardFocusAsync();
	else
		editor.grabKeyboardFocusAsync();

	if (foldButton.getToggleState())
		foldMap.rebuild();
}

void FullEditor::resized()
{
	;
	
	

	auto b = getLocalBounds();

	codeMap.setVisible(mapButton.getToggleState());
	foldMap.setVisible(foldButton.getToggleState());

	editor.getVerticalScrollBar().setVisible(!codeMap.isVisible());

	codeMap.preview = nullptr;

	if (codeMap.isVisible())
	{
		auto nb = b.removeFromRight(mapWidth);
		nb.removeFromTop(32);
		codeMap.setBounds(nb);
	}
	if (foldMap.isVisible())
	{
		auto b2 = b;
		auto nb = b.removeFromRight(foldMap.getBestWidth());
		nb.removeFromTop(32);
		
		foldMap.setBounds(nb);

		if (overlayFoldMap)
			b = b2;
	}

	editor.setBounds(b);

	auto top = getLocalBounds().removeFromTop(32);

	top.removeFromRight(18);

	mapButton.setBounds(top.removeFromRight(top.getHeight()).reduced(4));
	foldButton.setBounds(top.removeFromRight(top.getHeight()).reduced(4));
}

void FullEditor::paint(Graphics& g)
{
	g.fillAll(Helpers::getEditorColour(Helpers::EditorBackgroundColour));
}



void MarkdownPreviewSyncer::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    synchroniseTabs(scrollBarThatHasMoved == &e.editor.getVerticalScrollBar());
}

void MarkdownPreviewSyncer::codeDocumentTextInserted(const String& newText, int insertIndex)
{
	startTimer(500);
}

void MarkdownPreviewSyncer::codeDocumentTextDeleted(int startIndex, int endIndex)
{
	startTimer(500);
}

MarkdownPreviewSyncer::MarkdownPreviewSyncer(mcl::FullEditor& editor, MarkdownPreview& preview):
	p(preview),
	e(editor)
{
	e.editor.getTextDocument().getCodeDocument().addListener(this);
}

MarkdownPreviewSyncer::~MarkdownPreviewSyncer()
{
	e.editor.getTextDocument().getCodeDocument().removeListener(this);
}

void MarkdownPreviewSyncer::timerCallback()
{
	{
		MarkdownRenderer::ScopedScrollDisabler sds(p.renderer);
		ScopedValueSetter<bool> svs(recursiveScrollProtector, true);

		if (p.isShowing())
			p.setNewText(e.editor.getTextDocument().getCodeDocument().getAllContent(), {}, false);

		stopTimer();
	}

	synchroniseTabs(true);
}

XmlEditor::XmlEditor(const File& xmlFile, const String& content):
	tdoc(doc),
	editor(tdoc),
	resizer(this, nullptr)
{
	if (!content.isEmpty())
		doc.replaceAllContent(content);
	else
	{
		doc.replaceAllContent(xmlFile.loadFileAsString());
		setName(xmlFile.getFileName());
	}
			
	doc.clearUndoHistory();
	addAndMakeVisible(editor);
	editor.editor.setLanguageManager(new mcl::XmlLanguageManager());
	addAndMakeVisible(resizer);
	setSize(600, 400);
}

void XmlEditor::resized()
{
	auto b = getLocalBounds();
	b.removeFromTop(24);
	editor.setBounds(b);
	resizer.setBounds(b.removeFromBottom(15).removeFromRight(15));
}

void MarkdownPreviewSyncer::setEnableScrollbarListening(bool shouldListenToScrollBars)
{
    if (shouldListenToScrollBars)
    {
        p.viewport.getVerticalScrollBar().addListener(this);
        e.editor.getVerticalScrollBar().addListener(this);
    }
    else
    {
        p.viewport.getVerticalScrollBar().removeListener(this);
        e.editor.getVerticalScrollBar().removeListener(this);
    }
}

void MarkdownPreviewSyncer::synchroniseTabs(bool editorIsSource)
{
    if (recursiveScrollProtector)
        return;
    
    if(!e.isVisible() || !p.isVisible())
        return;

    ScopedValueSetter<bool> svs(recursiveScrollProtector, true);

    if (!editorIsSource)
    {
        auto yPos = (float)p.viewport.getViewPositionY();
        auto lineNumber = p.renderer.getLineNumberForY(yPos);

        e.editor.setFirstLineOnScreen(lineNumber);
    }
    else
    {
        auto currentLine = e.editor.getFirstLineOnScreen();
        auto yPos = p.renderer.getYForLineNumber(currentLine);
        p.viewport.setViewPosition(0, yPos);
    }
}


}
