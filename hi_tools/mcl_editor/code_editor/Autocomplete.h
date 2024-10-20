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
						public AsyncUpdater,
					    public ReferenceCountedObject
{
public:

	using Ptr = ReferenceCountedObjectPtr<TokenCollection>;

	static Array<Range<int>> getSelectionFromFunctionArgs(const String& input);

	/** A Token is the entry that is being used in the autocomplete popup (or any other IDE tools
	    that might use that database. */
	struct Token: public ReferenceCountedObject
	{
		Token(const String& text);;

		virtual ~Token();;

		/** Override the method and check whether the currently written input matches the token. */
		virtual bool matches(const String& input, const String& previousToken, int lineNumber) const;

		static bool matchesInput(const String& input, const String& code);

		virtual bool equals(const Token* other) const;

		bool operator==(const Token& other) const;

		

		virtual Array<Range<int>> getSelectionRangeAfterInsert(const String& input) const
		{
			return getSelectionFromFunctionArgs(getCodeToInsert(input));
		}

		virtual MarkdownLink getLink() const;;

		/** Override this method if you want to customize the code that is about to be inserted. */
		virtual String getCodeToInsert(const String& input) const;

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

	
	using List = ReferenceCountedArray<Token>;
	using TokenPtr = ReferenceCountedObjectPtr<Token>;

	TokenCollection::List getTokens() const
	{
		List l;

		SimpleReadWriteLock::ScopedReadLock sl(buildLock);
		l.addArray(tokens);
		return l;
	}

	/** A provider is a class that adds its tokens to the given list. 
	
		In order to use it, subclass from this and override the addTokens() method.
		
		Then whenever you want to rebuild the token list, call the signalRebuild method
		to notify the registered TokenCollection to rebuild its tokens.
	*/
	struct Provider
	{
		virtual ~Provider();

		/** Override this method and add all tokens to the given list. This method will be called on a background thread
		    to keep the UI thread responsive, so make sure you do not interfere with the message thread for longer than necessary. 
		*/
		virtual void addTokens(List& tokens) = 0;

        virtual bool shouldAbortTokenRebuild(Thread* t) const
        {
            return t != nullptr && t->threadShouldExit();
        }

		/** Call the TokenCollections rebuild method. This will not be executed synchronously, but on a dedicated thread. */
		void signalRebuild();

		void signalClear(NotificationType n);

		WeakReference<TokenCollection> assignedCollection;
	};

	/** A Listener interface that will be notified whenever the token list was rebuilt. */
	struct Listener
	{
		virtual ~Listener();;

		/** This method will be called on the message thread after the list was rebuilt. */
		virtual void tokenListWasRebuild() = 0;

		/** This method will be called synchronously indicating the state of the token rebuild process. */
		virtual void threadStateChanged(bool isRunning) {};

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	void setEnabled(bool shouldBeEnabled, bool isDirty);

	void signalRebuild();

	void signalClear(NotificationType n);

	void run() override;

	void clearTokenProviders();

	bool hasTokenProviders() const { return !tokenProviders.isEmpty(); }

	void updateIfSync()
	{
		if(!useBackgroundThread)
		{
			dirty = true;
			rebuild();
		}
	}

	void setUseBackgroundThread(bool shouldUseBackgroundThread)
	{
		useBackgroundThread = shouldUseBackgroundThread;
	}

	/** Register a token provider to this instance. Be aware that you can't register a token provider to multiple instances,
	    but this shouldn't be a problem. */
	void addTokenProvider(Provider* ownedProvider);

	TokenCollection(const Identifier& languageId);

	Identifier getLanguageId() const { return languageId;};

	~TokenCollection();

	bool hasEntries(const String& input, const String& previousToken, int lineNumber) const;

	void addListener(Listener* l);

	void removeListener(Listener* l);

	void handleAsyncUpdate() override;

	static int64 getHashFromTokens(const List& l);

	void rebuild();

	bool isEnabled() const;


	struct Sorter
	{
        struct AbortException: public std::exception
        {};
        
        Sorter(TokenCollection& t):
          parent(t)
        {};
        
		int compareElements(Token* first, Token* second) const;
        
        TokenCollection& parent;
	};

    struct FuzzySorter
    {
        FuzzySorter(const String& e);;
        
        int compareElements(Token* first, Token* second) const;

        String exactSearchTerm;
    };

    void sortForInput(const String& input);

    bool shouldAbort() const
    {
        for(auto p: tokenProviders)
        {
            if(p->shouldAbortTokenRebuild(const_cast<TokenCollection*>(this)))
               return true;
        }

        return false;
    }
    
private:

	Identifier languageId;

	bool rebuildPending = false;

	bool enabled = true;
	OwnedArray<Provider> tokenProviders;
	Array<WeakReference<Listener>> listeners;
	List tokens;
	int64 currentHash;
	std::atomic<bool> dirty = { false };

	mutable SimpleReadWriteLock buildLock;

	bool useBackgroundThread = true;

	JUCE_DECLARE_WEAK_REFERENCEABLE(TokenCollection);
};

/** A TokenCollection::Provider subclass that scans the current document and creates a list of all tokens. */
struct SimpleDocumentTokenProvider : public TokenCollection::Provider,
									 public CoallescatedCodeDocumentListener,
							         public Timer
{
	SimpleDocumentTokenProvider(CodeDocument& doc);

	void timerCallback() override;

	void codeChanged(bool, int, int) override;

	static void addTokensStatic(TokenCollection::List& tokens, const CodeDocument& doc);

	void addTokens(TokenCollection::List& tokens) override;
};

/** A code snippet provider that lets you define custom code snippets from multiple JSON files */
struct CodeSnippetProvider: public mcl::TokenCollection::Provider
{
	CodeSnippetProvider(const Array<File>& snippetFiles_, const Identifier& languageId_, const std::function<void(const String&)>& errorFunction_):
	  Provider(),
	  snippetFiles(snippetFiles_),
	  languageId(languageId_),
	  errorFunction(errorFunction_)
	{}

	virtual ~CodeSnippetProvider() {};

	struct CodeSnippetToken;

	void addTokens(mcl::TokenCollection::List& tokens) override;

private:

	Identifier getLanguageId() const { return languageId; }

	Array<File> getSnippetFiles() { return snippetFiles; }

	void reportParsingError(const String& errorMessage)
	{
		if(errorFunction)
			errorFunction(errorMessage);
	}

	std::function<void(const String&)> errorFunction;
	const Identifier languageId;
	Array<File> snippetFiles;
};

class Autocomplete : public Component,
	public KeyListener,
	public ScrollBar::Listener
{
public:

	using Token = TokenCollection::Token;

	struct ParameterSelection: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<ParameterSelection>;
		using List = ReferenceCountedArray<ParameterSelection>;

		ParameterSelection(TextDocument& doc, int start, int end);

		Selection getSelection() const;

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

		HelpPopup(Autocomplete* p);

		~HelpPopup();

		void componentMovedOrResized(Component& component, bool , bool );

		void refreshText();

		void resized() override;

		void paint(Graphics& g) override;

		Autocomplete* ac;
		SimpleMarkdownDisplay display;
		
	};

	struct Item : public Component
	{
		Item(TokenCollection::TokenPtr t, const String& input_);

		void mouseUp(const MouseEvent& e) override;

		AttributedString createDisplayText() const;

		void paint(Graphics& g) override;

		bool isSelected();

		TokenCollection::TokenPtr token;
		String input;
	};

	Autocomplete(TokenCollection::Ptr tokenCollection_, const String& input, const String& previousToken, int lineNumber, TextEditor* editor_);

	~Autocomplete();


	juce::DropShadower shadow;

	bool keyPressed(const KeyPress& key, Component*);

	void cancel();

	Array<Range<int>> getSelectionRange(const String& input) const;

	String getCurrentText() const;

	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

	void selectNextItem(bool showNext, int delta = 1);

	void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;

	void setDisplayedIndex(int index);

	Item* createItem(const TokenCollection::TokenPtr t, const String& input);

	bool isSingleMatch() const;

	void setInput(const String& input, const String& previousToken, int lineNumber);

	int getRowHeight() const;

	void paint(Graphics& g) override;


	void paintOverChildren(Graphics& g) override;

	int getNumDisplayedRows() const;

	float getScaleFactor() const;

	void resized() override;

	OwnedArray<Item> items;
	int viewIndex = 0;

	Range<int> displayedRange;
	String currentInput;

	TokenCollection::List currentList;

	TokenCollection::Ptr tokenCollection;
	ScrollBar scrollbar;
	ScrollbarFader fader;
	bool allowPopup = false;

	ScopedPointer<HelpPopup> helpPopup;

	WeakReference<TextEditor> editor;
};




}
