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


/** a simple markdown parser that renders markdown formatted code. */
class MarkdownParser
{
public:
	
    struct LayoutCache
    {
        void clear() { cachedLayouts.clear(); }
        
        const TextLayout& getLayout(const AttributedString& s, float w);
        
    private:
        
        struct Layout
        {
            Layout(const AttributedString& s, float w);
            
            TextLayout l;
            int64 hashCode;
            float width;
            
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Layout);
        };
        
        OwnedArray<Layout> cachedLayouts;
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(LayoutCache);
    };
        
	struct HyperLink
	{
		bool valid = false;
		Rectangle<float> area = {};
		String url = {};
		String tooltip;
		Range<int> urlRange = {};
	};

	struct Listener
	{
		virtual ~Listener() {};
		virtual void markdownWasParsed(const Result& r) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	struct LinkResolver
	{
		virtual ~LinkResolver() {};
		virtual String getContent(const String& url) = 0;
	};

	struct FileLinkResolver: public LinkResolver
	{
		FileLinkResolver(const File& root_) :
			root(root_)
		{};

		String getContent(const String& url) override;

		File root;
	};

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
		virtual Image getImage(const String& /*imageURL*/, float width);

	protected:
		MarkdownParser * parent;
	};

	class FileBasedImageProvider : public ImageProvider
	{
	public:

		FileBasedImageProvider(MarkdownParser* parent, const File& root);;

		Image getImage(const String& imageURL, float width) override;

		File r;
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
				return {};
		}

	private:

		FactoryType f;
	};

	MarkdownParser(const String& markdownCode, LayoutCache* cToUse=nullptr);

	void setFonts(Font normalFont, Font codeFont, Font headlineFont, float defaultFontSize);

	void setTextColour(Colour c) { textColour = c; }

	String resolveLink(const String& url)
	{
		for (auto lr : linkResolvers)
		{
			auto link = lr->getContent(url);

			if (link.isNotEmpty())
				return link;
		}

		return {};
	}

	Image resolveImage(const String& imageUrl, float width)
	{
		for (int i = imageProviders.size()-1; i >= 0; i--)
		{
			auto img = imageProviders[i]->getImage(imageUrl, width);
			
			if (img.isValid())
				return img;
		}

		return {};
	}

	void setLinkResolver(LinkResolver* ownedResolver)
	{
		linkResolvers.add(ownedResolver);
	}

	template <class ProviderType> void setNewImageProvider()
	{
		imageProviders.add(new ProviderType(this));
	};

	template <class ProviderType> void setImageProvider(ProviderType* newProvider)
	{
		imageProviders.add(newProvider);
	};

	void parse();

	Result getParseResult() const { return currentParseResult; }

	void setDefaultTextSize(float fontSize);

	float getHeightForWidth(float width);

	void draw(Graphics& g, Rectangle<float> area) const;

	bool canNavigate(bool back) const
	{
		if (back)
			return historyIndex < history.size();
		else
			return historyIndex < (history.size() - 1);
	}

	void navigate(bool back)
	{
		if (back)
			historyIndex = jmax<int>(historyIndex - 1, 0);
		else
			historyIndex = jmin<int>(historyIndex + 1, history.size() - 1);

		setNewText(history[historyIndex]);
	}

	void setNewText(const String& newText);
	bool gotoLink(const MouseEvent& event, Rectangle<float> area);

	HyperLink getHyperLinkForEvent(const MouseEvent& event, Rectangle<float> area);

	void addListener(Listener* l) { listeners.addIfNotAlreadyThere(l); }
	void removeListener(Listener* l) { listeners.removeAllInstancesOf(l); }

	struct SnippetTokeniser : public CodeTokeniser
	{
		SnippetTokeniser() {};

		int readNextToken(CodeDocument::Iterator& source) override;
		CodeEditorComponent::ColourScheme getDefaultColourScheme();
	};

	struct Tokeniser: public CodeTokeniser
	{
		Tokeniser() {};

		int readNextToken(CodeDocument::Iterator& source);
		CodeEditorComponent::ColourScheme getDefaultColourScheme();
	};

	const TextLayout& getTextLayoutForString(const AttributedString& s, float width);

private:

    WeakReference<LayoutCache> layoutCache;
    TextLayout uncachedLayout;
    
	class Iterator
	{
	public:

		Iterator(const String& text_);

		juce_wchar peek();
		bool advanceIfNotEOF(int numCharsToSkip = 1);
		bool advance(int numCharsToSkip = 1);
		bool advance(const String& stringToSkip);
		bool next(juce::juce_wchar& c);
		bool match(juce_wchar expected);
		void skipWhitespace();
		String getRestString() const;

	private:

		String text;
		CharPointer_UTF8 it;
	};

	struct Element
	{
		Element(MarkdownParser* parent_) :
			parent(parent_)
		{
			hyperLinks.insertArray(0, parent->currentLinks.getRawDataPointer(), parent->currentLinks.size());
		};

		virtual ~Element() {};

		virtual void draw(Graphics& g, Rectangle<float> area)  = 0;

		virtual float getHeightForWidth(float width) = 0;

		virtual int getTopMargin() const = 0;

		Array<HyperLink> hyperLinks;

		float getHeightForWidthCached(float width)
		{
			if (width != lastWidth)
			{
				cachedHeight = getHeightForWidth(width);
				lastWidth = width;
				return cachedHeight;
			}

			return cachedHeight;
		}

		static void recalculateHyperLinkAreas(TextLayout& l, Array<HyperLink>& links, float topMargin);

	protected:

		MarkdownParser* parent;

		float lastWidth = -1.0f;
		float cachedHeight = 0.0f;
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
		Array<HyperLink> cellLinks;
	};

	using RowContent = Array<CellContent>;

	OwnedArray<Element> elements;

	struct Helpers
	{
		static bool isNewElement(juce_wchar c);
		static bool isEndOfLine(juce_wchar c);
		static bool isNewToken(juce_wchar c, bool isCode);
		static bool belongsToTextBlock(juce_wchar c, bool isCode, bool stopAtLineEnd);
	};

	void resetForNewLine();

	void parseLine();
	void parseHeadline();
	void parseBulletList();
	void parseText(bool stopAtEndOfLine=true);
	void parseBlock();
	void parseJavascriptBlock();
	void parseTable();
	void parseComment();
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
	
	String markdownCode;
	Iterator it;
	Result currentParseResult;
	OwnedArray<ImageProvider> imageProviders;
	AttributedString currentlyParsedBlock;
	Array<HyperLink> currentLinks;
	
	OwnedArray<LinkResolver> linkResolvers;
	Array<WeakReference<Listener>> listeners;
	
	StringArray history;
	int historyIndex;
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
		OverlayRight,
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

	void setup()
	{
		parser = new MarkdownParser("");
		parser->setTextColour(Colours::white);
		parser->setDefaultTextSize(fontSizeToUse);
	}

	MarkdownParser* getParser() { return parser; }

	void addImageProvider(MarkdownParser::ImageProvider* newImageProvider)
	{
		if (parser != nullptr)
		{
			parser->setImageProvider(newImageProvider);
		}
		else
			jassertfalse; // you need to call setup before that.
	}

	template <class ProviderType=MarkdownParser::ImageProvider> void setHelpText(const String& markdownText)
	{
		if (parser == nullptr)
			setup();

		parser->setNewText(markdownText);
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
		case OverlayRight:
		{
			auto square = cBounds.removeFromRight(20);

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
