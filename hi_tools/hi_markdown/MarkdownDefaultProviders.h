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

#pragma once

namespace hise {
using namespace juce;

class MarkdownParser::DefaultLinkResolver : public LinkResolver
{
public:

	DefaultLinkResolver(MarkdownParser* parser_);

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("DefaultLinkResolver"); }
	String getContent(const MarkdownLink& ) override { return {}; }

	ResolveType getPriority() const override { return ResolveType::Fallback; }

	bool linkWasClicked(const MarkdownLink& url) override;
	LinkResolver* clone(MarkdownParser* newParser) const override;

	MarkdownParser* parser;
};

class MarkdownParser::FileLinkResolver : public LinkResolver
{
public:
	FileLinkResolver(const File& root_);;

	String getContent(const MarkdownLink& url) override;
	bool linkWasClicked(const MarkdownLink& url) override;
	ResolveType getPriority() const override { return ResolveType::FileBased; }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FileLinkResolver"); };
	LinkResolver* clone(MarkdownParser* /*p*/) const override { return new FileLinkResolver(root); }

	File root;
};


class MarkdownParser::FolderTocCreator : public LinkResolver
{
public:

	FolderTocCreator(const File& rootFile_);

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FolderTocCreator"); };

	ResolveType getPriority() const override { return ResolveType::FileBased; }

	String getContent(const MarkdownLink& url) override;
	LinkResolver* clone(MarkdownParser* parent) const override;

	File rootFile;
};

class MarkdownParser::GlobalPathProvider : public ImageProvider
{
public:

	struct GlobalPool
	{
		OwnedArray<PathFactory> factories;
	};

	constexpr static char path_wildcard[] = "/images/icon_";

	GlobalPathProvider(MarkdownParser* parent);;

	ResolveType getPriority() const override { return ResolveType::EmbeddedPath; };

	ImageProvider* clone(MarkdownParser* newParser) const override { return new GlobalPathProvider(newParser); }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("GlobalPathProvider"); };

	Image getImage(const MarkdownLink& urlName, float width) override;

	template <class T> void registerFactory()
	{
		ScopedPointer<T> newObj = new T();

		auto id = newObj->getId();

		for (auto f : factories->factories)
		{
			if (f->getId() == id)
				return;
		}

		factories->factories.add(newObj.release());
		factories->factories.getLast()->createPath("");
	}

	SharedResourcePointer<GlobalPool> factories;

};



class MarkdownParser::URLImageProvider : public ImageProvider
{
public:

	URLImageProvider(File tempdirectory_, MarkdownParser* parent);;

	Image getImage(const MarkdownLink& urlName, float width) override;

	ResolveType getPriority() const override { return ResolveType::WebBased; }

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("URLImageProvider"); };
	ImageProvider* clone(MarkdownParser* newParser) const { return new URLImageProvider(imageDirectory, newParser); }

	File imageDirectory;
};

class MarkdownParser::FileBasedImageProvider : public ImageProvider
{
public:

	static Image createImageFromSvg(Drawable* drawable, float width)
	{
		if (drawable != nullptr)
		{
			float maxWidth = jmax(10.0f, width);
			float height = drawable->getOutlineAsPath().getBounds().getAspectRatio(false) * maxWidth;

			Image img(Image::PixelFormat::ARGB, (int)maxWidth, (int)height, true);
			Graphics g(img);
			drawable->drawWithin(g, { 0.0f, 0.0f, maxWidth, height }, RectanglePlacement::centred, 1.0f);

			return img;
		}

		return {};
	}

	FileBasedImageProvider(MarkdownParser* parent, const File& root);;

	Image getImage(const MarkdownLink& imageURL, float width) override;

	ResolveType getPriority() const override { return ResolveType::FileBased; }

	ImageProvider* clone(MarkdownParser* newParent) const override { return new FileBasedImageProvider(newParent, r); }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FileBasedImageProvider"); }

	File r;
};

