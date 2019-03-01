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
	
	struct TableOfContent
	{
		int getMaxWidth(Font f)
		{
			int maxWidth = 0;

			for (const auto& e : entries)
			{
				maxWidth = jmax<int>(maxWidth, f.getStringWidth(e.title) + e.hierarchy * 10 + 10);
			}

			return maxWidth;
		}

		struct Entry
		{
			int hierarchy;
			String title;
			String anchorId;
		};

		Array<Entry> entries;
	};

    struct LayoutCache
    {
        void clear() { cachedLayouts.clear(); }
        
        const MarkdownLayout& getLayout(const AttributedString& s, float w);
        
    private:
        
        struct Layout
        {
            Layout(const AttributedString& s, float w);
            
            MarkdownLayout l;
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
		String displayString;
		Range<int> urlRange = {};
	};

	struct Listener
	{
		virtual ~Listener() {};
		virtual void markdownWasParsed(const Result& r) = 0;

		virtual void scrollToAnchor(float v) {};

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	struct LinkResolver
	{
		virtual ~LinkResolver() {};
		virtual String getContent(const String& url) = 0;
		virtual LinkResolver* clone(MarkdownParser* parent) const = 0;
		virtual Identifier getId() const = 0;
		
		/** Overwrite this method and do something if the url matches, then return 
		    true or false whether the resolver consumed this event. 
		*/
		virtual bool linkWasClicked(const String& url) 
		{ 
			return false; 
		}
	};

	struct DefaultLinkResolver : public LinkResolver
	{
		DefaultLinkResolver(MarkdownParser* parser_):
			parser(parser_)
		{

		}

		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("DefaultLinkResolver"); }

		String getContent(const String& url) override { return {}; }

		bool linkWasClicked(const String& url) override;

		LinkResolver* clone(MarkdownParser* newParser) const override
		{
			return new DefaultLinkResolver(newParser);
		}

		MarkdownParser* parser;
	};

	struct FileLinkResolver: public LinkResolver
	{
		FileLinkResolver(const File& root_) :
			root(root_)
		{};

		String getContent(const String& url) override;

		bool linkWasClicked(const String& url) override;

		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FileLinkResolver"); };
		LinkResolver* clone(MarkdownParser* p) const override { return new FileLinkResolver(root); }

		File root;
	};

	
	class ImageProvider
	{
	public:

		bool operator==(const ImageProvider& other) const
		{
			return getId() == other.getId();
		}

		ImageProvider(MarkdownParser* parent_):
			parent(parent_)
		{}

        virtual ~ImageProvider() {};
        
		/** Overwrite this method and return an image for the given URL.
		*
		*	The default function just returns a placeholder.
		*/
		virtual Image getImage(const String& /*imageURL*/, float width);
		virtual ImageProvider* clone(MarkdownParser* newParent) const { return new ImageProvider(newParent); }
		virtual Identifier getId() const { RETURN_STATIC_IDENTIFIER("EmptyImageProvider"); };

		/** Helper function that makes sure that the image doesn't exceed the given width. */
		static Image resizeImageToFit(const Image& otherImage, float width)
		{
			if (width == 0.0)
				return {};

			if (otherImage.isNull() || otherImage.getWidth() < (int)width)
				return otherImage;

			float ratio = (float)otherImage.getWidth() / width;
			return otherImage.rescaled((int)width, (int)((float)otherImage.getHeight() / ratio));
		}

	protected:
		MarkdownParser * parent;
	};

	class URLImageProvider : public ImageProvider
	{
	public:

		URLImageProvider(File tempdirectory_, MarkdownParser* parent) :
			ImageProvider(parent),
			tempDirectory(tempdirectory_)
		{
			if (!tempDirectory.isDirectory())
				tempDirectory.createDirectory();
		};

		Image getImage(const String& urlName, float width) override
		{
			if (URL::isProbablyAWebsiteURL(urlName))
			{
				URL url(urlName);

				auto path = url.getSubPath();
				auto imageFile = tempDirectory.getChildFile(path);

				if (imageFile.existsAsFile())
					return resizeImageToFit(ImageCache::getFromFile(imageFile), width);

				imageFile.create();

				ScopedPointer<URL::DownloadTask> task = url.downloadToFile(imageFile);

				if (task == nullptr)
				{
					jassertfalse;
					return {};
				}

				int timeout = 5000;

				auto start = Time::getApproximateMillisecondCounter();

				while (!task->isFinished())
				{
					if (Time::getApproximateMillisecondCounter() - start > timeout)
						break;

					Thread::sleep(500);
				}
				
				if (task->isFinished() && !task->hadError())
				{
					return resizeImageToFit(ImageCache::getFromFile(imageFile), width);
				}

				return {};
			}

			return {};
		}

		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("URLImageProvider"); };
		ImageProvider* clone(MarkdownParser* newParser) { return new URLImageProvider(tempDirectory, newParser); }

		File tempDirectory;
	};

	class HiseDocImageProvider: public ImageProvider
	{
	public:

		HiseDocImageProvider(MarkdownParser* parent) :
			ImageProvider(parent)
		{};
			
		Image getImage(const String& urlName, float width) override;

		ImageProvider* clone(MarkdownParser* newParent) const override { return new HiseDocImageProvider(newParent); }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("HiseDocImageProvider"); }
	};

	class FolderTocCreator : public LinkResolver
	{
	public:

		FolderTocCreator(const File& rootFile_) :
			LinkResolver(),
			rootFile(rootFile_)
		{

		}

		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FolderTocCreator"); };

		String getContent(const String& url) override;

		LinkResolver* clone(MarkdownParser* parent) const override
		{
			return new FolderTocCreator(rootFile);
		}

		File rootFile;
	};

	class FileBasedImageProvider : public ImageProvider
	{
	public:

		FileBasedImageProvider(MarkdownParser* parent, const File& root);;

		Image getImage(const String& imageURL, float width) override;
		ImageProvider* clone(MarkdownParser* newParent) const override { return new FileBasedImageProvider(newParent, r); }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FileBasedImageProvider"); }

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

				g.setColour(parent->styleData.textColour);
				g.fillPath(p);

				return img;
			}
			else
				return {};
		}

		ImageProvider* clone(MarkdownParser* newParent) const override { return new PathProvider<FactoryType>(newParent); }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("PathProvider"); }

	private:

		FactoryType f;
	};

	MarkdownParser(const String& markdownCode, LayoutCache* cToUse=nullptr);

	~MarkdownParser() 
	{ 
		setTargetComponent(nullptr);

		elements.clear();
		linkResolvers.clear();
		imageProviders.clear();
	}

	void setFonts(Font normalFont, Font codeFont, Font headlineFont, float defaultFontSize);

	void setTextColour(Colour c) { styleData.textColour = c; }

	String resolveLink(const String& url);

	Image resolveImage(const String& imageUrl, float width);

	void setLinkResolver(LinkResolver* ownedResolver)
	{
		ScopedPointer<LinkResolver> owned = ownedResolver;

		for (auto r : linkResolvers)
		{
			if (r->getId() == owned->getId())
				return;
		}

		linkResolvers.add(owned.release());
	}

	template <class ProviderType> void setNewImageProvider()
	{
		ScopedPointer<ImageProvider> newProvider = new ProviderType(this);

		for (auto p : imageProviders)
		{
			if (*p == *newProvider)
				return;
		}

		imageProviders.add(newProvider.release());
	};

	template <class ProviderType> void setImageProvider(ProviderType* newProvider)
	{
		ScopedPointer<ImageProvider> owned = newProvider;

		for (auto p : imageProviders)
		{
			if (*p == *newProvider)
				return;
		}

		imageProviders.add(owned.release());
	};

	void parse();

	Result getParseResult() const { return currentParseResult; }

	void setDefaultTextSize(float fontSize);

	float getHeightForWidth(float width);

	void draw(Graphics& g, Rectangle<float> totalArea, Rectangle<int> viewedArea = {}) const;

	TableOfContent createTableOfContent() const;

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

	String getCurrentText() const;

	String getLastLink(bool includeLink, bool includeAnchor) const 
	{ 
		String l;

		if (includeLink)
			l << lastLink;

		if (includeAnchor)
			l << lastAnchor;

		return l;
	}

	RectangleList<float> searchInContent(const String& searchString)
	{
		RectangleList<float> positions;

		float y = 0.0f;

		for (auto e : elements)
		{
			e->searchInContent(searchString);
			
			y += e->getTopMargin();

			for (auto r : e->searchResults)
			{
				positions.add(r.translated(0.0f, y));
			}

			y += e->getLastHeight();
		}

		return positions;
	}

	String getSelectionContent() const
	{
		String s;

		for (auto e : elements)
		{
			if (e->selected)
			{
				s << e->getTextToCopy() << "\n";
			}
		}

		return s;
	}

	void updateSelection(Rectangle<float> area)
	{
		Range<float> yRange(area.getY(), area.getBottom());

		float y = 0.0f;

		for (auto e : elements)
		{
			float h = e->getTopMargin() + e->getLastHeight();

			e->setSelected(Range<float>(y, y + h).intersects(yRange));

			y += h;
		}
	}

	void setNewText(const String& newText);
	bool gotoLink(const MouseEvent& event, Rectangle<float> area);

	bool gotoLink(const String& url);

	String getAnchorForY(int y) const;

	HyperLink getHyperLinkForEvent(const MouseEvent& event, Rectangle<float> area);

	static void createDatabaseEntriesForFile(File root, MarkdownDataBase::Item& item, File f, Colour c);

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

	const MarkdownLayout& getTextLayoutForString(const AttributedString& s, float width);

	void setStyleData(MarkdownLayout::StyleData newStyleData)
	{
		styleData = newStyleData;

		if (markdownCode.isNotEmpty())
		{
			setNewText(markdownCode);
		}
	}

	void setTargetComponent(Component* newTarget)
	{
		if (targetComponent == newTarget)
			return;

		if (auto existing = targetComponent.getComponent())
		{
			for (auto e : elements)
			{
				if(auto c = e->createComponent(existing->getWidth()))
					existing->removeChildComponent(c);
			}
		}

		targetComponent = newTarget;
	}

	void updateCreatedComponents()
	{
		if (targetComponent == nullptr)
			return;

		float y = 0.0f;

		for (auto e : elements)
		{
			y += e->getTopMargin();

			if (auto c = e->createComponent(targetComponent->getWidth()))
			{
				if(c->getParentComponent() == nullptr)
					targetComponent->addAndMakeVisible(c);

				jassert(c->getWidth() > 0);
				jassert(c->getHeight() > 0);

				c->setTopLeftPosition(0, (int)y);
			}
			
			y += e->getLastHeight();
		}
	}

