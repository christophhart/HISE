/** ============================================================================
 *
 * MCL Text Editor JUCE module 
 *
 * Copyright (C) Jonathan Zrake, Christoph Hart
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */

namespace mcl
{
using namespace juce;


//==========================================================================
mcl::HighlightComponent::HighlightComponent(TextDocument& document) : document(document)
{
	document.addFoldListener(this);
	setInterceptsMouseClicks(false, false);
}

void mcl::HighlightComponent::setViewTransform(const AffineTransform& transformToUse)
{
	transform = transformToUse;

	outlinePath.clear();
	
	for (const auto& s : document.getSelections())
		outlinePath.addPath(getOutlinePath(document, s));
	
    repaint(outlinePath.getBounds().getSmallestIntegerContainer());
}

void mcl::HighlightComponent::updateSelections()
{
	outlinePath.clear();
	
	for (const auto& s : document.getSelections())
		outlinePath.addPath(getOutlinePath(document, s.oriented()));
	
    repaint(outlinePath.getBounds().getSmallestIntegerContainer());
}

void mcl::HighlightComponent::paintHighlight(Graphics& g)
{
	//g.addTransform(transform);
	auto highlight = getParentComponent()->findColour(CodeEditorComponent::highlightColourId);
	g.setColour(highlight);

	DropShadow sh;
	sh.colour = Colours::black.withAlpha(0.4f);
	sh.radius = 8;
	sh.offset = { 0, 3 };
	//sh.drawForPath(g, outlinePath);

	auto c = highlight.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.6f));

	auto b = outlinePath.getBounds();

	g.setGradientFill(ColourGradient(c, 0.0f, b.getY(), c.darker(0.05f), 0.0f, b.getBottom(), false));
	g.fillPath(outlinePath);

	g.setColour(Colour(0xff959595).withAlpha(JUCE_LIVE_CONSTANT_OFF(0.2f)));
	g.strokePath(outlinePath, PathStrokeType(1.0f / transform.getScaleFactor()));

	auto ar = document.getSearchResults();

	for (int i = 0; i < ar.size(); i++)
	{
		auto sr = ar[i];
		auto r = document.getSelectionRegion(sr);

		for (auto h : r)
		{
			h.removeFromBottom(h.getHeight() * 0.15f);

			h = h.translated(0.0f, h.getHeight() * 0.05f).expanded(2.0f);

			g.setColour(Colours::white.withAlpha(0.2f));
			g.fillRoundedRectangle(h, 2.0f);
			g.setColour(Colours::red.withAlpha(0.4f));
			g.drawRoundedRectangle(h, 2.0f, 1.0f);
		}
	}
}

Path mcl::HighlightComponent::getOutlinePath(const TextDocument& doc, const Selection& s)
{
	if (s.isSingular())
		return {};

	RectangleList<float> list;
	auto top = doc.getUnderlines(s, TextDocument::Metric::top);
	auto bottom = doc.getUnderlines(s, TextDocument::Metric::baseline);
	Path p;

	if (top.isEmpty())
		return p;

	float currentPos = 0.0f;

	auto pushed = [&currentPos](Point<float> p, bool down)
	{
		if (down)
			p.y = jmax(currentPos, p.y);
		else
			p.y = jmin(currentPos, p.y);

		currentPos = p.y;
		return p;
	};

	float deltaY = -1.0f;

	p.startNewSubPath(pushed(top.getFirst().getEnd().translated(0.0f, deltaY), true));
	p.lineTo(pushed(bottom.getFirst().getEnd().translated(0.0f, deltaY), true));

	for (int i = 1; i < top.size(); i++)
	{
		p.lineTo(pushed(top[i].getEnd().translated(0.0f, deltaY), true));
		auto b = pushed(bottom[i].getEnd().translated(0.0f, deltaY), true);
		p.lineTo(b);
	}

	for (int i = top.size() - 1; i >= 0; i--)
	{
		p.lineTo(pushed(bottom[i].getStart().translated(0.0f, deltaY), false));
		p.lineTo(pushed(top[i].getStart().translated(0.0f, deltaY), false));
	}

	p.closeSubPath();
	return p.createPathWithRoundedCorners(2.0f);
}

mcl::HighlightComponent::~HighlightComponent()
{
	document.removeFoldListener(this);
}