template <class FactoryType> class PathProvider : public MarkdownParser::ImageProvider
{
public:

	PathProvider(MarkdownParser* parent) :
		ImageProvider(parent)
	{};

	virtual Image getImage(const MarkdownLink& imageURL, float width) override
	{
		Path p = f.createPath(imageURL.toString(MarkdownLink::UrlFull));

		if (!p.isEmpty())
		{
			auto b = p.getBounds();
			auto r = (float)b.getWidth() / (float)b.getHeight();
			p.scaleToFit(0.0f, 0.0f, floor(width), floor(width / r), true);

			Image img(Image::PixelFormat::ARGB, (int)p.getBounds().getWidth(), (int)p.getBounds().getHeight(), true);
			Graphics g(img);
			g.setColour(parent->getStyleData().textColour);
			g.fillPath(p);

			return img;
		}
		else
			return {};
	}

	MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Autogenerated; }

	ImageProvider* clone(MarkdownParser* newParent) const override { return new PathProvider<FactoryType>(newParent); }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("PathProvider"); }

private:

	FactoryType f;
};


struct MarkdownCodeComponentBase : public Component,
	public ButtonListener
{
	
	
	enum SyntaxType
	{
		Undefined,
		Cpp,
		Javascript,
		LiveJavascript,
		LiveJavascriptWithInterface,
		EditableFloatingTile,
		XML,
		Snippet,
		ScriptContent,
		numSyntaxTypes
	};

	struct HtmlHelpers
	{
		static String createCodeBlock(SyntaxType syntax, String code);

		static String createSnapshot(SyntaxType syntax, String code);
	};


	struct Helpers
	{
		static bool createProcessor(SyntaxType t)
		{
			return t == LiveJavascriptWithInterface || t == LiveJavascript || t == ScriptContent;
		}

		static bool createContent(SyntaxType t)
		{
			return t == LiveJavascriptWithInterface || t == ScriptContent;
		}
	};

	struct Factory : public PathFactory
	{
		String getId() const override { return "Copy Icon"; }

		Path createPath(const String& url) const override
		{
			Path p;

			LOAD_PATH_IF_URL("copy", EditorIcons::pasteIcon);

			return p;
		}
	};

	Factory f;

	virtual ~MarkdownCodeComponentBase() {};

	virtual void addImageLinks(Array<MarkdownLink>& sa)
	{
		ignoreUnused(sa);
	};

	virtual String generateHtml() const;

	MarkdownCodeComponentBase(SyntaxType syntax_, String code, float width, float fontsize, MarkdownParser* parent);

	void createChildComponents()
	{
		addAndMakeVisible(editor);
		addAndMakeVisible(o);

		addAndMakeVisible(expandButton = new TextButton("Expand this code"));
		expandButton->setLookAndFeel(&blaf);
		expandButton->addListener(this);
	}

	virtual void initialiseEditor()
	{
		usedDocument = ownedDoc;

		editor = new CodeEditorComponent(*usedDocument, tok);

		if (syntax == Cpp)
		{
			struct Type
			{
				const char* name;
				uint32 colour;
			};

			const Type types[] =
			{
				{ "Error", 0xffBB3333 },
				{ "Comment", 0xff77CC77 },
				{ "Keyword", 0xffbbbbff },
				{ "Operator", 0xffCCCCCC },
				{ "Identifier", 0xffDDDDFF },
				{ "Integer", 0xffDDAADD },
				{ "Float", 0xffEEAA00 },
				{ "String", 0xffDDAAAA },
				{ "Bracket", 0xffFFFFFF },
				{ "Punctuation", 0xffCCCCCC },
				{ "Preprocessor Text", 0xffCC7777 }
			};

			CodeEditorComponent::ColourScheme cs;

			for (unsigned int i = 0; i < sizeof(types) / sizeof(types[0]); ++i)  // (NB: numElementsInArray doesn't work here in GCC4.2)
				cs.set(types[i].name, Colour(types[i].colour));

			editor->setColourScheme(cs);
		}

		

		editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
		editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
		editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
		editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
		editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
		editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(fontSize));
		editor->setReadOnly(true);
	}

	virtual int getPreferredHeight() const
	{
		return autoHideEditor() ? (2 * editor->getLineHeight()) : (getEditorHeight() + editor->getLineHeight());
	}

	virtual bool autoHideEditor() const
	{
		return !isExpanded && (usedDocument->getNumLines() > 20);
	}

	int getGutterWidth() const
	{
		return editor->getGutterComponent()->getWidth();
	}

	void buttonClicked(Button* b) override
	{
		if (b == expandButton)
		{
			isExpanded = true;
			updateHeightInParent();
			return;
		}
	}

	int getEditorHeight() const
	{
		int height = editor->getLineHeight() * (usedDocument->getNumLines() + 1);

		return height;
	}

	struct Overlay : public Component
	{
		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xAA262626));
		}
	};

	void updateHeightInParent();

	void resized() override
	{
		editor->setBounds(getLocalBounds());

		editor->scrollToLine(0);

		auto b = getLocalBounds();
		b.removeFromLeft(getGutterWidth());

		if (autoHideEditor())
		{
			o.setVisible(true);
			o.setBounds(b);
			expandButton->setVisible(true);
			expandButton->setBounds(b.withSizeKeepingCentre(130, editor->getLineHeight()));
		}
		else
		{
			o.setVisible(false);
			expandButton->setVisible(false);
		}
	}

	Overlay o;

	SyntaxType syntax;
	float fontSize;
	ScopedPointer<CodeDocument> ownedDoc;
	WeakReference<CodeDocument> usedDocument;
	ScopedPointer<CodeTokeniser> tok;
	ScopedPointer<CodeEditorComponent> editor;

	AlertWindowLookAndFeel blaf;
	ScopedPointer<TextButton> expandButton;

	bool isExpanded = false;
	MarkdownParser* parent = nullptr;
};


