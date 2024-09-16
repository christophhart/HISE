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

namespace hise {
using namespace juce;

MarkdownParser::DefaultLinkResolver::DefaultLinkResolver(MarkdownParser* parser_) :
	parser(parser_)
{

}

bool MarkdownParser::DefaultLinkResolver::linkWasClicked(const MarkdownLink& url)
{
	if (url.getType() == MarkdownLink::WebContent)
	{
		URL u(url.toString(MarkdownLink::UrlFull));
		u.launchInDefaultBrowser();
		return true;
	}

	if(url.getType() == MarkdownLink::SimpleAnchor)
		parser->gotoLink(url);

	return false;
}

hise::MarkdownParser::LinkResolver* MarkdownParser::DefaultLinkResolver::clone(MarkdownParser* newParser) const
{
	return new DefaultLinkResolver(newParser);
}


MarkdownParser::FileLinkResolver::FileLinkResolver(const File& root_) :
	root(root_)
{

}


juce::String MarkdownParser::FileLinkResolver::getContent(const MarkdownLink& url)
{
	if (url.fileExists(root))
		return url.toString(MarkdownLink::ContentFull, root);

	return {};
}


bool MarkdownParser::FileLinkResolver::linkWasClicked(const MarkdownLink& )
{
	return false;
}

Image MarkdownParser::FileBasedImageProvider::createImageFromSvg(Drawable* drawable, float width)
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

MarkdownParser::FileBasedImageProvider::FileBasedImageProvider(MarkdownParser* parent, const File& root) :
	ImageProvider(parent),
	r(root)
{

}

juce::Image MarkdownParser::FileBasedImageProvider::getImage(const MarkdownLink& imageURL, float width)
{
	updateWidthFromURL(imageURL, width);

	if(imageURL.fileExists(r))
	{
		auto imageFile = imageURL.getImageFile(r);

		if (imageURL.getType() == MarkdownLink::SVGImage)
		{
			auto drawable = Drawable::createFromSVGFile(imageFile);

			return createImageFromSvg(drawable.get(), width);
		}
		else
			return resizeImageToFit(ImageCache::getFromFile(imageFile), width);
	}

	return {};
}


juce::String MarkdownParser::FolderTocCreator::getContent(const MarkdownLink& url)
{
	if (url.getType() == MarkdownLink::Folder)
	{
		auto readme = url.getMarkdownFile({});

		if (readme.existsAsFile())
			return readme.loadFileAsString();
		else
		{
			File directory = url.getDirectory({});

			if (directory.isDirectory())
			{
				String s;

				s << "## Content of " << url.getPrettyFileName() << "  \n";

				Array<File> files;

				directory.findChildFiles(files, File::findFilesAndDirectories, false);

				files.sort();

				for (auto f : files)
				{
					MarkdownLink fLink(url.getRoot(), f.getRelativePathFrom(rootFile));

					if (f.getFileNameWithoutExtension().toLowerCase() == "readme")
						continue;

					auto path = f.getRelativePathFrom(rootFile);

					s << fLink.toString(MarkdownLink::FormattedLinkMarkdown) + "  \n";
				}

				return s;
			}
		}
	}

	return {};
}

MarkdownParser::FolderTocCreator::FolderTocCreator(const File& rootFile_) :
	LinkResolver(),
	rootFile(rootFile_)
{

}

hise::MarkdownParser::LinkResolver* MarkdownParser::FolderTocCreator::clone(MarkdownParser* ) const
{
	return new FolderTocCreator(rootFile);
}

juce::Image MarkdownParser::URLImageProvider::getImage(const MarkdownLink& urlLink, float width)
{
	if(urlLink.getType() == MarkdownLink::WebContent)
	{
		URL url(urlLink.toString(MarkdownLink::UrlFull));

		auto path = urlLink.toString(MarkdownLink::SubURL);
		auto imageFile = imageDirectory.getChildFile(path);

		if (imageFile.existsAsFile())
			return resizeImageToFit(ImageCache::getFromFile(imageFile), width);

		imageFile.create();

		ScopedPointer<URL::DownloadTask> task = url.downloadToFile(imageFile).release();

		if (task == nullptr)
		{
			jassertfalse;
			return {};
		}

		uint32 timeout = 5000;

		auto start = Time::getApproximateMillisecondCounter();

		while (!task->isFinished())
		{
			if ((Time::getApproximateMillisecondCounter() - start) > timeout)
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

MarkdownParser::URLImageProvider::URLImageProvider(File tempdirectory_, MarkdownParser* parent) :
	ImageProvider(parent),
	imageDirectory(tempdirectory_)
{
	if (!imageDirectory.isDirectory())
		imageDirectory.createDirectory();
}

juce::Image MarkdownParser::GlobalPathProvider::getImage(const MarkdownLink& urlName, float width)
{
	if(urlName.getType() == MarkdownLink::Icon)
	{
		updateWidthFromURL(urlName, width);

		float widthToUseMax = jmax(width, 10.0f);

		Path p;

		String nameToCheck = urlName.toString(MarkdownLink::FormattedLinkIcon);

		for (auto f : factories->factories)
		{
			p = f->createPath(nameToCheck);

			if (!p.isEmpty())
				break;

		}

		if (p.isEmpty())
		{
			// Tried to resolve a global path without success
			return {};
		}

		p.scaleToFit(0.0f, 0.0f, widthToUseMax, widthToUseMax, true);

		Image img(Image::ARGB, (int)widthToUseMax, (int)widthToUseMax, true);
		Graphics g(img);
		
		g.setColour(Colour(0xFF424242));

		g.fillPath(p);

		return img;
	}

	return {};
}

MarkdownParser::GlobalPathProvider::GlobalPathProvider(MarkdownParser* parent) :
	ImageProvider(parent)
{

}

juce::String MarkdownCodeComponentBase::generateHtml() const
{
	return HtmlHelpers::createCodeBlock(syntax, usedDocument->getAllContent());
}

MarkdownCodeComponentBase::MarkdownCodeComponentBase(SyntaxType syntax_, String code, float , float fontsize, MarkdownParser* parent_) :
	syntax(syntax_),
	fontSize(fontsize),
	parent(parent_)
{
	ownedDoc = new CodeDocument();

	switch (syntax)
	{
	case Cpp: 
	{
		tok = new CPlusPlusCodeTokeniser();	
		break;
	}
	case LiveJavascript:
	case LiveJavascriptWithInterface:
	case EditableFloatingTile:
	case SyntaxType::ScriptContent:
	case Javascript: tok = new JavascriptTokeniser(); break;
	case XML:	tok = new XmlTokeniser(); break;
	case Snippet: tok = new MarkdownParser::SnippetTokeniser(); break;
	default: break;
	}

	ownedDoc->replaceAllContent(code);
}

void MarkdownCodeComponentBase::createChildComponents()
{
	addAndMakeVisible(editor);
	addAndMakeVisible(o);

	addAndMakeVisible(expandButton = new TextButton("Expand this code"));
	expandButton->setLookAndFeel(&blaf);
	expandButton->addListener(this);
}

void MarkdownCodeComponentBase::initialiseEditor()
{
	usedDocument = ownedDoc;

    MessageManagerLock mm;
    
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

void MarkdownCodeComponentBase::updateHeightInParent()
{
	if (auto renderer = dynamic_cast<MarkdownRenderer*>(parent))
	{
		renderer->updateHeight();
	}
}

void MarkdownCodeComponentBase::resized()
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


SnapshotMarkdownCodeComponent::SnapshotMarkdownCodeComponent(SyntaxType syntax, String code, float width,
	MarkdownParser* parent):
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

juce::String SnapshotMarkdownCodeComponent::generateHtml() const
{
	return HtmlHelpers::createSnapshot(syntax, "");
}

void SnapshotMarkdownCodeComponent::addImageLinks(Array<MarkdownLink>& sa)
{
	if (syntax == MarkdownCodeComponentBase::EditableFloatingTile)
	{
		sa.add(l);
	}
}

int SnapshotMarkdownCodeComponent::getPreferredHeight() const
{
	if (syntax == MarkdownCodeComponentBase::EditableFloatingTile && screenshot.isNull())
	{
		screenshot = parent->resolveImage(l, MarkdownParser::DefaultLineWidth);
	}

	return jmax<int>(50, screenshot.getHeight());
}

juce::String MarkdownCodeComponentBase::HtmlHelpers::createCodeBlock(SyntaxType syntax, String code)
{
	HtmlGenerator g;

	String syntaxString = "language-javascript";

	if (syntax == XML)
		syntaxString = "language-xml";

	if (syntax == SyntaxType::Cpp)
		syntaxString = "language-clike";

	String s = "<pre><code class=\"" + syntaxString + " line-numbers\">";
	s << code;
	s << "</code></pre>\n";

	return s;
}

juce::String MarkdownCodeComponentBase::HtmlHelpers::createSnapshot(SyntaxType syntax, String code)
{
	if (syntax == MarkdownCodeComponentBase::EditableFloatingTile)
	{
		HtmlGenerator g;

		MarkdownLink l;

		auto imageLink = g.surroundWithTag("", "img", "src=\"" + l.toString(MarkdownLink::FormattedLinkHtml) + "\"");

		return g.surroundWithTag(imageLink, "p");
	}
	else
	{
		return HtmlHelpers::createCodeBlock(syntax, code);
	}
}

}