private:

	String lastLink;
	String lastAnchor;

    WeakReference<LayoutCache> layoutCache;
    MarkdownLayout uncachedLayout;
    
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

		virtual Component* createComponent(int maxWidth) { return nullptr; }

		virtual String getTextForRange(Range<int> range) const
		{
			jassertfalse;
			return {};
		}

		void drawHighlight(Graphics& g, Rectangle<float> area)
		{
			if (selected)
			{
				g.setColour(parent->styleData.backgroundColour.contrasting().withAlpha(0.05f));
				g.fillRoundedRectangle(area, 3.0f);
			}

			for (auto r : searchResults)
			{
				g.setColour(Colours::red.withAlpha(0.5f));

				auto r_t = r.translated(area.getX(), area.getY());
				g.fillRoundedRectangle(r_t, 3.0f);
				g.drawRoundedRectangle(r_t, 3.0f, 1.0f);
			}
		};

		

		virtual void searchInContent(const String& s) { }

		Array<HyperLink> hyperLinks;

		float getLastHeight()
		{
			return cachedHeight;
		}

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

		static void recalculateHyperLinkAreas(MarkdownLayout& l, Array<HyperLink>& links, float topMargin);

		void setSelected(bool isSelected)
		{
			selected = isSelected;
		}

		virtual String getTextToCopy() const = 0;

		bool selected = false;
		RectangleList<float> searchResults;

	protected:

		static Array<Range<int>> getMatchRanges(const String& fullText, const String& searchString, bool countNewLines);

		void searchInStringInternal(const AttributedString& textToSearch, const String& searchString);

		MarkdownParser* parent;

		float lastWidth = -1.0f;
		float cachedHeight = 0.0f;
	};

	struct TextBlock;	struct Headline;		struct BulletPointList;		struct Comment;
	struct CodeBlock;	struct MarkdownTable;	struct ImageElement;		struct EnumerationList;
	struct ActionButton;

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
	void parseEnumeration();

	void parseText(bool stopAtEndOfLine=true);
	void parseBlock();
	void parseJavascriptBlock();
	void parseTable();
	void parseButton();
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

#if 0
	float defaultFontSize = 17.0f;
	Colour textColour = Colours::black;
	

	Font normalFont;
	Font boldFont;
	Font codeFont;
	Font headlineFont;
	
#endif
	
	Colour currentColour;
	Font currentFont;

	MarkdownLayout::StyleData styleData;

	String markdownCode;
	Iterator it;
	Result currentParseResult;
	OwnedArray<ImageProvider> imageProviders;
	AttributedString currentlyParsedBlock;
	Array<HyperLink> currentLinks;
	
	Array<Component::SafePointer<Component>> createdComponents;
	Component::SafePointer<Component> targetComponent;

	OwnedArray<LinkResolver> linkResolvers;
	Array<WeakReference<Listener>> listeners;
	
	StringArray history;
	int historyIndex;
	mutable bool firstDraw = true;
	float lastHeight = -1.0f;
	float lastWidth = -1.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownParser);
};




}

#endif
