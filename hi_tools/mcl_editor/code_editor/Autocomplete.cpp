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

TokenCollection::Token::Token(const String& text):
	tokenContent(text)
{}

TokenCollection::Token::~Token()
{}

bool TokenCollection::Token::matches(const String& input, const String& previousToken, int lineNumber) const
{
	auto textMatches = matchesInput(input, tokenContent);
	auto scopeMatches = tokenScope.isEmpty() || tokenScope.contains(lineNumber);
	return textMatches && scopeMatches;
}

bool TokenCollection::Token::matchesInput(const String& input, const String& code)
{
	auto inputLength = input.length();
            
	if (inputLength <= 2)
		return code.toLowerCase().startsWith(input.toLowerCase());
	else
		return code.toLowerCase().contains(input.toLowerCase());
}

bool TokenCollection::Token::equals(const Token* other) const
{
	return other->tokenContent == tokenContent;
}

bool TokenCollection::Token::operator==(const Token& other) const
{
	return equals(&other) && other.equals(this);
}

Array<Range<int>> TokenCollection::Token::getSelectionRangeAfterInsert(const String& input) const
{
	Array<Range<int>> parameterRanges;

	auto code = getCodeToInsert(input);

	auto ptr = code.getCharPointer();
	auto start = ptr;
	auto end = start + code.length();

	int thisRangeStart = 0;
	const int delta = 0;

	while (ptr != end)
	{
		auto c = *ptr;

		switch (c)
		{
		case '(':
		case '<':
			thisRangeStart = (int)(ptr - start) + 1;
			break;
		case ',':
			{
				auto pos = (int)(ptr - start);

				Range<int> r(thisRangeStart + delta, pos + delta);

				if(r.getLength() > 0)
					parameterRanges.add(r);

				thisRangeStart = pos + 1;

				if (ptr[1] == ' ')
					thisRangeStart++;
				break;
			}
		case ')':
		case '>':
			{
				auto pos = (int)(ptr - start);

				Range<int> r(thisRangeStart + delta, pos + delta);

				if(r.getLength() > 0)
					parameterRanges.add(r);
				break;
			}
		}

		ptr++;
	}

	return parameterRanges;
}

MarkdownLink TokenCollection::Token::getLink() const
{ return MarkdownLink(); }

String TokenCollection::Token::getCodeToInsert(const String& input) const
{ return tokenContent; }

TokenCollection::Token* const* TokenCollection::begin() const
{ return tokens.begin(); }

TokenCollection::Token* const* TokenCollection::end() const
{ return tokens.end(); }

TokenCollection::Token** TokenCollection::begin()
{ return tokens.begin(); }

TokenCollection::Token** TokenCollection::end()
{ return tokens.end(); }

TokenCollection::Provider::~Provider()
{}

void TokenCollection::Provider::signalRebuild()
{
	if (assignedCollection != nullptr)
		assignedCollection->signalRebuild();
}

void TokenCollection::Provider::signalClear(NotificationType n)
{
	if(assignedCollection != nullptr)
		assignedCollection->signalClear(n);
}

TokenCollection::Listener::~Listener()
{}

void TokenCollection::setEnabled(bool shouldBeEnabled)
{
	if (shouldBeEnabled != enabled)
	{
		enabled = shouldBeEnabled;

		if (enabled && !buildLock.writeAccessIsLocked())
			signalRebuild();
	}
}

void TokenCollection::signalRebuild()
{
	if (!enabled)
		return;

	stopThread(1000);
	startThread();
}

void TokenCollection::signalClear(NotificationType n)
{
	SimpleReadWriteLock::ScopedWriteLock sl(buildLock);
	dirty = false;
	tokens.clear();
	cancelPendingUpdate();
        
	for(auto l: listeners)
	{
		if(l != nullptr)
			l->tokenListWasRebuild();
	}
}

void TokenCollection::run()
{
	dirty = true;
	rebuild();
}

void TokenCollection::clearTokenProviders()
{
	tokenProviders.clear();
}

void TokenCollection::addTokenProvider(Provider* ownedProvider)
{
	if (tokenProviders.isEmpty())
		startThread();

	tokenProviders.add(ownedProvider);
	ownedProvider->assignedCollection = this;
}

TokenCollection::TokenCollection():
	Thread("TokenRebuildThread", HISE_DEFAULT_STACK_SIZE)
{

}

TokenCollection::~TokenCollection()
{
	stopThread(1000);
}

bool TokenCollection::hasEntries(const String& input, const String& previousToken, int lineNumber) const
{
	if (CharacterFunctions::isDigit(previousToken[0]))
		return false;

	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(buildLock))
	{
		for (auto t : tokens)
		{
			if (dirty || isThreadRunning())
				return false;

			if (t->matches(input, previousToken, lineNumber))
				return true;
		}
	}

	return false;
}

void TokenCollection::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void TokenCollection::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void TokenCollection::handleAsyncUpdate()
{
	for (auto l : listeners)
	{
		if (l != nullptr)
			l->tokenListWasRebuild();
	}
}

