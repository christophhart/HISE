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


#pragma once

namespace mcl
{
using namespace juce;


/** A TokenCollection handles the database for the autocomplete popup.

You can register new providers that will populate the given token list with their entries
and add listeners to be notified when the token list changes.

For a default implementation that just scans the current text content, take a look at the 
SimpleDocumentTokenProvider class.

*/
class TokenCollection : public Thread,
						public AsyncUpdater
{
	

public:

	/** A Token is the entry that is being used in the autocomplete popup (or any other IDE tools
	    that might use that database. */
	struct Token: public ReferenceCountedObject
	{
		Token(const String& text) :
			tokenContent(text)
		{};

		/** Override the method and check whether the currently written input matches the token. */
		virtual bool matches(const String& input, const String& previousToken, int lineNumber) const
		{
			auto textMatches = matchesInput(input, tokenContent);
			auto scopeMatches = tokenScope.isEmpty() || tokenScope.contains(lineNumber);
			return textMatches && scopeMatches;
		}

		static bool matchesInput(const String& input, const String& code)
		{
			if (input.length() == 1)
				return code.startsWith(input);
			else
				return code.contains(input);
		}

		bool operator==(const Token& other) const
		{
			return tokenContent == other.tokenContent;
		}

		virtual Array<Range<int>> getSelectionRangeAfterInsert() const
		{
			Array<Range<int>> parameterRanges;

			auto code = getCodeToInsert("");

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
					thisRangeStart = (ptr - start) + 1;
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

		/** Override this method if you want to customize the code that is about to be inserted. */
		virtual String getCodeToInsert(const String& input) const { return tokenContent; }

		String markdownDescription;
		String tokenContent;
		
		/** The base colour for displaying the entry. */
		Colour c = Colours::white;

		/** The priority of the token. Tokens with higher priority will show up first in the token list. */
		int priority = 0;

		/** This can be used to limit the scope of a token. */
		Range<int> tokenScope;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Token);
	};

	/** Make it iteratable. */
	Token** begin() const { return tokens.begin(); }
	Token** end() const { return tokens.end(); }

	using List = ReferenceCountedArray<Token>;
	using TokenPtr = ReferenceCountedObjectPtr<Token>;
	
	/** A provider is a class that adds its tokens to the given list. 
	
		In order to use it, subclass from this and override the addTokens() method.
		
		Then whenever you want to rebuild the token list, call the signalRebuild method
		to notify the registered TokenCollection to rebuild its tokens.
	*/
	struct Provider
	{
		virtual ~Provider() {}

		/** Override this method and add all tokens to the given list. This method will be called on a background thread
		    to keep the UI thread responsive, so make sure you do not interfere with the message thread for longer than necessary. 
		*/
		virtual void addTokens(List& tokens) = 0;

		/** Call the TokenCollections rebuild method. This will not be executed synchronously, but on a dedicated thread. */
		void signalRebuild()
		{
			if (assignedCollection != nullptr)
				assignedCollection->signalRebuild();
		}

		WeakReference<TokenCollection> assignedCollection;
	};

	/** A Listener interface that will be notified whenever the token list was rebuilt. */
	struct Listener
	{
		virtual ~Listener() {};

		/** This method will be called on the message thread after the list was rebuilt. */
		virtual void tokenListWasRebuild() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	void signalRebuild()
	{
		dirty = true;
		notify();
	}

	void run() override
	{
		while (!threadShouldExit())
		{
			rebuild();
			wait(3000);
		}
	}

	void clearTokenProviders()
	{
		tokenProviders.clear();
	}

	/** Register a token provider to this instance. Be aware that you can't register a token provider to multiple instances,
	    but this shouldn't be a problem. */
	void addTokenProvider(Provider* ownedProvider)
	{
		if (tokenProviders.isEmpty())
			startThread();

		tokenProviders.add(ownedProvider);
		ownedProvider->assignedCollection = this;
	}

	TokenCollection():
		Thread("TokenRebuildThread")
	{

	}


	~TokenCollection()
	{
		stopThread(1000);
	}

	bool hasEntries(const String& input, const String& previousToken, int lineNumber) const
	{
		for (auto t : tokens)
		{
			if (t->matches(input, previousToken, lineNumber))
				return true;
		}

		return false;
	}

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void handleAsyncUpdate() override
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->tokenListWasRebuild();
		}
	}

