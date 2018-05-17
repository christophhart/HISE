/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef MARKDOWN_H_INCLUDED
#define MARKDOWN_H_INCLUDED

namespace hise {
using namespace juce;


class MarkdownParser
{
public:

	
	class ImageProvider
	{
	public:

		ImageProvider(MarkdownParser* parent_):
			parent(parent_)
		{}

        virtual ~ImageProvider() {};
        
		/** Overwrite this method and return an image for the given URL.
		*
		*	The default function just returns a placeholder.
		*/
		virtual Image getImage(const String& /*imageURL*/, float width)
		{
			Image img = Image(Image::PixelFormat::ARGB, (int)width, (int)width, true);
			Graphics g(img);
			g.fillAll(Colours::grey);
			g.setColour(Colours::black);
			g.drawRect(0.0f, 0.0f, width, width, 1.0f);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Empty", 0, 0, (int)width, (int)width, Justification::centred);
			return img;
		}

	protected:
		MarkdownParser * parent;
	};

	template <class FactoryType> class PathProvider: public ImageProvider
	{
	public:

		PathProvider(MarkdownParser* parent) :
			ImageProvider(parent)
		{};

		virtual Image getImage(const String& imageURL, float width) override
		{
			Path p = f.createPath(imageURL);

			if (!p.isEmpty())
			{
				auto b = p.getBounds();

				auto r = (float)b.getWidth() / (float)b.getHeight();

				p.scaleToFit(0.0f, 0.0f, floorf(width), floorf(width / r), true);

				Image img(Image::PixelFormat::ARGB, (int)p.getBounds().getWidth(), (int)p.getBounds().getHeight(), true);

				Graphics g(img);

				g.setColour(parent->textColour);
				g.fillPath(p);

				return img;
			}
			else
				return ImageProvider::getImage(imageURL, width);
		}

	private:

		FactoryType f;
	};

	MarkdownParser(const String& markdownCode);

	void setFonts(Font normalFont, Font codeFont, Font headlineFont, float defaultFontSize);

	void setTextColour(Colour c) { textColour = c; }

	template <class ProviderType> void setNewImageProvider()
	{
		imageProvider = new ProviderType(this);
	};

	template <class ProviderType> void setImageProvider(ProviderType* newProvider)
	{
		imageProvider = newProvider;
	};

	void parse();

	void setDefaultTextSize(float fontSize)
	{
		defaultFontSize = fontSize;
	}

	float getHeightForWidth(float width)
	{
		float height = 0.0f;

		for (auto* e : elements)
		{
			height += e->getHeightForWidth(width);
		}

		return height;
	}

	void draw(Graphics& g, Rectangle<float> area)
	{
		for (auto* e : elements)
		{
			auto heightToUse = e->getHeightForWidth(area.getWidth());
			auto ar = area.removeFromTop(heightToUse);

			e->draw(g, ar);
		}
	}

private:

	struct Iterator
	{
	public:

		Iterator(const String& text_) :
			text(text_),
			it(text.getCharPointer())
		{

		}

		juce_wchar peek()
		{
			if (it.isEmpty())
				return 0;

			return *(it);
		}

		bool advance(int numCharsToSkip=1)
		{
			it += numCharsToSkip;

			return !it.isEmpty();
		}

		bool advance(const String& stringToSkip)
		{
			return advance(stringToSkip.length());
		}



		bool next(juce::juce_wchar& c)
		{
			if (it.isEmpty())
				return false;

			c = *it++;

			return c != 0;

		}

		bool match(juce_wchar expected)
		{
			juce_wchar c;

			if (!next(c))
				return false;

			jassert(c == expected);

			return c == expected;
		}

		void skipWhitespace();

		String getRestString() const
		{
			if (it.isEmpty())
				return {};

			return String(it);
		}

	private:
		const String text;
		CharPointer_UTF8 it;
	};

	struct Element
	{
		Element(MarkdownParser* parent_) :
			parent(parent_)
		{};

		virtual ~Element() {};

		virtual void draw(Graphics& g, Rectangle<float> area)  = 0;

		virtual float getHeightForWidth(float width) = 0;

	protected:

		MarkdownParser* parent;
	};

	

	struct TextBlock;	struct Headline;		struct BulletPointList;		struct Comment;
	struct CodeBlock;	struct MarkdownTable;	struct ImageElement;

	struct CellContent
	{
		bool isEmpty() const
		{
			return imageURL.isEmpty() && s.getText().isEmpty();
		}

		AttributedString s;

		String imageURL;
	};

	using RowContent = Array<CellContent>;

	OwnedArray<Element> elements;

	struct Helpers
	{
		static bool isEndOfLine(juce_wchar c);
	};


	void parseLine();

	void resetForNewLine();

	void parseHeadline();

	void parseBulletList();

	void parseText();

	void parseBlock();

	void parseJavascriptBlock();

	void parseTable();

	ImageElement* parseImage();

	RowContent parseTableRow();

	bool isJavascriptBlock() const;

	bool isImageLink() const;

	void addCharacterToCurrentBlock(juce_wchar c);

	void resetCurrentBlock();

	void skipTagAndTrailingSpace();

	bool isBold = false;
	bool isItalic = false;
	bool isCode = false;

	float defaultFontSize = 17.0f;