int64 TokenCollection::getHashFromTokens(const List& l)
{
	int64 hash = 0;

	for (auto& t : l)
	{
		hash += t->tokenContent.hashCode();
	}

	return hash;
}

void TokenCollection::rebuild()
{
	if (dirty)
	{
		SimpleReadWriteLock::ScopedWriteLock sl(buildLock);

		List newTokens;

		for (auto tp : tokenProviders)
			tp->addTokens(newTokens);

		Sorter ts;
		newTokens.sort(ts);

		auto newHash = getHashFromTokens(newTokens);

		if (newHash != currentHash)
		{
			tokens.swapWith(newTokens);
			triggerAsyncUpdate();
		}
			
		dirty = false;
	}
}

bool TokenCollection::isEnabled() const
{ return enabled; }

int TokenCollection::Sorter::compareElements(Token* first, Token* second)
{
	if (first->priority > second->priority)
		return -1;

	if (first->priority < second->priority)
		return 1;

	return first->tokenContent.compareIgnoreCase(second->tokenContent);
}

TokenCollection::FuzzySorter::FuzzySorter(const String& e):
	exactSearchTerm(e)
{
            
}

int TokenCollection::FuzzySorter::compareElements(Token* first, Token* second) const
{
	auto s1 = first->tokenContent;
	auto s2 = second->tokenContent;
            
	auto exactMatch1 = s1.contains(exactSearchTerm);
	auto exactMatch2 = s2.contains(exactSearchTerm);
            
	if(exactMatch1 && !exactMatch2)
		return -1;
            
	if(!exactMatch1 && exactMatch2)
		return 1;
            
	auto startsWith1 = s1.startsWith(exactSearchTerm);
	auto startsWith2 = s2.startsWith(exactSearchTerm);
            
	if(startsWith1 && !startsWith2)
		return -1;
            
	if(!startsWith1 && startsWith2)
		return 1;
            
	if(first->priority > second->priority)
		return -1;
            
	if(second->priority > first->priority)
		return 1;
            
	return first->tokenContent.compareIgnoreCase(second->tokenContent);
}

void TokenCollection::sortForInput(const String& input)
{
	FuzzySorter fs(input);
	tokens.sort(fs);
}

SimpleDocumentTokenProvider::SimpleDocumentTokenProvider(CodeDocument& doc):
	CoallescatedCodeDocumentListener(doc)
{}

void SimpleDocumentTokenProvider::timerCallback()
{
	signalRebuild();
	stopTimer();
}

void SimpleDocumentTokenProvider::codeChanged(bool cond, int i, int i1)
{
	startTimer(5000);
}

void SimpleDocumentTokenProvider::addTokens(TokenCollection::List& tokens)
{
	CodeDocument::Iterator it(lambdaDoc);
	String currentString;

	while (!it.isEOF())
	{
		auto c = it.nextChar();

		int numChars = 0;

		if (CharacterFunctions::isLetter(c) || (c == '_') || (currentString.isNotEmpty() && CharacterFunctions::isLetterOrDigit(c)))
		{
			currentString << c;
			numChars++;
		}
		else
		{
			if (numChars > 2 && numChars < 60)
			{
				bool found = false;

				for (auto& t : tokens)
				{
					if (t->tokenContent == currentString)
					{
						found = true;
						break;
					}
				}

				if(!found)
					tokens.add(new TokenCollection::Token(currentString));
			}
					
			currentString = {};
		}
	}
}

Autocomplete::ParameterSelection::ParameterSelection(TextDocument& doc, int start, int end):
	s(doc.getCodeDocument(), start),
	e(doc.getCodeDocument(), end)
{
	s.setPositionMaintained(true);
	e.setPositionMaintained(true);

	tooltip = doc.getCodeDocument().getTextBetween(s, e);
}

Selection Autocomplete::ParameterSelection::getSelection() const
{
	mcl::Selection sel(s.getLineNumber(), s.getIndexInLine(), e.getLineNumber(), e.getIndexInLine());
	return sel;
}

Autocomplete::HelpPopup::HelpPopup(Autocomplete* p):
	ac(p),
	corner(this, nullptr)
{
	addAndMakeVisible(display);
	p->addComponentListener(this);
	addAndMakeVisible(corner);
}

Autocomplete::HelpPopup::~HelpPopup()
{
	if (ac != nullptr)
		ac->removeComponentListener(this);
}

void Autocomplete::HelpPopup::componentMovedOrResized(Component& component, bool cond, bool cond1)
{
	setTopLeftPosition(component.getBounds().getBottomLeft());
	setSize(jmax(300, component.getWidth()), jmin<int>((int)display.totalHeight + 20, 250));
}

void Autocomplete::HelpPopup::refreshText()
{
	if (auto i = ac->items[ac->viewIndex])
	{
		auto t = i->token->markdownDescription;

		if (t.isEmpty())
		{
			setVisible(false);
			return;
		}
					
		setVisible(true);
		display.setText(t);
	}
}

