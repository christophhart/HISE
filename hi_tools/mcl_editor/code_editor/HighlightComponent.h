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

	SearchBoxComponent(TextDocument& d, float scaleFactor);

	~SearchBoxComponent();

	void buttonClicked(Button* b) override;

	void sendSearchChangeMessage();

	struct Listener
	{
        virtual ~Listener();;
		virtual void searchItemsChanged();;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	void textEditorTextChanged(juce::TextEditor& t) override;

	void setSearchInput(const String& text);

	bool keyPressed(const KeyPress& k, Component* c) override;

	void addListener(Listener* l);

	void removeListener(Listener* l);

	void paint(Graphics& g) override;

	Font getResultFont() const;

	void resized() override;

	struct Blaf : public LookAndFeel_V3
	{
		void drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

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