SearchBoxComponent::SearchBoxComponent(TextDocument& d, float scaleFactor):
	doc(d),
	find("next", this, *this),
	prev("prev", this, *this),
	findAll("selectAll", nullptr, *this),
    close("close", nullptr, *this),
    regexButton("regex", nullptr, *this),
    wholeButton("whole", nullptr, *this),
    caseButton("case", nullptr, *this)
{
	searchField.setFont(d.getFont().withHeight(d.getFontHeight() * scaleFactor));

	searchField.setCaretVisible(true);
	searchField.setColour(juce::CaretComponent::ColourIds::caretColourId, Colours::black);

	addAndMakeVisible(searchField);

	searchField.addKeyListener(this);
	searchField.addListener(this);

    
	
	find.addListener(this);
	prev.addListener(this);

	findAll.onClick = [this]()
	{
		doc.setSelections(doc.getSearchResults(), true);
		doc.setSearchResults({});
		sendSearchChangeMessage();
	};
    
    close.onClick = [this]()
    {
        auto parent = getParentComponent();
        MessageManager::callAsync([parent]()
        {
            parent->keyPressed(KeyPress(KeyPress::escapeKey));
        });
    };

    auto refresh = [this]()
    {
        setSearchInput(searchField.getText());
    };
    
    caseButton.onClick = refresh;
    wholeButton.onClick = refresh;
    regexButton.onClick = refresh;
    
    addAndMakeVisible(caseButton);
    addAndMakeVisible(wholeButton);
    addAndMakeVisible(regexButton);
    
    caseButton.setToggleModeWithColourChange(true);
    wholeButton.setToggleModeWithColourChange(true);
    regexButton.setToggleModeWithColourChange(true);
    
    
	addAndMakeVisible(find);
	addAndMakeVisible(prev);
	addAndMakeVisible(findAll);
    addAndMakeVisible(close);
    
    caseButton.setTooltip("Case sensitive search");
    regexButton.setTooltip("Enable regex pattern matching");
    wholeButton.setTooltip("Search for whole word");
    
    caseButton.setToggleStateAndUpdateIcon(true);
    
    find.setTooltip("Goto next match (Return)");
    prev.setTooltip("Goto previous match");
    findAll.setTooltip("Select all occurrences");
    close.setTooltip("Close search (Escape)");
}

SearchBoxComponent::~SearchBoxComponent()
{
	doc.setSearchResults({});

	sendSearchChangeMessage();
}

