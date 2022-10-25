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

class SearchBoxComponent : public Component,
					       public KeyListener,
						   public juce::TextEditor::Listener,
						   public Button::Listener
{
public:

	SearchBoxComponent(TextDocument& d, float scaleFactor) :
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

	~SearchBoxComponent()
	{
		doc.setSearchResults({});

		sendSearchChangeMessage();
	}

	void buttonClicked(Button* b) override
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

	void sendSearchChangeMessage()
	{
		for (auto l : listeners)
		{
			if (l.get() != nullptr)
				l->searchItemsChanged();
		}
	}

	struct Listener
	{
        virtual ~Listener() {};
		virtual void searchItemsChanged() {};

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	void textEditorTextChanged(juce::TextEditor& t) override
	{
		auto input = searchField.getText();
		setSearchInput(input);
	}

	void setSearchInput(const String& text)
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

	bool keyPressed(const KeyPress& k, Component* c) override
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

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void paint(Graphics& g) override
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

	Font getResultFont() const
	{
		return laf.f.withHeight(laf.f.getHeight() * 0.8f).boldened();
	}

	void resized() override
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
	
	struct Blaf : public LookAndFeel_V3
	{
		void drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
		{
			float alpha = 0.4f;

			if (shouldDrawButtonAsHighlighted)
				alpha += 0.1f;

			if (shouldDrawButtonAsDown)
				alpha += 0.2f;

			g.setColour(Colours::white.withAlpha(alpha));
			g.fillRoundedRectangle(b.getLocalBounds().toFloat().reduced(1.0f, 1.0f), 2.0f);
		}

		void drawButtonText(Graphics& g, TextButton& b, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

		Font f;
	};

	TextDocument& doc;

	Blaf laf;

	juce::TextEditor searchField;

	TextButton find;
	TextButton prev;
	TextButton findAll;

	Array<WeakReference<Listener>> listeners;
};


//==============================================================================
class HighlightComponent : public juce::Component,
								public FoldableLineRange::Listener
{
public:
	HighlightComponent(TextDocument& document);
	~HighlightComponent();
	void setViewTransform(const juce::AffineTransform& transformToUse);
	void updateSelections();

	//==========================================================================
	void paintHighlight(juce::Graphics& g);

	void foldStateChanged(FoldableLineRange::WeakPtr p)
	{
		updateSelections();
	}

	static juce::Path getOutlinePath(const TextDocument& doc, const Selection& rectangles);

private:
	

	RectangleList<float> outlineRects;

	//==========================================================================
	bool useRoundedHighlight = true;
	TextDocument& document;
	juce::AffineTransform transform;
	juce::Path outlinePath;
};

}
