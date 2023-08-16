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
	find("next", *this, this),
	prev("prev", *this, this),
	findAll("selectAll", *this, nullptr),
    close("close", *this, nullptr)
{
	searchField.setFont(d.getFont().withHeight(d.getFontHeight() * scaleFactor));

	laf.f = searchField.getFont();

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
            parent->keyPressed(KeyPress::escapeKey);
        });
    };

	addAndMakeVisible(find);
	addAndMakeVisible(prev);
	addAndMakeVisible(findAll);
    addAndMakeVisible(close);
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
	CodeDocument::Position p(doc.getCodeDocument(), 0);

	auto firstChar = text[0];
	int maxIndex = text.length();
	Array<Selection> searchResults;

	while (p.getPosition() < doc.getCodeDocument().getNumCharacters())
	{
		if (p.getCharacter() == firstChar)
		{
			auto e = p.movedBy(maxIndex);

			auto t = doc.getCodeDocument().getTextBetween(p, e);

			if (text == t)
			{
				Point<int> ps(p.getLineNumber(), p.getIndexInLine());
				Point<int> pe(e.getLineNumber(), e.getIndexInLine());

				searchResults.add({ ps, pe });
			}
		}

		p.moveBy(1);
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

	g.setColour(Colour(0xFF555555));
	g.fillRect(b);

	String s;
		
	s << String(doc.getSearchResults().size()) << " matches";
		
	g.setColour(Colours::white.withAlpha(0.8f));
	g.setFont(getResultFont());
	g.drawText(s, b.reduced(8.0f, 0.0f), Justification::centredLeft);

}

Font SearchBoxComponent::getResultFont() const
{
	return laf.f.withHeight(laf.f.getHeight() * 0.8f).boldened();
}

void SearchBoxComponent::resized()
{
	auto b = getLocalBounds();
	b.removeFromTop(2);
    b.removeFromBottom(2);
	auto okBox = b.removeFromRight(200);



	b.removeFromLeft(getResultFont().getStringWidth("1230 matches"));

	searchField.setBounds(b);

	find.setBounds(okBox.removeFromLeft(32));
	prev.setBounds(okBox.removeFromLeft(32));
    
	findAll.setBounds(okBox.removeFromLeft(32));
    close.setBounds(okBox.removeFromLeft(32));
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

}

Path SearchBoxComponent::createPath(const String& url) const
{
    Path p;
    
    LOAD_PATH_IF_URL("next", SearchIcons::nextIcon);
    LOAD_PATH_IF_URL("prev", SearchIcons::prevIcon);
    LOAD_PATH_IF_URL("selectAll", SearchIcons::selectAllIcon);
    LOAD_PATH_IF_URL("close", SearchIcons::closeIcon);
    
    return p;
}

}