struct SnapshotMarkdownCodeComponent : public MarkdownCodeComponentBase
{
	SnapshotMarkdownCodeComponent(SyntaxType syntax, String code, float width, MarkdownParser* parent) :
		MarkdownCodeComponentBase(syntax, code, width, parent->getStyleData().fontSize, parent)
	{
		initialiseEditor();
		createChildComponents();

		if (syntax == MarkdownCodeComponentBase::EditableFloatingTile)
		{
			String link = "/images/floating-tile_";

			String s = JSON::parse(code).getProperty("Type", "").toString();

			link << s << ".png";
			l = { {}, link };
			l = l.withPostData(code);
		}
	}

	String generateHtml() const override;

	void addImageLinks(Array<MarkdownLink>& sa)
	{
		if (syntax == MarkdownCodeComponentBase::EditableFloatingTile)
		{
			sa.add(l);
		}
	}

	int getPreferredHeight() const override
	{
		if (syntax == MarkdownCodeComponentBase::EditableFloatingTile && screenshot.isNull())
		{
			screenshot = parent->resolveImage(l, MarkdownParser::DefaultLineWidth);
		}

		return jmax<int>(50, screenshot.getHeight());
	}

	void paint(Graphics& g) override
	{
		g.drawImageAt(screenshot, 0, 0);
	}

	void resized() override
	{
		editor->setVisible(false);
	}

	MarkdownLink l;
	mutable Image screenshot;

};

struct MarkdownCodeComponentFactory
{
	static MarkdownCodeComponentBase* createBaseEditor(MarkdownParser* parent, MarkdownCodeComponentBase::SyntaxType syntax, const String& code, float width)
	{
		ScopedPointer<MarkdownCodeComponentBase> newC = new MarkdownCodeComponentBase(syntax, code, width, parent->getStyleData().fontSize, parent);
		newC->initialiseEditor();
		newC->createChildComponents();
		return newC.release();
	}

	static MarkdownCodeComponentBase* createSnapshotEditor(MarkdownParser* parent, MarkdownCodeComponentBase::SyntaxType syntax, const String& code, float width)
	{
		return new SnapshotMarkdownCodeComponent(syntax, code, width, parent);
	}

#if HI_MARKDOWN_ENABLE_INTERACTIVE_CODE
	static MarkdownCodeComponentBase* createInteractiveEditor(MarkdownParser* parent, MarkdownCodeComponentBase::SyntaxType syntax, const String& code, float width);
#endif
};


}