	Colour textColour = Colours::black;

	Colour currentColour;

	Font normalFont;
	Font boldFont;
	Font codeFont;
	Font headlineFont;

	Font currentFont;
	
	Iterator it;

	ScopedPointer<ImageProvider> imageProvider;

	AttributedString currentlyParsedBlock;
	String markdownCode;

	void parseComment();
};

class HiseShapeButton : public ShapeButton
{
public:

	
	HiseShapeButton(const String& name, ButtonListener* listener, PathFactory& factory, const String& offName = String()) :
		ShapeButton(name, Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white)
	{
		onShape = factory.createPath(name);

		if (offName.isEmpty())
			offShape = onShape;
		else
			offShape = factory.createPath(offName);

		if (listener != nullptr)
			addListener(listener);

		refreshShape();
		refreshButtonColours();
	}


	void refreshButtonColours()
	{
		if (getToggleState())
		{
			setColours(Colour(SIGNAL_COLOUR).withAlpha(0.8f), Colour(SIGNAL_COLOUR), Colour(SIGNAL_COLOUR));
		}
		else
		{
			setColours(Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white);
		}
	}

	bool operator==(const String& id) const
	{
		return getName() == id;
	}

	void refreshShape()
	{
		if (getToggleState())
		{
			setShape(onShape, false, true, true);
		}
		else
			setShape(offShape, false, true, true);
	}

	void refresh()
	{
		refreshShape();
		refreshButtonColours();
	}

	void toggle()
	{
		setToggleState(!getToggleState(), dontSendNotification);

		refresh();
	}

	void setShapes(Path newOnShape, Path newOffShape)
	{
		onShape = newOnShape;
		offShape = newOffShape;
	}

	Path onShape;
	Path offShape;
};


class MarkdownHelpButton : public ShapeButton,
						   public ButtonListener,
						   public ComponentListener
{
public:

	enum AttachmentType
	{
		Overlay,
		OverlayLeft,
		TopRight,
		Left,
		numAttachmentTypes
	};

	MarkdownHelpButton();

	~MarkdownHelpButton()
	{
		if(ownerComponent != nullptr)
			ownerComponent->removeComponentListener(this);
	}

	template <class ProviderType=MarkdownParser::ImageProvider> void setHelpText(const String& markdownText)
	{
		parser = new MarkdownParser(markdownText);
		parser->setTextColour(Colours::white);
		parser->setDefaultTextSize(fontSizeToUse);
		parser->setNewImageProvider<ProviderType>();

		parser->parse();
	}

	void setPopupWidth(int newPopupWidth)
	{
		popupWidth = newPopupWidth;
	}

	void setFontSize(float fontSize)
	{
		fontSizeToUse = fontSize;
	}

	void buttonClicked(Button* b) override;

	void attachTo(Component* componentToAttach, AttachmentType attachmentType_)
	{
		if (ownerComponent != nullptr)
			ownerComponent->removeComponentListener(this);

		ownerComponent = componentToAttach;
		attachmentType = attachmentType_;

		if (ownerComponent != nullptr)
		{
			jassert(getParentComponent() == nullptr);

			if (auto parent = ownerComponent->getParentComponent())
			{
				parent->addAndMakeVisible(this);
			}
			else
				jassertfalse; // You tried to attach a help button to a component without a parent...

			setVisible(ownerComponent->isVisible());
			ownerComponent->addComponentListener(this);
			componentMovedOrResized(*ownerComponent, true, true);
		}
	}

	void componentMovedOrResized(Component& c, bool /*wasMoved*/, bool /*wasResized*/) override
	{
		auto cBounds = c.getBoundsInParent();

		switch (attachmentType)
		{
		case Overlay:
		{
			setBounds(cBounds.withSizeKeepingCentre(16, 16));
			break;
		}
		case OverlayLeft:
		{
			auto square = cBounds.removeFromLeft(20);

			setBounds(square.withSizeKeepingCentre(16, 16));

			break;
		}
		case Left:
		{
			setBounds(cBounds.getX() - 20, cBounds.getY() + 2, 16, 16);
			break;
		}
		case TopRight:
		{
			Rectangle<int> r( cBounds.getRight() - 16, cBounds.getY() - 16, 16, 16 );
			setBounds(r);
		}
        default:
                break;
		}
	}

	void componentVisibilityChanged(Component& c) override
	{
		setVisible(c.isVisible());
	}

	void setIgnoreKeyStrokes(bool shouldIgnoreKeyStrokes)
	{
		setWantsKeyboardFocus(shouldIgnoreKeyStrokes);
		ignoreKeyStrokes = shouldIgnoreKeyStrokes;
		
	}

	void componentBeingDeleted(Component& component) override
	{
		component.removeComponentListener(this);

		getParentComponent()->removeChildComponent(this);

		delete this;
	}

	static Path getPath();

private:

	bool ignoreKeyStrokes = false;

	float fontSizeToUse = 17.0f;

	Component::SafePointer<CallOutBox> currentPopup;

	ScopedPointer<MarkdownParser> parser;

	int popupWidth = 400;

	Component::SafePointer<Component> ownerComponent;

	AttachmentType attachmentType;

	struct MarkdownHelp;

	
	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownHelpButton);
};


}

#endif