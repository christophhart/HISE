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
	find("Find"),
	prev("Find prev"),
	findAll("Find all")
{
	searchField.setFont(d.getFont().withHeight(d.getFontHeight() * scaleFactor));

	laf.f = searchField.getFont();

	searchField.setCaretVisible(true);
	searchField.setColour(juce::CaretComponent::ColourIds::caretColourId, Colours::black);

	addAndMakeVisible(searchField);

	searchField.addKeyListener(this);
	searchField.addListener(this);

	find.setLookAndFeel(&laf);
	prev.setLookAndFeel(&laf);
	findAll.setLookAndFeel(&laf);

	find.addListener(this);
	prev.addListener(this);

	findAll.onClick = [this]()
	{
		doc.setSelections(doc.getSearchResults(), true);
		doc.setSearchResults({});
		sendSearchChangeMessage();
	};

	addAndMakeVisible(find);
	addAndMakeVisible(prev);
	addAndMakeVisible(findAll);
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
	b.removeFromTop(5);
	auto okBox = b.removeFromRight(200);



	b.removeFromLeft(getResultFont().getStringWidth("1230 matches"));

	searchField.setBounds(b);

	find.setBounds(okBox.removeFromLeft(60));
	prev.setBounds(okBox.removeFromLeft(60));
	findAll.setBounds(okBox);
		
}

void SearchBoxComponent::Blaf::drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour,
	bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	float alpha = 0.4f;

	if (shouldDrawButtonAsHighlighted)
		alpha += 0.1f;

	if (shouldDrawButtonAsDown)
		alpha += 0.2f;

	g.setColour(Colours::white.withAlpha(alpha));
	g.fillRoundedRectangle(b.getLocalBounds().toFloat().reduced(1.0f, 1.0f), 2.0f);
}

void SearchBoxComponent::Blaf::drawButtonText(Graphics& g, TextButton& b, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	g.setFont(Font("Oxygen", 13.0f, Font::bold));
	g.setColour(Colours::black);
	g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
}

}
