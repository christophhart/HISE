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

	static Image createImageFromSvg(Drawable* drawable, float width);

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

			LOAD_EPATH_IF_URL("copy", EditorIcons::pasteIcon);

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

	void createChildComponents();

	virtual void initialiseEditor();

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

	void resized() override;

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
	SnapshotMarkdownCodeComponent(SyntaxType syntax, String code, float width, MarkdownParser* parent);

	String generateHtml() const override;

	void addImageLinks(Array<MarkdownLink>& sa) override;

	int getPreferredHeight() const override;

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
