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


void Autocomplete::Item::mouseUp(const MouseEvent& e)
{
	auto editor = findParentComponentOfClass<Autocomplete>()->editor;
	editor->closeAutocomplete(true, token->getCodeToInsert(input), token->getSelectionRangeAfterInsert(input));
}

juce::AttributedString Autocomplete::Item::createDisplayText() const
{
	AttributedString s;

	auto text = token->tokenContent;

	auto beforeIndex = text.toLowerCase().indexOf(input.toLowerCase());

	auto before = text.substring(0, beforeIndex);
	auto between = text.substring(beforeIndex, beforeIndex + input.length());
	auto after = text.substring(beforeIndex + input.length());

	auto sf = findParentComponentOfClass<Autocomplete>()->getScaleFactor();

	auto nf = GLOBAL_MONOSPACE_FONT().withHeight(16.0f * sf);
    
    
#if JUCE_LINUX
    auto bf = GLOBAL_BOLD_MONOSPACE_FONT().withHeight(16.0f * sf);
#else
	auto bf = nf.boldened();
#endif

	s.append(before, nf, Colours::white.withAlpha(0.7f));
	s.append(between, bf, Colours::white.withAlpha(1.0f));
	s.append(after, nf, Colours::white.withAlpha(0.7f));

	return s;
}

void Autocomplete::Item::paint(Graphics& g)
{
	auto selected = isSelected();

	auto over = isMouseOver(true);
	auto down = isMouseButtonDown(true);

	g.fillAll(Colour(0xFF373737));

	Colour c(0xFF444444);

	if (over)
		c = c.brighter(0.05f);
	if (down)
		c = c.brighter(0.1f);

	g.setColour(c);

	auto b = getLocalBounds().toFloat();
	b.removeFromBottom(1.0f);

	auto bar = b.removeFromLeft(3.0f);

	g.fillRect(b);




	if (selected)
	{
		g.setGradientFill(ColourGradient(Colour(0xFF666666), { 0.0f, 0.f }, Colour(0xFF555555), { 0.0f, (float)getHeight() }, false));
		g.fillRect(b);
	}

	Font f;

	g.setFont(f);
	g.setColour(Colours::white.withAlpha(0.8f));

	auto tBounds = getLocalBounds().toFloat();
	tBounds = tBounds.withSizeKeepingCentre(tBounds.getWidth() - 10.0f, tBounds.getHeight() * 0.8f);

	

	g.setColour(token->c);
	g.fillRect(bar);
	

	auto s = createDisplayText();
	s.draw(g, tBounds);
}



Autocomplete::Autocomplete(TokenCollection& tokenCollection_, const String& input, const String& previousToken, int lineNumber, TextEditor* editor_) :
	tokenCollection(tokenCollection_),
	scrollbar(true),
	shadow(DropShadow(Colours::black.withAlpha(0.7f), 5, Point<int>())),
	editor(editor_)
{
	addAndMakeVisible(scrollbar);
	setInput(input, previousToken, lineNumber);
	scrollbar.addListener(this);
}

bool Autocomplete::keyPressed(const KeyPress& key, Component*)
{
	allowPopup = true;
		
	if (key == KeyPress::returnKey)
	{
		if (editor->incParameter())
		{
			editor->closeAutocomplete(false, getCurrentText(), {});
			return true;
		}

		editor->closeAutocomplete(true, getCurrentText(), getSelectionRange(currentInput));
		
		return true;
	}

	if (key == KeyPress::escapeKey || key == KeyPress::leftKey || key == KeyPress::rightKey)
	{
		editor->closeAutocomplete(true, {}, {});
		return key == KeyPress::escapeKey;
	}

	if (key == KeyPress::pageDownKey || key == KeyPress::pageUpKey)
	{
		selectNextItem(key == KeyPress::pageDownKey, 7);
	}

	if (key == KeyPress::upKey || key == KeyPress::downKey)
	{
		
		selectNextItem(key == KeyPress::downKey);
		return true;
	}

	editor->repaint();
	return false;
}

void Autocomplete::cancel()
{
	setSize(0, 0);
	Component::SafePointer<Autocomplete> safeThis(this);

	auto f = [safeThis]()
	{
		if (safeThis.getComponent() != nullptr)
		{
			if (auto p = safeThis.getComponent()->editor)
			{
				p->closeAutocomplete(false, {}, {});
			}
		}
	};

	MessageManager::callAsync(f);
}

void Autocomplete::setInput(const String& input, const String& previousToken, int lineNumber)
{
	if (editor->includeDotInAutocomplete)
		currentInput = previousToken + input;
	else
		currentInput = input;

	auto currentlyDisplayedItem = getCurrentText();
	items.clear();

	viewIndex = 0;

    tokenCollection.sortForInput(input);
    
	for (auto t : tokenCollection)
	{
		if (t->matches(input, previousToken, lineNumber))
		{
			if (t->tokenContent == currentlyDisplayedItem)
				viewIndex = items.size();

			items.add(createItem(t, currentInput));

			addAndMakeVisible(items.getLast());
		}
	}

	int numLinesFull = 7;

	if (isPositiveAndBelow(numLinesFull, items.size()))
	{
		displayedRange = { 0, numLinesFull };

		displayedRange = displayedRange.movedToStartAt(viewIndex);

		if (displayedRange.getEnd() >= items.size())
		{
			displayedRange = displayedRange.movedToEndAt(items.size() - 1);
		}

	}
	else
		displayedRange = { 0, items.size() };

	scrollbar.setRangeLimits({ 0.0, (double)items.size() });

	setDisplayedIndex(viewIndex);

	auto h = getNumDisplayedRows() * getRowHeight();

	if (items.size() == 0)
		cancel();

	if (isSingleMatch())
	{
		cancel();
	}
	else
	{
		auto maxWidth = 0;

		auto nf = Font(Font::getDefaultMonospacedFontName(), 16.0f * getScaleFactor(), Font::plain);

		for (auto& i : items)
		{

			maxWidth = jmax(maxWidth, nf.getStringWidth(i->token->tokenContent) + 20);
		}

		setSize(maxWidth, h);
		resized();
		repaint();
	}
}

float Autocomplete::getScaleFactor() const
{
	return editor->transform.getScaleFactor();
}

void Autocomplete::ParameterSelection::rebuildPosition(TextDocument& doc, AffineTransform t)
{
	p = HighlightComponent::getOutlinePath(doc, getSelection());
	p.applyTransform(t);

#if 0
	auto sel = getSelection();

	for (auto l : doc.getUnderlines(sel.oriented(), mcl::TextDocument::Metric::top))
	{
		juce::Rectangle<float> s(l.getStart(), l.getEnd().translated(0.0f, doc.getRowHeight()));

		bounds.add(s.transformedBy(t));
	}
#endif
}

void Autocomplete::ParameterSelection::draw(Graphics& g, Ptr currentlyActive)
{
	g.setColour(Colour(0x1100bcff));
	g.fillPath(p);

	if (currentlyActive.get() == this)
	{
		g.setColour(Colour(0x6600bcff));
		g.strokePath(p, PathStrokeType(1.0f));
	}
}

}
