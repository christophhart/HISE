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
		
		if (parent != nullptr)
			g.setColour(parent->getStyleData().textColour);
		else
			g.setColour(Colours::white);

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

void MarkdownCodeComponentBase::updateHeightInParent()
{
	if (auto renderer = dynamic_cast<MarkdownRenderer*>(parent))
	{
		renderer->updateHeight();
	}
}



juce::String SnapshotMarkdownCodeComponent::generateHtml() const
{
	return HtmlHelpers::createSnapshot(syntax, "");
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