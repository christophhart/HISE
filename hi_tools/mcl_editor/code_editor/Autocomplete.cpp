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
	auto editor = findParentComponentOfClass<TextEditor>();
	editor->closeAutocomplete(true, token->getCodeToInsert(input), token->getSelectionRangeAfterInsert());
}

juce::AttributedString Autocomplete::Item::createDisplayText() const
{
	AttributedString s;

	auto text = token->tokenContent;

	auto beforeIndex = text.indexOf(input);

	auto before = text.substring(0, beforeIndex);
	auto between = input;
	auto after = text.substring(beforeIndex + input.length());

	auto nf = Font(Font::getDefaultMonospacedFontName(), 16.0f, Font::plain);
	auto bf = nf.boldened();

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
	tBounds = tBounds.withSizeKeepingCentre(tBounds.getWidth() - 10.0f, 18.0f);

	auto s = createDisplayText();
	s.draw(g, tBounds);
}



bool Autocomplete::keyPressed(const KeyPress& key, Component*)
{
	allowPopup = true;
	auto editor = findParentComponentOfClass<TextEditor>();

	if (key == KeyPress::returnKey)
	{
		if (editor->incParameter())
		{
			editor->closeAutocomplete(false, getCurrentText(), {});
			return true;
		}

		editor->closeAutocomplete(true, getCurrentText(), getSelectionRange());
		
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
			if (auto p = safeThis.getComponent()->findParentComponentOfClass<TextEditor>())
			{
				p->closeAutocomplete(false, {}, {});
			}
		}
	};

	MessageManager::callAsync(f);
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