	static int64 getHashFromTokens(const List& l)
	{
		int64 hash = 0;

		for (auto& t : l)
		{
			hash += t->tokenContent.hashCode64();
		}

		return hash;
	}

	void rebuild()
	{
		if (dirty)
		{
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

	private:

	struct Sorter
	{
		static int compareElements(Token* first, Token* second)
		{
			if (first->priority > second->priority)
				return -1;

			if (first->priority < second->priority)
				return 1;

			return first->tokenContent.compareIgnoreCase(second->tokenContent);
		}
	};

private:

	OwnedArray<Provider> tokenProviders;
	Array<WeakReference<Listener>> listeners;
	List tokens;
	int64 currentHash;
	std::atomic<bool> dirty = { false };

	JUCE_DECLARE_WEAK_REFERENCEABLE(TokenCollection);
};

/** A TokenCollection::Provider subclass that scans the current document and creates a list of all tokens. */
struct SimpleDocumentTokenProvider : public TokenCollection::Provider,
									 public CoallescatedCodeDocumentListener
{
	SimpleDocumentTokenProvider(CodeDocument& doc) :
		CoallescatedCodeDocumentListener(doc)
	{}

	void codeChanged(bool, int, int) override
	{
		signalRebuild();
	}

	void addTokens(TokenCollection::List& tokens) override
	{
		CodeDocument::Iterator it(lambdaDoc);
		String currentString;

		while (!it.isEOF())
		{
			auto c = it.nextChar();

			if (CharacterFunctions::isLetter(c) || (c == '_') || (currentString.isNotEmpty() && CharacterFunctions::isLetterOrDigit(c)))
				currentString << c;
			else
			{
				if (currentString.length() > 2)
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
};


struct Autocomplete : public Component,
	public KeyListener,
	public ScrollBar::Listener
{
	using Token = TokenCollection::Token;

	struct ParameterSelection: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<ParameterSelection>;
		using List = ReferenceCountedArray<ParameterSelection>;

		ParameterSelection(TextDocument& doc, int start, int end):
			s(doc.getCodeDocument(), start),
			e(doc.getCodeDocument(), end)
		{
			s.setPositionMaintained(true);
			e.setPositionMaintained(true);

			tooltip = doc.getCodeDocument().getTextBetween(s, e);
		}

		Selection getSelection() const
		{
			mcl::Selection sel(s.getLineNumber(), s.getIndexInLine(), e.getLineNumber(), e.getIndexInLine());
			return sel;
		}

		void rebuildPosition(TextDocument& doc, AffineTransform t);

		void draw(Graphics& g, Ptr currentlyActive);

		Path p;
		CodeDocument::Position s, e;
		String tooltip;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterSelection);

	};

	struct HelpPopup : public Component,
					   public ComponentListener
	{

		HelpPopup(Autocomplete* p) :
			ac(p),
			corner(this, nullptr)
		{
			addAndMakeVisible(display);
			p->addComponentListener(this);
			addAndMakeVisible(corner);
		}

		~HelpPopup()
		{
			if (ac != nullptr)
				ac->removeComponentListener(this);
		}

		void componentMovedOrResized(Component& component, bool , bool )
		{
			setTopLeftPosition(component.getBounds().getBottomLeft());
			setSize(jmax(300, component.getWidth()), jmin<int>((int)display.totalHeight + 20, 200));
		}

		void refreshText()
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

		void resized() override
		{
			display.setBounds(getLocalBounds().reduced(10));
			corner.setBounds(getLocalBounds().removeFromRight(10).removeFromBottom(10));
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF333333));
			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawRect(getLocalBounds().toFloat(), 1.0f);
		}

		Autocomplete* ac;
		SimpleMarkdownDisplay display;
		ResizableCornerComponent corner;
	};

	struct Item : public Component
	{
		Item(TokenCollection::TokenPtr t, const String& input_) :
			token(t),
			input(input_)
		{
			setRepaintsOnMouseActivity(true);
		}

		void mouseUp(const MouseEvent& e) override;

		AttributedString createDisplayText() const;

		void paint(Graphics& g) override;

		bool isSelected()
		{
			if (auto p = findParentComponentOfClass<Autocomplete>())
			{
				if (isPositiveAndBelow(p->viewIndex, p->items.size()))
					return p->items[p->viewIndex] == this;
			}

			return false;
		}

		TokenCollection::TokenPtr token;
		String input;
	};

	Autocomplete(TokenCollection& tokenCollection_, const String& input, const String& previousToken, int lineNumber) :
		tokenCollection(tokenCollection_),
		scrollbar(true),
		shadow(DropShadow(Colours::black.withAlpha(0.7f), 5, {}))
	{
		addAndMakeVisible(scrollbar);
		setInput(input, previousToken, lineNumber);
		scrollbar.addListener(this);
	}

	~Autocomplete()
	{
	}


	juce::DropShadower shadow;

	bool keyPressed(const KeyPress& key, Component*);

	void cancel();

	Array<Range<int>> getSelectionRange() const
	{
		if (isPositiveAndBelow(viewIndex, items.size()))
		{
			return items[viewIndex]->token->getSelectionRangeAfterInsert();
		}

		return {};
	}

	String getCurrentText() const
	{
		if (isPositiveAndBelow(viewIndex, items.size()))
		{
			return items[viewIndex]->token->getCodeToInsert(currentInput);
		}

		return {};
	}

	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
	{
		displayedRange = displayedRange.movedToStartAt((int)newRangeStart);
		resized();
	}

	void selectNextItem(bool showNext, int delta = 1)
	{
		if (showNext)
			viewIndex = jmin(viewIndex + delta, items.size() - 1);
		else
			viewIndex = jmax(0, viewIndex - delta);

		setDisplayedIndex(viewIndex);

	}

	void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override
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

	void setDisplayedIndex(int index)
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

	Item* createItem(const TokenCollection::TokenPtr t, const String& input)
	{
		return new Item(t, input);
	}

	bool isSingleMatch() const
	{
		if (items.size() == 1)
		{
			return items.getFirst()->token->tokenContent == currentInput;
		}

		return false;
	}

	void setInput(const String& input, const String& previousToken, int lineNumber)
	{
		currentInput = input;

		auto currentlyDisplayedItem = getCurrentText();
		items.clear();

		viewIndex = 0;

		for (auto t : tokenCollection)
		{
			if (t->matches(input, previousToken, lineNumber))
			{
				if (t->tokenContent == currentlyDisplayedItem)
					viewIndex = items.size();

				items.add(createItem(t, input));
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

			auto nf = Font(Font::getDefaultMonospacedFontName(), 16.0f, Font::plain);

			for (auto& i : items)
			{

				maxWidth = jmax(maxWidth, nf.getStringWidth(i->token->tokenContent) + 20);
			}

			setSize(maxWidth, h);
			resized();
			repaint();
		}
	}

	int getRowHeight() const
	{
		return 28;
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF282828));
		auto b = getLocalBounds().toFloat();

	}



	void paintOverChildren(Graphics& g) override
	{
		auto b = getLocalBounds();
		g.setColour(Colour(0xFF222222));
		g.drawRect(b.toFloat(), 1.0f);
	}

	int getNumDisplayedRows() const
	{
		return displayedRange.getLength();
	}

	void resized() override
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

	OwnedArray<Item> items;
	int viewIndex = 0;

	Range<int> displayedRange;
	String currentInput;

	TokenCollection& tokenCollection;
	ScrollBar scrollbar;
	bool allowPopup = false;

	ScopedPointer<HelpPopup> helpPopup;
};




}