void Autocomplete::HelpPopup::resized()
{
	display.setBounds(getLocalBounds().reduced(10));
	corner.setBounds(getLocalBounds().removeFromRight(10).removeFromBottom(10));
}

void Autocomplete::HelpPopup::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF333333));
	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRect(getLocalBounds().toFloat(), 1.0f);
}

Autocomplete::Item::Item(TokenCollection::TokenPtr t, const String& input_):
	token(t),
	input(input_)
{
	setRepaintsOnMouseActivity(true);
}

bool Autocomplete::Item::isSelected()
{
	if (auto p = findParentComponentOfClass<Autocomplete>())
	{
		if (isPositiveAndBelow(p->viewIndex, p->items.size()))
			return p->items[p->viewIndex] == this;
	}

	return false;
}

Autocomplete::~Autocomplete()
{
}

Array<Range<int>> Autocomplete::getSelectionRange(const String& input) const
{
	if (isPositiveAndBelow(viewIndex, items.size()))
	{
		return items[viewIndex]->token->getSelectionRangeAfterInsert(input);
	}

	return {};
}

String Autocomplete::getCurrentText() const
{
	if (isPositiveAndBelow(viewIndex, items.size()))
	{
		return items[viewIndex]->token->getCodeToInsert(currentInput);
	}

	return {};
}

void Autocomplete::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	displayedRange = displayedRange.movedToStartAt((int)newRangeStart);
	resized();
}

void Autocomplete::selectNextItem(bool showNext, int delta)
{
	if (showNext)
		viewIndex = jmin(viewIndex + delta, items.size() - 1);
	else
		viewIndex = jmax(0, viewIndex - delta);

	setDisplayedIndex(viewIndex);

}

void Autocomplete::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
	auto start = displayedRange.getStart();

	start -= (wheel.deltaY * 8);

	displayedRange = displayedRange.movedToStartAt(start);

	if (displayedRange.getEnd() >= items.size())
		displayedRange = displayedRange.movedToEndAt(items.size() - 1);
	if (displayedRange.getStart() < 0)
		displayedRange = displayedRange.movedToStartAt(0);

	scrollbar.setCurrentRange({ (double)displayedRange.getStart(), (double)displayedRange.getEnd() }, dontSendNotification);

	resized();
}

void Autocomplete::setDisplayedIndex(int index)
{
	if (displayedRange.isEmpty())
	{
		helpPopup = nullptr;
		return;
	}

	if (!displayedRange.contains(viewIndex))
	{
		if (viewIndex < displayedRange.getStart())
			displayedRange = displayedRange.movedToStartAt(viewIndex);
		else
			displayedRange = displayedRange.movedToEndAt(viewIndex + 1);
	}

	if (displayedRange.getEnd() > items.size())
		displayedRange = displayedRange.movedToEndAt(items.size() - 1);

	if (displayedRange.getStart() < 0)
		displayedRange = displayedRange.movedToStartAt(0);


	scrollbar.setCurrentRange({ (double)displayedRange.getStart(), (double)displayedRange.getEnd() });

		

	if (allowPopup && helpPopup == nullptr && getParentComponent() != nullptr)
	{
		helpPopup = new HelpPopup(this);

		getParentComponent()->addAndMakeVisible(helpPopup);
		helpPopup->setTransform(getTransform());
	}

	if (helpPopup != nullptr)
	{
		helpPopup->componentMovedOrResized(*this, false, false);
		helpPopup->refreshText();
		helpPopup->componentMovedOrResized(*this, false, false);
	}

	resized();
	repaint();
}

Autocomplete::Item* Autocomplete::createItem(const TokenCollection::TokenPtr t, const String& input)
{
	return new Item(t, input);
}

bool Autocomplete::isSingleMatch() const
{
	if (items.size() == 1)
	{
		return items.getFirst()->token->tokenContent == currentInput;
	}

	return false;
}

int Autocomplete::getRowHeight() const
{
	return roundToInt(28.0f * getScaleFactor());
}

void Autocomplete::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF282828));
}

void Autocomplete::paintOverChildren(Graphics& g)
{
	auto b = getLocalBounds();
	g.setColour(Colour(0xFF222222));
	g.drawRect(b.toFloat(), 1.0f);
}

int Autocomplete::getNumDisplayedRows() const
{
	return displayedRange.getLength();
}

void Autocomplete::resized()
{
	auto scrollbarVisible = items.size() != displayedRange.getLength();

	scrollbar.setVisible(scrollbarVisible);

	auto b = getLocalBounds();

	if (scrollbarVisible)
	{

		scrollbar.setBounds(b.removeFromRight(10));
	}

	auto h = getRowHeight();

	Rectangle<int> itemBounds = { b.getX(), b.getY() - displayedRange.getStart() * h, b.getWidth(), h };

	for (auto i : items)
	{
		i->setBounds(itemBounds);
		itemBounds.translate(0, h);
	}
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