void SearchBoxComponent::buttonClicked(Button* b)
{
	auto currentPos = doc.getSelection(0);
	auto sr = doc.getSearchResults();

	auto toUse = sr[0];

	if (b == &prev)
	{
		toUse = sr.getLast();

		for (int i = sr.size()-1; i >= 0; i--)
		{
			if (sr[i] < currentPos)
			{
				toUse = sr[i];
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < sr.size(); i++)
		{
			if (currentPos < sr[i])
			{
				toUse = sr[i];
				break;
			}
		}
	}

		


	doc.setSelections({ toUse.oriented() }, true);
	sendSearchChangeMessage();
}

void SearchBoxComponent::sendSearchChangeMessage()
{
	for (auto l : listeners)
	{
		if (l.get() != nullptr)
			l->searchItemsChanged();
	}
}

SearchBoxComponent::Listener::~Listener()
{}

void SearchBoxComponent::Listener::searchItemsChanged()
{}

void SearchBoxComponent::textEditorTextChanged(juce::TextEditor& t)
{
	auto input = searchField.getText();
	setSearchInput(input);
}

void SearchBoxComponent::setSearchInput(const String& text)
{
    Array<Selection> searchResults;
    
    if(regexButton.getToggleState())
    {
        auto matches = RegexFunctions::findRangesThatMatchWildcard(text, doc.getCodeDocument().getAllContent());
        
        for(const auto& m: matches)
        {
            CodeDocument::Position p(doc.getCodeDocument(), m.getStart());
            CodeDocument::Position e(doc.getCodeDocument(), m.getEnd());
            
            Point<int> ps(p.getLineNumber(), p.getIndexInLine());
            Point<int> pe(e.getLineNumber(), e.getIndexInLine());

            searchResults.add({ ps, pe });
        }
    }
    else
    {
        CodeDocument::Position p(doc.getCodeDocument(), 0);

        auto firstChar = text[0];
        int maxIndex = text.length();
        
        auto equals = [&](const String& text, const String& searchTerm)
        {
            if(caseButton.getToggleState())
                return text == searchTerm;
            else
                return text.toLowerCase() == searchTerm.toLowerCase();
        };
        
        while (p.getPosition() < doc.getCodeDocument().getNumCharacters())
        {
            if (p.getCharacter() == firstChar)
            {
                auto e = p.movedBy(maxIndex);

                auto pToUse = p;
                auto eToUse = e;
                
                if(wholeButton.getToggleState())
                {
                    while(CharacterFunctions::isLetterOrDigit(p.movedBy(-1).getCharacter()))
                    {
                        if(pToUse.getPosition() <= 0)
                            break;
                        
                        pToUse = pToUse.movedBy(-1);
                    }
                    
                    while(CharacterFunctions::isLetterOrDigit(eToUse.getCharacter()))
                    {
                        if(eToUse.getPosition() > doc.getCodeDocument().getNumCharacters())
                            break;
                        
                        eToUse = eToUse.movedBy(1);
                    }
                }
                
                auto t = doc.getCodeDocument().getTextBetween(pToUse, eToUse);

                if (equals(t, text))
                {
                    Point<int> ps(p.getLineNumber(), p.getIndexInLine());
                    Point<int> pe(e.getLineNumber(), e.getIndexInLine());

                    searchResults.add({ ps, pe });
                }
            }

            p.moveBy(1);
        }
    }
    
	doc.setSearchResults(searchResults);
	sendSearchChangeMessage();
}

bool SearchBoxComponent::keyPressed(const KeyPress& k, Component* c)
{
	if (k == KeyPress::returnKey)
	{
		find.triggerClick();
		return true;
	}
	if (k == KeyPress::escapeKey)
	{
		auto parent = getParentComponent();
		MessageManager::callAsync([parent, k]()
		{
			parent->keyPressed(k);
		});

		return true;
	}

	return false;
}

void SearchBoxComponent::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void SearchBoxComponent::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void SearchBoxComponent::paint(Graphics& g)
{
	auto b = getLocalBounds();
		
	DropShadow sh;
	sh.colour = Colours::black.withAlpha(0.8f);
	sh.radius = 5;
	sh.drawForRectangle(g, b.toNearestInt());

	g.setColour(Colour(0xFF444444));
	g.fillRect(b);

	String s;
		
	s << String(doc.getSearchResults().size()) << " matches";
		
	g.setColour(Colours::white.withAlpha(0.8f));
	g.setFont(getResultFont());
	g.drawText(s, b.reduced(8.0f, 0.0f), Justification::centredLeft);

}

Font SearchBoxComponent::getResultFont() const
{
    auto f = searchField.getFont();
    
	return f.withHeight(f.getHeight() * 0.8f).boldened();
}

void SearchBoxComponent::resized()
{
	auto b = getLocalBounds();
	b.removeFromTop(2);
    b.removeFromBottom(2);
	auto okBox = b.removeFromRight(32 * 4 + 10);



	b.removeFromLeft(getResultFont().getStringWidth("1230 matches"));

    caseButton.setBounds(b.removeFromLeft(32).reduced(6));
    regexButton.setBounds(b.removeFromLeft(32).reduced(6));
    wholeButton.setBounds(b.removeFromLeft(32).reduced(6));
    
	searchField.setBounds(b);

    prev.setBounds(okBox.removeFromLeft(32).reduced(6));
	find.setBounds(okBox.removeFromLeft(32).reduced(6));
	findAll.setBounds(okBox.removeFromLeft(32).reduced(6));
    okBox.removeFromLeft(10);
    close.setBounds(okBox.removeFromLeft(32).reduced(6));
}



namespace SearchIcons
{
static const unsigned char selectAllIcon[] = { 110,109,52,118,57,68,236,39,112,67,108,116,222,48,68,236,39,112,67,108,116,222,48,68,68,255,63,67,108,24,61,59,68,68,255,63,67,98,52,231,64,68,68,255,63,67,0,128,69,68,240,99,82,67,0,128,69,68,224,10,105,67,108,0,128,69,68,188,66,137,67,108,52,118,57,
68,188,66,137,67,108,52,118,57,68,236,39,112,67,99,109,204,137,19,68,236,39,112,67,108,204,137,19,68,188,66,137,67,108,0,128,7,68,188,66,137,67,108,0,128,7,68,224,10,105,67,98,0,128,7,68,240,99,82,67,204,24,12,68,68,255,63,67,232,194,17,68,68,255,63,
67,108,140,33,28,68,68,255,63,67,108,140,33,28,68,236,39,112,67,108,204,137,19,68,236,39,112,67,99,109,204,137,19,68,204,236,195,67,108,140,33,28,68,204,236,195,67,108,140,33,28,68,96,0,220,67,108,232,194,17,68,96,0,220,67,98,204,24,12,68,96,0,220,67,
0,128,7,68,200,206,210,67,0,128,7,68,148,122,199,67,108,0,128,7,68,76,189,178,67,108,204,137,19,68,76,189,178,67,108,204,137,19,68,204,236,195,67,99,109,52,118,57,68,204,236,195,67,108,52,118,57,68,76,189,178,67,108,0,128,69,68,76,189,178,67,108,0,128,
69,68,148,122,199,67,98,0,128,69,68,200,206,210,67,52,231,64,68,96,0,220,67,24,61,59,68,96,0,220,67,108,116,222,48,68,96,0,220,67,108,116,222,48,68,204,236,195,67,108,52,118,57,68,204,236,195,67,99,101,0,0 };

    static const unsigned char closeIcon[] = { 110,109,4,128,38,68,249,207,139,67,108,0,104,60,68,0,0,64,67,108,4,128,69,68,14,96,100,67,108,8,152,47,68,0,0,158,67,108,4,128,69,68,248,207,201,67,108,0,104,60,68,0,0,220,67,108,4,128,38,68,52,47,176,67,108,8,152,16,68,0,0,220,67,108,4,128,7,68,248,
207,201,67,108,104,104,29,68,0,0,158,67,108,4,128,7,68,14,96,100,67,108,8,152,16,68,0,0,64,67,108,4,128,38,68,249,207,139,67,99,101,0,0 };

    static const unsigned char  nextIcon[] = { 110,109,0,128,7,68,156,233,175,67,108,0,128,7,68,88,21,140,67,108,228,47,39,68,88,21,140,67,108,228,47,39,68,128,191,66,67,108,0,128,69,68,0,0,158,67,108,228,47,39,68,64,160,218,67,108,228,47,39,68,156,233,175,67,108,0,128,7,68,156,233,175,67,99,101,
        0,0 };

    static const unsigned char prevIcon[] = { 110,109,0,128,69,68,120,233,175,67,108,224,207,37,68,120,233,175,67,108,224,207,37,68,192,159,218,67,108,0,128,7,68,0,0,158,67,108,224,207,37,68,128,192,66,67,108,224,207,37,68,128,21,140,67,108,0,128,69,68,128,21,140,67,108,0,128,69,68,120,233,175,67,
99,101,0,0 };

static const unsigned char caseIcon[] = { 110,109,29,205,44,68,84,119,194,67,108,200,43,38,68,84,119,194,67,98,214,109,37,68,84,119,194,67,82,209,36,68,180,30,194,67,58,86,36,68,116,109,193,67,98,36,219,35,68,186,188,192,67,160,133,35,68,186,217,191,67,52,86,35,68,124,197,190,67,108,16,41,33,
68,122,234,177,67,108,12,36,19,68,122,234,177,67,108,232,246,16,68,124,197,190,67,98,162,203,16,68,132,183,191,67,106,119,16,68,50,145,192,67,65,250,15,68,140,83,193,67,98,23,125,15,68,230,21,194,67,175,227,14,68,84,119,194,67,74,46,14,68,84,119,194,
67,108,0,128,7,68,84,119,194,67,108,204,197,21,68,188,114,113,67,108,80,135,30,68,188,114,113,67,108,29,205,44,68,84,119,194,67,99,109,92,29,21,68,110,70,166,67,108,192,47,31,68,110,70,166,67,108,167,203,27,68,176,20,146,67,98,107,147,27,68,160,204,144,
67,154,81,27,68,0,74,143,67,49,6,27,68,204,140,141,67,98,134,186,26,68,34,208,139,67,228,111,26,68,168,238,137,67,142,38,26,68,100,232,135,67,98,94,225,25,68,120,247,137,67,28,154,25,68,40,223,139,67,197,80,25,68,128,160,141,67,98,112,7,25,68,214,97,
143,67,64,194,24,68,138,230,144,67,118,129,24,68,156,46,146,67,108,92,29,21,68,110,70,166,67,99,109,18,232,65,68,84,119,194,67,98,32,42,65,68,84,119,194,67,145,154,64,68,128,67,194,67,100,57,64,68,218,219,193,67,98,55,216,63,68,48,116,193,67,27,135,63,
68,20,156,192,67,82,70,63,68,4,84,191,67,108,204,183,62,68,122,113,188,67,98,92,15,62,68,136,142,189,67,103,109,61,68,216,136,190,67,236,209,60,68,248,96,191,67,98,112,54,60,68,20,57,192,67,132,149,59,68,142,240,192,67,106,239,58,68,94,135,193,67,98,
12,73,58,68,180,30,194,67,15,152,57,68,42,143,194,67,48,220,56,68,64,216,194,67,98,80,32,56,68,216,33,195,67,182,79,55,68,164,70,195,67,228,106,54,68,164,70,195,67,98,86,56,53,68,164,70,195,67,196,33,52,68,210,246,194,67,47,39,51,68,174,86,194,67,98,
154,44,50,68,14,183,193,67,28,88,49,68,194,203,192,67,50,169,48,68,204,148,191,67,98,71,250,47,68,214,93,190,67,69,115,47,68,52,219,188,67,42,20,47,68,232,12,187,67,98,16,181,46,68,32,63,185,67,164,133,46,68,212,41,183,67,164,133,46,68,2,205,180,67,98,
164,133,46,68,194,241,178,67,5,194,46,68,16,10,177,67,10,59,47,68,230,20,175,67,98,14,180,47,68,188,31,173,67,111,133,48,68,44,88,171,67,112,175,49,68,180,189,169,67,98,114,217,50,68,192,35,168,67,26,107,52,68,36,204,166,67,106,100,54,68,230,183,165,
67,98,186,93,56,68,170,163,164,67,174,218,58,68,48,8,164,67,136,219,61,68,118,229,163,67,108,136,219,61,68,92,171,161,67,98,136,219,61,68,38,153,158,67,175,138,61,68,34,97,156,67,186,232,60,68,76,3,155,67,98,196,70,60,68,118,165,153,67,168,96,59,68,206,
246,152,67,166,54,58,68,206,246,152,67,98,6,73,57,68,206,246,152,67,172,134,56,68,162,42,153,67,152,239,55,68,74,146,153,67,98,133,88,55,68,240,249,153,67,131,209,54,68,142,110,154,67,212,90,54,68,30,240,154,67,98,226,227,53,68,176,113,155,67,80,112,
53,68,76,230,155,67,26,0,53,68,244,77,156,67,98,230,143,52,68,154,181,156,67,238,9,52,68,112,233,156,67,114,110,51,68,112,233,156,67,98,84,228,50,68,112,233,156,67,174,110,50,68,146,166,156,67,130,13,50,68,88,32,156,67,98,84,172,49,68,160,154,155,67,
160,95,49,68,68,244,154,67,100,39,49,68,196,45,154,67,108,154,188,47,68,246,68,149,67,98,171,82,49,68,50,120,146,67,201,20,51,68,98,98,144,67,121,3,53,68,140,4,143,67,98,231,241,54,68,184,166,141,67,24,4,57,68,16,248,140,67,200,57,59,68,16,248,140,67,
98,217,207,60,68,16,248,140,67,191,61,62,68,180,123,141,67,255,131,63,68,252,130,142,67,98,253,201,64,68,200,138,143,67,124,222,65,68,182,249,144,67,58,193,66,68,78,208,146,67,98,248,163,67,68,104,167,148,67,151,81,68,68,160,214,150,67,155,202,68,68,
118,94,153,67,98,159,67,69,68,76,230,155,67,0,128,69,68,196,170,158,67,0,128,69,68,92,171,161,67,108,0,128,69,68,84,119,194,67,108,18,232,65,68,84,119,194,67,99,109,198,229,56,68,130,230,184,67,98,232,232,57,68,130,230,184,67,167,203,58,68,206,139,184,
67,0,142,59,68,106,214,183,67,98,90,80,60,68,6,33,183,67,199,20,61,68,42,251,181,67,136,219,61,68,90,101,180,67,108,136,219,61,68,150,27,173,67,98,224,73,60,68,78,62,173,67,122,255,58,68,62,131,173,67,88,252,57,68,228,234,173,67,98,54,249,56,68,140,82,
174,67,250,43,56,68,30,212,174,67,230,148,55,68,152,111,175,67,98,210,253,54,68,20,11,176,67,34,149,54,68,82,188,176,67,212,90,54,68,210,130,177,67,98,134,32,54,68,82,73,178,67,61,3,54,68,112,33,179,67,61,3,54,68,168,10,180,67,98,61,3,54,68,76,212,181,
67,244,65,54,68,36,22,183,67,29,191,54,68,172,207,183,67,98,70,60,55,68,188,137,184,67,0,244,55,68,130,230,184,67,198,229,56,68,130,230,184,67,99,101,0,0 };

static const unsigned char regexIcon[] = { 110,109,0,128,7,68,198,76,192,67,98,0,128,7,68,36,201,189,67,156,188,7,68,144,114,187,67,74,53,8,68,246,71,185,67,98,248,173,8,68,92,29,183,67,44,83,9,68,164,58,181,67,108,37,10,68,202,159,179,67,98,36,247,10,68,240,4,178,67,122,239,11,68,194,189,176,
67,92,13,13,68,100,204,175,67,98,198,43,14,68,6,219,174,67,120,100,15,68,226,98,174,67,254,183,16,68,226,98,174,67,98,236,2,18,68,226,98,174,67,84,55,19,68,6,219,174,67,54,85,20,68,100,204,175,67,98,160,115,21,68,194,189,176,67,146,109,22,68,240,4,178,
67,32,68,23,68,202,159,179,67,98,170,26,24,68,164,58,181,67,140,194,24,68,92,29,183,67,58,59,25,68,246,71,185,67,98,232,179,25,68,144,114,187,67,250,239,25,68,36,201,189,67,250,239,25,68,198,76,192,67,98,250,239,25,68,104,208,194,67,232,179,25,68,72,
43,197,67,58,59,25,68,116,94,199,67,98,140,194,24,68,162,145,201,67,170,26,24,68,186,121,203,67,32,68,23,68,148,20,205,67,98,146,109,22,68,110,175,206,67,160,115,21,68,64,241,207,67,54,85,20,68,6,218,208,67,98,84,55,19,68,206,194,209,67,236,2,18,68,170,
54,210,67,254,183,16,68,170,54,210,67,98,120,100,15,68,170,54,210,67,198,43,14,68,206,194,209,67,92,13,13,68,6,218,208,67,98,122,239,11,68,64,241,207,67,36,247,10,68,110,175,206,67,108,37,10,68,148,20,205,67,98,44,83,9,68,186,121,203,67,248,173,8,68,
162,145,201,67,74,53,8,68,116,94,199,67,98,156,188,7,68,72,43,197,67,0,128,7,68,104,208,194,67,0,128,7,68,198,76,192,67,99,109,216,181,47,68,90,15,180,67,108,216,181,47,68,210,239,161,67,98,216,181,47,68,156,138,160,67,28,193,47,68,7,32,159,67,164,215,
47,68,59,178,157,67,98,162,237,47,68,91,67,156,67,182,19,48,68,152,243,154,67,88,73,48,68,4,196,153,67,98,126,213,47,68,246,189,154,67,178,83,47,68,40,158,155,67,124,196,46,68,120,98,156,67,98,208,53,46,68,198,38,157,67,124,157,45,68,39,236,157,67,146,
252,44,68,119,176,158,67,108,220,86,37,68,0,152,167,67,108,104,145,33,68,166,32,155,67,108,32,55,41,68,29,57,146,67,98,38,225,41,68,206,116,145,67,46,139,42,68,62,202,144,67,172,52,43,68,146,59,144,67,98,178,222,43,68,230,172,143,67,4,141,44,68,160,65,
143,67,158,63,45,68,193,249,142,67,98,4,141,44,68,226,177,142,67,178,222,43,68,82,66,142,67,172,52,43,68,17,171,141,67,98,46,139,42,68,189,18,141,67,38,225,41,68,227,99,140,67,32,55,41,68,148,159,139,67,108,104,145,33,68,103,130,130,67,108,220,86,37,
68,120,224,107,67,108,146,252,44,68,116,80,126,67,98,124,157,45,68,20,217,127,67,244,55,46,68,53,182,128,67,118,203,46,68,43,132,129,67,98,244,94,47,68,16,81,130,67,230,226,47,68,140,53,131,67,194,86,48,68,127,47,132,67,98,124,235,47,68,66,207,129,67,
216,181,47,68,74,63,126,67,216,181,47,68,2,61,120,67,108,216,181,47,68,176,146,83,67,108,144,91,55,68,176,146,83,67,108,144,91,55,68,188,209,119,67,98,144,91,55,68,164,192,122,67,76,80,55,68,98,158,125,67,78,58,55,68,104,52,128,67,98,202,35,55,68,160,
153,129,67,180,253,54,68,174,237,130,67,16,200,54,68,127,47,132,67,98,116,60,55,68,140,53,131,67,66,190,55,68,16,81,130,67,238,76,56,68,43,132,129,67,98,36,220,56,68,53,182,128,67,236,115,57,68,20,217,127,67,212,20,58,68,116,80,126,67,108,142,186,65,
68,192,75,108,67,108,0,128,69,68,11,184,130,67,108,72,218,61,68,148,159,139,67,98,174,39,61,68,227,99,140,67,92,121,60,68,115,14,141,67,84,207,59,68,31,157,141,67,98,216,37,59,68,203,43,142,67,134,119,58,68,17,151,142,67,98,196,57,68,240,222,142,67,98,
134,119,58,68,207,38,143,67,252,39,59,68,95,150,143,67,78,214,59,68,179,46,144,67,98,160,132,60,68,244,197,144,67,204,48,61,68,206,116,145,67,72,218,61,68,29,57,146,67,108,0,128,69,68,74,86,155,67,108,142,186,65,68,162,205,167,67,108,212,20,58,68,119,
176,158,67,98,88,107,57,68,39,236,157,67,186,206,56,68,124,34,157,67,132,63,56,68,152,85,156,67,98,218,176,55,68,161,135,155,67,194,42,55,68,37,163,154,67,62,173,54,68,51,169,153,67,98,164,33,55,68,152,26,156,67,144,91,55,68,72,203,158,67,144,91,55,68,
48,186,161,67,108,144,91,55,68,90,15,180,67,108,216,181,47,68,90,15,180,67,99,101,0,0 };

    static const unsigned char wholeIcon[] = { 110,109,128,218,32,68,170,2,149,67,108,128,218,32,68,249,6,149,67,108,70,217,32,68,249,6,149,67,108,205,52,15,68,5,142,217,67,108,103,134,10,68,5,142,217,67,108,154,61,21,68,249,6,149,67,108,0,128,7,68,249,6,149,67,108,0,128,7,68,238,163,68,67,108,128,
218,32,68,238,163,68,67,108,128,218,32,68,170,2,149,67,99,109,178,127,69,68,175,34,149,67,108,0,128,69,68,255,38,149,67,108,197,126,69,68,255,38,149,67,108,76,218,51,68,11,174,217,67,108,152,43,47,68,11,174,217,67,108,202,226,57,68,255,38,149,67,108,
128,37,44,68,255,38,149,67,108,128,37,44,68,250,227,68,67,108,0,128,69,68,250,227,68,67,108,178,127,69,68,175,34,149,67,99,101,0,0 };

}

Path SearchBoxComponent::createPath(const String& url) const
{
    Path p;
    
    LOAD_PATH_IF_URL("next", SearchIcons::nextIcon);
    LOAD_PATH_IF_URL("prev", SearchIcons::prevIcon);
    LOAD_PATH_IF_URL("selectAll", SearchIcons::selectAllIcon);
    LOAD_PATH_IF_URL("close", SearchIcons::closeIcon);
    LOAD_PATH_IF_URL("case", SearchIcons::caseIcon);
    LOAD_PATH_IF_URL("regex", SearchIcons::regexIcon);
    LOAD_PATH_IF_URL("whole", SearchIcons::wholeIcon);
    
    return p;
}

}
